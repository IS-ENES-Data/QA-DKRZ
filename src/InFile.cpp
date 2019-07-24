#include "in_file.h"

InFile::InFile()
{
  initDefault();
}

void
InFile::applyOptions(void)
{
   Base::applyOptions();

   for( size_t i=0 ; i < optStr.size() ; ++i)
   {
      Split split(optStr[i],"=");

      if( split.size() == 0 )
        continue;

      if( split[0] == "aM" || split[0] == "arithMean"
          || split[0] == "arithmetic_mean" )
      {
        isArithmeticMean=true;
        continue;
      }

      if( split[0] == "dIN" || split[0] == "disableInfNanTest"
           || split[0] == "disable_inf_nan_test" )
      {
        isInfNan=false;
        continue;
      }

      if( split[0] == "excludedVariable"
             || split[0] == "eV" )
      {
        Split cvs(split[1],",");
        for( size_t i=0 ; i < cvs.size() ; ++i )
          excludedVariable.push_back(cvs[i]);
        continue;
      }

      if( split[0] == "path" )
      {
        setFilename(split[1] + "/");
        continue;
      }

      if( split[0] == "fixedFieldVars"
             || split[0] == "fFV" )
      {
        fixedFieldVars=split[1];
        continue;
      }

      if( split[0] == "unlimitedDimName"
             || split[0] == "uDN" )
      {
        unlimitedDimName = split[1];
        continue;
      }

      if( split[0] == "variableName"
             || split[0] == "vN" )
      {
        explicitVariable=split[1];
        continue;
      }
   }

   // this is special
/*
   for( size_t i=0 ; i < gD_VarUnlim.size() ; ++i)
   {
     if( gD[ mapIndex[gD_VarUnlim[i].name] ].isPrint() )
     {
       enableEntry=true;
       isPrintGMT=true;
     }
   }
*/

   return;
}

std::string
InFile::getAttValue(std::string attName, std::string varName)
{
   int ix;

   if( varName.size() )
     ix = getVarIndex( varName );
   else if( nc.is_global() )
     ix = varSz ;  // NC_GLOBAL is always this
   else
     return hdhC::empty ;

   return variable[ix].getAttValue(attName);
}

int
InFile::getDimPos(std::string &dName)
{
  int pos=-1;
  for( size_t i=0 ; i < variable.size() ; ++i )
    if( dName == variable[i].name )
       pos=i;

  return pos;
}

void
InFile::clearRecProperties(void)
{
  // reset
  isRecEndSet=false;
  externRecBeg=0;
  externRecEnd=0;
  ncRecBeg=0;
  ncRecEnd=0;
  recStride=1;
  recOffset=0;

  isRecSet=false;
  isRecSingle=false;

  return;
}

bool
InFile::entry(void)
{
  // Note: a time-window or any limit was applied in tC.init() such
  // that the record range of *this was adjusted.

/*
  // A time window constraint was put on the sequence of records.
  // currTime is related to a reference time set in tC->init()

  if( tC )
  {
    double currTime = Base::getTime(nc, currRec, "time");
    tC->syncEnd(currTime) ) ;  // reset the record limit of *this
  }
*/

  // read for record-type variables
  for( size_t i=0 ; i < dataVarIndex.size() ; ++i)
  {
    Variable &var = variable[dataVarIndex[i]];

    if( currRec && currRec >= nc.getNumOfRecords(var.name) )
      continue;

    var.getData(currRec);
  }

  // Only used in notes, when excluding records is enabled
  notes->setCurrentRecord(currRec);

  return false;
}

void
InFile::excludeVars(void)
{
    std::string time_clim( nc.getAttString("climatology", "time") );

    for( size_t i=0 ; i < variable.size() ; ++i)
    {
      // this is very special: if 'time' contains attribute
      // climatology, then ignore this

      if( time_clim == variable[i].name )
      {
        variable[i].isExcluded=true;
        continue;
      }

      // Any user-defined variables to be excluded?
      for( size_t j=0 ; j < excludedVariable.size() ; ++j )
      {
        if( variable[i].name == excludedVariable[j] )
        {
           variable[i].isExcluded=true;
           break;
        }
      }
    }

  return ;
}

void
InFile::getData( int rec )
{
  if( rec == 0 )
    for( size_t i=0 ; i < variable.size() ; ++i)
      variable[i].getData(rec);
  else
    for( size_t i=0 ; i < dataVarIndex.size() ; ++i)
      variable[i].getData(rec);

  return;
}

void
InFile::getData( int rec, std::string name )
{
  for( size_t i=0 ; i < variable.size() ; ++i)
    if( name == variable[i].name )
       variable[i].getData(rec);

  return;
}

