#include "qa_cnsty.h"

Consistency::Consistency(QA *p0, InFile *p1,
    std::vector<std::string> &opt, std::string tPath )
{
   pQA = p0;
   pIn = p1;
   checkEnabled=true;

   applyOptions(opt, tPath);

   excludedAttributes.push_back("comment");
   excludedAttributes.push_back("history");
   excludedAttributes.push_back("associated_files");
}

void
Consistency::applyOptions(std::vector<std::string> &optStr,
                          std::string& tablePath)
{
  for( size_t i=0 ; i < optStr.size() ; ++i)
  {
     Split split(optStr[i], "=");

     if( split[0] == "dCC"
          || split[0] == "disableConsistencyCheck"
             || split[0] == "disable_consistency_check")
     {
       checkEnabled=false;
       return;
     }

     if( split[0] == "tP"
          || split[0] == "tablePath" || split[0] == "table_path")
     {
       if( split.size() == 2 )
          tablePath=split[1];

       continue;
     }

     if( split[0] == "tPr"
          || split[0] == "tableProject" )  // dummy
     {
       if( split.size() == 2 )
          consistencyTableFile.setFile(split[1]);
     }

   }

   if( consistencyTableFile.path.size() == 0 )
      consistencyTableFile.setPath(tablePath);

   return;
}

bool
Consistency::check(void)
{
  bool status = false;

  for( size_t i=0 ; i < pIn->dataVarIndex.size() ; ++i )
  {
     Variable& var = pIn->variable[pIn->dataVarIndex[i]];

     std::string str(pQA->qaExp.getTableID() );
     if( str.size() == 0 )
     {
        str = pQA->qaExp.getFrequency() ;
        if( str.size() == 0 )
           str = "N/A" ;
     }

     std::string entryID( var.name + ",");
     entryID += str + "," ;

     if( check(var, entryID) )
     {
       write(var, entryID);
       status=true;
     }
  }

  return status;
}

