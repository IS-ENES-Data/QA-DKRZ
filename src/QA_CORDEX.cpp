//#include "qa.h"

DRS_CV::DRS_CV(QA* p)
{
  pQA = p;
  notes = pQA->notes;

  enabledCompletenessCheck=true;

  applyOptions(pQA->optStr);
}

void
DRS_CV::applyOptions(std::vector<std::string>& optStr)
{
  for( size_t i=0 ; i < optStr.size() ; ++i)
  {
     Split split(optStr[i], "=");

     if( split[0] == "dIP"
          || split[0] == "dataInProduction"
              || split[0] == "data_in_production" )
     {
        // effects completeness test in testPeriod()
        enabledCompletenessCheck=false;
        continue;
     }

     if( split[0] == "tGCM"
           || split[0] == "table_GCM_NAME" )
     {
       if( split.size() == 2 )
          GCM_ModelnameTable.setFile(split[1]) ;

       continue;
     }

     if( split[0] == "tRCM"
           || split[0] == "table_RCM_NAME" )
     {
       if( split.size() == 2 )
          RCM_ModelnameTable.setFile(split[1]) ;

       continue;
     }
   }

   // apply a general path which could have also been provided by setTablePath()
   if( GCM_ModelnameTable.path.size() == 0 )
      GCM_ModelnameTable.setPath(pQA->tablePath);

   if( RCM_ModelnameTable.path.size() == 0 )
      RCM_ModelnameTable.setPath(pQA->tablePath);

   return;
}

void
DRS_CV::checkDrivingExperiment(void)
{
  InFile& in = *(pQA->pIn);
  std::string str;

  // special: optional driving_experiment could contain
  // driving_model_id, driving_experiment_name, and
  // driving_model_ensemble_member
  str = in.getAttValue("driving_experiment") ;

  if( str.size() == 0 )
    return;  // optional attribute not available

  Split svs;
  svs.setSeparator(',');
  svs.enableEmptyItems();
  svs = str;

  std::vector<std::string> vs;
  for( size_t i=0 ; i < svs.size() ; ++i )
  {
    vs.push_back( svs[i] );
    vs[i] = hdhC::stripSides( vs[i] ) ;
  }

  if( vs.size() != 3 )
  {
    std::string key("2_9");
    if( notes->inq( key, pQA->fileStr ) )
    {
      std::string capt("global " + hdhC::tf_att("driving_experiment") );
      capt += "with wrong number of items" ;
      std::string text("Found " + hdhC::tf_val(str)) ;

      (void) notes->operate( capt, text) ;
      notes->setCheckStatus("DRS", pQA->n_fail);
    }

    return;
  }

  // index of the pseudo-variable 'NC_GLOBAL'
  if( in.variable.size() == in.varSz )
     return; // no global attributes; checked and notified elsewhere

  Variable &glob = in.variable[in.varSz];

  // att-names corresponding to the three items in drinving_experiment
  std::vector<std::string> rvs;
  rvs.push_back("driving_model_id");
  rvs.push_back("driving_experiment_name");
  rvs.push_back("driving_model_ensemble_member");

  std::string value;

  for( size_t i=0 ; i < rvs.size() ; ++i )
  {
    value = glob.getAttValue( rvs[i] ) ;

    // a missing required att is checked elsewhere
    if( value.size() == 0 )
      continue;

    // allow anything for r0i0p0
    if( value != vs[i] && value != "r0i0p0" && vs[i] != "r0i0p0" )
    {
      std::string key("2_10");
      if( notes->inq( key, pQA->fileStr ) )
      {
        std::string capt("global ");
        capt += hdhC::tf_att("driving_experiment(" + hdhC::itoa(i) + ")" );
        capt += "is in conflict with " + hdhC::tf_att(rvs[i]) ;

        std::string text(rvs[i]) ;
        text += "=" ;
        text += value ;
        text += "\ndriving_experiment=" ;
        if( i==0 )
        {
          text += vs[i] ;
          text += ", ..., ..." ;
        }
        else if( i==1 )
        {
          text += "...," ;
          text += vs[i] ;
          text += ", ..." ;
        }
        else if( i==2 )
        {
          text += ", ..., ..." ;
          text += vs[i] ;
        }

        (void) notes->operate( capt, text) ;
        notes->setCheckStatus("DRS", pQA->n_fail);
      }
    }
  }

  return;
}

void
DRS_CV::checkFilename(std::string& fName, struct DRS_CV_Table& drs_cv_table)
{
  Split x_filename;
  x_filename.setSeparator("_");
  x_filename.enableEmptyItems();
  x_filename = fName ;

  for( size_t i=0 ; i < x_filename.size() ; ++i )
  {
      if( x_filename[i].find('.') < std::string::npos )
      {
         std::string key("1_1b");
         if( notes->inq( key, pQA->qaTime.name) )
         {
           std::string capt("filename must no have a dot, found");
           capt += hdhC::tf_val(x_filename[i]);

           (void) notes->operate(capt) ;
           notes->setCheckStatus(drsF, pQA->n_fail);
         }

         return;
      }
  }

  // note that this test is not part of the QA_Time class, because
  // coding depends on projects
  std::string frq = pQA->qaExp.getFrequency();

  if( pQA->qaTime.testPeriod(x_filename) && frq.size() && frq != "fx" )
  {
    std::string key("1_6a");
    if( notes->inq( key, pQA->qaTime.name) )
    {
      std::string capt("filename requires StartTime-EndTime");

      (void) notes->operate(capt) ;
      notes->setCheckStatus(drsF, pQA->n_fail);
    }
  }

  checkFilenameEncoding(x_filename, drs_cv_table);

  return ;
}

void
DRS_CV::checkFilenameEncoding(Split& x_filename, struct DRS_CV_Table& drs_cv_table )
{
  // fNameEncodingStr: name of the encoding type
  // fNameEncoding:     sequence of DRS path components
  // encodingMap:      name in encoding vs. name of global attribute (or *)

  if( x_filename.size() == 0 )
    return;

  // components of the current path, these are given in the reverse order.
  Split& drs = x_filename;

  Variable& globalVar = pQA->pIn->variable[ pQA->pIn->varSz ] ;
  std::string n_ast="*";
  size_t enc_sz = drs_cv_table.fNameEncoding.size() ;

  Split x_enc[enc_sz];
  std::vector<size_t> countCI(enc_sz, 0);
  std::map<std::string, std::string> globMap[enc_sz] ;
  std::vector<std::vector<size_t> > specialFaultIx ;
  std::map<std::string, std::string>& cvMap = drs_cv_table.cvMap ;

  for( size_t ds=0 ; ds < enc_sz ; ++ds)
  {
    Split& x_e = x_enc[ds] ;
    std::map<std::string, std::string>& gM = globMap[ds] ;

    x_e.setSeparator("_");

    // could have a trailing ".nc" item; if yes, then remove this beforehand
    if( drs_cv_table.fNameEncoding[ds].rfind(".nc") < std::string::npos )
    {
      std::string& t = drs_cv_table.fNameEncoding[ds];
      // is it trailing?
      if( t.substr(t.size()-3) == ".nc" )
        x_e = t.substr(0, t.size()-3) ;
    }
    else
      x_e = drs_cv_table.fNameEncoding[ds] ;

    for(size_t x=0 ; x < x_e.size() ; ++x )
    {
      if( cvMap.count(x_e[x]) == 0 )
      {
        std::string key("IE_6");
        std::string capt("Fault in table ");
        capt += pQA->drs_cv_table.table_DRS_CV.getFile();
        capt += ": encoding";
        capt += hdhC::tf_assign("item", x_e[x]);
        capt += " not found in CV";

        if( notes->inq( key, drsF) )
        {
          (void) notes->operate(capt) ;
          notes->setCheckStatus(drsF, pQA->n_fail);
        }
      }

      if( cvMap[x_e[x]] == n_ast )
      {
        if( x_e[x] == "VariableName" )
          gM[x_e[x]] = pQA->qaExp.fVarname ;

        else if( x_e[x] == "StartTime-EndTime" )
          gM[x_e[x]] = n_ast ;

      }
      else
        gM[x_e[x]] = globalVar.getAttValue(cvMap[x_e[x]]) ;
    }

    //count coincidences between filename components and the DRS
    std::string t;
    size_t x_eSz = x_e.size();

    // append * in case of no period
    drs.append("*");

    for( size_t jx=0 ; jx < x_eSz ; ++jx)
    {
      t = gM[ x_e[jx] ];

      if( jx == drs.size() )
        break;

      if( drs[jx] == t || t == n_ast )
        ++countCI[ds];

      // special
      else if( x_e[jx] == "RCMVersionID" )
      {
         if( drs[jx][0] == 'v' )
         {
            if( drs[jx] == "vAny" )
            {
              drs.replace(jx, globalVar.getAttValue("rcm_version_id") );
              ++countCI[ds];
            }
         }
      }

      else if( drs[jx] == "r0i0p0" && pQA->qaExp.getFrequency() == "fx" )
      {
        drs.replace(jx, globalVar.getAttValue("driving_model_ensemble_member") );
        ++countCI[ds];
      }
    }

    if( countCI[ds] == x_e.size() )
      return;  // check passed; take into account one optional item
  }

  size_t m=0 ; // because there is only a single DRS-FS item

  // find details of the faults
  Split& x_e = x_enc[m] ;
  std::map<std::string, std::string>& gM = globMap[m] ;

  std::vector<std::string> text;
  std::vector<std::string> keys;

  std::string txt;
  findFN_faults(drs, x_e, gM, cvMap, txt) ;
  if( txt.size() )
  {
    keys.push_back("1_1b");
    text.push_back(txt);
  }

  if( text.size() )
  {
    std::string capt("DRS (filename):");

    for(size_t i=0 ; i < text.size() ; ++i )
    {
      if( notes->inq( keys[i], drsF) )
      {
        (void) notes->operate(capt+text[i]) ;
        notes->setCheckStatus(drsF, pQA->n_fail);
      }
    }
  }

  return;
}

void
DRS_CV::checkModelName(std::string &aName, std::string &aValue,
   char des, std::string instName, std::string instValue )
{
   hdhC::FileSplit* tbl;

   if( des == 'G' )
     tbl = &GCM_ModelnameTable ;
   else
     tbl = &RCM_ModelnameTable ;

   ReadLine ifs(tbl->getFile(), false);  // no test of existence

   if( ! ifs.isOpen() )
   {
      std::string key ;

      if( des == 'G' )
        key += "IE_7" ;
      else
        key += "IE_8" ;

      if( notes->inq(key) )
      {
         std::string capt("could not open " + tbl->getBasename()) ;

         if( notes->operate(capt) )
         {
           notes->setCheckStatus(drsP, pQA->n_fail );
           pQA->setExitState( notes->getExitState() ) ;
         }
      }

      return;
   }

   std::string line;
   Split x_line;

   // parse table; trailing ':' indicates variable or 'global'
   ifs.skipWhiteLines();
   ifs.skipComment();

   bool isModel=false;
   bool isInst=false;

   bool isRCM = (des == 'R') ? true : false ;
   if(!isRCM)
     isInst = true; // dummy, because not required for GCM

   while( ! ifs.getLine(line) )
   {
     x_line = line ;

     if( aValue == x_line[0]
            || (isRCM && aValue.substr(0,x_line[0].size()) == x_line[0] ) )
     {
        if( !isModel )
          isModel=true;

        if( isRCM )
        {
          // loops through all the file until a match for model && inst is found
          pQA->qaExp.RCMModelName = aValue;

          if( x_line.size() > 1 && instValue == x_line[1] )
          {
            pQA->qaExp.institute_id = instValue;
            isInst=true;
            break;
          }
        }
        else
        {
          pQA->qaExp.GCMModelName = aValue;
          break;
        }
     }
   }

   ifs.close();

   if( ! (isModel && isInst))
   {
     std::string key;
     if( des == 'G' )
       key = "1_3a" ;
     else
       key = "1_3b" ;

     if( notes->inq( key, pQA->fileStr) )
     {
       std::string capt;

       if( isRCM )
           capt = "RCM";
       else
           capt = "GCM";
       capt += " model";
       capt += hdhC::tf_val(aValue);
       if( isRCM )
       {
         capt += " for institute_id" ;
         capt += hdhC::tf_val(instValue);
       }
       capt += " is not registered in table " ;
       capt += tbl->getBasename();

       if( notes->operate(capt) )
       {
         notes->setCheckStatus(drsP,  pQA->n_fail );
         pQA->setExitState( notes->getExitState() ) ;
       }
     }
   }

   return;
}

void
DRS_CV::checkNetCDF(NcAPI* p_nc)
{
  // NC_FORMAT_CLASSIC (1)
  // NC_FORMAT_64BIT   (2)
  // NC_FORMAT_NETCDF4 (3)
  // NC_FORMAT_NETCDF4_CLASSIC  (4)

  if(!p_nc)
      p_nc=&(pQA->pIn->nc);

  NcAPI& nc = *p_nc;

  int fm = nc.inqNetcdfFormat();
  std::string s;
  bool is=false;

  if( fm == 1 )
  {
    is=true;
    s = "3, NC_FORMAT_CLASSIC";
  }
  else if( fm == 2 )
  {
    is=true;
    s = "3, NC_FORMAT_64BIT";
  }
  else if( fm == 3 )
  {
    is=true;
    s = "4, ";
  }
  else if( fm == 4 )
    s = "4, classic";

  if( fm > 2 )
  {
    if( ! nc.inqDeflate())
    {
      s += " not";
      is=true;
    }

    s+= " deflated";
  }

  if(is)
  {
    std::string key("9_4");
    if( notes->inq( key ) )
    {
      std::string capt("CORDEX requires NetCDF4 classic deflated (compressed)") ;
      capt += ", found NetCDF";
      capt += s;

      (void) notes->operate(capt) ;
      notes->setCheckStatus(drsF, pQA->n_fail);
    }
  }

  return;
}