std::vector<std::string>
InFile::getLimitedVarName(void)
{
  std::vector<std::string> t0( nc.getLimitedVarName() );

  // If no limited variable is defined and vName is provided, then
  // try for all variables, not depending on dName.*/
  if( !t0.size() && unlimitedName.size() )
  {
    std::vector<std::string> vs(nc.getVarName()) ;
    std::vector<std::string> vd;
    for( size_t i=0 ; i < vs.size() ; ++i )
    {
       bool is=true;
       vd = nc.getDimName(vs[i]);
       for( size_t j=0 ; j < vd.size() ; ++j )
       {
         if( vd[j] == unlimitedName )
         {
           is=false;
           break;
         }
       }

       if( is )
         t0.push_back(vs[i]);
    }
  }

  return t0;
}

bool
InFile::getNcAttVal(std::string vName,
    std::string aName, std::vector<double> &dVal)
{
  nc.getAttValues(dVal, aName, vName) ;

  if( dVal.size() > 0 )
    return true;

  return false;
}

size_t
InFile::getNumOfRecords(void)
{
  size_t len=nc.getNumOfRecords();

  if( !len && unlimitedName.size() )
    // If no limited variable is defined and vName is provided, then
    // try for dName.*/
    len=nc.getVarSize(unlimitedName);

  return len ;
}

std::vector<std::string>
InFile::getUnlimitedVars(void)
{
  std::vector<std::string> t0(nc.getUnlimitedVars() );

  // If no unlimited variable is defined, then
  // try for all variables, depending on unlimitedName.*/
  if( !t0.size() && unlimitedName.size() )
  {
    std::vector<std::string> vs(nc.getVarName()) ;
    std::vector<std::string> vd;
    for( size_t i=0 ; i < vs.size() ; ++i )
    {
       vd = nc.getDimName(vs[i]);
       for( size_t j=0 ; j < vd.size() ; ++j )
       {
         if( vd[j] == unlimitedName )
         {
           t0.push_back(vs[i]);
           break;
         }
       }
    }
  }

  return t0;
}

std::string
InFile::getTimeUnit(void)
{
   std::string any;
   return getTimeUnit(any);
}

std::string
InFile::getTimeUnit(std::string &vName)
{
  // Assumption:
  // the only variable depending on the UNLIMITED dimension
  // as single dimension and having the same name,
  // i.e. a variable representation of the unlimited dim.
  // Special: time is defined but not as unlimited

  if( unlimitedName.size() == 0 )
  {
    std::string str;
    return str;  // no variable, no attribute
  }

  vName=unlimitedName;
  std::vector<std::string> dn;
  nc.getAttValues(dn, "units", unlimitedName) ;

  if( dn.size() > 0 )
     return dn[0];

  std::string str;
  return str;  // attribute not found
}


void
InFile::getVariableAtt(Variable &var)
{
  var.id = nc.getVarID( var.name );

  // get all attributes
  var.attName = nc.getAttName(var.name) ;

  std::vector<std::string> vs;
  std::string t;

  if( var.name != "NC_GLOBAL" )
    t = var.name ;  // empty value for NC_GLOBAL

  // get values of attributes
  for( size_t i=0 ; i < var.attName.size() ; ++i )
  {
     var.attNameMap[ var.attName[i] ] = i ;

     if( hdhC::Lower()(var.attName[i]) == "units" )
        var.isUnitsDefined=true;

     vs.clear();

     nc_type a_id = nc.getAttType(var.attName[i], t);

     var.attType.push_back(a_id );

     if( a_id == NC_CHAR )
     {
        nc.getAttValues(vs, var.attName[i], t );
     }
     else if( a_id == NC_BYTE )
     {
        std::vector<int> av;
        nc.getAttValues(av, var.attName[i], t );
        for( size_t j=0 ; j < av.size() ; ++j )
        {
          vs.push_back( hdhC::double2String
             ( static_cast<double>(av[j]) ) );
          vs.back() += "b" ;
        }
     }
     else if( a_id == NC_INT )
     {
        std::vector<int> av;
        nc.getAttValues(av, var.attName[i], t );
        for( size_t j=0 ; j < av.size() ; ++j )
          vs.push_back( hdhC::double2String
             ( static_cast<double>(av[j]) ) );
     }
     else if( a_id == NC_SHORT )
     {
        std::vector<short> av;
        nc.getAttValues(av, var.attName[i], t );
        for( size_t j=0 ; j < av.size() ; ++j )
          vs.push_back( hdhC::double2String
             ( static_cast<double>(av[j]) ) );
     }
     else if( a_id == NC_FLOAT )
     {
        std::vector<float> av;
        nc.getAttValues(av, var.attName[i], t );
        for( size_t j=0 ; j < av.size() ; ++j )
          vs.push_back( hdhC::double2String
             ( static_cast<double>(av[j]), "p=5|adj" ) );
     }
     else if( a_id == NC_DOUBLE )
     {
        std::vector<double> av;
        nc.getAttValues(av, var.attName[i], t );
        for( size_t j=0 ; j < av.size() ; ++j )
          vs.push_back( hdhC::double2String( av[j], "p=5|adj" ) );
     }
     else if( a_id == NC_STRING )
        nc.getAttValues(vs, var.attName[i], t );

     if( ! vs.size() )
        vs.push_back("");

     // store in Variable
     var.attValue.push_back( vs );
  }

  if( var.name == "NC_GLOBAL" )
     return;

  // name is supported only for dimensions

  // get some meta data of variables defined in the base class.

  // type of variable to be used further
  var.type=nc.getVarType(var.name);

  // specific attributes of the variable.
  std::vector<double> val;

  if( getNcAttVal(var.name, "missing_value", val) )
    var.isMissingValue=true;

  if( getNcAttVal(var.name, "_FillValue", val) )
    var.isFillValue=true;

  if( getNcAttVal(var.name, "scale_factor", val) )
    var.scaleFactor = val[0];
  else
    var.scaleFactor = 1.;

  if( getNcAttVal(var.name, "add_offset", val) )
    var.addOffset = val[0];
  else
    var.addOffset = 0.;

  nc.getAttValues(vs, "units", var.name) ;
  if( vs.size() > 0 )
    var.units = vs[0];

  return ;
}