bool
Consistency::check(Variable &dataVar, std::string entryID)
{
  // return value is true, when there is not a project table, yet.

  // Search the project table for an entry
  // matching the varname and frequency|table_id (account also rotated).

  // Open project table. Mode:  read
  std::string str0(consistencyTableFile.getFile());

  std::ifstream ifs(str0.c_str(), std::ios::in);
  if( !ifs.is_open() )  // file does not exist
     return true;  // causes writing of a new entry

  size_t sz_PV = entryID.size();

  std::string t_md;
  bool notFound=true;

  while( getline(ifs, str0) )
  {
    if( str0.substr(0,sz_PV) == entryID )
    {
      // found a valid entry
      notFound=false;
      t_md = hdhC::stripSides(str0) ;

      // read aux-lines
      while( getline(ifs, str0) )
      {
        str0 = hdhC::stripSides(str0) ;
        if( str0.substr(0,4) != "aux=" )
          break ;  // found the end of the entry

        t_md += '\n';
        t_md += str0 ;
      }

      break;
    }
  }

  if(notFound)
    return true;  // entry not found

  // close the project table
  ifs.close();

  // get meta data info from the file
  std::string f_md;
  getMetaData(dataVar, entryID, f_md);

  // Comparison of meta-data from the project table and the file, respectively.
  // Note that the direct comparison fails because of different spaces.

  // Deviating distribution of auxiliaries and attributes is tolerated.
  // Use index-vector for book-keeping.
  // Meaning: x=split, t=file, t=table, a=attribute, eq=(att-name,att-value)

  Split x;
  x.setSeparator('\n');

  std::vector<std::string> vs_t;
  std::vector<std::string> vs_f;

  x = f_md;
  for( size_t i=0 ; i < x.size() ; ++i )
    vs_f.push_back( hdhC::stripSides(x[i]) );

  x = t_md;
  for( size_t i=0 ; i < x.size() ; ++i )
    vs_t.push_back( hdhC::stripSides(x[i]) );

  // simple test for identity
  if( vs_t.size() == vs_f.size() )
  {
    bool is=true;
    for( size_t i=0 ; i < x.size() ; ++i )
    {
       if( vs_t[i] != vs_f[i] )
       {
           is=false ;
           break;
       }
    }

    if( is )
       return false;
  }

  std::vector<std::vector<std::string> > vvs_f_aName;
  std::vector<std::vector<std::string> > vvs_f_aVal;
  std::vector<std::vector<std::string> > vvs_t_aName;
  std::vector<std::vector<std::string> > vvs_t_aVal;

  x.setSeparator(',');
  Split y;
  y.setSeparator('=');

  for( size_t i=0 ; i < vs_f.size() ; ++i )
  {
    vvs_f_aName.push_back( std::vector<std::string>() );
    vvs_f_aVal.push_back( std::vector<std::string>() );

    x = vs_f[i];
    for( size_t j=0 ; j < x.size() ; ++j )
    {
      y = x[j];

      if( y.size() )
      {
        if( y.size() == 1 )
        {
          if(i==1)
            vvs_f_aName.back().push_back("frequency|table_id");
          else
            vvs_f_aName.back().push_back("");

          // mostly aux=aName, but for the first entry item: only vName
          vvs_f_aVal.back().push_back(y[0]);
        }
        else
        {
          vvs_f_aName.back().push_back(y[0]);
          vvs_f_aVal.back().push_back(y[1]);
        }
      }
    }
  }

  for( size_t i=0 ; i < vs_t.size() ; ++i )
  {
    vvs_t_aName.push_back( std::vector<std::string>() );
    vvs_t_aVal.push_back( std::vector<std::string>() );

    x = vs_t[i];
    for( size_t j=0 ; j < x.size() ; ++j )
    {
      y = x[j];

      if( y.size() )
      {
        if( y.size() == 1 )
        {
          if(i==1)
            vvs_t_aName.back().push_back("frequency|table_id");
          else
            vvs_t_aName.back().push_back("");
          // mostly aux=aName, but for the first entry item: only vName

          vvs_t_aVal.back().push_back(y[0]);
        }
        else
        {
          // backward compatible
          if( y[0] == "values" )
             y[0] = "checksum";

          vvs_t_aName.back().push_back(y[0]);
          vvs_t_aVal.back().push_back(y[1]);
        }
      }
    }
  }

  // test for auxiliaries missing in the file.
  testAux("missing",
          vvs_t_aName, vvs_t_aVal, vvs_f_aName, vvs_f_aVal);

  // test for new auxiliaries introduced by the file
  testAux("new",
          vvs_f_aName, vvs_f_aVal, vvs_t_aName, vvs_t_aVal);

  // test atts of auxiliaries
  for( size_t i=1; i < vvs_f_aName.size() ; ++i )
  {
    for( size_t j=1; j < vvs_t_aName.size() ; ++j )
    {
       if( vvs_f_aVal[i][0] == vvs_t_aVal[j][0] )
       {
         testAttributes(vvs_f_aVal[i][0],  // name of the attribute
                        vvs_f_aName[i], vvs_f_aVal[i],
                        vvs_t_aName[j], vvs_t_aVal[j]);
         break;
       }
    }
  }

  return false;
}

void
Consistency::getAtts(Variable &var, std::string &s)
{
   size_t sz = var.attName.size();

   for(size_t j=0 ; j < sz ; ++j )
   {
     std::string &aN = var.attName[j] ;

     bool isCont=false;
     for( size_t x=0 ; x < excludedAttributes.size() ; ++x )
     {
       if( aN == excludedAttributes[x] )
       {
         isCont=true;
         break;
       }
     }

     if( isCont )
       continue;

     s += ',';
     s += aN ;
     s += '=';

     if( var.attValue[j].size() )
     {
        // convert comma --> space
       std::string &t = var.attValue[j][0] ;
       size_t pos;
       while( (pos=t.find(',')) < std::string::npos )
          t[pos]=' ';

       s += t ;
     }
   }

   // get checksum of values
   if( ! ( var.isUnlimited() || var.isDATA) )
     getValues(var, s);

   return;
}