void
DRS_CV::checkPath(std::string& path, struct DRS_CV_Table& drs_cv_table)
{
  // pathEncodingStr: name of the encoding type
  // pathEncoding:     sequence of DRS path components
  // encodingMap:      name in encoding vs. name of global attribute (or *)

  size_t enc_sz = drs_cv_table.pathEncoding.size() ;

  if( path.size() == 0 || enc_sz == 0 )
    return;

  // components of the current path, these are given in the reverse order.
//  path= "/hdh/data/CMIP5/CMIP5/output/MPI-M/MPI-ESM-LR/hysterical/day/artmos/r1i1p1/tas/";
//  path="/hdh/data/cordex/output/EUR-44/SMHI/ECMWF-ERAINT/evaluation/r1i1p1/SMHI-RCA4/v1/mon/pr";
//  path="/hdh/data/cordex/input/EUR-44/ECMWF-ERAINT/r1i1p1/evaluation/SMHI-RCA4/v1/mon/pr";

  Split drs(path, "/");

  Variable& globalVar = pQA->pIn->variable[ pQA->pIn->varSz ] ;
  std::string n_ast="*";

  Split x_enc[enc_sz];
  std::vector<size_t> countCI(enc_sz, 0);
  std::map<std::string, std::string> globMap[enc_sz] ;
  std::map<std::string, std::string>& cvMap = drs_cv_table.cvMap ;

  for( size_t ds=0 ; ds < enc_sz ; ++ds)
  {
    Split& x_e = x_enc[ds] ;
    std::map<std::string, std::string>& gM = globMap[ds] ;

    x_e.setSeparator("/");

    x_e = drs_cv_table.pathEncoding[ds] ;

    for(size_t x=0 ; x < x_e.size() ; ++x )
    {
      if( cvMap.count(x_e[x]) == 0 )
      {
        std::string key("IE_6");
        std::string capt("Fault in table ");
        capt += pQA->drs_cv_table.table_DRS_CV.getFile() ;
        capt += ": encoding " ;
        capt += hdhC::tf_assign("item", x_e[x]) ;
        capt += " not found in CV";

        if( notes->inq( key, drsP) )
        {
          (void) notes->operate(capt) ;
          notes->setCheckStatus(drsP, pQA->n_fail);
        }
      }

      if( cvMap[x_e[x]] == n_ast )
      {
        if( x_e[x] == "VariableName" )
          gM[x_e[x]] = pQA->qaExp.fVarname ;
      }
      else
        gM[x_e[x]] = globalVar.getAttValue(cvMap[x_e[x]]) ;
    }

    // special: at least for the HH ESGF node, which has an additional trailing
    // item for a versioning, e.g. 'v20121231'. The digit nature is checked. But,
    // not when rcm_version_id = vAny is set in PROJECT_CV_DRS.csv.
    size_t last = drs.size()-1 ;
    if( drs[last][0] == 'v' )
    {
        if( drs[last] == "vAny" || hdhC::isDigit(drs[last].substr(1)) )
          drs.erase(last);
    }

    //special: customisation
    for( size_t i=0 ; i < drs.size() ; ++i )
    {
      if( drs[i] == "r0i0p0" && pQA->qaExp.getFrequency() == "fx" )
        // never thought the CORDEX community about this; at least in the beginning
        drs.replace(i, globalVar.getAttValue("driving_model_ensemble_member") );
    }

    std::string t;
    int ix;
    if( (ix = getPathBegIndex( drs, x_e, gM )) == -1 )
      continue;

    size_t drsBeg = ix;
    size_t i;

    for( size_t j=0 ; j < x_e.size() ; ++j)
    {
      t = gM[ x_e[j] ];

      if( (i = drsBeg+j) < drs.size() )
      {
        //count coincidences between path components and the DRS
        if( drs[i] == t || t == n_ast )
          ++countCI[ds];
        // drs may have a different value, e.g. output1
        else if( x_e[j] == "product" )
        {
          checkProductName(drs[i], cvMap[x_e[j]+"_constr"], gM);
          ++countCI[ds];
        }

        // another type of check; not counting in countCI
        if( x_e[j] == "RCMModelName" )
        {
          std::string inst("Institution");
          checkModelName(x_e[j], t, 'R', inst, gM[inst]) ;
        }
        else if( x_e[j] == "GCMModelName" )
          checkModelName(x_e[j], t, 'G') ;
      }
    }

    if( countCI[ds] == x_e.size() )
      return;  // check passed
  }

  // find the encoding with maximum number of coincidences
  size_t m=0;

/* // necessary if more than a single DRS structure is available
  size_t mx=countCI[0];
  for( size_t ds=1 ; ds < countCI.size() ; ++ds )
  {
    if( countCI[ds] > mx )
    {
      mx = countCI[ds] ;
      m=ds;
    }
  }
*/


  Split& x_e = x_enc[m] ;
  std::map<std::string, std::string>& gM = globMap[m] ;

  std::vector<std::string> text;
  std::vector<std::string> keys;

  std::string txt;
  findPath_faults(drs, x_e, gM, cvMap, txt) ;
  if( txt.size() )
  {
    keys.push_back("1_1a");
    text.push_back(txt);
  }

  if( text.size() )
  {
    std::string capt("DRS (path): ");

    for( size_t i=0 ; i < text.size() ; ++i )
    {
      if( notes->inq( keys[i], pQA->fileStr) )
      {
        (void) notes->operate(capt+text[i]) ;
        notes->setCheckStatus(drsP, pQA->n_fail);
      }
    }
  }

  return;
}

void
DRS_CV::checkProductName(std::string& drs_product,
  std::string prod_choice,
  std::map<std::string, std::string>& gM)
{
  Split x_cp(prod_choice, "|");

  for( size_t i=0 ; i < x_cp.size() ; ++i )
  {
    if( drs_product == hdhC::stripSides(x_cp[i]) )
    {
      // adjustment for a test
      gM["product"] = drs_product ;
      return;
    }
  }

  std::string key("1_2b");

  if( notes->inq( key, drsP) )
  {
    std::string capt("DRS fault for path component <product>");

    std::string text("Found" + hdhC::tf_val(drs_product) ) ;
    text += ", expected";
    text += hdhC::tf_val(prod_choice);

    (void) notes->operate(capt, text) ;
    pQA->setExitState( notes->getExitState() ) ;
  }

  return;
}

void
DRS_CV::findFN_faults(Split& drs, Split& x_e,
                   std::map<std::string,std::string>& gM,
                   std::map<std::string, std::string>& cvMap,
                   std::string& text)
{
  std::string t;
  std::string n_ast="*";
  int x_eSz = x_e.size();
  int drsSz = drs.size() ;
  bool isMiss = drsSz < x_eSz ;

  if( isMiss )
    text = " suspicion of a missing item, " ;

  for( int j=0 ; j < x_eSz ; ++j)
  {
    t = gM[ x_e[j] ];

    if( !(drs[j] == t || t == n_ast) )
    {
      if( isMiss )
          text += " probably" + hdhC::tf_val(x_e[j]) ;
      else
      {
        text += " failed for " + hdhC::tf_val(x_e[j]);
        text += ", found in filename " + hdhC::tf_val(drs[j]) ;
        text += " and global " + hdhC::tf_att(hdhC::empty,cvMap[x_e[j]],t) ;
      }

      break;
    }
  }

  return ;
}

void
DRS_CV::findPath_faults(Split& drs, Split& x_e,
                   std::map<std::string,std::string>& gM,
                   std::map<std::string, std::string>& cvMap,
                   std::string& text)
{
  std::string t;
  std::string n_ast="*";
  int x_eSz = x_e.size();
  int drsSz = drs.size() ;
  int drsBeg=-1;

  if( drsSz > x_eSz )
  {
    std::vector<int> count(drsSz, 0);

    for( int i=0 ; i < drsSz ; ++i )
    {
      for( int j=0 ; j < x_eSz ; ++j )
      {
        int k = i+j;
        if( k < drsSz && drs[k] == gM[ x_e[j] ] )
          ++count[i];
      }
    }

    // highest count determines drsBeg
    int countMax=count[0];
    drsBeg=0;
    for( size_t i=1 ; i < count.size() ; ++i )
    {
      if( countMax < count[i] )
      {
        countMax = count[i];
        drsBeg = i;
      }
    }
  }

  bool isMiss = drsBeg < 0 ;

  if( isMiss)
    text = "suspicion of a missing item, " ;

  for( int j=0 ; j < x_eSz ; ++j)
  {
    t = gM[ x_e[j] ];

    int i = drsBeg+j ;

    if( !( drs[i] == t || t == n_ast) )
    {
      if( isMiss )
        text = " probably" + hdhC::tf_val(x_e[j]) ;
      else
      {
        if(x_e[j] == "activity" )
        {
          std::string s( hdhC::Upper()(drs[i]) ) ;
          if( s == t  && !notes->inq( "1_2a", pQA->fileStr, "INQ_ONLY") )
           continue;
        }

        text += " failed for " + hdhC::tf_assign(x_e[j],t) ;
        text += ", found" + hdhC::tf_val(drs[i]) + " in the path" ;
      }

      break;
    }
  }

  return ;
}

std::string
DRS_CV::getInstantAtt(void)
{
//  if( pQA->qaExp.getFrequency() == "6hr" )
//    return true;

  size_t i;
  for( i=0 ; i < pQA->qaExp.varMeDa.size() ; ++i )
    if( pQA->qaExp.varMeDa[i].attMap[pQA->n_cell_methods].size()
          && pQA->qaExp.varMeDa[i].attMap[pQA->n_cell_methods] != "time: point" )
        break;

  if( i == pQA->qaExp.varMeDa.size() )
      return "";
  else
      return hdhC::tf_att(pQA->qaExp.varMeDa[i].var->name,
                  pQA->qaExp.varMeDa[i].attMap[pQA->n_cell_methods] ) ;
}

int
DRS_CV::getPathBegIndex(
    Split& drs, Split& x_e,
    std::map<std::string, std::string>& gM )
{
  int ix=-1;

  // find the first match of a drs item with one of the gMs
  std::vector<std::string> val;
  for( size_t i=0 ; i < x_e.size() ; ++i)
      val.push_back(gM[ x_e[i] ]) ;

  for( size_t i=0 ; i < drs.size() ; ++i)
  {
    std::string s(drs[i]);
    std::string sU = hdhC::Upper()(s);

    bool is=false;

    if( hdhC::isAmong(s, val) )
       is=true ;
    else if( hdhC::isAmong(sU, val) )
       is=true ;

    if(is)
    {
        ix = static_cast<int>(i);
        // the match could be a leading false one, requested is the last one, so
        // search another one.
        for( size_t j=i+1 ; j < drs.size() ; ++j)
        {
            std::string tU = hdhC::Upper()(drs[j]);

            if( tU == sU )
                ix = static_cast<int>(j);
        }

        break;
    }
  }

  return ix;
}

void
DRS_CV::run(void)
{
  DRS_CV_Table& drs_cv_table = pQA->drs_cv_table ;

  // check for the required dir structure
  if( pQA->isCheckDRS_P )
  {
     drsP = "DRS(P)" ;
     checkPath(pQA->pIn->file.path, drs_cv_table) ;
  }

  // compare filename to netCDF global attributes
  if( pQA->isCheckDRS_F )
  {
     drsF = "DRS(F)" ;
     checkFilename(pQA->pIn->file.basename, drs_cv_table);
  }

  // is it NetCDF-4, is it compressed?
  checkNetCDF();

  // optional, but if, then with three prescribed members
  checkDrivingExperiment();

  return;
}

// Class with project specific purpose
QA_Exp::QA_Exp()
{
  initDefaults();
}

void
QA_Exp::applyOptions(std::vector<std::string>& optStr)
{
  for( size_t i=0 ; i < optStr.size() ; ++i)
  {
     Split split(optStr[i], "=");

     if( split[0] == "eA" || split[0] == "excludedAttribute"
         || split[0] == "excluded_attribute" )
     {
       Split cvs(split[1],",");
       for( size_t i=0 ; i < cvs.size() ; ++i )
         excludedAttribute.push_back(cvs[i]);

       continue;
     }

     if( split[0] == "fNVI"
          || split[0] == "file_name_var_index" )
     {
       if( split.size() == 2 )
          varnamePosition=hdhC::string2Double(split[1]);

       continue;
     }

     if( split[0] == "fq"
          || split[0] == "frequency" )
     {
       if( split.size() == 2 )
          frequency=split[1];

       continue;
     }

     if( split[0] == "tVR"
          || split[0] == "tableVariableRequirement" )
     {
       if( split.size() == 2 )
          varReqTable.setFile(split[1]);

       continue;
     }

     if( split[0] == "uS" || split[0] == "useStrict"
            || split[0] == "use_strict" )
     {
       isUseStrict=true ;
       pQA->setCheckMode("drs,cv,time");
       continue;
     }
   }

   // apply a general path which could have also been provided by setTablePath()
   if( varReqTable.path.size() == 0 )
      varReqTable.setPath(pQA->tablePath);

   return;
}

bool
QA_Exp::checkDateFormat(std::string rV, std::string aV)
{
    // a) return true for: does not match
    // b) leading and/or trailing '*' indicate that any corresponding
    //    non-digits are expected
    // c) leading and/or trailing non-digits are mandatory in val_is

    if( rV.size() == 0 || aV.size() == 0 )
        return true;

    // [DATE: ]request
    size_t i;
    if( rV.substr(0,5) == "DATE:" )
    {
       // skip following blanks
       for( i=5 ; i < rV.size() ; ++i )
          if( rV[i] != ' ' )
             break;

       rV = rV.substr(i);
    }

    // a prefix is separated by '|'
    size_t pos;
    std::string prefix;
    if( (pos=rV.find('|')) < std::string::npos )
    {
        prefix = rV.substr(0,pos) ;
        rV = rV.substr(pos+1);
    }

    size_t prefix_width = 0;

    if( prefix.size() )
    {
       size_t p_ast;
       if( (p_ast=prefix.find('*')) < std::string::npos )
       {
          if( prefix.size() == 1 )
             prefix_width=1;
          else
          {
              // non-static prefix (also with digits) of fixed size
              int sz = std::stoi(prefix.substr(0,p_ast)) ;
              if( rV.substr(0,sz) != aV.substr(0,sz) )
                 return true;
              else
                 prefix_width=sz;
          }
       }
       else
       {
          size_t sz = prefix.size();
          if( prefix != aV.substr(0, sz) )
             // prefix has to be identical
             return true;
          else
             prefix_width=sz;
       }
    }

    if( prefix_width > 0 )
        aV = aV.substr(prefix_width);

    // pure digits are requested, the available one should have the same size
    if( aV.size() && hdhC::isDigit(aV) )
    {
       if( rV.find('*') < std::string::npos )
          return false;
       else
          return aV.size() == rV.size() ? false : true ;
    }

    // a valid date format may have T substituted by a blank
    // and Z omitted or the latter be lower case.
    Split x_aV(aV, " T", true);
    Split x_rV(rV, " T", true);

    // '*' at any position inicates omissioni of leading 0
    bool isLeadingZero=true;
    if( rV.find('*') < std::string::npos )
    {
        isLeadingZero = false;
        size_t sz = rV.size()-1;
        if( rV[sz] == '*' )
            rV = rV.substr(0,sz);
    }

    Split x_aV_dt(x_aV[0], '-');
    Split x_rV_dt(x_rV[0], '-');

    if( x_aV.size() )
    {
        // the date
        if( x_aV_dt.size() != x_rV_dt.size() )
            return true;

        if( isLeadingZero )
        {
            for( size_t i=0 ; i < x_rV_dt.size() ; ++i )
                if( x_rV_dt[i].size() != x_aV_dt[i].size() )
                    return true;
        }
    }

    if( x_aV.size() < 3 )
    {
        // the time
        x_aV_dt.setSeparator(':');
        x_rV_dt.setSeparator(':');
        x_aV_dt=x_aV[1] ;
        x_rV_dt=x_rV[1] ;

        // time zone by an appending letter
        size_t last = x_rV_dt.size() -1 ;
        bool is_TZ_rV = false;
        if( x_rV_dt.size() == 3 && hdhC::isAlpha(x_rV_dt[2][last]) )
        {
            x_rV_dt[2] = x_rV_dt[2].substr(0,last);
            is_TZ_rV = true;
        }

        last = x_aV_dt.size() -1 ;
        bool is_TZ_aV = false;
        if( x_aV_dt.size() == 3 && hdhC::isAlpha(x_aV_dt[2][last]) )
        {
            x_aV_dt[2] = x_aV_dt[2].substr(0,last);
            is_TZ_aV = true;
        }
        else
        {
            // time-zone may be omitted for Z
            if( is_TZ_rV )
                return false;
        }

        if( is_TZ_aV != is_TZ_rV )
            return true;

        if( x_aV_dt.size() != x_rV_dt.size() )
            return true;

        if( isLeadingZero )
        {
            for( size_t i=0 ; i < x_rV_dt.size() ; ++i )
                if( x_rV_dt[i].size() != x_aV_dt[i].size() )
                    return true;
        }
    }

    return false;
}

void
QA_Exp::checkDimTableEntry(InFile &in,
    VariableMetaData &vMD,
    struct DimensionMetaData &f_DMD_entry,
    struct DimensionMetaData &t_DMD_entry)
{
  // Do the dimensional meta-data found in the netCDF file
  // match those in the table (standard or project)?
  // Failed checks against a project table result in issuing an
  // error message. Only warnings for a failed check against
  // the standard table.

  std::string text;
  std::string t0;
  std::string& t_DMD_outname = t_DMD_entry.attMap[n_outname] ;

  if( t_DMD_outname != f_DMD_entry.attMap[n_outname] )
     checkDimOutName(in, vMD, f_DMD_entry, t_DMD_entry);

  if( t_DMD_entry.attMap[n_standard_name] != f_DMD_entry.attMap[n_standard_name] )
     checkDimStndName(in, vMD, f_DMD_entry, t_DMD_entry);