int
InFile::getVarIndex(std::string s)
{
   int ix = -1 ;

   if( vIx.count(s) )
      ix = vIx[s] ;

   return ix ;
}

void
InFile::getVariableMD(std::vector<Variable>& variable, char mode, std::string vName)
{
  // get and analyse all variables and create Variable objects
  // mode: 0: variables && global (includes time)
  //       1: variables (includes time)
  //       2: global
  //       3: time [ + time_bnds]

  std::vector<std::string> fVar( nc.getVarName() );
  std::vector<std::string> actName;

  bool isGlobal = false;

  if( mode == 'V' )
    actName = fVar;
  else if( mode == 'G' )
      isGlobal = true;
  else if( mode == 'T' )
  {
      std::string tName( nc.getUnlimitedDimRepName() );

      if( tName.size() )
      {
        for( size_t i=0 ; i < variable.size() ; ++i)
          if(variable[i].name == tName)
            return;

        actName.push_back( tName) ;
        std::string bndsName( nc.getAttString("bounds", tName) );
        if( bndsName.size() && nc.isVariableValid(bndsName) )
           actName.push_back( bndsName );
      }
  }

  for(size_t j=0 ; j < actName.size(); ++j)
  {
     variable.push_back( *new Variable );
     makeVariable(actName[j], variable.back(), j );

     vIx[ actName[j] ] = j ;
     variableNames.push_back(variable.back().name);

     if( vName == variable.back().name )
       dataVarIndex.push_back(variable.back().id);

     getVariableMD(variable.back());
  }

  for( size_t i=0 ; i < variable.size() ; ++i)
  {
    if(variable[i].name == "NC_GLOBAL")
    {
      varSz = variable.size() -1;
      return;
    }
  }

  varSz = variable.size();

  // a pseudo-variable for the global attributes
  if( isGlobal && nc.getAttSize("NC_GLOBAL") )
  {
    variable.push_back( *new Variable );
    makeVariable("NC_GLOBAL", variable.back());

    variable.back().isExcluded=true;
    vIx[ "NC_GLOBAL" ] = variable.size() -1 ;

    // get meta data of variables.
    getVariableAtt(variable.back());

    vIx[ "NC_GLOBAL" ] = variable.size() -1 ;
  }

  return;
}


void
InFile::getVariableMD(Variable& var)
{
  std::vector<std::string> ds;

  var.pIn = this;

  // names of the dimensions
  ds = nc.getDimName( var.name );

  for( size_t j=0 ; j < ds.size() ; ++j)
  {
    var.dimName.push_back( ds[j] );
    var.dimSize.push_back( nc.getDimSize(ds[j]) );

    for( size_t k=0 ; k < variable.size() ; ++k)
    {
      if( variable[k].name == ds[j] )
      {
        var.dim_ix.push_back(k);
        break;
      }
    }
  }

  // get meta data of variables.
  getVariableAtt(var);

  // make objects, but GeoMeta which is postponed
  var.makeObj(isInfNan);

  // is it time or somethiong alike?
  if( nc.isVarUnlimited(var.name) && nc.isDimUnlimited(var.name) )
  {
    // time is coordinate variable
    // note: this is global; not specific to variables
    isTime=true ;
    timeName=var.name;
  }

  size_t iid = static_cast<size_t>(var.id);

  if( hdhC::isAmong(iid, dataVarIndex) )
  {
    if( ! var.isUnlimited() )
      var.isFixed = true;

    // inquire parameters for GeoMeta objects;
    // make GeoMeta obj for geo-related fields
    setGeoParam(var) ;
  }

  return;
}