void
Consistency::getMetaData(Variable &dataVar,
    std::string& entryID, std::string &md)
{
  // Put all meta-data to string.

  // beginning of extraction of meta data
  md = entryID;

  // dimensions
  md += "dims=";
  md += dataVar.getDimNameStr(false, ' ');

  // get attributes of the data variable
  getAtts(dataVar, md);
  getVarType(dataVar, md);

  // get meta-data of auxiliaries
  for( size_t i=0 ; i < pIn->varSz ; ++i )
  {
    if( pIn->variable[i].isDATA )
       continue;

    md += "\n  aux=" ;
    md += pIn->variable[i].name;

    getAtts(pIn->variable[i], md);
    getVarType(pIn->variable[i], md);
  }

  return;
}

void
Consistency::getValues(Variable &var, std::string &s)
{
   // Get the checksum of the limited variables.
   // The purpose of checksums of auxiliary variables is to
   // ensure consistency between follow-up experiments.
   // Note: targets have NC_DOUBLE and all other have NC_FLOAT

   uint32_t ck=0;
   std::string str;
   std::vector<std::string> vs;

   if( var.isUnlimited() )
     return;

   // get data and determine the checksum
   if( ! var.checksum )
   {
      if( var.type == NC_CHAR )
      {
        pIn->nc.getData(vs, var.name);
        bool reset=true;  // is set to false during the first call
        for(size_t l=0 ; l < vs.size() ; ++l)
        {
          vs[l] = hdhC::stripSides(vs[l]);
          ck = hdhC::fletcher32_cmip5(vs[l], &reset) ;
        }
      }
      else
      {
          MtrxArr<double> mv;
          pIn->nc.getData(mv, var.name );

          if( mv.size() > 0 )
            ck = hdhC::fletcher32_cmip5(mv.arr, mv.size()) ;
      }
   }

   s += ",checksum=";
   s += hdhC::double2String(ck, "p=|adj,float") ;

   return;
}

void
Consistency::getVarType(Variable &var, std::string &s)
{
   // Get the checksum of all non-unlimited variables
   // The purpose of checksums of auxiliary variables is to
   // ensure consistency between follow-up experiments.
   // Note: targets have NC_DOUBLE and all other have NC_FLOAT

   // get type of the variable
   std::string type(pIn->nc.getVarTypeStr( var.name ) ) ;
   s += ",type=";
   s += type;

   return;
}

bool
Consistency::lockFile(std::string &fName )
{
  //test for a lock
  std::string lockFile(fName);
  lockFile += ".lock" ;

  std::string test("/bin/bash -c \'test -e ") ;
  test += lockFile ;
  test += '\'' ;

  std::string lock("/bin/bash -c \'touch ");
  lock += lockFile ;
  lock += "  &> /dev/null\'" ;

  // see 'man system' for the return value, here we expect 0,
  // if file exists.

  size_t count=0;

  // THIS WILL lock until the lock-file is removed
  while ( ! system( test.c_str() ) )
  {
    sleep( 1 ) ;  // in seconds
    ++count;

    if( count == 1800 )  // 1/2 hr
    {
      std::string key("IE_3");
      if( notes->inq( key, "PT") )
      {
         std::string capt("consistency-check table is locked for 1/2 hour") ;

         (void) notes->operate(capt) ;
         pQA->setExitState( notes->getExitState() ) ;
      }
    }
  }

  if( system( lock.c_str() ) )
  {
     std::string key("IE_2");
     if( notes->inq( key, "PT") )
     {
        std::string capt("could not lock the consistency-check table") ;

        if( notes->operate(capt) )
          pQA->setExitState( notes->getExitState() ) ;
     }

     return true;
  }

  return false;
}