  if( t_DMD_entry.attMap[n_long_name] != f_DMD_entry.attMap[n_long_name] )
     checkDimLongName(in, vMD, f_DMD_entry, t_DMD_entry);

  if( t_DMD_entry.attMap[n_axis] != f_DMD_entry.attMap[n_axis] )
     checkDimAxis(in, vMD, f_DMD_entry, t_DMD_entry);

  // special for the unlimited dimension
  if( t_DMD_outname == "time" )
  {
    checkDimULD(vMD, f_DMD_entry, t_DMD_entry) ;
  }
  else
  {
    // all the items in this else-block have a transient meaning
    // for the time variable

    // special: 'plevs' check only for the 17 mandatory levels.
    //          This in ensured by the calling method.

    if( t_DMD_entry.size != f_DMD_entry.size )
      checkDimSize(in, vMD, f_DMD_entry, t_DMD_entry);

    // note: an error in size skips this test
    else if( t_DMD_entry.checksum != f_DMD_entry.checksum)
      checkDimChecksum(in, vMD, f_DMD_entry, t_DMD_entry);

    // special: units
    checkDimUnits(in, vMD, f_DMD_entry, t_DMD_entry);
  }

  return ;
}

void
QA_Exp::checkDimAxis(InFile &in,
    VariableMetaData &vMD,
    struct DimensionMetaData &f_DMD_entry,
    struct DimensionMetaData &t_DMD_entry)
{
  std::string key("4_9");
  std::string& t_DMD_axis = t_DMD_entry.attMap[n_axis] ;
  std::string& f_DMD_axis = f_DMD_entry.attMap[n_axis] ;

  if( notes->inq( key, vMD.var->name) )
  {
    std::string capt( getCaptIntroDim(vMD, f_DMD_entry, t_DMD_entry) );

    if( t_DMD_axis.size() && f_DMD_axis.size() )
      capt += pQA->s_mismatch ;
    else if( t_DMD_axis.size() )
      capt+= "not available in the file" ;
    else
      capt += "not available in the table" ;

    std::string text("axis (table)=") ;
    if( t_DMD_axis.size() )
      text += t_DMD_axis ;
    else
      text += pQA->notAvailable ;

    text += "\naxis (file)=" ;
    if( f_DMD_axis.size() )
      text += f_DMD_axis ;
    else
      text += pQA->notAvailable ;

    (void) notes->operate(capt, text) ;
    notes->setCheckStatus("CV", pQA->n_fail);
  }

  return;
}

void
QA_Exp::checkDimChecksum(InFile &in,
    VariableMetaData &vMD,
    struct DimensionMetaData &f_DMD_entry,
    struct DimensionMetaData &t_DMD_entry)
{
  // cf...: dim is variable across files
  if( f_DMD_entry.attMap[n_outname] == "loc" )
    return;

  std::string t0;

  std::string key("4_8");
  if( notes->inq( key, vMD.var->name) )
  {
    std::string capt( getCaptIntroDim(vMD, f_DMD_entry, t_DMD_entry) ) ;
    capt += "checksum of values changed" ;

    std::string text("checksum (table)=") ;
    text += hdhC::double2String(t_DMD_entry.checksum) ;
    text += "\nchecksum (file)=" ;
    text += hdhC::double2String(f_DMD_entry.checksum) ;

    (void) notes->operate(capt, text) ;
    notes->setCheckStatus("AUX", pQA->n_fail);
  }

  return;
}

void
QA_Exp::checkDimLongName(InFile &in,
    VariableMetaData &vMD,
    struct DimensionMetaData &f_DMD_entry,
    struct DimensionMetaData &t_DMD_entry)
{
  std::string& t_DMD_long_name = t_DMD_entry.attMap[n_long_name] ;
  std::string& f_DMD_long_name = f_DMD_entry.attMap[n_long_name] ;

  std::string key("4_3");
  if( notes->inq( key, vMD.var->name) )
  {
    std::string capt(getCaptIntroDim(vMD, f_DMD_entry, t_DMD_entry, pQA->n_long_name) ) ;

    if( t_DMD_long_name.size() && f_DMD_long_name.size() )
      capt += pQA->s_mismatch ;
    else if( t_DMD_long_name.size() )
      capt += "not available in the file" ;
    else
      capt += "not available in the table" ;

    std::string text("long name (table)=") ;
    if( t_DMD_long_name.size() )
      text += t_DMD_long_name ;
    else
      text += pQA->notAvailable ;

    text += "\nlong name (file)=" ;
    if( f_DMD_long_name.size() )
      text += f_DMD_long_name ;
    else
      text += pQA->notAvailable ;

    (void) notes->operate(capt, text) ;
    notes->setCheckStatus("CV", pQA->n_fail);
  }

  return;
}

void
QA_Exp::checkDimOutName(InFile &in,
    VariableMetaData &vMD,
    struct DimensionMetaData &f_DMD_entry,
    struct DimensionMetaData &t_DMD_entry)
{
  std::string key("4_1");
  std::string& t_DMD_outname = t_DMD_entry.attMap[n_outname] ;
  std::string& f_DMD_outname = f_DMD_entry.attMap[n_outname] ;

  if( notes->inq( key, vMD.var->name) )
  {
    std::string capt( getCaptIntroDim(vMD, f_DMD_entry, t_DMD_entry) );

    if( t_DMD_outname.size() && f_DMD_outname.size() )
    {
      capt += "output name with " ;
      capt += pQA->s_mismatch ;
    }
    else if( t_DMD_outname.size() )
      capt += "output name not available in the file" ;
    else
      capt += "output name not available in the table" ;

    std::string text("output name (table)=") ;
    if( t_DMD_outname.size() )
      text += t_DMD_outname ;
    else
      text += pQA->notAvailable ;

    text += "\noutput name (file)=" ;
    if( f_DMD_outname.size() )
      text += f_DMD_outname ;
    else
      text += pQA->notAvailable ;

    (void) notes->operate(capt, text) ;
    notes->setCheckStatus("CV", pQA->n_fail);
  }

  return ;
}

void
QA_Exp::checkDimSize(InFile &in,
    VariableMetaData &vMD,
    struct DimensionMetaData &f_DMD_entry,
    struct DimensionMetaData &t_DMD_entry)
{
  // cf...: dim is variable across files
  if( f_DMD_entry.attMap[n_outname] == "loc" )
    return;

  std::string key("4_5");
  if( notes->inq( key, vMD.var->name) )
  {
    std::string capt( getCaptIntroDim(vMD, f_DMD_entry, t_DMD_entry) ) ;
    capt += "different size";

    std::string text("dim-size (table)=") ;
    text += hdhC::double2String(t_DMD_entry.size) ;
    text += "\ndim-size (file)=" ;
    text += hdhC::double2String(f_DMD_entry.size) ;

    (void) notes->operate(capt, text) ;
    notes->setCheckStatus("CV", pQA->n_fail);
  }

  return;
}

void
QA_Exp::checkDimStndName(InFile &in,
    VariableMetaData &vMD,
    struct DimensionMetaData &f_DMD_entry,
    struct DimensionMetaData &t_DMD_entry)
{
  std::string key("4_2");
  std::string& t_DMD_standard_name = t_DMD_entry.attMap[n_standard_name] ;
  std::string& f_DMD_standard_name = f_DMD_entry.attMap[n_standard_name] ;

  if( notes->inq( key, vMD.var->name) )
  {
    std::string capt( getCaptIntroDim(vMD, f_DMD_entry, t_DMD_entry, pQA->n_standard_name) ) ;

    if( t_DMD_standard_name.size() && f_DMD_standard_name.size() )
      capt += pQA->s_mismatch ;
    else if( t_DMD_standard_name.size() )
      capt += "not available in the file" ;
    else
      capt += "not available in the table" ;

    std::string text("table=");
    text += varReqTable.filename + ", frequency=";
    text += getFrequency() + ", variable=";
    text += vMD.var->name + ", dimension=";
    text += f_DMD_entry.attMap[n_outname] + "\nstandard_name (table)=" ;
    if( t_DMD_standard_name.size() )
      text += t_DMD_standard_name ;
    else
      text += pQA->notAvailable ;

    text += "\nstandard_name (file)=" ;
    if( f_DMD_standard_name.size() )
      text += f_DMD_standard_name ;
    else
      text += pQA->notAvailable ;

    (void) notes->operate(capt, text) ;
    notes->setCheckStatus("CV", pQA->n_fail);
  }

  return;
}

void
QA_Exp::checkDimULD(
     VariableMetaData &vMD,
     struct DimensionMetaData &f_DMD_entry,
     struct DimensionMetaData &t_DMD_entry)
{
  // Special for the unlimited dimension

  // Do the dimensional meta-data found in the netCDF file
  // match those in the standard table?
  // Checking ranges is senseless, thus discarded.
  std::string& t_DMD_units = t_DMD_entry.attMap[n_units] ;
  std::string& f_DMD_units = f_DMD_entry.attMap[n_units] ;

  if( t_DMD_units != f_DMD_units )
  {
    // Note: a different date between the two might not be a fault.
    // If keywords match, but the dates do not, then emit
    // only a warning.
    if( ( t_DMD_units.find("days") < std::string::npos
          && t_DMD_units.find("since") < std::string::npos )
                        !=
        (  f_DMD_units.find("days") < std::string::npos
          &&  f_DMD_units.find("since") < std::string::npos ) )
    {
      std::string key("3_13");
      if( notes->inq( key, vMD.var->name) )
      {
        std::string capt( getCaptIntroDim(vMD, f_DMD_entry, t_DMD_entry, pQA->n_units) ) ;

        if( t_DMD_units.size() && f_DMD_units.size() )
          capt += "different periods" ;
        else if( t_DMD_units.size() )
          capt += "missing period string (file)" ;
        else
          capt += "missing period string (table)" ;

        std::string text("units (table)=") ;
        if( t_DMD_units.size() )
          text += t_DMD_units ;
        else
          text += "missing key string=days since" ;

        text += "\nunits (file): " ;
        if( f_DMD_units.size() )
          text += f_DMD_units ;
        else
          text += "missing key string=days since" ;

        (void) notes->operate(capt, text) ;
        notes->setCheckStatus("CV", pQA->n_fail);
      }
    }
    else // deviating reference dates
    {
      // There is nothing wrong with deviating references
      if( ! pQA->qaTime.isReferenceDate )
        return ;  // do not check at all

      // do not check at the beginning of a new experiment.
      if( ! pQA->qaTime.isReferenceDateAcrossExp
               && pQA->currQARec == 0 )
        return ;

      Date tbl_ref( t_DMD_units, pQA->qaTime.calendar );
      Date nc_ref( f_DMD_units, pQA->qaTime.calendar );

      std::string key("3_14");
      if( notes->inq( key, vMD.var->name) )
      {
        std::string capt( getCaptIntroDim(vMD, f_DMD_entry, t_DMD_entry, pQA->n_units) ) ;

        if( t_DMD_units.size() && f_DMD_units.size() )
          capt += "different reference dates" ;
        else if( t_DMD_units.size() )
          capt += "missing reference date in the file" ;
        else
          capt += "missing reference date in the table" ;

        std::string text ;
        if( tbl_ref != nc_ref )
        {
          text += "units (table)=" ;
          if( t_DMD_units.size() )
            text += t_DMD_units ;
          else
            text += pQA->notAvailable ;

          text += "\nunits (file)=" ;
          if( f_DMD_units.size() )
            text += f_DMD_units ;
          else
            text += pQA->notAvailable ;
        }

        (void) notes->operate(capt, text) ;
        notes->setCheckStatus("CV", pQA->n_fail);
      }
    }
  }

  return ;
}

void
QA_Exp::checkDimUnits(InFile &in,
     VariableMetaData &vMD,
     struct DimensionMetaData &f_DMD_entry,
     struct DimensionMetaData &t_DMD_entry)
{
  // Special: units related to any dimension but 'time'

  // Do the dimensional meta-data found in the netCDF file
  // match those in the table (standard or project)?
  std::string& t_DMD_units = t_DMD_entry.attMap[n_units] ;
  std::string& f_DMD_units = f_DMD_entry.attMap[n_units] ;

  // index axis has no units
  if( t_DMD_entry.attMap[n_index_axis] == "ok")
    return;

  if( t_DMD_units == f_DMD_units )
    return;

  //dimension's var-rep without units in the standard table
  std::string key("4_4");

  if( notes->inq( key, vMD.var->name) )
  {
    std::string capt( getCaptIntroDim(vMD, f_DMD_entry, t_DMD_entry, pQA->n_units) ) ;

    if( t_DMD_units.size() && f_DMD_units.size() )
      capt += pQA->s_mismatch ;
    else if( t_DMD_entry.isUnitsDefined )
      capt += "not available in the file" ;
    else
      capt += "not defined in the table" ;

    std::string text("units (table)=") ;
    if( t_DMD_units.size() )
      text += t_DMD_units ;
    else
      text += "not defined" ;

    text += "\nunits (file)=" ;
    if( f_DMD_units.size() )
      text += f_DMD_units ;
    else if( f_DMD_entry.isUnitsDefined )
      text += pQA->notAvailable ;
    else
      text += "not defined" ;

    (void) notes->operate(capt, text) ;
    notes->setCheckStatus("CV", pQA->n_fail);
  }

  return ;
}

void
QA_Exp::checkPressureCoord(InFile &in)
{
   // Check for missing pressure coordinate and for the correct value
   // This has to be done for all variables with appended number
   // indicating the pressure level.

   std::string fV = fVarname.substr(0,2);

   if ( !( fV == "ua" || fV == "va" || fV == "ta" || fV == "zg" ) )
      return;  //not a variable on a specific pressure level

   // exclude variables like tas, tasmax etc.
   // true: return 0 for no 1st number
   bool is = hdhC::string2Double(fVarname, 1, true) == 0. ;

   if( is && fVarname.size() > 2 )
     return;

   // a trailing level after the file-var-name?
   if( fVarname.size() == 2 )
   {
     std::string key("1_5a");
     if( notes->inq( key, fVarname ) )
     {
       std::string capt("The variable acronym");
       capt += hdhC::tf_val(fVarname, hdhC::blank) ;
       capt += "in the filename is missing a pressure level." ;

       (void) notes->operate( capt ) ;
       notes->setCheckStatus("CV", pQA->n_fail);
     }
   }

   std::string fVal = fVarname.substr(2);

   if( ! (fVal == "200" || fVal == "500" || fVal == "850" ) )
   {
     std::string key("1_5b");
     if( notes->inq( key ) )
     {
       std::string capt("Pressure level value=");
       capt += fVal + " in the filename is inappropriate" ;

       std::string text("Expected: 200 500 or 850");

       (void) notes->operate( capt, text ) ;
       notes->setCheckStatus("CV", pQA->n_fail);
     }
   }

   std::string pFile( fVarname.substr(2) + "00" ); // conversion to Pa
   std::string pVarname;
   int    plev_ix=-1;

   for( size_t ix=0 ; ix < in.varSz ; ++ix )
   {
     Variable &var = in.variable[ix];

     if( var.name == "plev")
       plev_ix=ix;
     else if( var.isUnlimited() )
     {
        // true: return 0 for no 1st number
        double anum = hdhC::string2Double(var.name, 1, true);
        if( anum > 0. )
           pVarname = hdhC::double2String(anum) + "00";
     }
   }

   if( plev_ix == -1 )
   {
     std::string key("5_3");
     if( notes->inq( key ) )
     {
       std::string capt("Auxiliary " + hdhC::tf_var("plev") + "is missing") ;

       (void) notes->operate( capt ) ;
       notes->setCheckStatus("CV", pQA->n_fail);
     }

     return ;
   }

   // is there a difference between filename and variable name?
   if( pFile.size() && pFile != pVarname )
   {
     std::string key("1_5c");
     if( notes->inq( key ) )
     {
       std::string capt("p-level in the filename and the variable name do not match") ;
       capt += ", found: " + hdhC::tf_assign("p(file)", pFile);
       capt += " and " + hdhC::tf_assign("p(var-name)", pVarname);

       (void) notes->operate( capt) ;
       notes->setCheckStatus("CV", pQA->n_fail);
     }
   }

   // look for pressure, units, Pa
   size_t ix=0;
   size_t jx=0;
   for( ; ix < in.varSz ; ++ix )
   {
     Variable &var = in.variable[ix];

     if( var.name == "plev")
     {
       for( jx=0 ; jx < var.attName.size() ; ++jx )
       {
         if( var.attName[jx] == pQA->n_units )
         {
            if( var.attValue[jx][0] != "Pa" )
              return;  // is annotated elsewhere;
                       // a check of a value is meaningless
         }
       }

       break;
     }
   }

   Variable &var = in.variable[ix];

   in.nc.getData(tmp_mv, var.name, 0, -1);
   if( tmp_mv.size() )
   {
     std::string pData( hdhC::double2String(tmp_mv[0]) );

     if( pData != pVarname )
     {
       std::string key("5_4");
       if( notes->inq( key, in.variable[ix].name ) )
       {
         std::string capt(hdhC::tf_var("plev", hdhC::colon));
         capt += "Data value does not match Pa units" ;

         std::string text("Found " + hdhC::tf_assign("p", pData));
         text += ", expected " + hdhC::tf_assign("p", pVarname);

         (void) notes->operate(capt, text) ;
         notes->setCheckStatus("CV", pQA->n_fail);
       }
     }
   }

   return;
}