void
InFile::help(void)
{
  std::cerr << "Additional to InFile:\n" ;
  std::cerr << "   path=path-to-netCDF-file\n" ;
  std::cerr << std::endl;
  return;
}

bool
InFile::init(void)
{
  notes->init() ;  // safe
  notes->file.setFile(file);

  // apply parsed command-line args
  applyOptions();

  // default for time checks
  isTime=false;

  if( notes )
    notes->file = file ;  // just for logging

  if( openNc() )
  {
    if( notes )
    {
      std::string key("IN_1");

      std::string level="L1";
      std::string task="";
      std::vector<std::string> freq;
      std::vector<std::string> value;
      std::vector<size_t> xRecord_01;

     	notes->push_front(key , "*", freq, level, task, "", value,
	                         xRecord_01, xRecord_01);
      notes->inq(key);

      std::string capt("Could not open NetCDF file.");

      std::string text ;

      if( exceptionHandling(key, capt, text) )
          finally(3);
    }
    else
    {
      std::ostringstream ostr(std::ios::app);
      ostr << "InFile::init()\n";
      ostr << "Could not open NetCDF file " << file.getFile();
      exceptionError( ostr.str() );

      finally(3, ostr.str());
    }
  }

  // set pointer to member function init()
  execPtr = &IObj::entry ;

  return false;
}

void
InFile::initDefault(void)
{
  Base::setObjName("IN");

  notes=0;
  pIn=0;
  fDI=0;
  pOper=0;
  pOut=0;
  qA=0;
  tC=0;

  clearRecProperties(); //set the default
  pSrcBase.push_back(0);  // only for InFile objects; used as mark

  enableEntry=false;
  isInfNan=true;
  isInit=false;
  isPrintGMT=false;

  varSz=0;

  // set pointer to member function init()
  execPtr = &IObj::init ;

  return ;
}

void
InFile::initRecProperties(int endRec)
{
  // Requires a previous call of accessNc.
  // Determine the range of records.
  // Here ncRecEnd points beyond the last record.
  if( externRecEnd > 0 )
    ncRecEnd=externRecEnd - externRecBeg + 1 ;
  else
  {
    if( nc.isDimUnlimited() )
    {
       // the default
      if( ! isRecEndSet )
        ncRecEnd=static_cast<size_t>(nc.getNumOfRecords());
    }
    else
      ncRecEnd=ncRecBeg + recOffset +1;  // for fixed field
  }
  // Note: ncRecEnd points behind the last record

  // If time is not given as unlimited variable, but as fixed, then
  // the method may be called again with the value of the time-dimension
  if( endRec > 0 )
    ncRecEnd = static_cast<size_t>(endRec);

  // Is there only a single record?
  if( (ncRecEnd - ncRecBeg) == 1 )
    isRecSingle=true;

  if( externRecBeg == 0 )
    externRecBeg=ncRecBeg ;

  currRec = ncRecBeg + recOffset;
  externRecBeg += recOffset;

  return;
}

bool
InFile::isVarUnlimited(std::string vName)
{
  bool is=nc.isVarUnlimited(vName) ;

  // If no unlimited variable is defined and vName is provided, then
  // try for all variables, depending on vName.*/
  if( !is && unlimitedName.size() )
  {
    std::vector<std::string> vs(nc.getVarName()) ;
    std::vector<std::string> vd;
    for( size_t i=0 ; i < vs.size() ; ++i )
    {
       vd = nc.getDimName(vs[i]);
       for( size_t j=0 ; j < vd.size() ; ++j )
       {
         if( vd[j] == unlimitedName )
         {
           is=true;
           break;
         }
       }
    }
  }

  return is;
}