void
Consistency::setExcludedAttributes(std::vector<std::string> &v)
{
   for( size_t i=0 ; i < v.size() ; ++i )
     excludedAttributes.push_back(v[i]) ;
   return;
}

void
Consistency::testAttributes( std::string& varName,
    std::vector<std::string>& vs_f_aName,
    std::vector<std::string>& vs_f_aVal,
    std::vector<std::string>& vs_t_aName,
    std::vector<std::string>& vs_t_aVal)
{
  // this method is called only, when then first item matched between
  // both the file and the table
  size_t f_ix, t_ix;

  for( f_ix=1 ; f_ix < vs_f_aName.size() ; ++f_ix )
  {
    bool isDifferent=false;
    if( hdhC::isAmong(vs_f_aName[f_ix], vs_t_aName, t_ix) )
    {
        if( vs_f_aVal[f_ix] != vs_t_aVal[t_ix] )
          isDifferent=true;
    }

    else if( vs_f_aName[f_ix] != "checksum" )
    {
      // additional attribute in the current sub-temp file
      std::string key("8_6");
      std::string capt(hdhC::tf_att(varName, vs_f_aName[f_ix])) ;
      capt += "is new across ";

      if( pQA->fileSequenceState == 's' || pQA->fileSequenceState == 'l' )
      {
          key += 'a';
          capt += "sub-temporal files" ;
      }
      else
      {
          key += 'b';
          capt += "experiments" ;
      }

      if(  notes->inq( key, varName ) )
      {
         (void) notes->operate(capt) ;
         notes->setCheckStatus("CNSTY","FAIL" );
      }
    }

    if(isDifferent)
    {
        std::string key("8_8");
        std::string capt;

        if( vs_f_aName[f_ix] == "checksum" )
        {
           capt = hdhC::tf_var(varName, hdhC::colon) ;
           capt += "Checksum of layout or data ";
        }
        else
           capt = hdhC::tf_att(varName, vs_f_aName[f_ix]) ;

        capt += "has changed across ";

        if( pQA->fileSequenceState == 's' || pQA->fileSequenceState == 'l' )
        {
            key += 'a';
            capt += "sub-temporal files, now" ;
        }
        else
        {
            key += 'b';
            capt += "experiments, now" ;
        }

        if( notes->inq(key, varName) )
        {
          capt += hdhC::tf_val(vs_f_aVal[f_ix]) ;
          capt += ", previously ";
          capt += hdhC::tf_val(vs_t_aVal[t_ix]) ;

          (void) notes->operate(capt) ;
          notes->setCheckStatus("CNSTY","FAIL" );
        }
    }
  }

  // test for missing attributes compared to the previous ones
  for( t_ix=1 ; t_ix < vs_t_aName.size() ; ++t_ix )
  {
    if( ! hdhC::isAmong(vs_t_aName[t_ix], vs_f_aName)
            &&  vs_t_aName[t_ix] != "values")
    {
      // aditional attribute in the current sub-temp file
      std::string key("8_7");
      std::string capt(hdhC::tf_att(varName, vs_t_aName[t_ix])) ;
      capt += "is missing across ";
      if( pQA->fileSequenceState == 's' || pQA->fileSequenceState == 'l' )
      {
          key += 'a';
          capt += "sub-temporal files" ;
      }
      else
      {
          capt += "experiments" ;
          key += 'b';
      }

      if(  notes->inq( key, varName ) )
      {

        (void) notes->operate(capt) ;
        notes->setCheckStatus("CNSTY","FAIL" );
      }
    }
  }

  return;
}