void
QA_Exp::checkTrackingID(std::string& rV, std::string& aV)
{
    if( rV.size() == 0 && aV.size() == 0 )
        return ;

    int is=0;

    if( rV.size() )
    {
        if( aV.size() )
        {
            if( aV.size() < rV.size() )
            {
                if( aV.substr(rV.size()) == rV )
                   is=1; //prefix does not match
            }
        }
        //else
        //  is=2; // missing   tested elsewhere
    }

    if( !is)
    {
       // check the uuid format, not the value; take into
       // account a prefix given by rV
       if( (rV.size()+36) != aV.size() )
         is=2;
       else
       {
           // check position of '-'
           std::vector<size_t> pos;
           pos.push_back(rV.size()+9);
           pos.push_back(rV.size()+14);
           pos.push_back(rV.size()+19);
           pos.push_back(rV.size()+24);

           for( size_t i=0 ; i < pos.size() ; ++i )
           {
              if( aV[pos[i]] != '-' )
              {
                  is=2;
                  break;
              }
           }
       }
    }

    if(is)
    {
        std::string key("2_12");
        if( notes->inq( key, pQA->s_global) )
        {
            std::string capt(pQA->s_global);
            capt += hdhC::blank;
            capt += hdhC::tf_att(hdhC::empty, n_tracking_id, aV);

            if( is == 1 )
                capt += " does not match requested prefix" + hdhC::tf_val(rV);
            else if( is == 2 )
                capt += " with ill-formatted uuid";

            (void) notes->operate(capt) ;
            notes->setCheckStatus("CV", pQA->n_fail );
        }
    }

    return ;
}


void
QA_Exp::domainCheck(void)
{
   std::vector<std::vector<std::string> > table1;
   std::vector<std::vector<std::string> > table2;

   size_t sz = pQA->drs_cv_table.section.size() ;
   std::string t1("Table1");
   std::string t2("Table2");

   Split x_line;
   x_line.setSeparator(',') ;
   x_line.setStripSides();

   for( size_t i=0 ; i < sz ; ++i )
   {
     if( pQA->drs_cv_table.section[i] == t1 )
     {
        for( size_t j=0 ; j < pQA->drs_cv_table.line[i].size() ; ++j )
        {
          x_line = pQA->drs_cv_table.line[i][j] ;
          table1.push_back( x_line.getItems() ) ;
        }
     }

     if( pQA->drs_cv_table.section[i] == t2 )
     {
        for( size_t j=0 ; j < pQA->drs_cv_table.line[i].size() ; ++j )
        {
          x_line = pQA->drs_cv_table.line[i][j] ;
          table2.push_back( x_line.getItems() ) ;
        }
     }
   }

   std::string s0;
   int ix_tbl1, ix_tbl2;

   std::string t_lon ;
   std::string t_lat ;
   std::string f_lonName;
   std::string f_latName;

   domainFindTableType(table1, table2, ix_tbl1, ix_tbl2) ;

   // name
   std::string domName ;
   bool isTbl_1st=false;
   bool isTbl_2nd=false;
   size_t irow;

   if( ix_tbl1 > -1 && ix_tbl2 == -1 )
   {
     isTbl_1st=true;
     irow = static_cast<size_t>( ix_tbl1 );
   }
   else if( ix_tbl1 ==-1 && ix_tbl2 > -1 )
   {
     isTbl_2nd=true;
     irow = static_cast<size_t>( ix_tbl2 );
   }

   if( ! isTbl_1st && ! isTbl_2nd )
   {
     // no valid Name was found at all,
     // so we don't know which Table to use.
     // Try to find out.
     int table_id;

     if( domainFindTableTypeByRange(table1, table2, table_id, irow) )
     {
        if( table_id == 1 )
          isTbl_1st = true;
        else if( table_id == 2 )
          isTbl_2nd = true;
     }
   }

   if( isTbl_1st )
   {
     // Note: if the data file is produced by a model not using rotated
     // coordinates, then the checks will be discarded.
     // The algorithm below works in the case of faults.
     bool is=true;

     for( size_t i=0 ; i < pQA->pIn->varSz ; ++i )
     {
       Variable& var = pQA->pIn->variable[i] ;

       // check variable name for key-word 'rotated' and 'pole'
       if( var.name.find("rotated") < std::string::npos
              &&  var.name.find("pole") < std::string::npos )
       {
          is=false;
          break;
       }

       // check attribute values for key-word 'rotated' and 'pole'
       for( size_t j=0 ; j < var.attValue.size() ; ++j )
       {
         if( var.attValue[j][0].find("rotated") < std::string::npos
              &&  var.attValue[j][0].find("pole") < std::string::npos )
         {
            is=false;
            break;
         }
       }

       if( !is )
         break;
     }

     // assumption that the model is non-rotational
     if( is )
        return;

     // check dimensions
     t_lon = table1[irow][5];
     t_lat = table1[irow][6];

     domainCheckDims("Nlon", t_lon, f_lonName, "1") ;
     domainCheckDims("Nlat", t_lat, f_latName, "1") ;

     t_lon = table1[irow][3];
     t_lat = table1[irow][4];

     domainCheckPole("NP lon", t_lon, f_lonName);
     domainCheckPole("NP lat", t_lat, f_latName);

     if( f_lonName.size() && f_latName.size() )
       domainCheckData(f_lonName, f_latName, table1[irow], "Table 1");

     return ;
   }

   if( isTbl_2nd )
   {
     isRotated=false;

     // check dimensions
     t_lon = table2[irow][3];
     t_lat = table2[irow][4];

     domainCheckDims("Nlon", t_lon, f_lonName, "2") ;
     domainCheckDims("Nlat", t_lat, f_latName, "2") ;

     // no pole check

     if( f_lonName.size() && f_latName.size() )
       domainCheckData(f_lonName, f_latName, table2[irow], "Table 2");

     return ;
  }

  // used domain name not found in Table 1 or 2
  std::string key = "7_6";
  if( notes->inq(key, pQA->fileStr) )
  {
    std::string capt("domain does not match Table ") ;
    if( isTbl_1st )
      capt += " 1";
    else
      capt += " 2";
    capt += " of the CORDEX Archive Design";

    (void) notes->operate(capt) ;
    notes->setCheckStatus("CV", pQA->n_fail);
  }

  return ;
}

bool
QA_Exp::domainFindTableTypeByRange(
   std::vector<std::vector<std::string> > &T1,
   std::vector<std::vector<std::string> > &T2,
   int &table_id, size_t &row )
{
   // Try to identify table and row  by data properties.

   // Identify a target variable and get its dimensions.
   // Collect all candidates for being (r)lon or (r)lat.
   std::vector<std::string> cName;
   std::vector<size_t> cSize;

   for(size_t i=0 ; i < pQA->pIn->varSz ; ++i )
   {
      if( pQA->pIn->variable[i].isDATA )
      {
         for(size_t j=0 ; j < pQA->pIn->variable[i].dimName.size() ; ++j )
         {
           std::string &name = pQA->pIn->variable[i].dimName[j];

           for(size_t k=0 ; k < pQA->pIn->varSz ; ++k )
           {
              if( pQA->pIn->variable[k].name == name )
              {
                if( pQA->pIn->variable[k].isUnlimited() )
                  break;

                if( pQA->pIn->variable[k].coord.isCoordVar )
                {
                  cName.push_back( name );  // is a candidate
                  cSize.push_back( pQA->pIn->nc.getDimSize(name) ) ;
                }

                break;
              }
           }
         }
      }
   }

   // remove duplicates
   std::vector<std::string> candidate;
   std::vector<std::string> candidate_sz;

   for( size_t i=0 ; i < cName.size() ; ++i )
   {
     size_t j;
     for( j=0 ; j < candidate.size() ; ++j )
       if( cName[i] == candidate[j] )
         break;

     if( j == candidate.size() )
     {
       candidate_sz.push_back( hdhC::itoa(
                      static_cast<int>(cSize[i] ) ) );
       candidate.push_back( cName[i] ) ;
     }
   }

   if( candidate.size() == 0 )
     return false;  // no candidate found at all; no further checking
              // is possible.

   // the combination of Nlon and Nlat in Table 1 and 2 is
   // an unambiguous indicator for table and row.

   // search in Table1
   for( size_t l=0 ; l < T1.size() ; ++l )
   {
     for( size_t i=0 ; i < candidate_sz.size() ; ++i )
     {
       if( T1[l][5] != candidate_sz[i] )
         continue; //no match

       for( size_t j=0 ; j < candidate_sz.size() ; ++j )
       {
         if( T1[l][6] == candidate_sz[j] )
         {
           // found a candidate for a matching row in Tables 1
           table_id = 1 ;
           row = l ;
           return true;
         }
       }
     }
   }

   // search in Table2
   for( size_t l=0 ; l < T2.size() ; ++l )
   {
     for( size_t i=0 ; i < candidate_sz.size() ; ++i )
     {
       if( T2[l][3] != candidate_sz[i] )
         continue; //no match

       for( size_t j=0 ; j < candidate_sz.size() ; ++j )
       {
         if( T2[l][4] == candidate_sz[j] )
         {
           // found a candidate for a matching row in Tables 2
           table_id = 2 ;
           row = static_cast<int>(l) ;
           return true;
         }
       }
     }
   }

   return false;
}

void
QA_Exp::domainCheckData(std::string &var_lon, std::string &var_lat,
    std::vector<std::string> &row, std::string tName)
{
  // compare values of lat/lon specified in CORDEX Table 1 with
  // corresponding values of the file.

  size_t add=0 ;
  if( tName == "Table 1" )
    add=2 ;

  // get data
  MtrxArr<double> mv_lon;
  MtrxArr<double> mv_lat;
  pQA->pIn->nc.getData(mv_lon, var_lon);
  pQA->pIn->nc.getData(mv_lat, var_lat);

  if( mv_lon.size() < 2 || mv_lat.size() < 2 )
  {
    std::string key = "7_9";
    if( notes->inq(key, pQA->fileStr) )
    {
      std::string capt("CORDEX domain" + hdhC::tf_val(tName)) ;
      capt += " with missing data for ";

      if( mv_lon.size() < 2 && mv_lat.size() < 2 )
      {
        capt += hdhC::tf_var(var_lon) ;
        capt += "and " + hdhC::tf_var(var_lat);
      }
      else if( mv_lon.size() < 2 )
        capt += hdhC::tf_var(var_lon) ;
      else
        capt += hdhC::tf_var(var_lat) ;

      (void) notes->operate(capt) ;
      notes->setCheckStatus("CV", pQA->n_fail);
    }

    return;
  }

  // check resolution (with rounding)

  // resolution from table
  std::string t_resol(hdhC::double2String(
      hdhC::string2Double( row[2]), -5) ) ;

  std::string f_resol_lon;
  bool is_lon=false;
  if( mv_lon.size() > 1 )
  {
     f_resol_lon = hdhC::double2String( (mv_lon[1] - mv_lon[0]), -5);
     if( f_resol_lon != t_resol )
       is_lon=true;
  }
  else
    is_lon=true;

  std::string f_resol_lat;
  bool is_lat=false;
  if( mv_lat.size() > 1 )
  {
     f_resol_lat = hdhC::double2String( (mv_lat[1] - mv_lat[0]), -5);
     if( f_resol_lat != t_resol )
       is_lat=true;
  }
  else
    is_lat=true;

  if( is_lon || is_lat )
  {
    std::string key = "7_10";
    if( notes->inq(key, pQA->fileStr) )
    {
      std::string capt("resolution of CORDEX ");
      capt += hdhC::tf_assign("domain", tName) ;
      capt += " does not match";

      std::string text("Found") ;
      if( is_lon && is_lat )
      {
        text += hdhC::tf_val(f_resol_lon) ;
      }
      else if( is_lon )
      {
        text += hdhC::tf_val(f_resol_lon) ;
        text += " for " ;
        text += var_lon ;
      }
      else
      {
        text += hdhC::tf_val(f_resol_lat) ;
        text += " for " ;
        text += var_lat ;
      }

      text += ", required is "  + t_resol;

      (void) notes->operate(capt, text) ;
      notes->setCheckStatus("CV", pQA->n_fail);
    }
  }

  // check edges of the domain for grid-cell centres
  // vs. the boundaries from file data. Note: file data range may
  // be enlarged.
  double edge_file[4];
  double edge_row[4];

  // false: not misaligned
  bool is_edge[] = {false, false, false, false};

  {
     int j = pQA->pIn->getVarIndex(var_lon) ;
     Variable &var = pQA->pIn->variable[j];

     std::string units(pQA->pIn->nc.getAttString(pQA->n_units, var.name) );

     size_t i_1st  = 0;
     size_t i_last = mv_lon.size()-1;
     if( units == "degrees_south" )
     {
        i_1st = i_last;
        i_last= 0;
     }

     edge_file[0] = mv_lon[i_1st] ;
     edge_row[0] = hdhC::string2Double(row[5+add]);

     int precision=5;

     is_edge[0] = hdhC::compare(edge_file[0], "<=", edge_row[0], precision)
                     ? false : true;

     edge_file[1] = mv_lon[i_last] ;
     edge_row[1] = hdhC::string2Double(row[6+add]);

     is_edge[1] = hdhC::compare(edge_file[1], ">=", edge_row[1], precision)
                     ? false : true;
  }

  {
     int j = pQA->pIn->getVarIndex(var_lat) ;
     Variable &var = pQA->pIn->variable[j];

     std::string units( var.getAttValue(pQA->n_units) );

     size_t i_1st  = 0;
     size_t i_last = mv_lat.size()-1;
     if( units == "degrees_west" )
     {
        i_1st = i_last;
        i_last= 0;
     }

     int precision=5;

     edge_file[2] = mv_lat[i_1st] ;
     edge_row[2] = hdhC::string2Double(row[7+add]);

     is_edge[2] = hdhC::compare(edge_file[2], "<=", edge_row[2], precision)
                     ? false : true;

     edge_file[3] = mv_lat[i_last] ;
     edge_row[3] = hdhC::string2Double(row[8+add]);

     is_edge[3] = hdhC::compare(edge_file[3], ">=", edge_row[3], precision)
                     ? false : true;
  }

  std::string text1;
  std::string text2;
  bool is=false;
  bool isComma=false;
  for( size_t i=0 ; i < 4 ; ++i )
  {
    if( is_edge[i] )
    {
      std::string wesn;

      if( i==0 )
      {
        wesn="West";
        text1 += wesn + "=";  // required
        text1 += row[5+add] ;
        text2 += hdhC::tf_assign(wesn,hdhC::double2String(edge_file[0])) ;
        isComma=true;
      }
      else if( i==1 )
      {
        if(isComma)
        {
          text1 += ", ";
          text2 += ", ";
        }

        wesn="East";
        text1 += wesn + "=";  // required
        text1 += row[6+add] ;
        text2 += hdhC::tf_assign(wesn, hdhC::double2String(edge_file[1])) ;
        isComma=true;
      }

      if( i==2 )
      {
        if(isComma)
        {
          text1 += ", ";
          text2 += ", ";
        }

        wesn="South";
        text1 += wesn + "=" ;  // required
        text1 += row[7+add] ;
        text2 += hdhC::tf_assign(wesn,hdhC::double2String(edge_file[2])) ;
        isComma=true;
      }
      else if( i==3 )
      {
        if(isComma)
        {
          text1 += ", ";
          text2 += ", ";
        }

        wesn="North";
        text1 += wesn + "=" ;  // required
        text1 += row[8+add] ;
        text2 += hdhC::tf_assign(wesn,hdhC::double2String(edge_file[3])) ;
        isComma=true;
      }

      is = true;
    }
  }

  if( is )
  {
    std::string key = "7_11";
    if( notes->inq(key, pQA->fileStr) )
    {
      std::string capt("unmatched CORDEX boundaries for the ");
      capt += tName ;
      capt += " domain";

      std::string text("Found ") ;
      text += text2;
      text += ", required " ;
      text += text1;

      (void) notes->operate(capt, text) ;
      notes->setCheckStatus("CV", pQA->n_fail);
    }
  }

  return ;
}