void
InFile::linkObject(IObj *p)
{
  std::string className = p->getObjName();

  if( className == "X" )
    notes = dynamic_cast<Annotation*>(p) ;

#ifndef CF_MACRO
  else if( className == "FD_interface" )
    fDI = dynamic_cast<FD_interface*>(p) ;
  else if( className ==  "IN" )
    pIn = dynamic_cast<InFile*>(p) ;
  else if( className == "Oper" )
    pOper = dynamic_cast<Oper*>(p) ;
  else if( className == "Out" )
    pOut = dynamic_cast<OutFile*>(p) ;
  else if( className == "QA" )
    qA = dynamic_cast<QA*>(p) ;
  else if( className == "TC" )
    tC = dynamic_cast<TimeControl*>(p) ;
#endif

  return;
}

void
InFile::makeVariable (std::string name, Variable& var, int id)
{
  var.name=name;

  var.pGM=0;
  var.pDS=0;
  var.id=-1;
  var.pSrcBase=0;
  var.pIn=0;
  var.pNc=&nc;
  var.setID(id);

  if( id == -1 )  // global
    varNameMap[name] = variable.size() ;
  else
    varNameMap[name] = id ;

  return;
}

bool
InFile::openNc(bool isNew)
{
  variable.clear();

  if( notes )
    nc.setNotes(notes);

  std::string str(path);
  if( isNew && nc.isOpen())
  {
    nc.close();
  }

  if( isNew )
  {
    try
    {
      if( path.size() )
        nc.setPath(str);
      if( unlimitedDimName.size() )
        nc.setUnlimitedDimName(unlimitedDimName);

      if( ! nc.open(file.getFile().c_str()) )
        throw "Exception";
    }
    catch (char const*)
    {
      // it is a feature for files built by pattern, if the
      // time frame extends the range of files.
      // Caller is responsible for trapping any errors
      return true;
    }

    pNc = &nc; // a pointer declared in Base class
  }

  if( isInit )
    return false;
  isInit=true;

  // time-range properties
  clearRecProperties();

  isUnlimited = nc.isDimUnlimited() ;
  if( isUnlimited )
  {
    unlimitedName=nc.getUnlimitedDimName();
    initRecProperties();
  }
  else
  {
    // a second try for a defined, but limited time
    isUnlimited = nc.isVariableValid( "time" );
    if( isUnlimited )
      unlimitedName="time";

    // adjust the end-rec-num; if time is invalid, then -1 is returned
    // which will be accounted for in initRecProperties()
    initRecProperties( nc.getDimSize("time") );
  }

  return false;
}

void
InFile::pullMetaData(char c, std::string vName)
{
  // mode: 2: MD(global)
  getVariableMD(variable, c, vName);

  return;
}