void
Consistency::testAux(std::string mode,
    std::vector<std::vector<std::string> >& vvs_1st_aName,
    std::vector<std::vector<std::string> >& vvs_1st_aVal,
    std::vector<std::vector<std::string> >& vvs_2nd_aName,
    std::vector<std::vector<std::string> >& vvs_2nd_aVal)
{
  std::string& varName = vvs_1st_aVal[0][0] ;

  for( size_t i=0 ; i < vvs_1st_aVal.size() ; ++i )
  {
    // is across hdhC::isAmong(), thus do it manually
    bool is=true;
    for( size_t j=0 ; j < vvs_2nd_aVal.size() ; ++j )
    {
      if( vvs_1st_aVal[i][0] == vvs_2nd_aVal[j][0] )
      {
        is = false;
        break;
      }
    }

    // test directly existence; e.g.
    // find time_bnds where time:bound=time_bnds is missing

    if(is)
    {
      for( size_t j=0 ; j < pIn->varSz ; ++j )
      {
        if( pIn->variable[j].name == vvs_1st_aVal[i][0] )
        {
           is=false;
           break;
        }
      }
    }

    if(is)
    {
      if( mode == "missing" )  // missing in the current sub-temp file
      {
        std::string key("8_4");
        std::string capt("auxiliary ");
        capt += hdhC::tf_var(vvs_1st_aVal[i][0]) ;
        capt += "is missing across ";

        if( pQA->fileSequenceState == 's' || pQA->fileSequenceState == 'l' )
        {
            key += 'a';
            capt += "sub-temporal files" ;
        }
        else
        {
            key += 'b';
            capt += "experiments" ;
        }

        if( notes->inq( key, varName ) )
        {
          (void) notes->operate(capt) ;
          notes->setCheckStatus("CNSTY","FAIL" );
        }
      }
      else if( mode == "new" )  // file introduces a new attribute
      {
        std::string key("8_5");
        std::string capt("new auxiliary ");
        capt += hdhC::tf_var(vvs_1st_aVal[i][0]) ;
        capt += "across ";
        if( pQA->fileSequenceState == 's' || pQA->fileSequenceState == 'l' )
        {
            key += 'a';
            capt += "sub-temporal files" ;
        }
        else
        {
            key += 'b';
            capt += "experiments" ;
        }

        if( notes->inq( key, varName) )
        {
          (void) notes->operate(capt) ;
          notes->setCheckStatus("CNSTY","FAIL" );
        }
      }
    }
  }

  return;
}

bool
Consistency::unlockFile(std::string &fName )
{
  //test for a lock
  std::string lockFile(fName);
  lockFile += ".lock" ;

  std::string unlock("/bin/bash -c \'rm -f ");
  unlock += lockFile ;
  unlock += '\'' ;

  // see 'man system' for the return value, here we expect 0,
  // if file exists.

  size_t retries=5;

  for( size_t r=0 ; r < retries ; ++r )
  {
    if( ! system( unlock.c_str() ) )
      return false;
    sleep(1);
  }

  return true;
}

void
Consistency::write(Variable &dataVar, std::string& entryID)
{
  // store meta data info
  std::string md;

  getMetaData(dataVar, entryID, md);

  std::string pFile =  consistencyTableFile.getFile();

  lockFile(pFile);

  // open the file for appending.
  std::fstream oifs;

  oifs.open(pFile.c_str(),
          std::ios::in | std::ios::out );

  if( !oifs) // does not exist
    oifs.open(pFile.c_str(),
          std::ios::out | std::ios::trunc | std::ios::in);

  if (! oifs.is_open() )
  {
    std::string key("IE_1");
    if( notes->inq( key, "PT") )
    {
      std::string capt("could not create a consistency-check table") ;
      std::string text("tried: " + pFile);

      if( notes->operate(capt, text) )
      {
        notes->setCheckStatus("QA_PT_table", pQA->n_fail );
        pQA->setExitState( notes->getExitState() ) ;
      }
    }
  }
  else
  {
    // append
    oifs.seekp(0, std::ios::end);

    oifs << "\n" + md << std::flush;
    oifs.close();
  }

  unlockFile(pFile);

  return;
}