void
QA_Exp::domainCheckDims(std::string item,
    std::string &t_num_str, std::string &f_name, std::string tbl_id)
{
  // compare lat/lon specified in CORDEX Table 1 or 2 with the dimensions
  // and return corresponding name.
  int f_num=-1;
  int t_num = hdhC::string2Double(t_num_str);

  // names of all dimensions
  std::vector<std::string> dNames(pQA->pIn->nc.getDimName() );

  for( size_t i=0 ; i < dNames.size() ; ++i )
  {
     int j = pQA->pIn->getVarIndex(dNames[i]);
     if( j == -1 )
       continue;

     Variable& var = pQA->pIn->variable[j] ;
     if( item == "lon" || item == "Nlon" )
       if( ! (var.coord.isCoordVar && var.coord.isC[0]) )
        continue;
     if( item == "lat" || item == "Nlat")
       if( ! (var.coord.isCoordVar && var.coord.isC[1]) )
        continue;

     f_num = pQA->pIn->nc.getDimSize(dNames[i]);

     f_name = dNames[i] ;

     if( f_num < t_num )
     {
       std::string key = "7_7";
       if( notes->inq(key, pQA->fileStr) )
       {
         std::string capt("CORDEX domain Table ") ;
         capt += tbl_id ;
         capt += ": Dimension";
         capt += hdhC::tf_val(f_name) ;
         capt += " does not match the grid definition";

         std::string text("Found") ;
         text += hdhC::tf_val(hdhC::itoa(f_num)) ;
         text += ", required" ;
         text += hdhC::tf_val(t_num_str);

         (void) notes->operate(capt, text) ;
         notes->setCheckStatus("CV", pQA->n_fail);
       }
     }
  }

  return ;
}

void
QA_Exp::domainCheckPole(std::string item,
    std::string &t_num, std::string &f_name)
{
  // compare lat/lon of N. Pole  specified in CORDEX Table 1 with
  // corresponding values in the NetCF file.

  std::string f_num;

  bool isItem = item == "NP lon" ;

  if( isItem && t_num == "N/A" )
    return;  // any lon value would do for the North pole in rotated coord.

  // remove trailing zeros and a decimal point
  t_num = hdhC::double2String( hdhC::string2Double(t_num), -5) ;

  // find grid-mapping variable with
  // grid_mapping_name==rotated_latitude_longitude
  size_t ix;
  int jx = -1;

  bool is=false;

  for(size_t i=0 ; i < pQA->pIn->varSz ; ++i )
  {
    Variable& var = pQA->pIn->variable[i] ;

    if(isItem)
    {
      if( (jx=var.getAttIndex("grid_north_pole_longitude")) > -1 )
      {
        ix=i;
        is=true;
        break;
      }
    }
    else
    {
      if( (jx=var.getAttIndex("grid_north_pole_latitude")) > -1 )
      {
        ix=i;
        is=true;
        break;
      }
    }
  }

  if(is)
  {
    Variable &var = pQA->pIn->variable[ix];

    if(isItem)
    {
      if( jx > -1 )
      {
        std::string& s = var.attValue[jx][0] ;

        if( s.size() )
        {
          // found the attribute
          f_num = hdhC::double2String( hdhC::string2Double(s), -5) ;

          if( isItem && t_num == "N/A" )
            t_num = f_num;  // any lon value would do for the North pole in rotated coord.

          if( f_num == t_num )
          {
            if( !f_name.size() )
              f_name=var.name;

            return;
          }
        }
      }
    }
    else
    {
      if( jx > -1 )
      {
        std::string& s = var.attValue[jx][0] ;

        if( s.size() )
        {
          // found the attribute
          f_num = hdhC::double2String( hdhC::string2Double(s), -5) ;
          if( f_num == t_num )
          {
            if( !f_name.size() )
              f_name=var.name;

            return;
          }
        }
      }
    }

    // may-be the names didn't match.
    std::string key = "7_8";
    if( notes->inq(key, pQA->fileStr) )
    {
      size_t item_start=0;

      if( item.substr(0,3) == "NP " )
          item_start = 3;

      std::string capt("rotated N.Pole of CORDEX domain Table 1 does not match");
      std::string text("Found ");
      text += hdhC::tf_assign(item.substr(item_start), f_num);
      text += ", required" ;
      text += hdhC::tf_val(t_num);

      (void) notes->operate(capt, text) ;
      notes->setCheckStatus("CV", pQA->n_fail);
    }
  }

  return ;
}

void
QA_Exp::domainFindTableType(
    std::vector<std::vector<std::string> > &tbl1,
    std::vector<std::vector<std::string> > &tbl2,
    int &ix_1, int &ix_2)
{
   // CORDEX Area: table vs. file properties.
   // Return the index if it is unambiguous.
   std::string domName;

   // guess the format of the domain: rotated vs. back-rotated

   std::vector<std::string> candidate ;

   // a) the domain is given as attribute
   candidate.push_back( (pQA->pIn->getAttValue("CORDEX_domain")) ) ;

   // b) domain name from the filename, e.g. EUR-11
   Split fd(pQA->pIn->file.getFilename(), "_");
   if( fd.size() > 1 )
      candidate.push_back( fd[1] );
   else
      candidate.push_back("");

   // looking for a valid index in the two cases a) and/or b)
   int sz[2] ;
   sz[0] = static_cast<int>(tbl1.size());
   sz[1] = static_cast<int>(tbl2.size());

   int it_1[2] ; // index for the row of table 1; two cases
   int it_2[2] ;

   // don't confuse index of sz and it_1, it_2

   for( size_t i=0 ; i < 2 ; ++i )
   {
     if( i==1 && candidate[0] == candidate[1] )
     {
       it_1[1] = it_1[0] ;
       it_2[1] = it_2[0] ;
       break;
     }

     // looking for a valid index in table1
     for( it_1[i]=0 ; it_1[i] < sz[0] ; ++it_1[i] )
        if( candidate[i] == tbl1[it_1[i]][1] )
          break;

     if( it_1[i] == sz[0])  // no match found
     {
       it_1[i] = -1;

       // looking for a valid index in table2
       for( it_2[i]=0 ; it_2[i] < sz[1] ; ++it_2[i] )
          if( candidate[i] == tbl2[it_2[i]][1] )
            break;

       if( it_2[i] == sz[1])  // no match found
         it_2[i] = -1 ;
     }
     else
       it_2[i] = -1;
   }

   if( (it_1[0] == it_1[1]) && (it_2[0] == it_2[1] ) )
   {
      if( it_1[0] > -1 && it_2[0] == -1 )
      {
        ix_1 = it_1[0] ;
        ix_2 = -1;
        return  ; // found unambiguous name in table 1
      }

      if( it_1[0] == -1 && it_2[0] > -1 )
      {
        ix_1 = -1 ;
        ix_2 = it_2[0];
        return  ; // found unambiguous name in table 2
      }
   }

   ix_1 = -1 ;
   ix_2 = -1 ;

  return ;
}

void
QA_Exp::checkHeightValue(InFile &in)
{
   // Check near-surface height value between 0 - 10m
   // Note that a variable height may be available for something differnt,
   // e.g. TOA

   // find pattern 'near-surface' in the long_name. If long_name is missing,
   // then a false fault could be raised for height value

   int ix;
   if( (ix=pQA->pIn->getVarIndex("height")) == -1 )
     return;

   Variable &var = in.variable[ix];

   // check the units
   std::string units(var.getAttValue(pQA->n_units));

   if( units.size() == 0 )
     return;  // annotated elsewhere

   if( units != "m" )
     return;  // annotated elsewhere

   std::string longName(var.getAttValue(pQA->n_long_name));

   // try for a variable height and a variable with pattern nearXsurface
   // in lon_name where X means space, - or _
   Split x_longName;
   x_longName.setSeparator(" -_", true);
   x_longName = hdhC::Lower()(longName) ;

   bool is=false;
   for( size_t i=0 ; i < x_longName.size()-1 ; ++i )
   {
     if( x_longName[i] == "near" && x_longName[i+1] == "surface" )
     {
       is=true;
       break;
     }
   }

   if( is )
   {
     is=false;
     in.nc.getData(tmp_mv, var.name, 0, -1);
     if( tmp_mv.size() )
     {
       if( tmp_mv[0] < 0. || tmp_mv[0] > 10. )
         is=true ;
     }
     else
       is=true;

     if( is )
     {
       std::string key("5_6");
       if( notes->inq( key, var.name ) )
       {
         std::string capt(hdhC::tf_var("height") + "requires a value [0-10]m") ;
         std::string text;
         if( tmp_mv.size() )
         {
           text= "Found" ;
           text += hdhC::tf_val( hdhC::double2String( tmp_mv[0]) ) ;
         }

         (void) notes->operate( capt, text) ;
         notes->setCheckStatus("CV", pQA->n_fail);
       }
     }
   }

   return;
}

void
QA_Exp::checkMetaData(InFile &in)
{
  domainCheck();

  // check attributes required in the meta data section of the file
  reqAttCheck() ;

   getSubTable() ;


  // check existance (and data) of the pressure coordinate for those
  // variables defined on a level indicated by a trailing number
  checkPressureCoord(in);

  // check basic properties between this file and
  // requests in the table. When a MIP-subTable is empty, use the one
  // from the previous instance.
  std::vector<struct DimensionMetaData> dimNcMeDa;

  if(varReqTable.is() )
  {
    for( size_t i=0 ; i < varMeDa.size() ; ++i )
    {
      // Scan through the standard table.
      if( ! varMeDa[i].var->isExcluded )
        varReqTableCheck(in, varMeDa[i], dimNcMeDa) ;
    }
  }

  // inquire whether the meta-data checks passed
  int ev;
  if( (ev = notes->getExitState()) > 1 )
    pQA->setExitState( ev );

  return;
}

void
QA_Exp::createVarMetaData(void)
{
  // sub table name, i.e. frequency, has previously been checked

  // create instances of VariableMetaData. These have been identified
  // previously at the opening of the nc-file and marked as
  // Variable::VariableMeta(Base)::isDATA == true. The index
  // of identified targets is stored in vector in.dataVarIndex.

  size_t i;
  for( i=0 ; i < pQA->pIn->dataVarIndex.size() ; ++i )
  {
    Variable &var = pQA->pIn->variable[pQA->pIn->dataVarIndex[i]];

    //push next instance
    pushBackVarMeDa( &var );

    VariableMetaData &vMD = varMeDa.back() ;

    for( size_t k=0; k < var.dimName.size() ; ++k)
    {
      int sz;
      if( ! pQA->pIn->nc.isVarUnlimited(var.name) &&
            (sz=pQA->pIn->nc.getDimSize( var.dimName[k] )) == 1 )
      {
        std::string key("4_7");
        if( notes->inq( key, var.name) )
        {
          std::string capt(hdhC::tf_var(var.getDimNameStr(true), hdhC::colon));
          capt += "CORDEX favours a scalar variable for dimension " ;

          for( size_t l=0 ; l < var.dimName.size() ; ++l )
          {
             if( pQA->pIn->nc.getDimSize(var.dimName[l]) == 1 )
             {
               capt += var.dimName[l] ;
               capt += "=1";
               break;
             }
          }

          (void) notes->operate(capt) ;
          notes->setCheckStatus("CV", pQA->n_fail );
        }
      }
    }

    // some more properties
    vMD.attMap[n_long_name]     = var.getAttValue(pQA->n_long_name) ;
    vMD.attMap[n_positive]      = var.getAttValue(pQA->n_positive) ;
  }

  if( pQA->pIn->dataVarIndex.size() > 1 )
  {
     std::string key("9_1");
     if( notes->inq( key, pQA->fileStr) )
     {
       std::string name = getVarnameFromFilename() ;

       std::string capt(hdhC::tf_var(name, hdhC::colon));
       capt += " Multiple data variables are present";

       std::string text("Found also ");

       size_t count=0;
       for( size_t j=0 ; j < pQA->pIn->dataVarIndex.size() ; ++j )
       {
         Variable &var = pQA->pIn->variable[pQA->pIn->dataVarIndex[j]];

         if( var.name == name )
             continue;

         if(count)
           text += ", ";
         else
             ++count;

         text += var.name;
       }

       (void) notes->operate(capt, text) ;
       notes->setCheckStatus("CV", pQA->n_fail );
     }
  }

  return;
}

void
QA_Exp::checkVarTableEntry(
    VariableMetaData &vMD,
    VariableMetaData &t_DMD_entry)
{
  // Do the variable properties found in the netCDF file
  // match those in the table (standard or project)?
  // Exceptions: priority is not defined in the netCDF file.

  // This has already been tested in the calling method
  // if( t_DMD_entry.name != varMeDa.name )

  checkVarTableEntry_standardName(vMD, t_DMD_entry);
  checkVarTableEntry_longName(vMD, t_DMD_entry);
  checkVarTableEntry_units(vMD, t_DMD_entry);
  checkVarTableEntry_cell_methods(vMD, t_DMD_entry);
  checkVarTableEntry_name(vMD, t_DMD_entry);

  return;
}