void
InFile::setGeoCellShape(Variable &var)
{
  // Singular points or areas, rectangular areas (bounds)
  // or irregular shapes of areas (vertices)?

  if( var.isArithmeticMean )
    return;

  std::vector<std::string> bounds;
  std::vector<std::string> d_name;
  std::vector<size_t> v_pos;
  std::vector<size_t> d_pos;
  std::vector<size_t> d_sz;

  // dimension loc or site must only have vertices
  bool isSite=false;

  for( size_t i=0 ; i < var.dimName.size() ; ++i )
  {
     if( var.dimName[i] == "site"
          || var.dimName[i] == "loc" )
       isSite=true;
  }

  for( size_t i=0 ; i < var.dimName.size() ; ++i )
  {
     std::string &name = var.dimName[i] ;
     int dPos = getDimPos( name );

     if( dPos < 0 )  // not represented by a variable
       continue;

     if( isSite )
     {
       if( name == "loc" || name == "site" )
       {
          d_name.push_back( name ) ;
          d_pos.push_back( i ) ;
          v_pos.push_back( static_cast<size_t>(dPos) ) ;
          bounds.push_back( variable[dPos].bounds );
          d_sz.push_back( nc.getDimSize(name) );
          break;
       }
     }
     else
     {
       // both 1D or 2D are possible; however, 1D only with vertices
       if( name == "lon" || name == "lat" )
       {
          d_name.push_back( name ) ;
          d_pos.push_back( i ) ;
          v_pos.push_back( static_cast<size_t>(dPos) ) ;
          bounds.push_back( variable[dPos].bounds );
          d_sz.push_back( nc.getDimSize(name) );
       }
     }
  }

  // A) 2D: check for bounds of a rectangular grid

  // true when both lon and lat exist
  if( bounds.size() == 2 )
  {
     // regular grid; bounds with 2 dims.
     if( bounds[0].find("bnds") < std::string::npos
           && bounds[1].find("bnds") < std::string::npos )
     {
//        var.pGM->setRegularGrid(false);
//        var.pGM->setLayerDim( "lon",
//             variable[ilon].dimParam[i].size, i );
       size_t lat_ix=0;
       size_t lon_ix=1;
       if( d_name[0] == "lon" )
       {
         lon_ix=0;
         lat_ix=1;
       }

       setVerticesFromGridBounds(var, bounds[lat_ix],
            bounds[lon_ix], d_sz[lat_ix], d_sz[lon_ix]);
       return ;
     }

     if( bounds[0].find("vert") < std::string::npos
           && bounds[1].find("vert") < std::string::npos )
     {
       return;
     }
  }

  if( bounds.size() == 1 && bounds[0].find("vert") )
  {
    // must have vertices defined
    if( bounds[0].find("vert") < std::string::npos )
    {
      return;
    }

  }

// ---------------

  // no bounds or vertices declared by attributes?
  // third party dimension(s) are the link between the variable
  // and some kind of bounds.
  size_t ilatB, ilonB;
  ilatB = ilonB = 0 ;
  std::vector<size_t> latB_dim_sz ;
  std::vector<size_t> lonB_dim_sz ;

  for( size_t i=0 ; i < variable.size() ; ++i )
  {
     std::string &name = variable[i].name ;

     if( name == "lat" )
     {
        // find coresponding index for the bounds
        if( variable[i].bounds.size() )
        {
          for( ilatB=0 ; ilatB < variable.size() ; ++ilatB )
          {
             if( variable[i].bounds
                   == variable[ilatB].name )
             {
                latB_dim_sz = nc.getVarDimSize(variable[ilatB].name);
                break;
            }
          }
        }
     }

     if( name == "lon" )
     {
        // find coresponding index for the bounds
        if( variable[i].bounds.size() )
        {
          for( ilonB=0 ; ilonB < variable.size() ; ++ilonB )
          {
             if( variable[i].bounds
                   == variable[ilonB].name )
             {
                lonB_dim_sz = nc.getVarDimSize(variable[ilonB].name);
                break;
            }
          }
        }
     }
  }

  // vertices found?
  if( latB_dim_sz.size() == 3 )
  {
    if( latB_dim_sz.size() == lonB_dim_sz.size() )
    {
      MtrxArr<double> mv_vrt;
      nc.getData(mv_vrt, variable[ilatB].name, 0, -1 );

      var.pGM->setVertices("lat", (void*) &mv_vrt, "double",
             latB_dim_sz[2], 2, latB_dim_sz[0], 0,
               latB_dim_sz[1], 1);

      nc.getData(mv_vrt, variable[ilonB].name, 0, -1 );

      var.pGM->setVertices("lon", (void*) &mv_vrt, "double",
             lonB_dim_sz[2], 2, lonB_dim_sz[0], 0,
               lonB_dim_sz[1], 1);

      var.pGM->setRegularGrid(true);
      return;
    }
  }

  // no bounds/vertices found or invalid
  var.pGM->disableCellShape();
  var.isArithmeticMean=true;

  return ;
}