void
QA_Exp::checkVarTableEntry_cell_methods(
    VariableMetaData &vMD,
    VariableMetaData &t_DMD_entry)
{
  // many CORDEX data are derived on the basis of cell_methods, given in a former
  // version of CORDEX_variables_requirements_table, which does not comply to CMIP5.
  // The current table provides two variants of cell_methods: the former and the CMIP5 alike
  // called 'cell_methods (2nd option)' in the caption of the table.

  // Rule: If a file is based on the former variant of cell_methods, then this is only checked.
  // Does the file contain more words than given in the former table, then the check
  // has to pass the CMIP5 compliant variant.

  // the direct comparison be the total string is discarded, because,
  // the 'time: point' possibiliity is checked below

  if( frequency == "fx" )
    return;

  // disjoint into words; this method ingnores spaces
  Split x_t_cm(t_DMD_entry.attMap[n_cell_methods]);
  Split x_t_cmo(t_DMD_entry.attMap[n_cell_methods_opt]);
  Split x_f_cm;

  bool isOpt=false;
  if( x_t_cmo.size() )
      isOpt=true;

  int cm_ix = vMD.var->getAttIndex(pQA->n_cell_methods) ;

  if( cm_ix > -1 && ( x_t_cm.size() || x_t_cmo.size() ) )
  {
    // the definition of cell_methods changed for some CORDEX variables.
    // The CORDEX ADD contains two definitions. The older ones has fewer terms.
    // Here, the one is taken which matches the cm attribute in the file.
    std::string& cm_name = vMD.var->attName[cm_ix] ;
    std::string& cm_val = vMD.var->attValue[cm_ix][0];

    x_f_cm = cm_val;

    if( x_f_cm.size() == x_t_cm.size() )
    {
      size_t i;
      for( i=0 ; i < x_t_cm.size() ; ++i )
        if( x_f_cm[i] != x_t_cm[i] )
          break;

      if( i == x_t_cm.size() )
        return; // identical
    }

    if( x_f_cm.size() == x_t_cmo.size() )
    {
      size_t i;
      for( i=0 ; i < x_t_cmo.size() ; ++i )
        if( x_f_cm[i] != x_t_cmo[i] )
          break ;

      if( i == x_t_cmo.size() )
        return; // identical
    }

    // found a difference
    std::string key("3_7");
    if( notes->inq( key, vMD.var->name) )
    {
      std::string capt;
      std::string text;

      if( cm_val.size() )
      {
        capt = hdhC::tf_att(vMD.var->name, cm_name) ;
        capt += "does not match";

        text = "Found" ;
        text += hdhC::tf_val(cm_val);
        text += ", expected";
        if( isOpt )
          text += hdhC::tf_val(t_DMD_entry.attMap[n_cell_methods_opt]) ;
        else
          text += hdhC::tf_val(t_DMD_entry.attMap[n_cell_methods]) ;
      }
      // else caught in CF

      (void) notes->operate(capt, text) ;
      notes->setCheckStatus("CV", pQA->n_fail );
    }
  }

  return;
}

void
QA_Exp::checkVarTableEntry_longName(
    VariableMetaData &vMD,
    VariableMetaData &t_VMD_entry)
{
  std::string& t_VMD_longName = t_VMD_entry.attMap[n_long_name] ;
  std::string& f_VMD_longName = vMD.attMap[n_long_name] ;

  if( t_VMD_longName == f_VMD_longName )
    return;

  // tolerate case differences
  std::string f( hdhC::Lower()(f_VMD_longName) );
  std::string t( hdhC::Lower()(t_VMD_longName) );

  // case independence is accepted
  if( f == t )
    return;

  // long name is not ruled by conventions
  // Accepted variations:
  // a) '-' instead of ' '
  // b) ignore words (pre-positions) smaller than 3 character
  // c) one typo of two sequent characters per word in the long name components is tolerated
  // e) position of the words does not matter
  // d) one deviating word ok, if the number of requested words >= 3

  Split f_splt;
  Split t_splt;

  // split in a way that '-' is equivalent to a space
  f_splt.setSeparator(" -", true);
  t_splt.setSeparator(" -", true);

  t_splt = t ;
  f_splt = f ;

  size_t t_splt_sz=t_splt.size();
  size_t f_splt_sz=f_splt.size();

  int count_t=0;
  int count_f=0;

  // check words; position doesn't matter
  for( size_t i=0 ; i < t_splt_sz ; ++i )
  {
    if( t_splt[i].size() < 3 )
       continue;  // skip pre-positions

    ++count_t;

    bool is=true;
    for( size_t j=0 ; j < f_splt_sz ; ++j )
    {
      if( f_splt[j].size() < 3 )
         continue;  // skip pre-positions

      if( t_splt[i] == f_splt[j] )
      {
        ++count_f;
        is=false;
        break;
      }
    }

    if( is )  // check for one or two typos
    {
      std::string tt( t_splt[i] );
      size_t t_sz=tt.size() ;

      for( size_t j=0 ; j < f_splt_sz ; ++j )
      {
        std::string ff( f_splt[j] );
        size_t f_sz=ff.size() ;

        size_t c_left=0;
        size_t c_right=0;

        // the shorter one
        size_t fix=0;
        size_t tix=0;

        // check equality from the left
        while( fix < f_sz && tix < t_sz )
        {
           if( ff[fix++] == tt[tix++] )
             ++c_left;
           else
             break;
        }

        // check equality from the right
        fix=f_sz - 1 ;
        tix=t_sz - 1 ;
        while( fix >= 0 && tix >= 0 )
        {
           if( ff[fix--] == tt[tix--] )
             ++c_right;
           else
             break;
        }

        if( (c_left + c_right + 2) >= t_sz )
        {
          ++count_f;
          break;
        }
      }
    }
  }

  if( count_t > 3 && (count_t - count_f) < 2  )
     return;

  std::string key("3_4");
  if( notes->inq( key, vMD.var->name) )
  {
    std::string capt;

    if( f_VMD_longName.size() )
    {
      capt = hdhC::tf_att(vMD.var->name, pQA->n_long_name, f_VMD_longName) ;
      capt += "does not match required value" ;
      capt += hdhC::tf_val(t_VMD_longName);
    }
    else
    {
      capt = hdhC::tf_att(vMD.var->name, pQA->n_long_name) ;
      capt += "is missing required value" ;
      capt += hdhC::tf_val(t_VMD_longName);
    }

    (void) notes->operate(capt) ;
    notes->setCheckStatus("CV", pQA->n_fail );
  }

  return;
}

void
QA_Exp::checkVarTableEntry_name(
    VariableMetaData &vMD,
    VariableMetaData &t_DMD_entry)
{
  if( t_DMD_entry.name != vMD.var->name )
  {
    // This will happen only if case-insensitivity occurred
    std::string key("3_8");
    if( notes->inq( key, vMD.var->name) )
    {
      std::string capt(hdhC::tf_var(vMD.var->name, hdhC::colon) ) ;
      capt += "output variable name matches only for case-insensitivity" ;

      (void) notes->operate(capt) ;
      notes->setCheckStatus("CV", pQA->n_fail );
    }
  }

  return;
}

void
QA_Exp::checkVarTableEntry_standardName(
    VariableMetaData &vMD,
    VariableMetaData &t_VMD_entry)
{
  std::string& t_VMD_stdName = t_VMD_entry.attMap[n_standard_name] ;

  if( t_VMD_stdName != vMD.var->std_name
        && vMD.var->name != "evspsblpot" )
  {  // note that evspsblpot is not a standard_name definded by CF conventions
     // CORDEX_variables_requirement table
    std::string key("3_3");
    if( notes->inq( key, vMD.var->name) )
    {
      std::string capt;

      if( vMD.var->std_name.size() )
      {
        capt += hdhC::tf_att(vMD.var->name, pQA->n_standard_name, vMD.var->std_name) ;
        capt += "does not match required" ;
        capt += hdhC::tf_val(t_VMD_stdName);
      }
      else
      {
        capt += hdhC::tf_att(vMD.var->name, pQA->n_standard_name) ;
        capt += "is missing" ;
      }

      (void) notes->operate(capt) ;

      notes->setCheckStatus("CV", pQA->n_fail );
    }
  }

  return ;
}

void
QA_Exp::checkVarTableEntry_units(
    VariableMetaData &vMD,
    VariableMetaData &t_VMD_entry)
{
  std::string& t_VMD_units = t_VMD_entry.attMap[n_units] ;

  if( t_VMD_units != vMD.var->units )
  {
      std::string key("3_6");
      if( notes->inq( key, vMD.var->name) )
      {
        std::string capt( hdhC::tf_att(vMD.var->name,
               pQA->n_units, vMD.var->units, hdhC::no_blank) ) ;
        capt += ", but required is" ;
        capt += hdhC::tf_val(t_VMD_units);

        (void) notes->operate(capt) ;
        notes->setCheckStatus("CV", pQA->n_fail );
      }
  }

  return;
}

void
QA_Exp::checkVariableType(void)
{
  // check variable type; uses DRS_CV_Table
  DRS_CV_Table& table = pQA->drs_cv_table ;

  for( size_t i=0 ; i < table.varName.size() ; ++i )
  {
    std::string& tName = table.varName[i];

    for( size_t j=0 ; j < table.attName[i].size() ; ++j )
    {
      std::string& tAttName = table.attName[i][j] ;

      if( tAttName == "VAR_TYPE" )
      {
        size_t v;
        for(v=0 ; v < pQA->pIn->varSz ; ++v )
        {
          Variable& var = pQA->pIn->variable[v] ;

          if( var.name == tName )
            checkVariableTypeX(v, i, j, tName);
        }

        for(v=0 ; v < pQA->pIn->varSz ; ++v )
        {
          Variable& var = pQA->pIn->variable[v] ;

          if( var.isDATA && tName == "DATA_VAR" )
            checkVariableTypeX(v, i, j, tName);
          else if( var.isAUX && tName == "AUX_VAR" )
            checkVariableTypeX(v, i, j, tName);
        }
      }
    }
  }

  return;
}

void
QA_Exp::checkVariableTypeX(size_t v, size_t i, size_t j, std::string& tName)
{
  DRS_CV_Table& table = pQA->drs_cv_table ;

  Variable& var = pQA->pIn->variable[v] ;
  std::string& tAttValue = table.attValue[i][j] ;

  std::string s(pQA->pIn->nc.getVarTypeStr(var.name));

  if( tAttValue != s && !var.isVoid)
  {
      std::string key;
      if( tName == "DATA_VAR" )
          key="3_2a";
      else
          key="3_2b";

    if( notes->inq( key, var.name) )
    {
      std::string capt;
      if( tName == "DATA_VAR" )
        capt = "data ";
      else if( tName == "AUX_VAR" )
        capt = "auxiliary ";

      capt += hdhC::tf_var(var.name);
      capt += "has wrong data type";

      std::string text("Found");
      text += hdhC::tf_val(s);
      text += ", expected";
      text += hdhC::tf_val(tAttValue);

      (void) notes->operate(capt, text) ;
      notes->setCheckStatus("CV", pQA->n_fail );
    }
  }

  return;
}

bool
QA_Exp::findTableEntry(ReadLine &ifs, std::string &name_f,
     size_t col_outName, std::string &str0 )
{
   Split splt_line;
   splt_line.setSeparator(',');
   splt_line.enableEmptyItems();

   std::string name_t;

   do
   {
     splt_line=str0;

     if( splt_line.size() > 1 && splt_line[1] == "Table:" )
       return false;

     name_t = splt_line[col_outName] ;

     if( name_t == name_f )
       return true;
   }
   while( ! ifs.getLine(str0) ) ;

   // netCDF variable not found in the table
   return false;
}

bool
QA_Exp::findTableEntry(ReadLine &ifs, std::string &name_f,
   VariableMetaData &t_VMD_entry)
{
   // return true: entry is not the one we look for.

   std::map<std::string, size_t> col;

   Split splt_line;
   splt_line.setSeparator(',');
   splt_line.enableEmptyItems();

   std::string name_t;
   std::string str0;

   bool isFound=false;

   std::vector<std::string> vs_freq;
   vs_freq.push_back(frequency);
   vs_freq.push_back("all");

   bool isRewind=true;

   for(size_t i=0 ; i < vs_freq.size() ; ++i )
   {
     if( readTableCaptions(ifs, vs_freq[i], col, str0) )
     {
       if( (isFound = findTableEntry(ifs, name_f, col["outVarName"], str0) ) )
       {
         splt_line = str0 ;

         // delete footnotes from the table entries
         std::string str1;
         for(size_t j=0 ; j < splt_line.size() ; ++j )
         {
            std::string &s = splt_line[j] ;
            size_t sz = s.size();
            if( sz && s[sz-1] == ')' && isdigit(s[sz-2]) )
            {
              --sz;
              while( sz && isdigit(s[sz-1]) )
                --sz;
            }

            str1 += s.substr(0, sz) ;
            str1 += ',' ;
         }

         splt_line = str1;
         // found an entry and the shape matched.
         if( t_VMD_entry.name.size() == 0 &&
                col.find("outVarName") != col.end() )
             t_VMD_entry.name = splt_line[ col["outVarName"] ];

         if( t_VMD_entry.attMap.count(n_standard_name) == 0
                && col.find(pQA->n_standard_name) != col.end() )
             t_VMD_entry.attMap[n_standard_name] = splt_line[ col[pQA->n_standard_name] ];

         if( t_VMD_entry.attMap.count(n_long_name) == 0
                && col.find(pQA->n_long_name) != col.end() )
         {
             t_VMD_entry.attMap[n_long_name] = splt_line[ col[pQA->n_long_name] ];
             t_VMD_entry.attMap[n_long_name]
                = hdhC::unique(t_VMD_entry.attMap[n_long_name], ' ');
         }

         if( t_VMD_entry.attMap.count(n_units) == 0 &&
                col.find(pQA->n_units) != col.end() )
         {
             t_VMD_entry.attMap[n_units] = splt_line[ col[pQA->n_units] ];
             t_VMD_entry.attMap[n_units]
                = hdhC::unique(t_VMD_entry.attMap[n_units], ' ');

             if( t_VMD_entry.attMap[n_units].size() )
               t_VMD_entry.isUnitsDefined=true;
             else
               t_VMD_entry.isUnitsDefined=false;
         }

         if( t_VMD_entry.attMap.count(n_positive) == 0 &&
                col.find(pQA->n_positive) != col.end() )
                   t_VMD_entry.attMap[n_positive] = splt_line[ col[pQA->n_positive] ];

         if( t_VMD_entry.attMap.count(n_cell_methods) == 0 &&
                col.find(pQA->n_cell_methods) != col.end() )
                   t_VMD_entry.attMap[n_cell_methods] = splt_line[ col[pQA->n_cell_methods] ];

         if( t_VMD_entry.attMap.count(n_cell_methods_opt) == 0 &&
                col.find("cell_methods_opt") != col.end() )
                   t_VMD_entry.attMap[n_cell_methods_opt] = splt_line[ col["cell_methods_opt"] ];
       }
       else
         return isFound ; //== false
     }
     else
     {
       // if 'all' precedes frequency
       if( isRewind )
       {
          // do it again
          ifs.rewind();
          --i;
          isRewind=false;
       }
     }
   }

   return isFound;
}

std::string
QA_Exp::getCaptIntroDim(VariableMetaData &vMD,
                   struct DimensionMetaData &f_DMD_entry,
                   struct DimensionMetaData &t_DMD_entry,
                   std::string att )
{
  std::string& t_DMD_outname = t_DMD_entry.attMap[n_outname] ;

  std::string intro("var=");
  intro += vMD.var->name + ", dim=";
  intro += f_DMD_entry.attMap[n_outname] ;

  if( t_DMD_outname == "basin" )
    intro += ", var-rep=region";
  if( t_DMD_outname == "line" )
    intro += ", var-rep=passage";
  if( t_DMD_outname == "type" )
    intro += ", var-rep=type_description";

  if( att.size() )
  {
    intro += "att=" ;
    intro += att + ", " ;
  }

  return intro;
}

bool
QA_Exp::getDimMetaData(InFile &in,
      VariableMetaData &vMD,
      struct DimensionMetaData &t_DMD_entry,
      std::string &dName)
{
  // return 0:
  // collect dimensional meta-data in the struct.

  // Note: this method is called from two different spots, from
  // a standard table check and for project purposes.

  // pre-set
  t_DMD_entry.checksum=0;
  t_DMD_entry.attMap[n_outname]=dName;

  // dName is a dimension name from the table. Is it also
  // a dimension in the ncFile? A size of -1 indicates: no
  int sz = in.nc.getDimSize(dName);
  if( sz == -1 )
     return true;
  t_DMD_entry.size = static_cast<size_t>(sz);

  // regular: variable representation of the dim
  // except.: variable specified in coords_attr and not a regular case

  // is dimension name also variable name?
  if( in.nc.getVarID( dName ) == -2 )
     return true;  // nothing was found

  // attributes from the file
  for(size_t l=0 ; l < vMD.var->dim_ix.size() ; ++l)
  {
    Variable &var = pQA->pIn->variable[vMD.var->dim_ix[l]];

    if( var.name != dName )
       continue;

    for( size_t j=0 ; j < var.attName.size() ; ++j)
    {
      std::string &aN = var.attName[j] ;
      std::string &aV = var.attValue[j][0] ;

      if( aN == "bounds" )
          t_DMD_entry.attMap[n_bnds_name] = aV ;
      else if( aN == "climatology" && aN == pQA->qaTime.name )
          t_DMD_entry.attMap[n_bnds_name] = aV ;
      else if( aN == pQA->n_units )
      {
          if( var.isUnitsDefined )
          {
            t_DMD_entry.attMap[n_units] = aV ;
            t_DMD_entry.isUnitsDefined=true;
          }
          else
            t_DMD_entry.isUnitsDefined=false;
      }
      else if( aN == pQA->n_long_name )
          t_DMD_entry.attMap[n_long_name] = aV ;
      else if( aN == pQA->n_standard_name )
          t_DMD_entry.attMap[n_standard_name] = aV ;
      else if( aN == pQA->n_axis )
          t_DMD_entry.attMap[n_axis] = aV ;
    }
  }

  // determine the checksum of limited var-presentations of dim
  if( ! in.nc.isDimUnlimited(dName) )
  {
    std::vector<std::string> vs;

    if( in.nc.getVarType(dName) == NC_CHAR )
    {
      in.nc.getData(vs, dName);

      bool reset=true;  // is set to false during the first call
      for(size_t i=0 ; i < vs.size() ; ++i)
      {
        vs[i] = hdhC::stripSides(vs[i]);
        t_DMD_entry.checksum = hdhC::fletcher32_cmip5(vs[i], &reset) ;
      }
    }
    else
    {
      MtrxArr<double> mv;
      in.nc.getData(mv, dName, 0, -1);

      bool reset=true;
      for( size_t i=0 ; i < mv.size() ; ++i )
        t_DMD_entry.checksum = hdhC::fletcher32_cmip5(mv[i], &reset) ;
    }
  }

  if( dName == pQA->qaTime.name )
  {
    // exclude time from size
    t_DMD_entry.size = 0;

  }

  return false;
}

std::string
QA_Exp::getFrequency(void)
{
  if( frequency.size() )
    return frequency;  // already known

  // get frequency from attribute (it is required)
  frequency = pQA->pIn->nc.getAttString(pQA->n_frequency) ;

  if( frequency.size() == 0 )
  {
    // not found in the global attributes; try the filename
    std::string f( pQA->pIn->file.basename );

    Split splt(f, "_");
  }

  if( frequency == "fx" && pQA->pIn->nc.isDimUnlimited() )
  {
     std::string key("9_3");
     if( notes->inq( key, pQA->fileStr) )
     {
       std::string capt("Fixed variable with unexpected time dependency" );

       if( notes->operate(capt) )
       {
         notes->setCheckStatus("CV",pQA->n_fail);
         pQA->setExitState( notes->getExitState() ) ;
        }
     }
  }

  return frequency ;
}

void
QA_Exp::getSubTable(void)
{
  if( subTable.size() )
    return ;  // already checked

  // the counter-parts in the attributes

  //check for valid names
/*
  if( ! hdhC::isAmong(getFrequency(),  possibleFrequencies) )
  {
     std::string key("7_5");
     if( notes->inq( key, pQA->fileStr) )
     {
       std::string capt(hdhC::tf_assign(pQA->n_frequency, getFrequency()));
       capt += " not defined in the CORDEX_variables_requirement table" ;

       subTable.clear();

       if( notes->operate(capt) )
       {
         notes->setCheckStatus("CV",pQA->n_fail);
         pQA->setExitState( notes->getExitState() ) ;
        }
     }
  }
*/
  subTable=getFrequency();
  return ;
}

std::string
QA_Exp::getTableEntryID(std::string vName)
{
  vName += "," ;
  vName += getFrequency() ;
  if( !isRotated )
    vName += "_i" ;

  return vName + ",";
}

std::string
QA_Exp::getVarnameFromFilename(std::string fName)
{
  if( ! fName.size() )
     fName = getVarnameFromFilename(pQA->pIn->file.getFilename()) ;

  size_t pos;
  if( (pos = fName.find("_")) < std::string::npos )
    fName = fName.substr(0,pos) ;

  return fName;
}

void
QA_Exp::init(std::vector<std::string>& optStr)
{
   // apply parsed command-line args
   applyOptions(optStr);

   if( pQA->isCheckDRS_F || pQA->isCheckCV || pQA->isCheckTime )
   {
     fVarname = getVarnameFromFilename();
     getFrequency();
   }

   if( pQA->isCheckCV || pQA->isCheckData )
   {
     // Create and set VarMetaData objects.
     createVarMetaData() ;

     // Check varname from filename with those in the file.
     // Is the shortname in the filename also defined in the nc-header?
     std::vector<std::string> vNames( pQA->pIn->nc.getVarName() ) ;
     size_t i;
     for( i=0 ; i < vNames.size() ; ++i )
       if( fVarname == vNames[i] )
         break;

     if( i == vNames.size() )
     {
       std::string key("1_4");
       if( notes->inq( key, pQA->fileStr) )
       {
         std::string capt("variable ");
         capt += hdhC::tf_assign("acronym", fVarname);
         capt += " in the filename does not match any variable in the file" ;

         (void) notes->operate(capt) ;
         notes->setCheckStatus("DRS(F)", pQA->n_fail );
       }
     }
   }

   return;
}

void
QA_Exp::initDataOutputBuffer(void)
{
    for( size_t i=0 ; i < varMeDa.size() ; ++i)
      varMeDa[i].qaData.initBuffer(pQA, pQA->currQARec, bufferSize);

  return;
}

void
QA_Exp::initDefaults(void)
{
  isCheckParentExpID=true;
  isCheckParentExpRIP=true;
  isRotated=true;
  isUseStrict=false;

  varnamePosition=-1;

  n_axis="axis";
  n_bnds_name="bnds_name";
  n_coordinates="coordinates";
  n_index_axis="index_axis";
  n_long_name="long_name";
  n_outname="outname";
  n_standard_name="standard_name";
  n_type="type";
  n_units="units";

  n_cell_methods="cell_methods";
  n_cell_measures="cell_measures";
  n_tracking_id="tracking_id";

  bufferSize=1500;

  return;
}

void
QA_Exp::initResumeSession(std::vector<std::string>& prevTargets)
{
  if( !pQA->isCheckData )
    return;

  // a missing variable?
  for( size_t i=0 ; i < prevTargets.size() ; ++i)
  {
    size_t j;
    for( j=0 ; j < varMeDa.size() ; ++j)
      if( prevTargets[i] == varMeDa[j].var->name )
        break;

    if( j == varMeDa.size() )
    {
       std::string key("3_9");
       if( notes->inq( key, prevTargets[i]) )
       {
         std::string capt(hdhC::tf_var(prevTargets[i]));
         capt +=  + "is missing in sub-temporal file" ;

         if( notes->operate(capt) )
         {
           notes->setCheckStatus("Consistency", pQA->n_fail );
           pQA->setExitState( notes->getExitState() ) ;
         }
       }
    }
  }

  // a new variable?
  for( size_t j=0 ; j < varMeDa.size() ; ++j)
  {
    size_t i;
    for( i=0 ; i < prevTargets.size() ; ++i)
      if( prevTargets[i] == varMeDa[j].var->name )
        break;

    if( i == prevTargets.size() )
    {
       std::string key("3_10");
       if( notes->inq( key, varMeDa[j].var->name) )
       {
         std::string capt(hdhC::tf_var(varMeDa[j].var->name));
         capt += "is new in sub-temporal file" ;

         if( notes->operate(capt) )
         {
           notes->setCheckStatus("Consistency", pQA->n_fail );
           pQA->setExitState( notes->getExitState() ) ;
         }
       }
    }
  }

  return;
}

bool
QA_Exp::inqTables(void)
{
  bool ret=false;
  std::vector<std::string*> pPath;

  if( ! varReqTable.exist(varReqTable.path) )
    ret=true;

  for( size_t i=0 ; i <  pPath.size() ; ++i)
  {
     std::string key("IE_4");
     if( notes->inq( key, pQA->fileStr) )
     {
        std::string capt("no path to the tables, tried " + *(pPath[i])) ;

        if( notes->operate(capt) )
        {
          notes->setCheckStatus("QA_path", pQA->n_fail );
          pQA->setExitState( notes->getExitState() ) ;
        }
     }
  }

  return ret;
}

void
QA_Exp::pushBackVarMeDa(Variable *var)
{
   varMeDa.push_back( VariableMetaData(pQA, var) );

   if( var )
   {
     VariableMetaData &vMD = varMeDa.back();

     vMD.forkAnnotation(notes);

     // disable tests by given options
     vMD.qaData.disableTests(var->name);

     vMD.qaData.init(pQA, var->name);
   }

   return;
}

bool
QA_Exp::readTableCaptions(ReadLine &ifs, std::string freq,
   std::map<std::string, size_t> &v_col, std::string &str0 )
{
   // Each sub table is indicted by "Table:" in the second column and
   // the frequency in the third.

   // Each csv between # and a pure integer indicates a caption of a column.
   // Captions may span several lines, but not corresponding table entries.

   Split splt_line;
   splt_line.setSeparator(',');
   splt_line.enableEmptyItems();

   std::string sx;

   // find the begin of the sub table
   while( ! ifs.getLine(str0) )
   {
     splt_line=str0;

     if( splt_line[1] == "Table:" )
     {
       // there are two tables of different format; the only difference
       // in parsing should be given here.
       str0 = hdhC::stripSides(splt_line[2], "()") ;

       if( str0 == freq )
       {
         while( ! ifs.getLine(str0) )
         {
           sx = hdhC::clearChars(str0, " ") ;
           if( sx.find("outputvariablename") < std::string::npos )
             goto BREAK2; // positioned to the beginning of a caption
         }
       }
     }
   }

BREAK2:

   // sub-table not found?
   if( ifs.eof() )
     return false;

   splt_line = str0;

   while( ! ifs.getLine(str0) )
   {
     // find the first line after a (multiple) header-line
     size_t p0=str0.size();
     size_t p;
     if( (p=str0.find(',')) < std::string::npos )
       p0=p;

     if( hdhC::isAlpha( str0.substr(0,p0) ) )
         break;

     splt_line += str0;
   }

   std::vector<std::string> vs_ix;
   vs_ix.push_back( "outVarName" );
   vs_ix.push_back( pQA->n_cell_methods );
   vs_ix.push_back( pQA->n_cell_methods + "_opt" );
   vs_ix.push_back( pQA->n_long_name );
   vs_ix.push_back( pQA->n_standard_name );
   vs_ix.push_back( pQA->n_units );
   vs_ix.push_back( pQA->n_positive );

   std::vector<std::string> vs_val;
   vs_val.push_back( pQA->n_outputVarName );
   vs_val.push_back( pQA->n_cell_methods );
   vs_val.push_back( pQA->n_cell_methods + " (2nd option)" );
   vs_val.push_back( pQA->n_long_name );
   vs_val.push_back( pQA->n_standard_name );
   vs_val.push_back( pQA->n_units );
   vs_val.push_back( pQA->n_positive );

   std::string t;

   // Now, look for the indexes of the variable's columns
   for( size_t c=0 ; c < splt_line.size() ; ++c)
   {
     if( splt_line[c].substr(0,3) == "frq" || splt_line[c] == "ag" )
       continue;

     t = hdhC::unique(splt_line[c], ' ');

     // some user like to set a trailing ':' randomly
     if ( t[ t.size()-1 ] == ':' )
       t = t.substr(0,t.size()-1) ;

     for( size_t i=0 ; i < vs_ix.size() ; ++i )
     {
       if( t == vs_val[i] && v_col.find( vs_ix[i] ) == v_col.end() )
       {
           v_col[ vs_ix[i] ] = c;
           break;
       }
     }
   }

   return true;
}

void
QA_Exp::reqAttCheck(void)
{
  // check required attributes of specific variables
  for(size_t i=0 ; i < pQA->drs_cv_table.varName.size() ; ++i)
  {
    // global attributes
    if( pQA->drs_cv_table.varName[i] == pQA->s_global
            && pQA->pIn->variable[pQA->pIn->varSz].name == "NC_GLOBAL" )
      reqAttCheckGlobal(pQA->pIn->variable[pQA->pIn->varSz]);
    else
    {
      for( size_t j=0 ; j < pQA->pIn->varSz ; ++j )
      {
        // check for required attributes of variables (whether coordinates var
        // or data var doesn't matter).
        if( pQA->drs_cv_table.varName[i] == pQA->pIn->variable[j].name )
        {
          reqAttCheckVariable(pQA->pIn->variable[j]);
          break;
        }
      }
    }
  }

  // check attribute _FillValue and missing_value, if available
  std::string fV("_FillValue");
  std::string mV("missing_value");

  std::vector<std::string> fVmv;
  fVmv.push_back(fV);
  fVmv.push_back(mV);

  for( size_t j=0 ; j < pQA->pIn->varSz ; ++j )
  {
    Variable& var = pQA->pIn->variable[j] ;

    for( size_t l=0 ; l < 2 ; ++l )
    {
      int k;
      if( (k=var.getAttIndex(fVmv[l])) > -1 )
      {
        std::vector<float> aV;
        pQA->pIn->nc.getAttValues(aV, fVmv[l], var.name ) ;
        float rV = 1.E20 ;

        if( aV.size() == 0 || aV[0] != rV )
        {
          std::string key("3_11");

          if( notes->inq( key, var.name) )
          {
            std::string capt(
              hdhC::tf_att(var.name, fVmv[l], var.attValue[j][0])) ;
            capt += "does not match required value=1.E20";

            (void) notes->operate(capt) ;
            notes->setCheckStatus("CV", pQA->n_fail );
          }
        }
      }
    }
  }

  // for data variables,both _FillValue and missing_value
  // should be defined in CORDEX
  for( size_t i=0 ; i < pQA->pIn->dataVarIndex.size() ; ++i)
  {
    Variable &var = pQA->pIn->variable[pQA->pIn->dataVarIndex[i]];

    bool is_fV = var.isValidAtt(fV) ;
    bool is_mV = var.isValidAtt(mV) ;

    if( (is_fV || is_mV) && (is_fV != is_mV) )
    {
      std::string key("3_12");

      if( notes->inq( key, var.name) )
      {
        std::string capt(hdhC::tf_var(var.name, hdhC::colon)) ;
        capt += "if " + hdhC::tf_att(fV);
        capt += "or " + mV ;
        capt += ", then both should be defined";

        (void) notes->operate(capt) ;
        notes->setCheckStatus("CV", pQA->n_fail );
      }
    }
  }

  return ;
}