void
InFile::setGeoLayer(Variable &var)
{
  // search for layer defining variables lon/lat
  size_t count=0;
  std::vector<std::string> lonDimName;
  std::vector<std::string> latDimName;

  for( size_t i=0 ; i < variable.size() ; ++i )
  {
    // accepts also longitude/latitude, but not ..._...
    if( variable[i].name.find('_') == std::string::npos )
    {
      if( variable[i].name.substr(0,3) == "lon" )
      {
        lonDimName=nc.getDimName(variable[i].name) ;
        ++count;
      }

      if( variable[i].name.substr(0,3) == "lat" )
      {
        latDimName=nc.getDimName(variable[i].name) ;
        ++count;
      }
    }
  }

  // both variables must be defined
  if( count == 1 )
  {
    if( !(latDimName[0] == "loc" || latDimName[0] == "site") )
      return;  // not a layer
  }
  else if( count == 0 )
    return;  // may-be an implicit layer; not CMIP5

  // not for defining layers, but for layers with associated areas
  if( lonDimName.size() == latDimName.size() )
  {
    if( lonDimName.size() > 1 )
       if( lonDimName[0] == unlimitedName )
         var.pGM->enableMutableLayerDims();
  }
  else
    return ; //strange layer definition; not CMIP5

  std::vector<std::string> layerDimName;
  std::vector<size_t> layerDimSize;
  std::vector<size_t> layerDimPos;
  std::vector<std::string> nonLayerDimName;
  std::vector<size_t> nonLayerDimSize;
  std::vector<size_t> nonLayerDimPos;

  // Adjust for the dimension of records (when given, then the 1st dim).
  bool adjustForRecDimName=false;
  for( size_t i=0 ; i < var.dimName.size() ; ++i )
  {
     if( var.dimName[i] == unlimitedName )
     {
       adjustForRecDimName=true;
       break;
     }
  }

  // indexes not defining a layer
  for( size_t i=0 ; i < var.dimName.size() ; ++i )
  {
    // The index for time is discarded.
    if( var.dimName[i] == unlimitedName )
      continue;

    // having time-dependent lons and lats, but a fixed variable
    // would be rather amazing; this is not checked.

    bool is=false;
    for( size_t j=0 ; j < latDimName.size() ; ++j )
    {
      if( latDimName[j] == unlimitedName )
        continue;
      if( var.dimName[i] == latDimName[j] )
        is=true;
    }

    for( size_t j=0 ; j < lonDimName.size() ; ++j )
    {
      if( lonDimName[j] == unlimitedName )
        continue;
      if( var.dimName[i] == lonDimName[j] )
        is=true;
    }

    if( is )
      continue;

    // value of dimension
    nonLayerDimName.push_back( var.dimName[i] );
    nonLayerDimSize.push_back( nc.getDimSize(var.dimName[i]) );

    // position in the MtrxArr object
    if( adjustForRecDimName )
      nonLayerDimPos.push_back(i-1);
    else
      nonLayerDimPos.push_back(i);
  }

  if( nonLayerDimSize.size() )
    var.pGM->setNonLayerDim(nonLayerDimSize, nonLayerDimPos);

  // indexes defining a layer
  for( size_t i=0 ; i < var.dimName.size() ; ++i )
  {
    // The index for time is discarded.
    if( var.dimName[i] == unlimitedName )
      continue;

    bool cont2=false;
    for( size_t j=0 ; j < nonLayerDimName.size() ; ++j )
    {
      if( var.dimName[i] == nonLayerDimName[j] )
      {
        cont2=true;
        break;
      }
    }

    if( cont2 )
      continue;

    // name of the dimension
    layerDimName.push_back( var.dimName[i] );

    // value of the dimension
    layerDimSize.push_back( nc.getDimSize(var.dimName[i]) );

    // position in the MtrxArr object
    if( adjustForRecDimName )
      layerDimPos.push_back(i-1);
    else
      layerDimPos.push_back(i);
  }

  if( layerDimName.size() )
    var.pGM->setLayer(layerDimName, layerDimPos, layerDimSize);

  return ;
}

void
InFile::setGeoParam(Variable &var)
{
  if( ! var.pGM )
    return;

  // link data obj to GeoMeta obj of this variable
  var.pGM->setData( (void*) var.pMA );

  // inquire whether it is a regular grid or something else
  if( isArithmeticMean )
    var.isArithmeticMean = true;
  else
  {
     setGeoCellShape(var) ;

     // inquire layer properties
     // note: isArithmeticMean is set true in setGeoCellShape, if
     // cells got no shape.
     if( ! var.isArithmeticMean )
       setGeoLayer(var) ;
  }

/*
  if( var.isFillValue )
    var.pGM->setFillingValue( static_cast<double>(var.fillValue) ) ;
  if( var.isMissingValue )
    var.pGM->setMissingValue( static_cast<double>(var.missingValue) ) ;
*/

  return ;
}

void
InFile::setVerticesFromGridBounds(Variable &var,
  std::string &lat_bnds_name, std::string &lon_bnds_name,
  size_t lat_dim_sz, size_t lon_dim_sz)
{
  MtrxArr<double> mv_bnds;

  // vertices for the latitudes
  nc.getData(mv_bnds, lat_bnds_name, 0, -1);

  // Construct four vertices for each lat/lon cell
  MtrxArr<double> mv_vrt_lat(lat_dim_sz, lon_dim_sz, 4);
  MtrxArr<double> mv_vrt_lon(lat_dim_sz, lon_dim_sz, 4);

  double x0,x1,x2,x3;
  for( size_t i=0 ; i < lat_dim_sz ; ++i)
  {
    x0=mv_bnds.get(i,0);
    x1=mv_bnds.get(i,0);
    x2=mv_bnds.get(i,1);
    x3=mv_bnds.get(i,1);

    for( size_t j=0 ; j < lon_dim_sz ; ++j)
    {
       mv_vrt_lat.put( x0, i, j, 0);
       mv_vrt_lat.put( x1, i, j, 1);
       mv_vrt_lat.put( x2, i, j, 2);
       mv_vrt_lat.put( x3, i, j, 3);
    }
  }

  // feed vertices to the gD obj
  var.pGM->setVertices("lat", (void*) &mv_vrt_lat, "double",
               4, 2, lat_dim_sz, 0, lon_dim_sz, 1);

  // vertices for the longitudes
  nc.getData(mv_bnds, lon_bnds_name, 0, -1);
  for( size_t j=0 ; j < lon_dim_sz ; ++j)
  {
    x0=mv_bnds.get(j,0);
    x1=mv_bnds.get(j,1);
    x2=mv_bnds.get(j,0);
    x3=mv_bnds.get(j,1);

    for( size_t i=0 ; i < lat_dim_sz ; ++i)
    {
       mv_vrt_lon.put( x0, i, j, 0);
       mv_vrt_lon.put( x1, i, j, 1);
       mv_vrt_lon.put( x2, i, j, 2);
       mv_vrt_lon.put( x3, i, j, 3);
    }
  }

  // feed vertices to the gD obj
  var.pGM->setVertices("lon", (void*) &mv_vrt_lon, "double",
              4, 2, lat_dim_sz, 0, lon_dim_sz, 1);

  var.pGM->setRegularGrid(true);

  return;
}