void
QA_Exp::reqAttCheckCloudValues(
      std::string &auxName, std::string &reqA)
{
  // reqA  := vector of required: att_name=att_value
  // vName := name of an existing variable (already tested)
  // auxName := name of an existing auxiliary (already tested)

  Split reqValues;
  reqValues.setSeparator('|');
  reqValues=reqA ;

  std::vector<int> iReqValues;

  // check required values.

  MtrxArr<double> fValues;
  bool isErr=true;

  int index=-1;
  if( fVarname == "clh" )
    index=0;
  else if( fVarname == "clm" )
    index=1;
  else if( fVarname == "cll" )
    index=2;

  if( auxName == "plev" )
  {
    pQA->pIn->nc.getData(fValues, "plev", 0, -1 );
    int iV = static_cast<int>( fValues[0] );
    iReqValues.push_back( iV );

    if( fValues.size() == 1 && iV == reqValues.toInt(index))
      isErr=false;
  }

  if( auxName == "plev_bnds" )
  {
    pQA->pIn->nc.getData(fValues, "plev_bnds", 0, -1 );
    int iV0, iV1;

    if( fValues.size() == 2 )
    {
       Split rV;
       rV.setSeparator(',');
       rV = reqValues[index] ;
       int iRV0 = rV.toInt(0) ;
       int iRV1 = rV.toInt(1) ;

       // swap, if necessary
       int tmp;
       if( iRV0 < iRV1 )
       {
          tmp=iRV0;
          iRV0=iRV1;
          iRV1=tmp;
       }

       iReqValues.push_back( iRV0 );
       iReqValues.push_back( iRV1 );

       iV0 = static_cast<int>( fValues[0] );
       iV1 = static_cast<int>( fValues[1] );

       if( iV0 == iRV0 && iV1 == iRV1 )
         isErr=false;
    }
  }

  if( isErr )
  {
    std::string key("5_1");
    if( notes->inq( key, auxName) )
    {
       std::string capt( "auxiliary ");
       capt += hdhC::tf_var(auxName + " for cloud amounts") ;
       capt += " no match to required value";
       if( iReqValues.size() != 1 )
         capt += "s" ;
       capt += "=";
       capt += reqValues[index] ;

       std::ostringstream ostr(std::ios::app);

       if( iReqValues.size() == 1 )
       {
         ostr << "\n" << auxName << " value (required)=" << iReqValues[0] ;
         ostr << "\n" << auxName << " value (file)=" << fValues[0] ;
       }
       else if( iReqValues.size() == 2 )
       {
         ostr << "\n" << auxName << " values (required)=  [ " << iReqValues[0] ;
         ostr << ", " << iReqValues[1] << " ]" ;
         ostr << "\n" << auxName << " values (file)=[ " << fValues[0] ;
         ostr << ", " << fValues[1] << " ]" ;
       }

       (void) notes->operate(capt, ostr.str()) ;
       notes->setCheckStatus("CV", pQA->n_fail );
     }
  }

  return;
}

void
QA_Exp::reqAttCheckGlobal(Variable& glob)
{
  // check required attributes
  DRS_CV_Table& drs = pQA->drs_cv_table;

  size_t ix;
  for( ix=0 ; ix < drs.varName.size() ; ++ix )
    if( drs.varName[ix] == pQA->s_global )
      break;

  if( ix == drs.varName.size() )
    return;

  for( size_t k=0 ; k < drs.attName[ix].size(); ++k )
  {
    std::string& rqName  = drs.attName[ix][k] ;
    std::string& rqValue = drs.attValue[ix][k] ;

    if(rqName == "institute_id")
        drs.attValue[ix][k] = institute_id ;
    else if(rqName == "model_id")
        drs.attValue[ix][k] = RCMModelName ;
    else if(rqName == "driving_model_id")
        drs.attValue[ix][k] = GCMModelName ;

    // missing required global attribute
    if( ! glob.isValidAtt(rqName) )
    {
       std::string key("2_6");

       if( notes->inq( key, pQA->fileStr, "NO_MT") )
       {
         std::string capt("required ");
         capt += pQA->s_global;
         capt += hdhC::blank;
         capt += hdhC::tf_att(rqName);
         capt += "is missing" ;

         (void) notes->operate(capt) ;
         notes->setCheckStatus("CV", pQA->n_fail );

         continue;
       }
    }

    // required attribute expects a required value
    if( rqValue.size() )  // a value is required
    {
      std::string aV = glob.getAttValue(rqName) ;

      if( aV.size() == 0 )
      {
        std::string key("2_7");

        if( notes->inq( key, pQA->s_global) )
        {
           std::string text;

           std::string capt(pQA->s_global);
           capt += hdhC::blank;

           if( rqValue.find(',') < std::string::npos )
           {
             capt += hdhC::tf_att(rqName, hdhC::colon);
             capt += " Missing among required values";
             text = "required: " + rqValue;
           }
           else
           {
             capt += hdhC::tf_att(rqName, hdhC::colon);
             capt += " Missing required value";
             capt += hdhC::tf_val(rqValue) ;
           }

           (void) notes->operate(capt, text) ;
           notes->setCheckStatus("CV", pQA->n_fail );
         }
       }

       else if( rqName == n_tracking_id )
         checkTrackingID(rqValue, aV) ;

       else if( rqValue.substr(0,5) == "DATE:"
                   || rqValue.substr(0,4) == "YYYY" )
       {
         if( checkDateFormat(rqValue, aV) )
         {
           std::string key("7_12");
           if( notes->inq( key, pQA->s_global) )
           {
             std::string capt(pQA->s_global);
             capt += hdhC::blank;
             capt += hdhC::tf_att(hdhC::empty, rqName, aV);
             capt += "does not comply with DRS_CV request ";
             if( rqValue.substr(0,5) == "DATE:" )
                capt += rqValue.substr(5);
             else
                capt += rqValue ;

             (void) notes->operate(capt) ;
             notes->setCheckStatus("CV", pQA->n_fail );
           }
         }
       }

       else if( aV != rqValue )
       {
         // there is a value. is it the required one?
         if( rqName == pQA->n_frequency )
         {
            // one of those specified in DRS_CV
            Split x_f(rqValue, " ,", true);
            if( hdhC::isAmong(aV, x_f.getItems()) )
               continue;
         }

         std::string key("2_8");
         if( notes->inq( key, pQA->s_global ) )
         {
           std::string capt(pQA->s_global);
           capt += hdhC::blank;
           capt += hdhC::tf_att(hdhC::empty, rqName, aV);
           capt += "does not match required value";
           capt += hdhC::tf_val(rqValue) ;

           (void) notes->operate(capt) ;
           notes->setCheckStatus("CV", pQA->n_fail );

           continue;
         }
       }
    }
  } // end of for-loop

  return;
}

void
QA_Exp::reqAttCheckVariable(Variable &var)
{
  // vName := name of an existing variable (already tested)
  // ix    := index of in.variable[ix]

  DRS_CV_Table& drs = pQA->drs_cv_table;

  std::string &vName = var.name;

  // check required attributes
  size_t ix;
  for( ix=0 ; ix < drs.varName.size() ; ++ix )
    if( drs.varName[ix] == vName )
      break;

  // special: is the variable == plev_bnds; i.e. for cloud amounts?
  bool isCloudAmount=false;
  if( pQA->pIn->getVarIndex("cll") > -1
        || pQA->pIn->getVarIndex("clm") > -1
           || pQA->pIn->getVarIndex("clh") > -1 )
    isCloudAmount=true;

  // find required attributes missing in the file
  for( size_t k=0 ; k < drs.attName[ix].size(); ++k )
  {
    std::string& rqName  = drs.attName[ix][k] ;
    std::string& rqValue = drs.attValue[ix][k] ;

    // skip 'pseudo-att'
    if( rqName == "VAR_TYPE" )
      continue;

    // find corresponding att_name of given vName in the file
    int jx=var.getAttIndex(rqName);

    // special: values may be available for cloud amounts

    // missing required attribute
    if( jx == -1 && rqName != "values" )
    {
       std::string key("2_1");

       if( notes->inq( key, vName) )
       {
         std::string capt(hdhC::tf_att(vName,rqName));
         capt += "is missing";

         (void) notes->operate(capt) ;
         notes->setCheckStatus("CV", pQA->n_fail );

         continue;
       }
    }

    // special: for cloud amounts
    if( isCloudAmount )
    {
      // check for attribute: plev_bnds
      std::string plev("plev");

      if( vName == plev )
      {
         if( !var.isValidAtt("bounds") )
         {
           std::string key("2_11");

           if( notes->inq( key, vName) )
           {
             std::string capt("auxiliary ");
             capt += hdhC::tf_var(plev + " for cloud amounts", hdhC::colon);
             capt += "missing " + hdhC::tf_att("plev_bnds");

             (void) notes->operate(capt) ;
             notes->setCheckStatus("CV", pQA->n_fail );
           }
         }

         // check for auxiliary: plev_bnds
         if( pQA->pIn->getVarIndex("plev_bnds") == -1 )
         {
           std::string key("5_2");

           if( notes->inq( key, vName) )
           {
             std::string capt("auxiliary ");
             capt += hdhC::tf_var("plev_bnds") ;
             capt += "for cloud amounts is missing";

             (void) notes->operate(capt) ;
             notes->setCheckStatus("CV", pQA->n_fail );
           }
         }
       }

       if( rqName == "values" )
       {
          reqAttCheckCloudValues( vName, rqValue);

          // word 'values' is only for cloud amount case.
          continue;
       }
    }

    std::string &aN = var.attName[jx] ;

    // required attribute expects a value (not for cloud amounts)
    if( rqValue.size() )  // a value is required
    {
      if( ! isCloudAmount && rqName == "values" )
	       continue;

      std::string aV( var.attValue[jx][0] ) ;
      for( size_t i=1 ; i < var.attValue[jx].size() ; ++i )
      {
        aV += " " ;
        aV += var.attValue[jx][i] ;
      }

      if( aV.size() == 0 )
      {
        std::string key("2_2");
        if( notes->inq( key, vName) )
        {
           std::string capt(hdhC::tf_att(vName, aN, hdhC::colon));
           capt="missing required value" ;
           capt += hdhC::tf_val(rqValue);

           (void) notes->operate(capt) ;
           notes->setCheckStatus("CV", pQA->n_fail );

           continue;
         }
       }

       // there is a value. is it the required one?
       else if( aV != rqValue )
       {
         bool is=true;
         if( rqName == pQA->n_long_name)
         {
           // mismatch tolerated, because the table does not agree with CMIP5
           if( rqValue == "pressure" && aV == "pressure level" )
             is=false;
           if( rqValue == "pressure level" && aV == "pressure" )
             is=false;
         }
         else if( (rqName == pQA->n_positive) || (rqName == pQA->n_axis) )
         {
           // case insensitive
           if( rqValue == hdhC::Lower()(aV) )
             is=false;
         }

         std::string key("2_3");
         if( is &&  notes->inq( key, vName ) )
         {
           std::string capt(hdhC::tf_att(vName, aN, aV));
           capt += "does not match required value" ;
           capt += hdhC::tf_val(rqValue);

           (void) notes->operate(capt) ;
           notes->setCheckStatus("CV", pQA->n_fail );

           continue;
         }
       }
    }
  } // end of for-loop

  return;
}

void
QA_Exp::run(void)
{
   bool isNoTable = inqTables() ;

   if( pQA->drs_cv_table.table_DRS_CV.is() )
   {
     if(pQA->isCheckDRS_F || pQA->isCheckDRS_P)
     {
       DRS_CV drsFN(pQA);
       drsFN.run();
     }
   }

     if(pQA->isCheckCV)
   {
     // check variable type; uses DRS_CV_Table
     checkVariableType();

     if( isNoTable )
       notes->setCheckStatus("CV", pQA->n_fail);
     else
        // get meta data from file and compare with tables
        checkMetaData(*(pQA->pIn));
   }

   return ;
}

void
QA_Exp::setParent(QA* p)
{
   pQA = p;
   notes = p->notes;
   return;
}

void
QA_Exp::varReqTableCheck(InFile &in, VariableMetaData &vMD,
             std::vector<struct DimensionMetaData> &dimNcMeDa)
{
   // scanning the standard table.

//   std::fstream ifs(str0.c_str(), std::ios::in);
   // This class provides the feature of putting back an entire line
   ReadLine ifs(varReqTable.getFile());

   if( ! ifs.isOpen() )
   {
      std::string key("IE_11") ;
      if( notes->inq( key, vMD.var->name) )
      {
         std::string capt(
             "could not open the CORDEX_variables_requirement table, tried") ;
         capt += hdhC::tf_val(varReqTable.getFile());

         if( notes->operate(capt) )
         {
           notes->setCheckStatus("CV", pQA->n_fail );

           pQA->setExitState( notes->getExitState() ) ;
         }
      }
   }

   // remove all " from input
   ifs.skipCharacter('"');

   VariableMetaData t_DMD_entry(pQA);

   // find the sub table, corresponding to the frequency column
   // try to identify the name of the sub table in str0,
   // return true, if not found
//   if( ! find_CORDEX_SubTable(ifs, str0, vMD) )
//     return false;

   // find entry for the required variable
   if( findTableEntry(ifs, vMD.var->name, t_DMD_entry) )
   {
     // netCDF properties are compared to those in the table.
     // Exit in case of any difference.
     checkVarTableEntry(vMD, t_DMD_entry);

     // a specifi check for the value of height if available
     checkHeightValue(in);

     // get meta-data of var-reps of dimensions from nc-file
     std::map<std::string, size_t> d_col;

     for(size_t l=0 ; l < vMD.var->dim_ix.size() ; ++l)
     {
       Variable& dVar = pQA->pIn->variable[ vMD.var->dim_ix[l] ] ;

       if( dVar.dimSize[l] == 1 )
          continue;

       // new instance
       dimNcMeDa.push_back( DimensionMetaData() );

       // the spot where meta-data of var-reps of dims is taken
       // note: CORDEX standard tables doesn't provide anything, but
       // it is necessary for writing the project table.
       getDimMetaData(in, vMD, dimNcMeDa.back(), dVar.name) ;
     }

     return;
   }

   // no match
   std::string key("3_1") ;
   if( notes->inq( key, vMD.var->name) )
   {
     std::string capt(hdhC::tf_var(vMD.var->name));
     capt += " for " ;
     capt += hdhC::tf_assign(pQA->n_frequency, getFrequency()) ;
     capt += " not specified in the CORDEX_variables_requirement table";

     (void) notes->operate(capt) ;
     notes->setCheckStatus("CV", pQA->n_fail );
   }

   // variable not found in the standard table.
   return;
}



VariableMetaData::VariableMetaData(QA *p, Variable *v)
{
   pQA = p;
   var = v;

   qaData.setVar(v);
}

VariableMetaData::~VariableMetaData()
{
  dataOutputBuffer.clear();
}

void
VariableMetaData::verifyPercent(void)
{
   // % range
   if( var->units == "%" )
   {
      if( qaData.statMin.getSampleMin() >= 0.
           && qaData.statMax.getSampleMax() <= 1. )
      {
        std::string key("6_8");
        if( notes->inq( key, var->name) )
        {
          std::string capt( hdhC::tf_var(var->name, ":"));
          capt += "Suspicion of fractional data range for units [%]";
          std::string text("Found range [");
          text += hdhC::double2String(qaData.statMin.getSampleMin());
          text += ", " + hdhC::double2String(qaData.statMax.getSampleMax()) + "]" ;

          (void) notes->operate(capt, text) ;
          notes->setCheckStatus("CV", pQA->n_fail );
        }
      }
   }

   if( var->units == "1" || var->units.size() == 0 )
   {
      // is it % range? Not all cases are detectable
      if( qaData.statMin.getSampleMin() >= 0. &&
           qaData.statMax.getSampleMax() > 1.
             && qaData.statMax.getSampleMax() <= 100.)
      {
        std::string key("6_9");
        if( notes->inq( key, var->name) )
        {
          std::string capt("Suspicion of percentage data range for units <1>"); std::string text("Found range [" ) ;
          text += hdhC::double2String(qaData.statMin.getSampleMin());
          text += ", " + hdhC::double2String(qaData.statMax.getSampleMax()) + "]" ;

          (void) notes->operate(capt, text) ;
          notes->setCheckStatus("MD(variable)", pQA->n_fail );
        }
      }
   }

   return;
}

int
VariableMetaData::finally(int xCode)
{
  // write pending results to qa-file.nc. Modes are considered there
  qaData.flush();

  // annotation obj forked by the parent VMD
  notes->printFlags();

  int rV = notes->getExitState();
  xCode = ( xCode > rV ) ? xCode : rV ;

  return xCode ;
}

void
VariableMetaData::forkAnnotation(Annotation *n)
{
   notes = new Annotation(n);

   // this is not a mistaken
   qaData.setAnnotation(n);

   return;
}

void
VariableMetaData::setAnnotation(Annotation *n)
{
   notes = n;

   qaData.setAnnotation(n);

   return;
}