void
InFile::setVerticesFromGridCellCentres(Variable &var,
  std::string &lat_name, std::string &lon_name,
  size_t lat_dim_sz, size_t lon_dim_sz)
{
  // Construct four vertices for each lat/lon cell
  MtrxArr<double> mv_vrt(lat_dim_sz, lon_dim_sz, 4);
  double x0,x1,x2,x3;

  MtrxArr<double> mv ;
  nc.getData(mv, lat_name, 0, -1);

  double lat_b[lat_dim_sz][2] ;
  double dif=0.;
  size_t j=0;
  for( ; j < lat_dim_sz-1 ; ++j)
  {
     // works for both j==0: +90 and -90 (or in between)
     dif=mv[j] - mv[j+1];
     lat_b[j][0] = mv[j] + dif/2.;
     lat_b[j][1] = mv[j] - dif/2.;
  }
  j=lat_dim_sz-1;
  lat_b[j][0] = mv[j] + dif/2.;
  lat_b[j][1] = mv[j] - dif/2.;

  if( lat_b[j][1] < -90. )
     lat_b[j][1] = -90. ;
  if( lat_b[j][1] > 90. )
     lat_b[j][1] = 90. ;
  j=0;
  if( lat_b[j][0] < -90. )
     lat_b[j][0] = -90. ;
  if( lat_b[j][0] > 90. )
     lat_b[j][0] = 90. ;

  for( size_t i=0 ; i < lat_dim_sz ; ++i)
  {
    x0=lat_b[i][0];
    x1=lat_b[i][0];
    x2=lat_b[i][1];
    x3=lat_b[i][1];

    for( j=0 ; j < lon_dim_sz ; ++j)
    {
       mv_vrt.put( x0, i, j, 0);
       mv_vrt.put( x1, i, j, 1);
       mv_vrt.put( x2, i, j, 2);
       mv_vrt.put( x3, i, j, 3);
    }
  }

  // feed vertices to the gD obj
  var.pGM->setVertices("lat", (void*) &mv_vrt, "double",
               4, 2, lat_dim_sz, 0, lon_dim_sz, 1);

  nc.getData(mv, lon_name, 0, -1);
  double lon_b[lon_dim_sz][2] ;
  for( j=0 ; j < lon_dim_sz-1 ; ++j)
  {
     // works for both i==0: +180 and -180 (or in between)
     dif=mv[j] - mv[j+1];
     lon_b[j][0] = mv[j] + dif/2.;
     lon_b[j][1] = mv[j] - dif/2.;
  }
  j=lon_dim_sz-1;
  lon_b[j][0] = mv[j] + dif/2.;
  lon_b[j][1] = mv[j] - dif/2.;

  if( lon_b[j][1] < -360. )
     lon_b[j][1] = -360. ;
  if( lon_b[j][1] > 360. )
     lon_b[j][1] = 360. ;
  j=0;
  if( lon_b[j][0] < -360. )
     lon_b[j][0] = -360. ;
  if( lon_b[j][0] > 360. )
     lon_b[j][0] = 360. ;

  for( j=0 ; j < lon_dim_sz ; ++j)
  {
    x0=lon_b[j][0];
    x1=lon_b[j][1];
    x2=lon_b[j][0];
    x3=lon_b[j][1];

    for( size_t i=0 ; i < lat_dim_sz ; ++i)
    {
       mv_vrt.put( x0, i, j, 0);
       mv_vrt.put( x1, i, j, 1);
       mv_vrt.put( x2, i, j, 2);
       mv_vrt.put( x3, i, j, 3);
    }
  }

  // feed vertices to the gD obj
  var.pGM->setVertices("lon", (void*) &mv_vrt, "double",
               4, 0, lat_dim_sz, 1, lon_dim_sz, 2);

  var.pGM->setRegularGrid(true);

  return;
}

