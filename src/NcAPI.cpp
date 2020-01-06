#include "nc_api.h"

NcAPI::NcAPI()
{
  init();
}

NcAPI::NcAPI( const NcAPI& )
{
  init();
}

NcAPI::NcAPI( std::string in, std::string mode)
{
  init();
  setFilename(in, mode);

  open(in, mode);
}

NcAPI::~NcAPI(void)
{
  clearLayout();
  close();
}

void
NcAPI::addAttToLayout( int varid, std::string attName, nc_type type,
    size_t len)
{
   if( varid == NC_GLOBAL )
   {
      layout.globalAttName.push_back( attName );
      layout.globalAttType.push_back( type );
      layout.globalAttValSize.push_back( len );
      layout.globalAttMap[attName] = layout.globalAttName.size() -1;
      return;
   }

   // For each variable, even if there are no attributes,
   // vectors and map have been pushed elsewhere

   layout.varAttName[varid].push_back( attName );
   // don't use varAttName.size(), it's already one too long
   layout.varAttMap[varid][attName] = layout.varAttType[varid].size();
   layout.varAttType[varid].push_back( type );
   layout.varAttValSize[varid].push_back( len );

   return;
}

void
NcAPI::clear(void)
{
  clearLayout();
  close();

  return;
}

void
NcAPI::clearLayout(void)
{
  // clear global attributes
  layout.globalAttName.clear() ;
  layout.globalAttMap.clear() ;
  layout.globalAttType.clear() ;
  layout.globalAttValSize.clear() ;

  // clear attributes
  for( size_t i=0 ; i < layout.varAttName.size() ; ++i)
  {
    layout.varAttName[i].clear() ;
    layout.varAttMap[i].clear() ;
    layout.varAttType[i].clear() ;
    layout.varAttValSize[i].clear() ;
  }

  layout.varAttName.clear() ;
  layout.varAttMap.clear() ;
  layout.varAttType.clear() ;
  layout.varAttValSize.clear() ;

  // clear variables
  for( size_t i=0 ; i < layout.varName.size() ; ++i)
  {
    layout.varName[i].clear();
    for( size_t j=0 ; j < layout.varDimName[i].size() ; ++j)
      layout.varDimName[i][j].clear();
    layout.varDimName[i].clear();
    layout.varDimSize[i].clear();

    layout.varIsRecordType.clear();
    layout.noData.clear();
    hasEffVarUnlimitedDim.clear();

    delete [] layout.rec_start[i] ;
    delete [] layout.rec_count[i] ;

    delete [] layout.varChunkSize[i] ;
  }

  layout.varName.clear();
  layout.varType.clear();
  layout.varMap.clear();
  layout.varidMap.clear();
  layout.varTypeMap.clear();
  layout.varIsRecordType.clear();
  layout.noData.clear();
  hasEffVarUnlimitedDim.clear() ;
  layout.varDimName.clear();
  layout.varDimSize.clear();
  layout.rec_start.clear();
  layout.rec_count.clear();
  layout.rec_index.clear();
  layout.rec_leg_begin.clear();
  layout.rec_leg_sz.clear() ;
  layout.rec_leg_end.clear();

  layout.varStorage.clear();
  layout.varChunkSize.clear();
  layout.varShuffle.clear();
  layout.varDeflate.clear();
  layout.varDeflateLevel.clear();
  layout.varEndian.clear();
  layout.varFletcher32.clear();

  // clear dimensions
  layout.dimName.clear();
  layout.dimSize.clear();

  return;
}

void
NcAPI::close(void)
{
  if( isThisOpen )
  {
    status=nc_close(ncid) ;
    if(status)
    {
      std::string key("NC_1_1");

      std::string capt("Could not close NetCDF file.");

      exceptionHandling(key, capt);
    }

    isThisOpen=false;
  }

  isDefineMode=false;

  return;
}

template <typename T_in, typename T_out>
void
NcAPI::convert( size_t N, T_in *arr_in, T_out *&arr_out)
{
  // Convert array.
  // The caller is responsible for deleting T_out

  arr_out = new T_out [N] ;

  for( size_t l=0; l < N ; ++l )
    arr_out[l]=static_cast<T_out>(arr_in[l]) ;

  return ;
}

template <typename T>
nc_type
NcAPI::convTypeID(T x)
{
  std::string s( typeid(x).name() );
  nc_type type=NC_NOERR;

  if( s[0] == 'c' )
    type = NC_CHAR ;
  else if( s[0] == 'f' )
    type = NC_FLOAT ;
  else if( s[0] == 'd' )
    type = NC_DOUBLE ;
  else if( s[0] == 'i' )
    type = NC_INT ;
  else if( s[0] == 'a' )
    type = NC_BYTE ;
  else if( s[0] == 's' )
    type = NC_SHORT ;
  else if( s[0] == 'h' )
    type = NC_UBYTE ;
  else if( s[0] == 't' )
    type = NC_USHORT ;
  else if( s[0] == 'j' )
    type = NC_UINT ;
  else if( s[0] == 'y' )
    type = NC_UINT64 ;
  else if( s[0] == 'x' )
    type = NC_INT64 ;
  else if( s == "Ss" )
    type = NC_STRING ;

   return type;
}

void
NcAPI::copyAtt(NcAPI &from, int varid_from, int varid, std::string aName)
{
  // attribute and data data of any type is copied.

  int attid_from;
  nc_type ncType;
  size_t sz;

  if( varid_from > NC_GLOBAL )
  {
    if( from.layout.varAttMap[varid_from].count(aName) == 0 )
      return ;

    if( (attid_from = from.getAttID(aName, varid_from)) < 0 )
      return ;

    sz = from.layout.varAttValSize[varid_from][attid_from];
    ncType=from.layout.varAttType[varid_from][attid_from] ;
  }
  else  // global Attribute
  {
    if( from.layout.globalAttMap.count(aName) == 0 )
      return ;

    attid_from = from.getAttID(aName, -1) ;
    sz = from.layout.globalAttValSize[attid_from];

    ncType=from.layout.globalAttType[from.getAttID(aName, NC_GLOBAL)];
  }

  int ncid_from = from.getNcid();
  int status2=NC_NOERR;

  switch ( ncType )
  {
    case NC_CHAR:
    {
       char *arr = new char [sz] ;
       status=nc_get_att_text(ncid_from, varid_from, aName.c_str(), arr);

       if( !status )
         status2=nc_put_att_text(ncid, varid, aName.c_str(), sz, arr);
       delete [] arr;
    }
    break;
    case NC_BYTE:
    {
       signed char *arr = new signed char [sz] ;
       status=nc_get_att_schar(ncid_from, varid_from, aName.c_str(), arr);

       if( !status )
         status2=nc_put_att_schar(ncid, varid, aName.c_str(), ncType, sz, arr);
       delete [] arr;
    }
    break;
    case NC_SHORT:
    {
       short *arr = new short [sz] ;
       status=nc_get_att_short(ncid_from, varid_from, aName.c_str(), arr);

       if( !status )
         status2=nc_put_att_short(ncid, varid, aName.c_str(), ncType, sz, arr);
       delete [] arr;
    }
    break;
    case NC_INT:
    {
       int *arr = new int [sz] ;
       status=nc_get_att_int(ncid_from, varid_from, aName.c_str(), arr);

       if( !status )
       status2=nc_put_att_int(ncid, varid, aName.c_str(), ncType, sz, arr);
       delete [] arr;
    }
    break;
    case NC_FLOAT:
    {
       float *arr = new float [sz] ;
       status=nc_get_att_float(ncid_from, varid_from, aName.c_str(), arr);

       if( !status )
         status2=nc_put_att_float(ncid, varid, aName.c_str(), ncType, sz, arr);
       delete [] arr;
    }
    break;
    case NC_DOUBLE:
    {
       double *arr = new double [sz] ;
       status=nc_get_att_double(ncid_from, varid_from, aName.c_str(), arr);

       if( !status )
         status2=nc_put_att_double(ncid, varid, aName.c_str(), ncType, sz, arr);
       delete [] arr;
    }
    break;

    case NC_UBYTE:
    {
       unsigned char *arr = new unsigned char [sz] ;
       status=nc_get_att_uchar(ncid_from, varid_from, aName.c_str(), arr);

       if( !status )
         status2=nc_put_att_uchar(ncid, varid, aName.c_str(), ncType, sz, arr);
       delete [] arr;
    }
    break;

    case NC_USHORT:
    {
       unsigned short *arr = new unsigned short [sz] ;
       status=nc_get_att_ushort(ncid_from, varid_from, aName.c_str(), arr);

       if( !status )
         status2=nc_put_att_ushort(ncid, varid, aName.c_str(), ncType, sz, arr);
       delete [] arr;
    }
    break;

    case NC_UINT:
    {
       unsigned int *arr = new unsigned int [sz] ;
       status=nc_get_att_uint(ncid_from, varid_from, aName.c_str(), arr);

       if( !status )
         status2=nc_put_att_uint(ncid, varid, aName.c_str(), ncType, sz, arr);
       delete [] arr;
    }
    break;

    case NC_UINT64:
    {
       unsigned long long *arr = new unsigned long long [sz] ;
       status=nc_get_att_ulonglong(ncid_from, varid_from, aName.c_str(), arr);

       if( !status )
         status2=nc_put_att_ulonglong(ncid, varid, aName.c_str(), ncType, sz, arr);
       delete [] arr;
    }
    break;

    case NC_INT64:
    {
       long long *arr = new long long [sz] ;
       status=nc_get_att_longlong(ncid_from, varid_from, aName.c_str(), arr);

       if( !status )
         status2=nc_put_att_longlong(ncid, varid, aName.c_str(), ncType, sz, arr);
       delete [] arr;
    }
    break;
  }

  if(status)
  {
    std::string key("NC_3_1");
    std::string capt("Could not get attribute values.") ;

    std::string text("Variable (from) <");
    text += from.getVarnameFromVarID(varid_from);
    text += ">, variable (to) <";
    text += getVarnameFromVarID(varid);
    text += ">";

    exceptionHandling(key, capt, text, getVarnameFromVarID(varid));
  }
  else if(status2 )  // status2 was onnly set for status==0
  {
    status=status2;
    std::string key("NC_3_2");
    std::string capt("Could not put attribute values.") ;

    std::string text("Variable (from) <");
    text += from.getVarnameFromVarID(varid_from);
    text += ">, variable (to) <";
    text += getVarnameFromVarID(varid);
    text += ">, attribute <";
    text += aName;
    text += ">";

    exceptionHandling(key, capt, text, getVarnameFromVarID(varid));
  }

  return ;
}

void
NcAPI::copyAtts(NcAPI &from, int varid_from, int varid,
  std::vector<std::string> *paN)
{
  // Copy all attributes of given variable from source and
  // add to variable of this.
  // If paN is empty, then copy all attributes of varid_from.

  if( varid_from == NC_GLOBAL )
  {
    copyGlobalAtts(from, varid_from, varid, paN) ;
    return;
  }

  int attNum;

  int ncid_from=from.getNcid();

  // Get number from the input nc-file.
  if( paN )
    attNum=paN->size();
  else
  {
    status=nc_inq_varnatts(ncid_from, varid_from, &attNum) ;
    if(status)
    {
      std::string key("NC_3_3");
      std::string capt("Could not get number of attributes.") ;

      std::string text("Variable (from) <");
      text += from.getVarnameFromVarID(varid_from);
      text += ">, variable (to) <";
      text += getVarnameFromVarID(varid);
      text += ">";

      exceptionHandling(key, capt, text, getVarnameFromVarID(varid));
    }
  }

  if( attNum == 0 )
    return ;  // no global attributes

  // copy attributes
  enableDefMode();

  std::string aName;

  for( size_t j=0 ; j < from.layout.varAttName[varid_from].size() ; ++j)
  {
    // only those atts given in pvs
    if( paN )
    {
       bool isContinue=true;
       std::string t;
       for( int i=0 ; i < attNum ; ++i )
       {
         if(  (*paN)[i] == from.layout.varAttName[varid_from][j] )
         {
           isContinue=false;
           aName = (*paN)[i];
         }
       }

       if( isContinue )
         continue;
    }
    else
      aName = from.layout.varAttName[varid_from][j];

    copyAtt(from, varid_from, varid, aName );

//    nc_copy_att(ncid_from, varid_from,
//           from.layout.varAttName[varid_from][j].c_str(), ncid, varid );

    addAttToLayout( varid, aName,
                        from.layout.varAttType[varid_from][j],
                           from.layout.varAttValSize[varid_from][j] ) ;
  }

  return;
}

void
NcAPI::copyGlobalAtts(NcAPI &from, int varid_from, int varid,
  std::vector<std::string> *paN)
{
  // Copy all global attributes from source and add to *this.
  // If paN is empty, then copy all attributes of varid_from.
  int attNum;

  int ncid_from=from.getNcid();

  // Get number from the input nc-file.
  if( paN )
    attNum=paN->size();
  else
  {
    status = nc_inq_varnatts(ncid_from, varid_from, &attNum) ;
    if(status)
    {
      std::string key("NC_3_4");
      std::string capt("Could not get number of global attributes.") ;

      std::string text("Variable (from) <");
      text += from.getVarnameFromVarID(varid_from);
      text += ">, variable (to) <" ;
      text += getVarnameFromVarID(varid);
      text += ">" ;

      exceptionHandling(key, capt, text, getVarnameFromVarID(varid));
    }
  }

  if( attNum == 0 )
    return ;  // no global attributes

  // copy attributes
  enableDefMode();

  std::string aName;

  for( size_t j=0 ; j < from.layout.globalAttName.size() ; ++j)
  {
    // only those atts given in pvs
    if( paN )
    {
       bool isContinue=true;
       std::string t;
       for( int i=0 ; i < attNum ; ++i )
       {
         if(  (*paN)[i] == from.layout.globalAttName[j] )
         {
           isContinue=false;
           aName = (*paN)[i];
         }
       }

       if( isContinue )
         continue;
    }
    else
      aName = from.layout.globalAttName[j];

    copyAtt(from, varid_from, varid, aName );

    addAttToLayout( varid, aName,
                        from.layout.globalAttType[j],
                           from.layout.globalAttValSize[j] ) ;
  }

  return;
}

void
NcAPI::copyChunking(NcAPI &from, int varid_from, int varid_to)
{
  // Pushes have been done in defineVar().
  enableDefMode();

  size_t sz = layout.varDimName[varid_to].size();

  if( from.layout.format == NC_FORMAT_NETCDF4 )
  {
    layout.varStorage[varid_to] = from.layout.varStorage[varid_from] ;

    for( size_t i=0 ; i < sz ; ++i )
      layout.varChunkSize[varid_to][i] =
           from.layout.varChunkSize[varid_from][i];
  }
  else
  {
    layout.varStorage[varid_to] = NC_CONTIGUOUS ;

    for( size_t i=0 ; i < layout.varDimName[varid_to].size() ; ++i )
    {
      // true for strict
      if( isDimUnlimited(layout.varDimName[varid_to][i]), true )
      {
        layout.varChunkSize[varid_to][i] = 1 ;
        layout.varStorage[varid_to] = NC_CHUNKED ;
      }
      else
        layout.varChunkSize[varid_to][i] = layout.varDimSize[varid_to][i] ;
    }
  }

  status=nc_def_var_chunking(ncid, varid_to,
       layout.varStorage[varid_to], layout.varChunkSize[varid_to]);

  if(status)
  {
    std::string key("NC_4_1");
    std::string capt("Could not define chunking.") ;

    std::string text("Variable (from) <");
    text += from.getVarnameFromVarID(varid_from);
    text += ">, variable (to) <";
    text += getVarnameFromVarID(varid_to);
    text += ">";

    exceptionHandling(key, capt, text, getVarnameFromVarID(varid_to));
  }

  return;
}

void
NcAPI::copyData(NcAPI &from, size_t rec_in, size_t rec_out)
{
  // Copy data of the rec-th record for all variables

  for( size_t i=0 ; i < from.layout.varName.size() ; ++i )
    if( from.layout.varIsRecordType[i] )
      copyData(from, from.layout.varName[i], rec_in, rec_out);

  return;
}

void
NcAPI::copyData(NcAPI &from, std::string srcVName, std::string vName)
{
  if( vName.size() == 0 )
    vName=srcVName;

  int varid = getVarID(vName);
  int varid_from = from.getVarID(srcVName);

  size_t numRec=1;  // for non-record data sets

  if( layout.varIsRecordType[varid] )
    numRec = from.getNumOfRecords();

  for( size_t rec=0 ; rec < numRec ; ++rec )
    copyData(from, varid_from, varid, rec);

  return ;
}

void
NcAPI::copyData(NcAPI &from, std::string srcVName,
     std::string vName, size_t rec_in, size_t rec_out)
{
  if( rec_out == UINT_MAX )
    rec_out = rec_in;

  if( vName.size() == 0 )
    vName=srcVName;

  int varid = getVarID(vName);
  int varid_from = from.getVarID(srcVName);

  copyData(from, varid_from, varid, rec_in, rec_out);
}

void
NcAPI::copyData(NcAPI &from, int varid_from, int varid,
  size_t rec_in, size_t rec_out )
{
  if( rec_out == UINT_MAX )
    rec_out = rec_in;

  disableDefMode();

  void *p = from.getData(varid_from, rec_in);

  if( p == 0 )
    return;

  switch ( layout.varType[varid] )
  {
    case NC_CHAR:
      putData(rec_out, varid, static_cast<char*>(p) ) ;
    break;
    case NC_BYTE:
      putData(rec_out, varid, static_cast<signed char*>(p) ) ;
    break;
    case NC_SHORT:
      putData(rec_out, varid, static_cast<short*>(p) ) ;
    break;
    case NC_INT:
      putData(rec_out, varid, static_cast<int*>(p) ) ;
    break;
    case NC_FLOAT:
      putData(rec_out, varid, static_cast<float*>(p) ) ;
    break;
    case NC_DOUBLE:
      putData(rec_out, varid, static_cast<double*>(p) ) ;
    break;
    case NC_UBYTE:
      putData(rec_out, varid, static_cast<unsigned char*>(p) ) ;
    break;
    case NC_USHORT:
      putData(rec_out, varid, static_cast<unsigned short*>(p) ) ;
    break;
    case NC_UINT:
      putData(rec_out, varid, static_cast<unsigned int*>(p) ) ;
    break;
    case NC_UINT64:
      putData(rec_out, varid, static_cast<unsigned long long*>(p) ) ;
    break;
    case NC_INT64:
      putData(rec_out, varid, static_cast<long long*>(p) ) ;
    break;
    case NC_STRING:
      if( rec_val_string[varid].size() > 0 )
        nc_free_string( layout.recSize[varid],
             rec_val_string[varid].begin() );

      putData(rec_out, varid, static_cast<char**>(p));
    break;
  }

  return;
}

void
NcAPI::copyDeflate(NcAPI &from, int varid_from, int varid_to)
{
  // Pushes have been done in defineVar().
  if( from.layout.format == NC_FORMAT_NETCDF4 )
  {
    layout.varShuffle[varid_to] = from.layout.varShuffle[varid_from];
    layout.varDeflate[varid_to] = from.layout.varDeflate[varid_from] ;
    layout.varDeflateLevel[varid_to] = layout.varDeflateLevel[varid_from] ;
  }
  else
  {
    layout.varShuffle[varid_to] = 0;
    layout.varDeflate[varid_to] = 0 ;
    layout.varDeflateLevel[varid_to] = 0 ;
  }

  enableDefMode();

  status =
        nc_def_var_deflate(ncid, varid_to,
        layout.varShuffle[varid_to],
        layout.varDeflate[varid_to],
        layout.varDeflateLevel[varid_to] );

  if(status)
  {
    std::string key("NC_4_2");
    std::string capt("Could not define deflate.") ;

    std::string text("Variable (from) <");
    text += from.getVarnameFromVarID(varid_from);
    text += ">, variable (to) <" ;
    text += getVarnameFromVarID(varid_to);
    text += ">" ;

    exceptionHandling(key, capt, text, getVarnameFromVarID(varid_to));
 }

  return;
}

void
NcAPI::copyDim(NcAPI &from, std::string dName)
{
  // define dimension out of those from source u
  std::string uName;

  // loop over dimensions and pick the one with name==d
  for( size_t i=0 ; i < from.layout.dimName.size() ; ++i)
  {
    uName=from.layout.dimName[i];

    // find the right one or take them all
    if( dName.size() > 0 && uName != dName )
      continue;

    // safe for already defined dimensions
    if( uName == from.unlimitedDimName )
      defineDim(uName, NC_UNLIMITED) ;
    else
      defineDim(uName, from.layout.dimSize[i]) ;
  }

  return;
}

void
NcAPI::copyEndian(NcAPI &from, int varid_from, int varid_to)
{
  // Pushes have been done in defineVar().
  layout.varEndian[varid_to] = from.layout.varEndian[varid_from] ;

  enableDefMode();

  status = nc_def_var_endian(ncid, varid_to,
       layout.varEndian[varid_to]);

  if(status)
  {
    std::string key("NC_4_3");
    std::string capt("Could  not define endianess.") ;

    std::string text("Variable (from) <");
    text += from.getVarnameFromVarID(varid_from);
    text += ">, variable (to) <" ;
    text += getVarnameFromVarID(varid_to);
    text += ">" ;

    exceptionHandling(key, capt, text, getVarnameFromVarID(varid_to));
  }

  return;
}

void
NcAPI::copyFillValue(NcAPI &from, int varid_from, int varid_to)
{
  enableDefMode();

  int no_fill;

  // note: NULL means ignored
  status = nc_inq_var_fill(from.ncid, varid_from, &no_fill, NULL);

  if(status)
  {
    std::string key("NC_3_5");
    std::string capt("Could not get _FillValue.") ;

    std::string text("Variable (from) <");
    text += from.getVarnameFromVarID(varid_from);
    text += ">, variable (to) <" ;
    text += getVarnameFromVarID(varid_to);
    text += ">" ;

    exceptionHandling(key, capt, text, getVarnameFromVarID(varid_to));
  }
  else if( no_fill == 1 )
    return;  // no fill mode for this variable (only NC4)

  status=nc_def_var_fill(ncid, varid_to, no_fill, NULL) ;
  if(status)
  {
    if(status != NC_ENOTNC4 )
    {
      std::string key("NC_3_6");
      std::string capt("Could not define _FillValue.") ;

      std::string text("Variable (from) <");
      text += from.getVarnameFromVarID(varid_from);
      text += ">, variable (to) >" ;
      text += getVarnameFromVarID(varid_to);
      text += ">" ;

      exceptionHandling(key, capt, text, getVarnameFromVarID(varid_to));
    }
  }

  // copy _FillValue attribute, if specified.
  copyAtt(from, varid_from, varid_to, "_FillValue");

  return;
}

void
NcAPI::copyFletcher32(NcAPI &from, int varid_from, int varid_to)
{
  // Pushes have been done in defineVar().
  if( from.layout.format == NC_FORMAT_NETCDF4 )
    layout.varFletcher32[varid_to]
         = from.layout.varFletcher32[varid_from] ;
  else
    layout.varFletcher32[varid_to] = 0;

  enableDefMode();

  status = nc_def_var_fletcher32(ncid, varid_to,
       layout.varFletcher32[varid_to]) ;

  if(status)
  {
    std::string key("NC_4_4");
    std::string capt("Could not define Fletcher32 properties.") ;

    std::string text("Variable (from) <");
    text += from.getVarnameFromVarID(varid_from);
    text += ">, variable (to) <";
    text += getVarnameFromVarID(varid_to);

    exceptionHandling(key, capt, text, getVarnameFromVarID(varid_to));
  }

  return;
}

void
NcAPI::copyGlobalAtts(NcAPI &from)
{
  // copy global Attributes;
  int attNum;

  // Global attributes are associated with variable name 'GLOBAL'.
  // Get number from the input nc-file.
  status = nc_inq_varnatts(from.getNcid(), NC_GLOBAL, &attNum) ;

  if(status)
  {
    std::string key("NC_3_7");

    std::string capt("Could not get number of global attributes.");
    std::string text ;

    exceptionHandling(key, capt, text);
  }

  if( attNum == 0 )
    return ;  // no global attributes

  // copy attributes
  enableDefMode();

  int ncid_from=from.getNcid();

  for( size_t j=0 ; j < from.layout.globalAttName.size() ; ++j)
    nc_copy_att(ncid_from, NC_GLOBAL,
           from.layout.globalAttName[j].c_str(), ncid, NC_GLOBAL );

  // copy global attributes
  layout.globalAttName    = from.layout.globalAttName;
  layout.globalAttMap     = from.layout.globalAttMap;
  layout.globalAttType    = from.layout.globalAttType;
  layout.globalAttValSize = from.layout.globalAttValSize;

  return;
}

void
NcAPI::copyLayout(NcAPI &from)
{
  // Copy definitions and settings from 'from'.

  // retains the order of dimensions
//  for( int i=0 ; i < from.layout.dimName.size() ; ++i )
//    defineDim(&from, from.layout.dimName[i]);
  copyDim(from);

  for( size_t i=0 ; i < from.layout.varName.size() ; ++i )
  {
    if( from.layout.varIsRecordType[i] )
      // definitions and attribute (leaving define mode at return)
      copyVarDef(from,from.layout.varName[i]);
    else // also the data
      copyVar(from,from.layout.varName[i]);
  }

  copyGlobalAtts(from);

  return;
}

void
NcAPI::copyRecord(NcAPI &from, size_t rec_in, size_t rec_out)
{
  // Copy records of unlimited vars from 'from'.

  for( size_t i=0 ; i < from.layout.varName.size() ; ++i )
    if( from.hasEffVarUnlimitedDim[i] )
      copyData(from,from.layout.varName[i], rec_in, rec_out);

  return;
}

bool
NcAPI::copyVarDef(NcAPI &from, std::string srcVName ,std::string newVarName, bool isKeepOrder)
{
  // This member is used recursively, if the variable depends on
  // dimensions whose name is also a variable name of its own.
  // Lacking dimensions will be defined.

  // isKeepOrder retains the original sequence of variables,
  // thus a recursive search is prevented.

  // If newVarName is given, then transfer from
  // old vName to the new one.
  std::string vName;

  int varid_from=from.getVarID(srcVName) ;

  if( newVarName.size() == 0 )
  {
    // set temp name for an unlimited variable to be copied
    vName=srcVName;
  }
  else
    vName = newVarName ;

  // Was variable already copied?
  // This will mainly prevent multiple trials
  // if a dimension is also a variable.
  for( size_t i=0 ; i < layout.varName.size() ; ++i)
  {
    if( vName == layout.varName[i] )
      return false;
  }

  size_t from_dimSz=from.layout.varDimName[varid_from].size() ;

  // get dimensions of a variable
  for( size_t j=0 ; j < from_dimSz ; ++j)
  {
    // Is dim from source also dim of *this? Call is safe.
    defineDim( from.layout.varDimName[varid_from][j],
       from.layout.dimSize[ from.layout.dimMap[
                from.layout.varDimName[varid_from][j] ] ] );
  }

  //define variables:
  //1. Is any dim of current var also a var? Yes, then
  //   copy the var with the name of the dim by recursion.
  //   This may scatter the original sequence.
  if( ! isKeepOrder && vName == from.layout.varName[varid_from] )
  {
    std::string sV(from.layout.varName[varid_from]);
    for( size_t j=0 ; j < from_dimSz ; ++j)
    {
      std::string sD(from.layout.dimName[j]);
      // if next is true, then a dim is also a variable.
      // ... > 1 prevents a dead-lock.

      if( sV == sD && from_dimSz > 1 )
        copyVar(from, sV);
    }
  }

  // 2. Define the variable.
  // Return value was checked in defineVar()
  int varid=defineVar(vName, from.layout.varType[varid_from],
                  from.layout.varDimName[varid_from]) ;

  // chunking. Vectors were adjusted in defineVar().
  if( isChunking )
    copyChunking(from, varid_from, varid);

  // deflation. Vectors were adjusted in defineVar().
  if( isDeflate )
    copyDeflate(from, varid_from, varid);

  // endian. Vectors were adjusted in defineVar().
  if( isEndianess )
    copyEndian(from, varid_from, varid);

  // FillValue. Copy _FillValue attribute, if any. For NC4 inquire
  // also if no_fill was set. Yes? Set! (not available for NC3)
  copyFillValue(from, varid_from, varid);

  // Fletcher32. Vectors were adjusted in defineVar().
  if( isFletcher32 )
    copyFletcher32(from, varid_from, varid);

  // copy the attributes of variables.
  copyAtts(from, varid_from, varid);

  return true;
}

bool
NcAPI::copyVar(NcAPI &from, std::string srcVName ,std::string newVName, bool isKeepOrder)
{
  // definitions and attribute (leaving define mode at return)
  copyVarDef(from, srcVName, newVName, isKeepOrder);

  // it is safe to call for data with unlimited dimension.
  if( newVName.size() == 0 )
    copyData(from, srcVName);
  else
    copyData(from, srcVName, newVName);

  return true ;
}

bool
NcAPI::create(std::string f, std::string mode, bool isExitOnError)
{
  if( f.size() == 0 && !isNcFilenameChanged )
    return true ;  // filename is unchanged

  if( isThisOpen ) // only if previously opened
     clearLayout();

  if( f.size() > 0 )
    setFilename(f, mode);

  Split fam(fileAccessMode, "|");

  int cmode=0 ; // default == NC_NOCLOBBER

  for( size_t i=0 ; i < fam.size() ; ++i )
  {
    if( fam[i] == "NC_NETCDF4" )
    {
      isNC4 = true;
      cmode |= NC_NETCDF4;
    }
    else if( fam[i] == "NC_NOCLOBBER" )
      cmode |= NC_NOCLOBBER;

    else if( fam[i] == "NC_64BIT_OFFSET" )
      cmode |= NC_64BIT_OFFSET;

    else if( fam[i] == "NC_SHARE" )
      cmode |= NC_SHARE;

    else if( fam[i] == "NC_CLASSIC_MODEL" )
    {
      cmode |= NC_CLASSIC_MODEL ;
      isNC4=false;
    }
    else if( fam[i].substr(0,8) == "COMPRESS" )
    {
       xtrnlSet.isDeflate=true;
       xtrnlSet.shuffle=0;
       xtrnlSet.deflate=1;
       xtrnlSet.deflate_level= static_cast<int>
        (hdhC::string2Double(fam[i]) );
    }
  }

  layout.format=cmode;  // by default, i.e. (1)

  // the only spot to create a file
  status = nc_create(ncFilename.c_str(), cmode, &ncid) ;

  if(status)
  {
    if( isExitOnError )
    {
      std::string key("NC_1_2");

      std::string capt("Could not do nc_create().");
      std::string text ;

      exceptionHandling(key, capt, text);

      isThisOpen=false;
      return isThisOpen;
    }
    else
      return false;
  }
  else
    isDefineMode=true;

  // set flag for opened
  isThisOpen=true;

  // reset new-filename flag
  isNcFilenameChanged=false;

  return isThisOpen;
}

int
NcAPI::defineDim(std::string name, size_t dimsize)
{
  int dim_ID;

  // test whether a dimension is already defined
  for( size_t j=0 ; j < layout.dimName.size() ; ++j)
     if( name == layout.dimName[j] )
       return -1;

  enableDefMode();

  // Note: NC_UNLIMITED == 0L
  status=nc_def_dim(ncid, name.c_str(), dimsize, &dim_ID);

  if(status)
  {
    std::string key("NC_5_1");
    std::string capt("Could not define variable.");

    std::string text("Dimension <");
    text += name ;
    text += ">:";

    exceptionHandling(key, capt, text, name);
  }

  if( dimsize == NC_UNLIMITED )
  {
    unlimitedDimID=dim_ID;
    unlimitedDimName=name;
  }

  // update layout
  layout.dimName.push_back( name );
  layout.dimMap[name] = dim_ID ;
  layout.dimSize.push_back( dimsize ) ;

  return dim_ID;
}

void
NcAPI::defineLayout(NcAPI &from)
{
  return;
}

int
NcAPI::defineVar(std::string vName, nc_type type)
{
  std::vector<std::string> dimName;
  return defineVar(vName, type, dimName);
}

int
NcAPI::defineVar(std::string vName, nc_type type,
  std::vector<std::string> &currDimName)
{
  int varid;

  // test whether a variable is already defined
  for( size_t j=0 ; j < layout.varName.size() ; ++j)
     if( vName == layout.varName[j] )
       return -1;

  enableDefMode();

  // test whether all partial dimensions are defined
  std::vector<size_t> currDimSize;
  for( size_t i=0 ; i < currDimName.size() ; ++i )
    currDimSize.push_back( getDimSize(currDimName[i]) );

  for( size_t i=0 ; i < currDimName.size() ; ++i )
  {
    bool is=false;
    for( size_t j=0 ; j < layout.dimName.size() ; ++j)
    {
       if( currDimName[i] == layout.dimName[j] )
       {
          is=true;
          break;
       }
    }

    if( ! is )
    {
      status=0;
      std::string key("NC_5_2");
      std::string capt("Undefined dimension name in parameter list.");

      std::string text("Variable <");
      text += vName;
      text += ">";

      exceptionHandling(key, capt, text, vName);
    }
  }

  // prepare a pointer to an array containing the dim-ids
  int *pDim = 0;
  if( currDimName.size() )
    pDim = new int [currDimName.size()];

  for( size_t i=0 ; i < currDimName.size() ; ++i )
     pDim[i] = layout.dimMap[ currDimName[i] ] ;

  // Here, a variable is defined
  status=nc_def_var(ncid, vName.c_str(), type,
          static_cast<int>(currDimName.size()), pDim, &varid);

  if(status)
  {
    std::string key("NC_5_3");
    std::string capt("Could not define variable.") ;

    std::string text("Variable <");
    text += vName;
    text += ">";

    exceptionHandling(key, capt, text, vName);
  }

  // update layout contents
  layout.varName.push_back( vName );
  layout.varMap[vName] = varid ;
  layout.varidMap[varid] = vName ;
  layout.varType.push_back( type ) ;
  layout.varTypeMap[vName]= type ;
  layout.varDimName.push_back( currDimName );
  layout.varDimSize.push_back( currDimSize );

  layout.varIsRecordType.push_back(false) ;
  layout.noData.push_back(false) ;
  hasEffVarUnlimitedDim.push_back( false) ;

  isDataEmpty.push_back(true);
  isDataEmptyChecked.push_back(false);

  for( size_t i=0 ; i < currDimName.size() ; ++i )
  {
    if( currDimName[i] == unlimitedDimName )
    {
      layout.varIsRecordType.back()=true;
      hasEffVarUnlimitedDim.back()=true;

      if( !layout.varIsRecordType.back() )
         hasEffVarUnlimitedDim.back()=true ;
    }
  }

  // push internal vectors for holding data and fill values
  layoutVarAttPushes();
  layoutVarDataPushes(vName, type);

  // shape of corners and edges of data arrays
  layout.rec_start.push_back( new size_t [currDimName.size()] );
  layout.rec_count.push_back( new size_t [currDimName.size()] );
  layout.rec_index.push_back( UINT_MAX ); //indicates non-record

  layout.rec_leg_begin.push_back( 0 );
  layout.rec_leg_sz.push_back( 0 );  // default for large multi-dim arrays
  layout.rec_leg_end.push_back( 0 );

  size_t sz=1;  // Set 1 for limited variables, not used.

  for( size_t i=0 ; i < currDimName.size() ; ++i )
  {
    size_t m = layout.dimMap[ currDimName[i] ] ;

    if( isDimUnlimited()
          && m == static_cast<size_t>(unlimitedDimID) )
    {
      layout.rec_start.back()[i]=0;  //place-holder for current record
      layout.rec_count.back()[i]=1;
      layout.rec_index.back()=i;
      continue;
    }

    size_t dSz = layout.dimSize[ m ] ;
    sz *=  dSz;

    layout.rec_start.back()[i]=0;
    layout.rec_count.back()[i]=dSz;
  }

  if( currDimName.size() == 0 )
  {
    layout.rec_start.back()[0]=0;
    layout.rec_count.back()[0]=1;
  }

  layout.recSize.push_back( sz );

  // chunking
  size_t n=1;
  if( currDimName.size() > 1 )
    n=currDimName.size();

  layout.varChunkSize.push_back( new size_t [n] );
  layout.varStorage.push_back( 0 );

  // push new members
  layout.varShuffle.push_back( 0 );
  layout.varDeflate.push_back( 0 );
  layout.varDeflateLevel.push_back( 0 );
  if( xtrnlSet.isDeflate )
  {
    layout.varShuffle.back()=xtrnlSet.shuffle;
    layout.varDeflate.back()=xtrnlSet.deflate;
    layout.varDeflateLevel.back()=xtrnlSet.deflate_level;
    setDeflate(varid,
       xtrnlSet.shuffle,
         xtrnlSet.deflate,
           xtrnlSet.deflate_level) ;
  }

  layout.varEndian.push_back( 0 );

  layout.varFletcher32.push_back( 0 );

  delete [] pDim;

  return varid;
}

size_t
NcAPI::delAtt(std::string aName, int varid)
{
   // Deletes values of an attribute. Safe, if not defined.
   // Return the index of the attribute in the layout vectors or
   // UINT_MAX, if nothing was deleted.

   if( varid == -1 )
   {
     enableDefMode();

     for( size_t i=0 ; i < layout.globalAttName.size() ; ++i)
     {
        if( aName == layout.globalAttName[i] )
        {
          status=nc_del_att(ncid, varid, aName.c_str());

          if(status)
          {
            std::string key("NC_6_1");
            std::string capt("Could not delete attribute.") ;

            std::string text("Variable <");
            text += getVarnameFromVarID(varid);
            text += ">, attribute <";
            text += aName;
            text += ">";

            exceptionHandling(key, capt, text, getVarnameFromVarID(varid));
          }

          // clear for specific global attribute
          layout.globalAttName[i].clear();
          layout.globalAttMap.erase("aName");
          layout.globalAttType[i]=NC_NAT;
          layout.globalAttValSize[i] = 0;
          return i;
        }
     }
   }
   else if( varid < 0 )  // invalid variable
     ;
   else// variable att
   {
     int i_sz=layout.varAttName.size();
     if( i_sz == 0 || i_sz <= varid )
       return UINT_MAX ;

     enableDefMode();

     for( size_t i=0 ; i < layout.varAttName[varid].size() ; ++i)
     {
        if( aName == layout.varAttName[varid][i] )
        {
          status=nc_del_att(ncid, varid, aName.c_str());

          if(status)
          {
            std::string key("NC_6_2");
            std::string capt("Could not delete attribute.") ;

            std::string text("Variable <");
            text += getVarnameFromVarID(varid);
            text += ">, attribute <";
            text += aName;

            exceptionHandling(key, capt, text, getVarnameFromVarID(varid));
          }

          // clear for specific global attribute
          layout.varAttName[varid][i].clear();
          layout.varAttMap[varid].erase("aName");
          layout.varAttType[varid][i]=NC_NAT;
          layout.varAttValSize[varid][i] = 0;
          return i;
        }
     }
   }

  return UINT_MAX ;
}

inline
void
NcAPI::disableDefMode(void)
{
  if( isDefineMode )
  {
    nc_enddef(ncid) ;
    isDefineMode=false;
  }

  return;
}

bool
NcAPI::isEmptyData(int varid)
{
  if( isDataEmptyChecked[varid] )
      return isDataEmpty[varid] ;

  MtrxArr<double> ma;
  size_t nrec=getNumOfRecords();
  if(nrec == 0)
      ++nrec;

  for( size_t rec=0 ; rec < nrec ; ++rec )
  {
    temporarily_disable_annotation();

    getData(ma, varid, rec);

    isDataEmpty[varid] = ma.validRangeBegin.size() ? false : true ;

    if( !isDataEmpty[varid] )
        break;
  }

  isDataEmptyChecked[varid]=true;

  return isDataEmpty[varid] ;
}

inline
void
NcAPI::enableDefMode(void)
{
  if( isDefineMode )
      return;
  else
  {
    nc_redef(ncid) ;
    isDefineMode=true;
  }
}

void
NcAPI::exceptionError(std::string str)
{
  // Occurrence of an error stops the run immediately.
  xcptn.ofsError=0;
  xcptn.strError = "error_NC_API.txt" ;

  // open file for appending data
  if( ! xcptn.ofsError )
    xcptn.ofsError
       = new std::ofstream(xcptn.strError.c_str(), std::ios::app);

  *xcptn.ofsError << str << std::endl;

  return ;
}

void
NcAPI::exceptionHandling(std::string key,
  std::string capt, std::string text,
  std::string vName)
{
   if( status )
   {
     text += ", NetCDF status=";
     text += hdhC::itoa(status);
     text += "; ";
   }

   if( status )
   {
     text += "\n";
     text += nc_strerror(status) ;
   }

   if( notes )
   {

//     bool sendEmail=false;
//     std::string table="*";
//     std::string task="L1";
//     notes->push_front(key , vName, sendEmail, table, "L1", "");

     if( ! notes->inq(key, vName ) )
       return ;  // if discarded

     (void) notes->operate(capt, text);

      notes->setCheckStatus("NC", "FAIL");
   }
   else
   {
     text = capt + text ;
     print_error(text);
   }

   return ;
}

void
NcAPI::generate(std::string rFile)
{

  // Parse ncgen.conf file.
  // This text file resembles layout output by ncdump,
  // except that a trailing section with instructions
  // for limited variables exists.

  ReadLine rC( rFile );

  // these booleans are used to allow nested sections
  bool isDummy=false;
  bool isDims=false;
  bool isVars=false;
  bool isWrite=false;
  bool isGlobal=false;
  bool *prevBool; // set the last section-bool to false

  prevBool = &isDummy;

  std::vector<std::string> dimStrs;
  std::vector<int> dimIndexes;

  std::string str;
  std::string newFilename;

  std::string sectStr;

  while( ! rC.readLine() )
  {
     // Note: ReadLine skips empty and white lines by default
     Split split( rC.getLine() );

     // identify sections ( these have no leading blanks )
     sectStr = rC.getLine();
     for( size_t i=0 ; i < sectStr.size() ; ++i )
       sectStr[i] = toupper(sectStr[i]);

     if( sectStr.substr(0,6) == "NETCDF" )
     {
       // don't care about leading blanks.
       newFilename=split[1];
       if( newFilename[newFilename.size()-1] == '{' )
         newFilename.erase(newFilename.size()-1);
       if( newFilename.substr(newFilename.size()-3) != ".nc" )
         newFilename += ".nc";

       open( newFilename, "Replace" );

       *prevBool=false;
        prevBool = &isDummy ;
        continue; //read next line
     }
     if( sectStr.substr(0,10) == "DIMENSIONS" )
     {
       *prevBool=false;
       isDims = true ;
       prevBool = &isDims;
       continue; // read next line
     }
     if( sectStr.substr(0,9) == "VARIABLES" )
     {
       // section with reading
       *prevBool=false;
       isVars = true ;
       prevBool = &isVars;
       continue; //read next line
     }
     if( sectStr.substr(0,5) == "WRITE" )
     {
       // section with reading; no continue
       *prevBool=false;
       isWrite = true ;
       prevBool = &isWrite;
     }
     if( sectStr.substr(0,6) == "GLOBAL" )
     {
       // section with reading; no continue
       *prevBool=false;
       isGlobal = true ;
       prevBool = &isGlobal;
    }

     // Now go through the sections.
     Split splitA(rC.getLine(), " \t=", true);

     if( isDims )
     {
        // possible design: spaces part1 = part2
        double a;
        if( hdhC::string2Double(splitA[1], 1, &a) )
          defineDim( splitA[0], static_cast<size_t>( a ));
        else
          // unlimited dim
          defineDim( splitA[0]);

        dimStrs.push_back( splitA[0] );
        continue;
     }

     if( isVars )
     {
       // Required that the dimension section was parsed.
       // Parse each variable in the section
       generateParseVar(rC, dimStrs) ;
       continue;
     }

     if( isGlobal )
     {
       generateParseAtt(rC);
       continue;
     }

     if( isWrite )
     {
       // write calculated variables
       while( ! rC.readLine() )
       {
          std::string s=rC.getLine();

         std::string vName( rC.getItem(0) );
        if( vName[ vName.size()-1 ] == ':' )
          vName.erase(vName.size()-1, 1);

//         NcVar *pV=nc->get_var(vName.c_str());
//         NcType type = pV->type();

//         generateWriteLimited(rC.getLine(), vName, type );
       }
    }

  }

  return;
}

void
NcAPI::generateParseAtt(ReadLine &rC, std::string vName)
{
  // parse attributes
  // possible format:
  //   i) spaces varname: part1 = part2_may_have_'white space'
  //      separation at blank and '='
  //  ii) spaces part1 = ...
  // Note: ii) should be preferred, because then this is a cross-check.

  // types netCDF knows about.
  std::string vType[] = {"byte", "char", "short", "int",
     "float", "double", "ubyte", "ushort", "uint", "int64",
     "uint64", "string" };

  // types netCDF knows about.
  std::map<std::string, nc_type> typeMap;
  typeMap[vType[0]] = NC_BYTE ;
  typeMap[vType[1]] = NC_CHAR ;
  typeMap[vType[2]] = NC_SHORT ;
  typeMap[vType[3]] = NC_INT ;
  typeMap[vType[4]] = NC_FLOAT ;
  typeMap[vType[5]] = NC_DOUBLE ;
  typeMap[vType[6]] = NC_UBYTE ;
  typeMap[vType[7]] = NC_USHORT ;
  typeMap[vType[8]] = NC_UINT ;
  typeMap[vType[9]] = NC_INT64 ;
  typeMap[vType[10]] = NC_UINT64 ;
  typeMap[vType[11]] = NC_STRING ;

  Split splitA;
  splitA.addSeparator(':');
  splitA.addSeparator('=');
  std::string aName;  //name of attribute
  std::string aTypeName;

  // loop through a block
  while( ! rC.readLine() )
  {
    splitA=rC.getLine();

    size_t startIndex=0; // for global att

    // Left the block? Or global att?
    // global attribute, if vName is empty.
    if( vName.size() == 0 )
    {
      // left block for global atts?
      if( splitA[0] == "write" )
      {
         rC.putBackLine();
         return;
      }
    }
    else if( splitA[0] != vName )
    {
       // left block for current variable?
       rC.putBackLine();
       return;
    }
    else
      ++startIndex;  // for variable

    // An attribute may have a leading type.
    // If omitted, the default type is 'char'
    if( typeMap.find( splitA[startIndex]) == typeMap.end() )
    {
      // default type: char
      aTypeName="char";
      aName = splitA[startIndex];
    }
    else
    {
      aTypeName=splitA[startIndex];
      aName = splitA[startIndex+1];
    }

    // vector for all values of an attribute.
    std::vector<std::string> aArgs;
    aArgs.clear();

    // Get values; check for '=' in assignment
    size_t pos=rC.getLine().find('=');
    if( pos < std::string::npos ) // && aTypeName != "char" )
    {
      // analyse the part of the line beyond the '='
      // May be a comma-separated list, may-be appending blanks.
      Split splitC( rC.subLine(pos+1), " ,", true ) ;

      // clear and then assign
      for( size_t j=0 ; j < splitC.size() ; ++j )
      {
         aArgs.push_back( splitC[j] );
         while( (pos=aArgs.back().find('"')) < std::string::npos )
           aArgs.back().erase(pos,1);

         // only for the last splitK: is there a ';'?
         if( aArgs.back()[aArgs.back().size()-1] == ';' )
           aArgs.back().erase(aArgs.back().size()-1, 1);
      }
    }

    // now define attributes
    if( aTypeName == "char" )
    {
      char x='a';  // here aArgs contains only a single string.
      setAtt( vName, aName, aArgs, x);
    }
    else if( aTypeName == "byte" )
    {
      signed char x='1';
      setAtt( vName, aName, aArgs, x);
    }
    else if( aTypeName == "double" )
    {
      double x=0.;
      setAtt( vName, aName, aArgs, x);
    }
    else if( aTypeName == "float" )
    {
      float x=0.;
      setAtt( vName, aName, aArgs, x);
    }
    else if( aTypeName == "short" )
    {
      short x=0;
      setAtt( vName, aName, aArgs, x);
    }
    else if( aTypeName == "int" )
    {
      int x=0;
      setAtt( vName, aName, aArgs, x);
    }
    else if( aTypeName == "ubyte" )
    {
      unsigned char x='1';
      setAtt( vName, aName, aArgs, x);
    }
    else if( aTypeName == "ushort" )
    {
      unsigned short x=0;
      setAtt( vName, aName, aArgs, x);
    }
    else if( aTypeName == "uint" )
    {
      unsigned int x=0;
      setAtt( vName, aName, aArgs, x);
    }
    else if( aTypeName == "int64" )
    {
      long long x=0;
      setAtt( vName, aName, aArgs, x);
    }
    else if( aTypeName == "uint64" )
    {
      unsigned long long x=0;
      setAtt( vName, aName, aArgs, x);
    }
    else if( aTypeName == "ushort" )
    {
      unsigned short x=0;
      setAtt( vName, aName, aArgs, x);
    }
    else if( aTypeName == "string" )
    {
      std::string key("NC_6_3");

      std::string capt("TypeName=string is a still ToDo.");
      std::string text ;

      exceptionHandling(key, capt, text);
    }
  } // end of loop over a block
}

void
NcAPI::generateParseVar(ReadLine &rC,
   std::vector<std::string> &dimStrs)
{
  std::string vType[] = {"byte", "char", "short", "int",
     "float", "double", "ubyte", "ushort", "uint", "int64",
     "uint64", "string" };

  // types netCDF knows about.
  std::map<std::string, nc_type> typeMap;
  typeMap[vType[0]] = NC_BYTE ;
  typeMap[vType[1]] = NC_CHAR ;
  typeMap[vType[2]] = NC_SHORT ;
  typeMap[vType[3]] = NC_INT ;
  typeMap[vType[4]] = NC_FLOAT ;
  typeMap[vType[5]] = NC_DOUBLE ;
  typeMap[vType[6]] = NC_UBYTE ;
  typeMap[vType[7]] = NC_USHORT ;
  typeMap[vType[8]] = NC_UINT ;
  typeMap[vType[9]] = NC_INT64 ;
  typeMap[vType[10]] = NC_UINT64 ;
  typeMap[vType[11]] = NC_STRING ;

  // prepare definition of variable
  Split splitV(rC.getLine());

  // the first item is a type
  nc_type type=typeMap[ splitV[0] ];

  // separate var and dimensions
  Split splitD;
  splitD.addSeparator("(,)", true);
  splitD=splitV[1];

  // varname
  std::string vName(splitD[0]);

  // get variable's dimensions; could be none
  std::vector<std::string> varDims2;
  for( size_t i=1 ; i < splitD.size() ; ++i )
  {
    if( splitD[i] == ";" || splitD[i].size() == 0 )
      continue;

    varDims2.push_back( splitD[i] );
  }

  if( varDims2.size() > 0 )
  {
    // Are declared dimensions consistent?
    bool is=true;
    for( size_t i=0 ; i<dimStrs.size() ; ++i)
      for( size_t j=0 ; j<varDims2.size() ; ++j)
        if( dimStrs[i] == varDims2[j] )
          is=false;

    if( is )
    {
      status=0;
      std::string key("NC_6_4");

      std::string capt("Dimensions in the conf-file are inconsistent.");
      std::string text ;

      exceptionHandling(key, capt, text, vName);
    }
  }

  // define variable
  defineVar( vName, type, varDims2);

  // parse and set attributes
  generateParseAtt(rC, vName);

  return;
}

/*
void
NcAPI::generateWriteLimited( std::string line, std::string &vName,
   NcType &nctype)
{
   double range[2];
   double  numVals;

   Split splt(line," ,", true);
   std::string str;
   size_t pos;

   if( (pos=line.find("cells")) < std::string::npos )
     numVals=hdhC::string2Double(line.substr(pos));
   else
     numVals=hdhC::string2Double(line,3);

   bool isCentral=true;
   if( line.find("bound") < std::string::npos )
   {
     ++numVals;
     isCentral = false;
   }

   if( (pos=line.find("range")) < std::string::npos )
   {
     range[0]=hdhC::string2Double(line.substr(pos), 1);
     range[1]=hdhC::string2Double(line.substr(pos), 2);
   }
   else
   {
     range[0]=hdhC::string2Double(line, 1);
     range[1]=hdhC::string2Double(line, 2);
   }

   // the size
   long counts=static_cast<long>(numVals);

   // calculate cell or bound values
   double *vals = new double [counts] ;

   // width of a cell
   double diff=(range[1]-range[0])/numVals;

   // centre of the first cell
   double val_1st;
   if( isCentral )
      val_1st = range[0] + fabs( diff/2. ) ;
   else
      val_1st = range[0] ;

   double ai=0.;
   for( long i=0; i < counts ; ++i, ++ai )
     vals[i] = val_1st + diff*ai ;

   // convert and put
   switch ( nctype )
   {
//      case ncByte:
//      break;
//      case ncChar:
//     break;
     case ncShort:
     {
       short *xType;
       convert(counts, vals, xType);
       put(vName, xType, &counts);
       delete [] xType ;
     }
     break;
     case ncInt:
     {
       int *xType;
       convert(counts, vals, xType);
       put(vName, xType, &counts);
       delete [] xType ;
     }
     break;
     case ncFloat:
     {
       float *xType;
       convert(counts, vals, xType);
       put(vName, xType, &counts);
       delete [] xType ;
     }
     break;
     case ncDouble:
     {
       double *xType;
       convert(counts, vals, xType);
       put(vName, xType, &counts);
       delete [] xType ;
     }
     break;
   }

   delete [] vals ;

   return;
}
*/

/*
template <typename Type>
void
NcAPI::get(std::string name, Type *x, long *counts, long *start)
{
  open();  // if filename status has changed

  NcVar *var = layout.var[ layout.varMap[name] ];

  if( start > 0 )
    var->set_cur(start);

  //get the data
  var->get(x, counts ) ;

  return ;
}
*/

int
NcAPI::getAttID(std::string &attName, int varid )
{
  if( attName.size() == 0 )
  {
    std::string key("NC_3_8");
    std::string capt("Empty attribute name.");

    std::string text("Variable <");
    text += getVarnameFromVarID(varid);
    text += ">";

    exceptionHandling(key, capt, text, getVarnameFromVarID(varid));
  }

  if( varid < 0 )
  {
    // global Attribute
    if( layout.globalAttMap.count(attName) == 0 )
      return -2 ;

    return layout.globalAttMap[attName] ;
  }
  else
  {
    if( layout.varAttMap[varid].count(attName) == 0 )
      return -2;

    return layout.varAttMap[varid][attName];
  }
}

//! Get names of the attributes of given variable.
/// Returns empty vector, if no attributes were defined
/// or the given variable name is not a netCDF variable.
std::vector<std::string>
NcAPI::getAttName(std::string vName)
{
  int varid=getVarID(vName);

  if( varid < 0 )  // try for global atts
  {
    if( layout.globalAttName.size() > 0 )
      return layout.globalAttName;
    else
    {
      std::vector<std::string> vs ;
      return vs;
    }
  }
  else  // atts of a variable. A vector is always defined
    return layout.varAttName[varid] ;
}

//! Get names of the attributes of given variable.
/// Returns empty vector, if no attributes were defined
/// or the given variable name is not a netCDF variable.
size_t
NcAPI::getAttSize(std::string vName)
{
  int varid=getVarID(vName);

  if( varid < 0 )  // try for global atts
    return layout.globalAttName.size() ;
  else  // atts of a variable. A vector is always defined
    return layout.varAttName[varid].size() ;
}

std::string
NcAPI::getAttString(std::string attName, std::string varName)
{
  // Purpose of this method. Facilitated access to a single string
  // value or the first string element of a vector.

  std::vector<std::string> vs;
  getAttValues(vs, attName, varName);
  if( vs.size() )
    return vs[0];

  return "";
}

std::string
NcAPI::getAttString(std::string attName, std::string varName,
  bool &isDefined)
{
  // Purpose of this specialisation: having a defined, but
  // empty attribute.

  // The boolean is set false, if there is no string stored.

  std::vector<std::string> vs;
  getAttValues(vs, attName, varName);
  if( vs.size() )
    return vs[0];
  else
  {
    isDefined=false;
    std::string s;
    return s;
  }
}

nc_type
NcAPI::getAttType(std::string attName, std::string varName)
{
  int v_id = getVarID(varName);
  int a_id = getAttID(attName, varName) ;

  nc_type ncType ;

  if( v_id == -1 )
    ncType=layout.globalAttType[a_id] ;
  else
    ncType=layout.varAttType[v_id][a_id] ;

  return ncType ;
}

double
NcAPI::getAttValue(std::string attName, std::string varName)
{
  std::vector<double> dv;
  getAttValues(dv, attName, varName );

  if( dv.size() > 0 )
    return dv[0];
  else
    return MAXDOUBLE ;
}

template <typename Type>
void
NcAPI::getAttValues(std::vector<Type> &v, std::string attName, std::string varName)
{
  // attribute data of any type is converted to type double or
  // string. So, be careful.

  int varid;
  int attid;
  nc_type ncType;
  size_t sz;
  v.clear();

  if( varName.size() > 0 )
  {
    if( layout.varMap.count(varName) == 0 )
      return;  // if var does not exist, return empty vector

    varid=layout.varMap[varName];

    if( layout.varAttMap[varid].count(attName) == 0 )
      return ;

    attid=getAttID(attName, varid);
    sz = layout.varAttValSize[varid][attid];

    ncType=layout.varAttType[varid][attid] ;
  }
  else  // global Attribute
  {
    if( layout.globalAttMap.count(attName) == 0 )
      return ;

    varid=NC_GLOBAL;
    attid = getAttID(attName, -1) ;
    sz = layout.globalAttValSize[attid];

    ncType=layout.globalAttType[getAttID(attName, -1)];
  }

  disableDefMode();

  bool isTextType=false;

  switch ( ncType )
  {
    case NC_BYTE:
    {
       signed char *arr = new signed char [sz] ;
       status=nc_get_att_schar(ncid, varid, attName.c_str(), arr);
       for(size_t i=0 ; i < sz ; ++i)
         v.push_back( static_cast<Type>(arr[i]) ) ;
       delete [] arr;
    }
    break;
    case NC_CHAR:
    {
       char *arr = new char [sz] ;
       status=nc_get_att_text(ncid, varid, attName.c_str(), arr);
       for(size_t i=0 ; i < sz ; ++i)
         v.push_back( static_cast<Type>(arr[i]) ) ;
       delete [] arr;
    }
    break;
    case NC_SHORT:
    {
       short *arr = new short [sz] ;
       status=nc_get_att_short(ncid, varid,attName.c_str(), arr);
       for(size_t i=0 ; i < sz ; ++i)
         v.push_back( static_cast<Type>(arr[i]) ) ;
       delete [] arr;
    }
    break;
    case NC_INT:
    {
       int *arr = new int [sz] ;
       status=nc_get_att_int(ncid, varid,attName.c_str(), arr);
       for(size_t i=0 ; i < sz ; ++i)
         v.push_back( static_cast<Type>(arr[i]) ) ;
       delete [] arr;
    }
    break;
    case NC_FLOAT:
    {
       float *arr = new float [sz] ;
       status=nc_get_att_float(ncid, varid,attName.c_str(), arr);
       for(size_t i=0 ; i < sz ; ++i)
         v.push_back( static_cast<Type>(arr[i]) ) ;
       delete [] arr;
    }
    break;
    case NC_DOUBLE:
    {
       double *arr = new double [sz] ;
       status=nc_get_att_double(ncid, varid,attName.c_str(), arr);
       for(size_t i=0 ; i < sz ; ++i)
         v.push_back( static_cast<Type>(arr[i]) ) ;
       delete [] arr;
    }
    break;

    case NC_UBYTE:
    {
       unsigned char *arr = new unsigned char [sz] ;
       status=nc_get_att_uchar(ncid, varid,attName.c_str(), arr);
       for(size_t i=0 ; i < sz ; ++i)
         v.push_back( static_cast<Type>(arr[i]) ) ;
       delete [] arr;
    }
    break;
    case NC_USHORT:
    {
       unsigned short *arr = new unsigned short [sz] ;
       status=nc_get_att_ushort(ncid, varid,attName.c_str(), arr);
       for(size_t i=0 ; i < sz ; ++i)
         v.push_back( static_cast<Type>(arr[i]) ) ;
       delete [] arr;
    }
    break;
    case NC_UINT:
    {
       unsigned int *arr = new unsigned int [sz] ;
       status=nc_get_att_uint(ncid, varid,attName.c_str(), arr);
       for(size_t i=0 ; i < sz ; ++i)
         v.push_back( static_cast<Type>(arr[i]) ) ;
       delete [] arr;
    }
    break;
    case NC_UINT64:
    {
       unsigned long long *arr = new unsigned long long [sz] ;
       status=nc_get_att_ulonglong(ncid, varid,attName.c_str(), arr);
       for(size_t i=0 ; i < sz ; ++i)
         v.push_back( static_cast<Type>(arr[i]) ) ;
       delete [] arr;
    }
    break;

    case NC_INT64:
    {
       long long *arr = new long long [sz] ;
       status=nc_get_att_longlong(ncid, varid,attName.c_str(), arr);
       for(size_t i=0 ; i < sz ; ++i)
         v.push_back( static_cast<Type>(arr[i]) ) ;
       delete [] arr;
    }
    break;

    /*
    case NC_STRING:
    {
      char** arr = new char*[sz];
      for(size_t i=0 ; i < sz ; ++i )
        arr[i] = new char[2500];

      if( ! (status=nc_get_att_string(ncid, varid, attName.c_str(), arr)) )
      {
        for( size_t i=0 ; i < sz ; ++i )
        {
          size_t len = sizeof(arr[i]) / sizeof(arr[i][0]) ;
          for( size_t j=0 ; j < len ; ++j )
            if( !j )
              v.push_back(static_cast<Type>(arr[i][j]) ) ;
            else
              v.back() +=  static_cast<Type>(arr[i][j]) ;
        }
      }

      for(size_t i=0 ; i < sz ; ++i )
        delete [] arr[i];
      delete [] arr;
    }
    break;
*/

    default:
      status=0;
      isTextType=true;
  }

  if(status)
  {
    std::string key="NC_3_9";
    std::string capt("nc_get_att_NCTYPE error.") ;

    std::string text;
    if( varName.size() )
    {
      text = "Variable <";
      text += varName ;
      text += ">, ";
    }
    else
      text = "Global ";

    text += "attribute <";
    text += attName ;
    text += ">";

    exceptionHandling(key, capt, text, varName);
  }

  if( isTextType )
  {
    std::string key="NC_3_11";
    std::string capt("Text type was tried instead of numerical one.") ;

    std::string text;
    if( varName.size() )
    {
      text = "Variable <";
      text += varName ;
      text += ">, ";
    }
    else
      text = "Global ";

    text += "attribute <";
    text += attName ;
    text += ">" ;

    exceptionHandling(key, capt, text, varName);
  }

  return ;
}

void
NcAPI::getAttValues(std::vector<std::string> &v, std::string attName, std::string varName )
{
  //get string like attribute values
  int varid;
  int attid;
  nc_type ncType;
  size_t sz;

  v.clear();

  if( varName.size() > 0 )
  {
    if( layout.varMap.count(varName) == 0 )
      return ;  // if var does not exist, return empty vector

    varid=layout.varMap[varName];

    if( layout.varAttMap[varid].count(attName) == 0 )
      return ;

    attid=getAttID(attName, varid);
    sz = layout.varAttValSize[varid][attid];

    ncType=layout.varAttType[varid][getAttID(attName, varid)] ;
  }
  else  // global Attribute
  {
    if( layout.globalAttMap.count(attName) == 0 )
      return ;

    varid=NC_GLOBAL;
    attid = getAttID(attName, -1) ;
    sz = layout.globalAttValSize[attid];

    ncType=layout.globalAttType[getAttID(attName, -1)];
  }

  disableDefMode();

  switch ( ncType )
  {
    case NC_CHAR:
    {
       // if '\0' termination was stored, then multiple strings
       // are retrieved.
       char *arr = new char [sz] ;
       status=nc_get_att_text(ncid, varid, attName.c_str(), arr);
       size_t p0=0;
       size_t i;
       std::string t;
       for( i=0 ; i < sz ; ++i)
       {
         if( arr[i] == '\0' )
         {
           for(size_t j=p0 ; j < i ; ++j)
             t += arr[j] ;
           v.push_back( t );
           t.clear();
           p0=i+1;  //begin of next string, if any
         }
       }
       // the last string or none at all was not terminated.
       if( p0 < sz )
       {
         for(size_t j=p0 ; j < sz ; ++j)
           t += arr[j] ;
         v.push_back( t );
       }

       // Some values are composed like: abcd\0\0\0...\0
       // Trailing \0, which do not terminate anything, are considered
       // spurious and removed until a real string is found;
       // exception: the first item in the vector may be empty.
       if( (sz=v.size()) > 1 )
       {
         do
         {
           if( v[--sz].size() )
             break;

           v.erase( v.begin() + sz );
         } while ( sz > 1 );
       }

       delete [] arr;
    }
    break;
    case NC_STRING:
    {
       char **arr = new char* [sz] ;
       status=nc_get_att_string(ncid, varid, attName.c_str(), arr);
       for(size_t i=0 ; i < sz ; ++i)
         v.push_back( arr[i] ) ;

       nc_free_string( sz, arr);
       delete [] arr;
    }
    break;
    case NC_INT:
    {
       std::vector<int> iv ;
       getAttValues(iv, attName, varName);
       for( size_t ii=0 ; ii < iv.size() ; ++ii )
         v.push_back( hdhC::itoa( iv[ii]) ) ;
       return;
    }
    break;
    case NC_FLOAT:
    {
       std::vector<float> fv ;
       getAttValues(fv, attName, varName);
       for( size_t ii=0 ; ii < fv.size() ; ++ii )
         v.push_back( hdhC::double2String(fv[ii]) ) ;
       return;
    }
    case NC_DOUBLE:
    {
       std::vector<double> dv ;
       getAttValues(dv, attName, varName);
       for( size_t ii=0 ; ii < dv.size() ; ++ii )
         v.push_back( hdhC::double2String(dv[ii]) ) ;
       return;
    }
    break;
  }

  if(status)
  {
    std::string key("NC_3_10");
    std::string capt("Could not get attribute values.") ;

    std::string text("Variable <");
    text += varName;
    text += ">, attribute <";
    text += attName;
    text += ">";

    exceptionHandling(key, capt, text, varName);
  }

  return ;
}

template <typename Type>
int
NcAPI::getConstCharSize(Type v, size_t range)
{
  for(size_t i=0 ; i< range ; ++i)
    if( v[i] == '\0' )
      return i-1 ;

  return -1 ;
}

void
NcAPI::getChunking(NcAPI &from, int varid,
    size_t &storage, std::vector<size_t> &chunks)
{
  enableDefMode();

  storage = from.layout.varStorage[varid] ;

  size_t sz = layout.varDimName.size();

  for( size_t i=0 ; i < sz ; ++i )
    chunks.push_back( layout.varChunkSize[varid][i] );

  return;
}

void*
NcAPI::getData(int varid, size_t rec, int leg)
{
  // if rec_leg <= rec, then only the requested rec.
  // if leg < 0, then all records

  size_t rank = layout.varDimName[varid].size();

  if( layout.rec_index[varid] < UINT_MAX )
    layout.rec_start[varid][ layout.rec_index[varid] ] = rec;

  std::vector<size_t> dim ;
  size_t* curr_count = new size_t [rank] ;

  // a scalar is defined as 0-dimensional variable
  if( rank == 0 )
    dim.push_back(1);
  else
  {
    for( size_t i=0 ; i < rank ; ++i)
    {
      dim.push_back(layout.rec_count[varid][i]);
      curr_count[i] = layout.rec_count[varid][i] ;
    }
  }

  if( leg > 0 )
     dim[0] = curr_count[0] = leg;

  void *p=0;

  disableDefMode();

  switch ( layout.varType[varid] )
  {
    case NC_BYTE:
    {
      rec_val_schar[varid].resize( dim ) ;
      p=rec_val_schar[varid].begin() ;

      if( dim.size() == 0 )
      {
         delete [] curr_count ;
         return p ;
      }

      status = nc_get_vara_schar(ncid, varid,
                layout.rec_start[varid], curr_count,
                  rec_val_schar[varid].arr );

      if( !status )
        rec_val_schar[varid].testValueException();
    }
    break;
    case NC_CHAR:
    {
      rec_val_text[varid].resize( dim ) ;
      p=rec_val_text[varid].begin() ;

      status = nc_get_vara_text(ncid, varid,
                layout.rec_start[varid], curr_count,
                  rec_val_text[varid].arr );

      if( !status )
        rec_val_text[varid].testValueException();
    }
    break;
    case NC_SHORT:
    {
      rec_val_short[varid].resize( dim ) ;
      p=rec_val_short[varid].begin() ;

      status = nc_get_vara_short(ncid, varid,
                layout.rec_start[varid], curr_count,
                  rec_val_short[varid].arr );

      if( !status )
        rec_val_short[varid].testValueException();
    }
    break;
    case NC_INT:
    {
      rec_val_int[varid].resize( dim ) ;
      p=rec_val_int[varid].begin() ;

      status = nc_get_vara_int(ncid, varid,
                layout.rec_start[varid], curr_count,
                  rec_val_int[varid].arr );

      if( !status )
        rec_val_int[varid].testValueException();
    }
    break;
    case NC_FLOAT:
    {
      rec_val_float[varid].resize( dim ) ;
      p=rec_val_float[varid].begin() ;

      status = nc_get_vara_float(ncid, varid,
                layout.rec_start[varid], curr_count,
                  rec_val_float[varid].arr );

      if( !status )
        rec_val_float[varid].testValueException();
    }
    break;
    case NC_DOUBLE:
    {
      rec_val_double[varid].resize( dim ) ;
      p = rec_val_double[varid].begin() ;

      status = nc_get_vara_double(ncid, varid,
                layout.rec_start[varid], curr_count,
                  rec_val_double[varid].arr );

      if( !status )
        rec_val_double[varid].testValueException();
    }
    break;
    case NC_UBYTE:
    {
      rec_val_uchar[varid].resize( dim ) ;
      p=rec_val_uchar[varid].begin() ;

      status = nc_get_vara_uchar(ncid, varid,
                layout.rec_start[varid], curr_count,
                  rec_val_uchar[varid].arr );

      if( !status )
        rec_val_uchar[varid].testValueException();
    }
    break;
    case NC_USHORT:
    {
      rec_val_ushort[varid].resize( dim ) ;
      p=rec_val_ushort[varid].begin() ;

      status = nc_get_vara_ushort(ncid, varid,
                layout.rec_start[varid], curr_count,
                  rec_val_ushort[varid].arr );

      if( !status )
        rec_val_ushort[varid].testValueException();
    }
    break;
    case NC_UINT:
    {
      rec_val_uint[varid].resize( dim ) ;
      p=rec_val_uint[varid].begin() ;

      status = nc_get_vara_uint(ncid, varid,
                layout.rec_start[varid], curr_count,
                  rec_val_uint[varid].arr );

      if( !status )
        rec_val_uint[varid].testValueException();
    }
    break;
    case NC_UINT64:
    {
      rec_val_ulonglong[varid].resize( dim ) ;
      p=rec_val_ulonglong[varid].begin() ;

      status = nc_get_vara_ulonglong(ncid, varid,
                layout.rec_start[varid], curr_count,
                  rec_val_ulonglong[varid].arr );

      if( !status )
        rec_val_ulonglong[varid].testValueException();
    }
    break;
    case NC_INT64:
    {
      rec_val_longlong[varid].resize( dim ) ;
      p=rec_val_longlong[varid].begin() ;

      status = nc_get_vara_longlong(ncid, varid,
                layout.rec_start[varid], curr_count,
                  rec_val_longlong[varid].arr );

      if( !status )
        rec_val_longlong[varid].testValueException();
    }
    break;
    case NC_STRING:
    {
      if( rec_val_string[varid].size() > 0 )
        nc_free_string( layout.recSize[varid],
             rec_val_string[varid].begin() );

      rec_val_string[varid].resize( layout.recSize[varid] ) ;
      p=rec_val_string[varid].begin() ;

      status = nc_get_vara_string(ncid, varid,
                layout.rec_start[varid], curr_count,
                  rec_val_string[varid].arr );

//      if( !status )
//      rec_val_string.testValueException();
    }
  }

  if(status && with_annotation)
  {
    std::string key("NC_7_1");
    std::string capt("Could  not get data.") ;

    std::string vName(getVarnameFromVarID(varid));

    std::string text("Variable <");
    text += vName;
    text += ">: ";

    exceptionHandling(key, capt, text, vName);
  }

  delete [] curr_count ;

  with_annotation=true;
  return p;
}

template <typename ToT>
ToT
NcAPI::getData(MtrxArr<ToT> &x, std::string vName, int r, int leg)
{
   size_t rec;
   if( r < 0 )
      r += static_cast<int>(getNumOfRecords(vName));

   rec = static_cast<size_t>(r);

   // rec==0 is ok for the unlimited case
   if( rec && getNumOfRecords(vName) < rec )
     return 0;

   return getData(x, getVarID(vName), rec, leg);
}

template <typename ToT>
ToT
NcAPI::getData(MtrxArr<ToT> &to, int varid, int r, int leg)
{
  // Get netCDF data and store in a MtrxArr object.
  // The first value is returned.
  if( varid < 0 )
    return 0;

  size_t rec;
  if( r < 0 )
     r += static_cast<int>(getNumOfRecords());

  rec = static_cast<size_t>(r);

  layout.rec_leg_begin[varid] = rec;

  size_t lleg;

  if( isVarUnlimited(varid) )
  {
    if( leg == -1 )
      lleg = getNumOfRecords();
    else
      lleg = static_cast<size_t>(leg);

    layout.rec_leg_end[varid] = rec + lleg ;

    if( layout.rec_leg_end[varid] > layout.varDimSize[varid][0] )
    {
        // take into growth due to writing
        std::vector<size_t> vs_vdSz( getVarDimSize(layout.varidMap[varid]) );

        if( !vs_vdSz.size() )
            vs_vdSz.push_back(1);  // scalar variable

        layout.rec_leg_end[varid] = vs_vdSz[0];
        lleg = vs_vdSz[0] - rec ;
    }
  }
  else
    lleg=0;

  (void) getData(varid, rec, lleg);

  // the taking obj had been used in a different context
  size_t to_ix = rec - layout.rec_leg_begin[varid] ;

  switch ( layout.varType[varid] )
  {
    case NC_CHAR:   // unsigned char
      to = rec_val_text[varid] ;
    break;
    case NC_BYTE:   // signed char
      to = rec_val_schar[varid] ;
    break;
    case NC_SHORT:  // short
      to = rec_val_short[varid] ;
    break;
    case NC_INT:    // int
      to = rec_val_int[varid] ;
    break;
    case NC_FLOAT:  // float
      to = rec_val_float[varid] ;
    break;
    case NC_DOUBLE: // double
      to = rec_val_double[varid] ;
    break;
    case NC_UINT:   // unsigned int
      to = rec_val_uint[varid] ;
    break;
    case NC_UBYTE:  // unsigned char
      to = rec_val_uchar[varid] ;
    break;
    case NC_USHORT: // unsigned short
      to = rec_val_ushort[varid] ;
    break;
    case NC_UINT64: // unsigned long long
      to = rec_val_ulonglong[varid] ;
    break;
    case NC_INT64:  // long long
      to = rec_val_longlong[varid] ;
    break;
    case NC_STRING:
    {
      status=0;
      std::string key("NC_7_2");
      std::string capt("No rules to convert NC_STRING to MtrxArr.") ;

      std::string vName(getVarnameFromVarID(varid));
      std::string text("Variable <");
      text +=  vName;
      text += ">";

      exceptionHandling(key, capt, text, vName);
      return to[to_ix];
    }
    break;
  }

  return to[to_ix];
}

void
NcAPI::getData(std::vector<std::string> &v, std::string varName, int r )
{
  /*
  if( r < 0 )
     r += static_cast<int>(getNumOfRecords(varName));

  size_t rec = static_cast<size_t>(r);
  */

  v.clear();
  if( layout.varMap.count(varName) == 0 )
    return ;  // if var does not exist, return empty vector

  int varid=getVarID(varName);
  if( varid < 0 )
    return ;

  //get string like attribute values
  size_t n = layout.varDimName[varid].size();

  if( n == 0 )
    return ;  // no dimension, no data

  std::vector<size_t> dim ;
  for( size_t i=0 ; i < n ; ++i)
    dim.push_back(layout.rec_count[varid][i]);

  size_t sz=layout.recSize[varid];
  disableDefMode();

  switch ( layout.varType[varid] )
  {
    case NC_CHAR:
    {
       // if '\0' termination was stored, then multiple strings
       // are retrieved.
       char *arr = new char [sz+1] ;
       status=nc_get_vara_text(ncid, varid, layout.rec_start[varid],
                  layout.rec_count[varid], arr );

       if( dim.size() == 1 )
       {  // a single string
         size_t p0=0;
         for( p0=0 ; p0 < sz ; ++p0)
           if( arr[p0] == '\0' )
             break;

         // the last (or the only) string was not terminated.
         if( p0 )
         {
           v.push_back( "" );

           for(size_t j=0 ; j < p0 ; ++j)
             v.back() += arr[j] ;
         }
       }
       else
       {
         // fix-sized strings, possibly \0 terminated

         // take into account an arbitrary number of dims, but
         // use the last one for property determination.
         size_t sz_l  = dim[dim.size()-1];
         size_t l0 = 0;
         size_t l1 = sz_l ;
         size_t pos;
         while( l0 < sz )
         {
           v.push_back( "" );
           std::string &vv = v.back();
           for( pos=l0 ; pos < l1 ; ++pos )
           {
             if( arr[pos] == '\0' )
               break;
             vv += arr[pos] ;
           }

           l0 += sz_l;
           l1 += sz_l;
         }
       }

       delete [] arr;
    }
    break;
    case NC_STRING:
    {
       char **arr = new char* [sz] ;
       status=nc_get_vara_string(ncid, varid, layout.rec_start[varid],
                  layout.rec_count[varid], arr );
       for(size_t i=0 ; i < sz ; ++i)
         v.push_back( arr[i] ) ;

       nc_free_string( sz, arr);
       delete [] arr;
    }
    break;
  }

  if(status && with_annotation)
  {
    std::string key("NC_7_3");
    std::string capt("Could not get data.") ;

    std::string text("Variable <");
    text += varName;
    text += ">";

    exceptionHandling(key, capt, text, varName);
  }

  with_annotation=true;
  return ;
}

std::pair<int, int>
NcAPI::getDataIndexRange(int varid)
{
  return std::pair<int,int>
     (layout.rec_leg_begin[varid], layout.rec_leg_end[varid]) ;
}

size_t
NcAPI::getDataSize(std::string vName)
{
  int varid=getVarID(vName);
  if( varid < 0 )
    return 0;

  return getDataSize(varid);
}

size_t
NcAPI::getDataSize(int varid)
{
  // return total number of data values.
  size_t num = getNumOfRecords();

  num *= layout.recSize[varid];

  return num ;
}

void
NcAPI::getDeflate(NcAPI &from, int varid,
         int &shuffle, int &deflate, int &level )
{
  enableDefMode();

  shuffle = from.layout.varShuffle[varid] ;
  deflate = from.layout.varDeflate[varid] ;
  level = from.layout.varDeflateLevel[varid] ;

  return;
}

template <typename T>
void
NcAPI::getDefaultFillValue(nc_type type, T& x)
{
   if( type == NC_BYTE )
      x = (T) NC_FILL_BYTE ;
   else if( type == NC_CHAR )
      x = (T) NC_FILL_CHAR ;
   else if( type == NC_SHORT )
      x = (T) NC_FILL_SHORT ;
   else if( type == NC_INT )
      x = (T) NC_FILL_INT ;
   else if( type == NC_LONG )
      x = (T) NC_FILL_INT ;  // correct
   else if( type == NC_FLOAT )
      x = (T) NC_FILL_FLOAT ;
   else if( type == NC_DOUBLE )
      x = (T) NC_FILL_DOUBLE ;
   else if( type == NC_UBYTE )
      x = (T) NC_FILL_UBYTE ;
   else if( type == NC_USHORT )
      x = (T) NC_FILL_USHORT ;
   else if( type == NC_UINT )
      x = (T) NC_FILL_UINT ;
   else if( type == NC_INT64 )
      x = (T) NC_FILL_INT64 ;
   else if( type == NC_UINT64 )
      x = (T) NC_FILL_UINT64 ;

   return;
}

int
NcAPI::getDimID(std::string dName)
{
  //get dimensions of a variable
  if( layout.dimMap.count(dName) == 0 )
     return -1 ;

  return layout.dimMap[dName];
}

std::vector<std::string>
NcAPI::getDimName(std::string vName)
{
  //get dimensions names of a variable

  if( vName.size() == 0 )
    return layout.dimName ;

  int varid=getVarID( vName );

  if( varid < 0 )
    return std::vector<std::string>() ;

  return layout.varDimName[ varid ];
}

int
NcAPI::getDimSize(int d)
{
  if( d < 0 || d >= static_cast<int>(layout.dimName.size()) )
     return -1;

  return layout.dimSize[d];
}

template <typename T>
bool
NcAPI::get_FillValue(std::string& vName,
   std::vector<T>& fV, std::vector<char>* mode)
{
    // Get fill values
    // If no attribute defined fill-value is available, then _FV and/or MV
    // will return the default. This is also indicated by a false return value.

   fV.clear();
   mode->clear();

   bool isR=false;
   nc_type type;

   int vid;
   if( (vid=getVarID(vName)) > -1 )
     type = layout.varType[vid];
   else
   {
     mode=0;
     return isR;
   }

   std::string str_FV("_FillValue");
   std::string str_MV("missing_value");
   std::string str_VN("valid_min");
   std::string str_VX("valid_max");
   std::string str_VR("valid_range");

   int aid_FV = getAttID(str_FV, vid);
   int aid_MV = getAttID(str_MV, vid);
   int aid_VN = getAttID(str_VN, vid);
   int aid_VX = getAttID(str_VX, vid);
   int aid_VR = getAttID(str_VR, vid);

	std::string str;
   if( aid_FV > -1 && getAttType(str_FV, vName) != type )
     str=str_FV ;
   if( aid_MV > -1 && getAttType(str_MV, vName) != type )
	{
	  if( str.size() )
   	  str += "," + str_MV;
	  else
   	  str = str_MV;
	}
   if( aid_VN > -1 && getAttType(str_VN, vName) != type )
	{
	  if( str.size() )
   	  str += "," + str_VN;
	  else
   	  str = str_VN;
	}
   if( aid_VX > -1 && getAttType(str_VX, vName) != type )
	{
	  if( str.size() )
   	  str += "," + str_VX;
	  else
   	  str = str_VX;
	}
   if( aid_VR > -1 && getAttType(str_VR, vName) != type )
	{
	  if( str.size() )
   	  str += "," + str_VR;
	  else
   	  str = str_VR;
	}

   if( str.size() )
   {
     std::string key="NC_3_12";
     std::string capt("types of variable <" + vName + "> and ");
     capt += str + " do(es) not match" ;

     exceptionHandling(key, capt, "", vName);
     mode=0;
     return isR;
   }

   T x;
   getDefaultFillValue(type, x);
   fV.push_back(x);
   mode->push_back('=');

   if( aid_FV > -1 )
   {
      std::vector<T> vv;
      getAttValues(vv, str_FV, vName) ;
      fV.push_back(vv[0]);
      mode->push_back('=');
      isR=true;
   }

   if( aid_MV > -1 )
   {
      std::vector<T> vv;
      getAttValues(vv, str_MV, vName) ;
      fV.push_back(vv[0]);
      mode->push_back('=');
      isR=true;
   }

   if( aid_VN > -1 )
   {
      std::vector<T> vv;
      getAttValues(vv, str_VN, vName) ;
      fV.push_back(vv[0]);
      mode->push_back('<');
      isR=true;
   }

   if( aid_VX > -1 )
   {
      std::vector<T> vv;
      getAttValues(vv, str_VX, vName) ;
      fV.push_back(vv[0]);
      mode->push_back('>');
      isR=true;
   }

   if( aid_VR > -1 )
   {
      std::vector<T> vv;
      getAttValues(vv, str_VR, vName) ;
      fV.push_back(vv[0]);
      fV.push_back(vv[1]);
      mode->push_back('R');
      mode->push_back('R');
      isR=true;
   }

   return isR;
}

void
NcAPI::getLayout(void)
{
  isNC4 = true ;
  status = nc_inq_format(ncid, &ncFormat);

  // NC_FORMAT_CLASSIC (1)
  // NC_FORMAT_64BIT   (2)
  // NC_FORMAT_NETCDF4 (3)
  // NC_FORMAT_NETCDF4_CLASSIC  (4)
  if( ncFormat < 3 )
    isNC4 = false;

  // DIMENSIONS
  // inquire number of dimensions, num of variables
  // num of global atts, and the dim ID for unlimited dim.
  int dimNum, varNum, globalAttNum;

  status = nc_inq(ncid, &dimNum, &varNum, &globalAttNum,
                      &unlimitedDimID);

  if(status)
  {
    std::string key="NC_2_1";
    std::string capt("nc_inq() error.");

    exceptionHandling(key, capt);
  }

  status=nc_inq_format(ncid, &layout.format) ;
  if(status)
  {
    std::string key="NC_2_2";
    std::string capt("nc_inq_format() error.");

    exceptionHandling(key, capt);
  }

  char name_buf[NC_MAX_NAME+1];
  size_t len;
  int attNum;

  // inquire dim IDs
  for( int id=0 ; id < dimNum ; ++id)
  {
     status=nc_inq_dim(ncid, id, name_buf, &len) ;
     if(status)
     {
       std::string key="NC_2_3";
       std::string capt("nc_inq_dim() error.");

       std::string text("dimension=");
       capt += name_buf ;

       exceptionHandling(key, capt, text);
     }

     layout.dimName.push_back( name_buf );
     layout.dimSize.push_back( len );
     layout.dimMap[name_buf] = id ;

     if( id == unlimitedDimID )
     {
       unlimitedDimName=layout.dimName[unlimitedDimID];
       isGenuineUnlimitedDim = true;
     }
     else if( unlimitedDimID < 0 && unlimitedDimName == layout.dimName.back() )
     {
       unlimitedDimID = id ;
       isGenuineUnlimitedDim = false;
     }
  }

  for( int id=0 ; id < varNum ; ++id)
  {
     // Variables
     int dimids[NC_MAX_VAR_DIMS];
     nc_type type;
     int dimsp;

     std::string vName;

     status=nc_inq_var(ncid, id, name_buf,
        &type, &dimsp, dimids, &attNum) ;

     vName = name_buf;

     // push some internal vectors
     layoutVarAttPushes();

     if(status)
     {
       std::string key="NC_2_4";
       std::string capt("nc_inq_var() error.") ;

       std::string text("variable=");
       text += vName ;

       exceptionHandling(key, capt, text, vName);
     }

     layout.varName.push_back( name_buf);
     layout.varType.push_back( type );
     layout.varDimName.push_back( std::vector<std::string>() );
     layout.varDimSize.push_back( std::vector<size_t>() );
     for( int j=0 ; j < dimsp ; ++j )
     {
       layout.varDimName[id].push_back(layout.dimName[dimids[j]]);

       int sz = getDimSize(layout.varDimName[id][j]) ;
       layout.varDimSize[id].push_back( static_cast<size_t>(sz) );
     }

     if( dimsp == 0 )  // scalar variable
        layout.varDimSize[id].push_back(1);

     layout.varMap[name_buf] = id ;
     layout.varidMap[id] = name_buf ;
     layout.varTypeMap[name_buf] = type ;

     isDataEmpty.push_back(true);
     isDataEmptyChecked.push_back(false);

     // ATTRIBUTES of variables
     for( int j=0 ; j < attNum ; ++j )
     {
       status=nc_inq_attname(ncid, id, j, name_buf);

       if(status)
       {
         std::string key="NC_2_5";
         std::string capt("nc_inq_attname() error.");

         std::string text("variable=");
         text += vName ;
         text += ", attribute=";
         text += name_buf;

         exceptionHandling(key, capt, text, vName);
       }

       nc_type aType;
       status=nc_inq_atttype(ncid, id, name_buf, &aType) ;
       if(status)
       {
         std::string key="NC_2_6";
         std::string capt("nc_inq_atttype() error.");

         std::string text("variable=");
         text += vName ;
         text += ", attribute=";
         text += name_buf;

         exceptionHandling(key, capt, text, vName);
       }

       status=nc_inq_attlen(ncid, id, name_buf, &len) ;
       if(status)
       {
         std::string key="NC_2_7";
         std::string capt("nc_inq_attlen() error.");

         std::string text("variable=");
         text += vName ;
         text += ", attribute=";
         text += name_buf;

         exceptionHandling(key, capt, text, vName);
       }

       addAttToLayout( id, name_buf, aType, len) ;
     }
  }

  // any unlimited dimension? representative variable of such?
  // If not, then any limited time variable as substitution?
  getUnlimitedDimRepName();

  for( int id=0 ; id < varNum ; ++id)
  {
     std::string vName(layout.varName[id]);

     // initiate the data containers
     layoutVarDataPushes(vName, layout.varType[id]);

     // Record size (times all dimensions except the unlimited
     // one). This contains also the total size of limited vars.
     size_t sz=1;
     size_t rank = layout.varDimName[id].size();

     // shape of corners and edges of data arrays
     layout.rec_start.push_back( new size_t [rank] );
     layout.rec_count.push_back( new size_t [rank] );
     layout.rec_index.push_back( UINT_MAX );  //indicates non-record

     layout.rec_leg_begin.push_back( 0 );
     layout.rec_leg_sz.push_back( 0 );
     layout.rec_leg_sz.push_back( 0 );  // default for large multi-dim arrays
     layout.rec_leg_end.push_back( 0 );

     // compression
     int shuffle=0;
     int deflate=0;
     int deflate_level=0;

     status = nc_inq_var_deflate(ncid, id,
                 &shuffle, &deflate, &deflate_level);

     if( !isNC4 )
       ;
     else if(status)
     {
       std::string key="NC_2_8";
       std::string capt("nc_inq_var_deflate() error.");

       std::string text("variable=");
       text += vName ;
       text += ", status(nc_inq_var_deflate()=";
       text += hdhC::itoa(status);

       exceptionHandling(key, capt, text, vName);
       layout.deflate[vName] = -1;
     }
     else
     {
       if( deflate && deflate_level )
         isDeflate=true;  // globally

       layout.deflate[vName] = deflate;
       layout.deflate_level[vName] = deflate_level;
       layout.shuffle[vName] = shuffle;

       // if NC4, then create anyway
       layout.varShuffle.push_back( shuffle );
       layout.varDeflate.push_back( deflate );
       layout.varDeflateLevel.push_back( deflate_level );
     }

     // chunking
     int storage ; // NC_CONTINGUOUS or NC_CHUNKED
     layout.varChunkSize.push_back( new size_t [rank] );
     status = nc_inq_var_chunking(ncid, id, &storage,
                             layout.varChunkSize.back()) ;

     if( !isNC4 )
       ;
     else if(status)
     {
       std::string key="NC_2_9";
       std::string capt("nc_inq_var_chunking() error.");

       std::string text("variable=");
       text += vName ;
       text += ", status(nc_inq_var_chunking()=";
       text += hdhC::itoa(status);

       exceptionHandling(key, capt, text, vName);
       layout.storage[vName]=-1;
     }
     else
     {
       layout.storage[vName]=storage;
       layout.varStorage.push_back( storage );
       isChunking=true;
     }

     // endianess
     int endian ;
     status = nc_inq_var_endian(ncid, id, &endian) ;

     if( !isNC4 )
       ;
     else if(status)
     {
       std::string key="NC_2_10";
       std::string capt("nc_inq_var_endian() error.");

       std::string text("variable=");
       text += vName ;
       text += ", status(nc_inq_var_endian()=";
       text += hdhC::itoa(status);

       exceptionHandling(key, capt, text, vName);

       layout.endian[vName]=-1;
     }
     else
     {
       layout.endian[vName]=endian;
       layout.varEndian.push_back( endian );
       isEndianess=true;
     }

     int fletcher32 ;
     status = nc_inq_var_fletcher32(ncid, id, &fletcher32) ;

     if( !isNC4 )
       ;
     else if(status)
     {
       std::string key="NC_2_11";
       std::string capt("nc_inq_var_fletcher32() error.");

       std::string text("variable=");
       text += vName ;
       text += ", status(nc_inq_var_fletcher32()=";
       text += hdhC::itoa(status);

       exceptionHandling(key, capt, text, vName);
       layout.fletcher32[vName] = fletcher32;
     }
     else
     {
       if(fletcher32 == NC_FLETCHER32 )
         isFletcher32=true;

       layout.fletcher32[vName] = fletcher32;
       layout.varFletcher32.push_back( fletcher32 );
     }

     // rank is the size of the dimension vector
     for( size_t i=0 ; i < rank ; ++i)
     {
         size_t m = layout.dimMap[ layout.varDimName[id][i] ] ;

         if( isDimUnlimited()
                && m == static_cast<size_t>(unlimitedDimID) )
         {
            layout.rec_start.back()[i]=0;
            layout.rec_count.back()[i]=1;
            layout.rec_index.back()=i;
         }
         else
         {
            size_t dSz = layout.dimSize[ m ] ;
            sz *= dSz ;

            layout.rec_start.back()[i]=0;
            layout.rec_count.back()[i]=dSz;
         }
     }

     // special: scalar defined by 0-dimensional variable
     // Note: sz == 1
     if( rank == 0 )
     {
        layout.rec_start.back()[0]=0;
        layout.rec_count.back()[0]=1;
     }

     layout.recSize.push_back( sz );
  }

  // global attributes are associated
  // with variable name 'NC_GLOBAL'
  status=nc_inq_varnatts(ncid, NC_GLOBAL, &attNum) ;
  if(status)
  {
     std::string key="NC_2_12";

     std::string capt("Could not get number of global attributes.");

     exceptionHandling(key, capt);
  }

  if( attNum == 0 )
  {
     is_NC_GLOBAL=false;
     return ;  // no global attributes
  }

  for( int j=0 ; j < attNum ; ++j )
  {
     status=nc_inq_attname(ncid, NC_GLOBAL, j, name_buf) ;
     if(status)
     {
       std::string key="NC_2_13";
       std::string capt("Could not get name of attribute.");

       std::string text("Global attribute <");
       text += name_buf;
       text += ">";

       exceptionHandling(key, capt, text);
     }

     layout.globalAttName.push_back( name_buf );
     layout.globalAttMap[name_buf] = j;

     nc_type type;
     status=nc_inq_atttype(ncid, NC_GLOBAL, name_buf,  &type) ;
     if(status)
     {
       std::string key="NC_2_14";
       std::string capt("nc_inq_atttype() error.");

       std::string text("Global attribute <");
       text += name_buf;
       text += ">";

       exceptionHandling(key, capt, text);
     }

     layout.globalAttType.push_back( type );

     status=nc_inq_attlen(ncid, NC_GLOBAL, name_buf, &len) ;
     if(status)
     {
       std::string key="NC_2_15";
       std::string capt("Could not get length of attribute values.") ;

       std::string text("Global attribute <");
       text += name_buf;
       text += ">";

       exceptionHandling(key, capt, text);
     }

     layout.globalAttValSize.push_back( len );
  }

  return;
}

std::vector<std::string>
NcAPI::getLimitedVarName(void)
{
  std::vector<std::string> t0;
  for( size_t i=0 ; i < layout.varName.size() ; ++i)
    if( ! hasEffVarUnlimitedDim[i] )
      t0.push_back( layout.varName[i] );

  return t0;
}

size_t
NcAPI::getNumOfRecords(std::string vName, bool force)
{
   if( isRecordType(vName) )
      return getNumOfRecords(force) ;

   return 0;
}

size_t
NcAPI::getNumOfRecords(bool force)
{
  if( numOfRecords > -1 && !force )
    return static_cast<size_t>(numOfRecords);

  size_t len=0;

  if( isDimUnlimited() )
  {
    status=nc_inq_dimlen(ncid, unlimitedDimID, &len);

    if(status)
    {
      std::string key("NC_6_5");
      std::string capt("Could not get number of records.");
      std::string text ;

      exceptionHandling(key, capt, text);
    }
    else
      layout.dimSize[ unlimitedDimID ] = len;
  }

  numOfRecords = static_cast<int>( len );

  return len ;
}

size_t
NcAPI::getNumOfRows(std::string vName)
{
  int varid=getVarID(vName);
  if( varid < 0 )
    return 0; // due to scalar variables this will never be a valid number

  return layout.varDimSize[varid][0];
}

size_t
NcAPI::getRecordSize(std::string vName)
{
  // return size of a record.

  int varid=getVarID(vName);
  if( varid < 0 )
    return 0;

  return getRecordSize(varid);
}

size_t
NcAPI::getRecordSize(int varid)
{
  // return size of a record.

  // return 0, if vName is limited, i.e. for non-records.
//  if( ! hasEffVarUnlimitedDim[varid] )
//    return 0 ;

  return layout.recSize[varid] ;
}

std::string
NcAPI::getTypeStr(nc_type ncType)
{
  if( ncType == NC_BYTE )
    return "NC_BYTE" ;
  else if( ncType == NC_CHAR )
    return "NC_CHAR" ;
  else if( ncType == NC_SHORT )
    return "NC_SHORT" ;
  else if( ncType == NC_INT )
    return "NC_INT" ;
  else if( ncType == NC_FLOAT )
    return "NC_FLOAT" ;
  else if( ncType == NC_DOUBLE )
    return "NC_DOUBLE" ;
  else if( ncType == NC_UINT )
    return "NC_UINT" ;
  else if( ncType == NC_UBYTE )
    return "NC_UBYTE" ;
  else if( ncType == NC_USHORT )
    return "NC_USHORT" ;
  else if( ncType == NC_UINT64 )
    return "NC_UINT64" ;
  else if( ncType == NC_INT64 )
    return "NC_INT64" ;
  else if( ncType == NC_STRING )
    return "NC_STRING" ;

  return "";  // no value, no type
}

std::string
NcAPI::getUnlimitedDimRepName(void)
{
  // Return the name of the variable representation
  // of the unlimited dimension. Return empty string,
  // if there is none.

  if( unlimitedDimRepName.size() )
      return unlimitedDimRepName;

  std::string ulDimName( getUnlimitedDimName() );

  std::vector<std::string> vs_d;
  std::vector<std::string> vs_uv( getUnlimitedVars() );

  // scan through the vars with unlimited dim
  for( size_t i=0 ; i < vs_uv.size() ; ++i)
  {
     // get names of the dimensions of the current variable
     vs_d = getDimName(vs_uv[i]);
     if( vs_d.size() == 1  // only one dim
            && vs_d[0] == ulDimName  // dim is unlimited
                &&  vs_uv[i] == ulDimName )  // same name
     {
       unlimitedDimRepName = vs_d[0] ;
       break;
     }
  }

  // Not found
  // Is there any limited variable depending on time properties?
  if( unlimitedDimName.size() == 0 )
  {
    vs_uv = getLimitedVarName() ;
    std::vector<std::string> att_name;
    std::string av;
    std::string str_days("days");
    std::string str_since("since");

    // scan through the vars for a single dim with same name
    for( size_t i=0 ; i < vs_uv.size() ; ++i)
    {
       vs_d = getDimName(vs_uv[i]);

       if( vs_d.size() == 1 )  // only one dim
       {
          if( vs_uv[i] == vs_d[0] )  // same name
          {
             // scan attributes for CF Convention time properties
             att_name = getAttName(vs_d[0]);

             for( size_t j=0 ; j < att_name.size() ; ++j)
             {
                if( att_name[j] == "standard_name" )
                {
                   av = hdhC::Lower()(getAttString(att_name[j], vs_d[0])) ;
                   if( av == "time" )
                   {
                     unlimitedDimRepName = vs_d[0] ;
                     break;
                   }
                }

                if( att_name[j] == "long_name" )
                {
                   av = hdhC::Lower()(getAttString(att_name[j], vs_d[0])) ;
                   if( av == "time" )
                   {
                     unlimitedDimRepName = vs_d[0] ;
                     break;
                   }
                }

                if( att_name[j] == "calendar" )
                {
                  unlimitedDimRepName = vs_d[0] ;
                  break;
                }

                if( att_name[j] == "units" )
                {
                   av = hdhC::Lower()(getAttString(att_name[j], vs_d[0])) ;
                   Split x_av(av);

                   if( av.size() > 2 && x_av[0] == str_days && x_av[1] == str_since )
                   {
                     unlimitedDimRepName = vs_d[0] ;
                     break;
                   }
                }
             }

             unlimitedDimName = unlimitedDimRepName;
             vs_d = getDimName();
             for( size_t j=0 ; j < vs_d.size() ; ++j)
             {
                 if( vs_d[j] == unlimitedDimRepName )
                 {
                     unlimitedDimID = j;
                     break;
                 }
             }
          }
       }
    }
  }

  return unlimitedDimRepName;  // empty if not found
}

std::vector<std::string>
NcAPI::getUnlimitedVars(void)
{
  std::vector<std::string> ulName;

  for( size_t id=0 ; id < layout.varName.size() ; ++id)
  {
    layout.varIsRecordType.push_back( false) ;
    layout.noData.push_back( false) ;
    hasEffVarUnlimitedDim.push_back( false) ;

    for( size_t j=0 ; j < layout.varDimName[id].size() ; ++j)
    {
       if( layout.varDimName[id][j] == unlimitedDimName )
       {
         layout.varIsRecordType.back()=true;
         hasEffVarUnlimitedDim.back()=true ;
         ulName.push_back(layout.varName[id]);
         break;
       }
    }
  }

  return ulName;
}

std::vector<size_t>
NcAPI::getVarDimSize(std::string vName)
{
  std::vector<size_t> szs;

  // return empty vector, if vName doesn't match
  std::vector<std::string> vn(getDimName(vName));

  for(size_t i=0 ; i < vn.size() ; ++i)
      szs.push_back( getDimSize(vn[i]) );

  return szs;
}

int
NcAPI::getVarID(std::string vName)
{
  if( vName.size() == 0 || vName == "NC_GLOBAL" )
    return NC_GLOBAL ;

  //get dimensions of a variable
  if( layout.varMap.count(vName) == 0 )
     return -2 ;

  return layout.varMap[vName];
}

std::string
NcAPI::getVarnameFromVarID(int vi)
{
  std::string vName;

  //get dimensions of a variable
  if( layout.varidMap.count(vi) == 0 )
     return vName ;

  return layout.varidMap[vi];
}

//! Get the type of of a variable.
nc_type
NcAPI::getVarType(std::string vName)
{
  int varid=getVarID( vName );

  if( varid < 0 )
  {
    std::string key("NC_6_6");
    std::string capt("Invalid variable name.");

    std::string text("variable=");
    text += vName ;

    exceptionHandling(key, capt, text, vName);
  }

  return layout.varType[varid];
}

//! Get the string representation of the variable type.
std::string
NcAPI::getVarTypeStr(std::string vName)
{
  nc_type nt = getVarType(vName) ;

  switch ( nt )
  {
    case NC_CHAR:
    return "char";
    case NC_BYTE:
    return "byte";
    case NC_FLOAT:
    return "float";
    case NC_DOUBLE:
    return "double";
    case NC_INT:
    return "int";
    case NC_SHORT:
    return "short";

    case NC_UBYTE:
    return "unsigned byte";
    case NC_USHORT:
    return "unsigned short";
    case NC_UINT:
    return "unsigned int";
    case NC_UINT64:
    return "unsigned long long";
    case NC_INT64:
    return "long long";
    case NC_STRING:
    return "string";
  }

  return "";
}

/*
std::vector<std::string>
NcAPI::getVarName(void)
{
  std::vector<std::string> vN;

  //get variables
  for( int j=0 ; j < layout.var.size() ; ++j)
  {
    NcVar *pVar = layout.var[j] ;
    vN.push_back( pVar->name() );
  }

  return vN;
}
*/

/*
//! Get all variable names (limited and unlimited).
std::vector<NcVar*>
NcAPI::getVars(void)
{
  std::vector<NcVar*> vPVar;

  NcVar *pVar ;
  for( int i=0 ; i < nc->num_vars() ; ++i)
  {
    pVar = nc->get_var(i);
    vPVar.push_back( pVar );
  }

  return vPVar ;
}
*/

bool
NcAPI::isIndexType(std::string vName)
{
  bool is=false;

  if( isVariableValid(vName) )
  {
     nc_type tp = getVarType(vName);
     if( tp == NC_BYTE || tp == NC_INT || tp == NC_SHORT || tp == NC_USHORT
            || tp == NC_UINT || tp == NC_INT64 || tp == NC_UINT64 )
     {
       std::string str(getAttString("units", vName));
       if(str.size() == 0. || str == "1" )
         is=true;
     }
  }

  return is;
}

bool
NcAPI::isNumericType(std::string vName, std::string aName)
{
  if( aName.size() == 0 && vName.size() == 0 ) // insane
     return false;

  if( aName.size() == 0 )  // variable type
  {
     if( isIndexType(vName) )
        return true;
     else
     {
       if( isVariableValid(vName) )
       {
         nc_type tp = getVarType(vName);
         if( tp == NC_FLOAT || tp == NC_DOUBLE )
            return true;
       }
     }
  }
  else  // global attribute or such of a variable
  {
     nc_type tp = getAttType(aName, vName) ;
     if( tp == NC_BYTE || tp == NC_INT || tp == NC_SHORT || tp == NC_USHORT
           || tp == NC_UINT || tp == NC_INT64 || tp == NC_UINT64
               || tp == NC_FLOAT || tp == NC_DOUBLE )
        return true;
  }

  return false;
}

void
NcAPI::init(void)
{
  isThisOpen  = false;
  with_annotation=true;
  is_NC_GLOBAL=true;
  numOfRecords=-1;
  max_read_sz=100000;

  isChunking  = false ;
  isDeflate   = false ;
  isDefineMode= false;
  isEndianess = false ;
  isFletcher32= false ;

  xtrnlSet.isChunking  = false ;
  xtrnlSet.isDeflate  = false ;
  xtrnlSet.isDefineMode= false;
  xtrnlSet.isEndianess = false ;
  xtrnlSet.isFletcher  = false ;

  notes=0;  // default for this pointer

  return;
}

bool
NcAPI::inqDeflate(std::string var)
{
  bool is;

  if( var.size() )
  {
    is = layout.deflate[var] > 0 ? true : false ;
  }
  else
    is = isDeflate;

  return is ;
}

bool
NcAPI::isAnyRecord(void)
{
  if( isDimUnlimited() )
    for( size_t i=0 ; i < layout.varName.size() ; ++i )
      if( hasEffVarUnlimitedDim[i] )
         if( getNumOfRecords(layout.varName[i]) )
            return true;

  return false;
}

bool
NcAPI::isAnyRecord(std::string vName)
{
  if( getNumOfRecords(vName) )
    return true;

  return false;
}

bool
NcAPI::isDimUnlimited(void)
{
   bool is=(unlimitedDimName.size() > 0) ? true : false ;

   return is;
}

bool
NcAPI::isDimUnlimited(std::string &dName)
{
  bool is=false;

  if( isDimValid(dName) )
    is = (unlimitedDimName == dName) ? true : false;

  return is;
}

bool
NcAPI::isDimValid(std::string dName)
{
  for( size_t i=0 ; i < layout.dimName.size() ; ++i )
    if( dName == layout.dimName[i] )
      return true;

  return false;
}

bool
NcAPI::isRecordType(std::string vName)
{
  int varid=getVarID(vName);
  if( varid < 0 )
    return false;

  if( layout.varIsRecordType[varid] )
    return true;

  return false;
}

bool
NcAPI::isVarUnlimited(std::string vName)
{
  for( size_t i=0 ; i < layout.varName.size() ; ++i )
    if( layout.varName[i] == vName )
      if( hasEffVarUnlimitedDim[i] )
         return true;

  return false;
}

bool
NcAPI::isVarUnlimited(int varid)
{
  if( hasEffVarUnlimitedDim[varid] )
    return true;

  return false;
}

bool
NcAPI::isVariableValid(std::string vName)
{
  for( size_t i=0 ; i < layout.varName.size() ; ++i )
    if( layout.varName[i] == vName )
       return true;

  return false;
}

bool
NcAPI::isVariableValid(std::string vName, std::string &statusText)
{
  if( isVariableValid(vName) )
     return true;

  statusText="NcAPI: invalid variable name <";
  statusText += vName ;
  statusText += ">";

  return false;
}

void
NcAPI::layoutVarAttPushes(void)
{
  // attributes
  layout.varAttName.push_back( std::vector<std::string>() );
  layout.varAttMap.push_back( std::map<std::string, int>() );
  layout.varAttType.push_back( std::vector<nc_type>() );
  layout.varAttValSize.push_back( std::vector<size_t>() );

  return;
}

void
NcAPI::layoutVarDataPushes(std::string& vName, nc_type type)
{
  // MtrxArr instances for potential record data
  if( type == NC_FLOAT )
  {
    rec_val_float.push_back(*new MtrxArr<float>()) ;
    layoutVarDataPushesT(rec_val_float.back(), vName);
  }
  else if( type == NC_DOUBLE )
  {
    rec_val_double.push_back(*new MtrxArr<double>()) ;
    layoutVarDataPushesT(rec_val_double.back(), vName);
  }
  else if( type == NC_BYTE )
  {
    rec_val_schar.push_back(*new MtrxArr<signed char>());
    layoutVarDataPushesT(rec_val_schar.back(), vName);
  }
  else if( type == NC_UBYTE )
  {
    rec_val_uchar.push_back(*new MtrxArr<unsigned char>()) ;
    layoutVarDataPushesT(rec_val_uchar.back(), vName);
  }
  else if( type == NC_CHAR )
  {
    rec_val_text.push_back(*new MtrxArr<char>()) ;
    layoutVarDataPushesT(rec_val_text.back(), vName);
  }
  else if( type == NC_SHORT )
  {
    rec_val_short.push_back(*new MtrxArr<short>()) ;
    layoutVarDataPushesT(rec_val_short.back(), vName);
  }
  else if( type == NC_USHORT )
  {
    rec_val_ushort.push_back(*new MtrxArr<unsigned short>()) ;
    layoutVarDataPushesT(rec_val_ushort.back(), vName);
  }
  else if( type == NC_INT )
  {
    rec_val_int.push_back(*new MtrxArr<int>()) ;
    layoutVarDataPushesT(rec_val_int.back(), vName);
  }
  else if( type == NC_UINT )
  {
    rec_val_uint.push_back(*new MtrxArr<unsigned int>()) ;
    layoutVarDataPushesT(rec_val_uint.back(), vName);
  }
  else if( type == NC_UINT64 )
  {
    rec_val_ulonglong.push_back(*new MtrxArr<unsigned long long>()) ;
    layoutVarDataPushesT(rec_val_ulonglong.back(), vName);
  }
  else if( type == NC_INT64 )
  {
    rec_val_longlong.push_back(*new MtrxArr<long long >()) ;
    layoutVarDataPushesT(rec_val_longlong.back(), vName);
  }
  else if( type == NC_STRING )
  {
    rec_val_string.push_back(*new MtrxArr<char*>()) ;
    layoutVarDataPushesStr(rec_val_string.back(), vName);
  }

  // push_back with zero to stay in sync with varid indexes
  layoutVarDataPushesVoid(type);

  return;
}

void
NcAPI::layoutVarDataPushesStr(MtrxArr<char*>& ma, std::string& vName)
{
  char pp='\0';
  char* p=&pp;

  std::vector<char*> fMV;
  fMV.push_back(p);
  std::string s("inf NaN");
  ma.setExceptionValue(fMV, 0,  &s);

  return;
}

template <typename T>
void
NcAPI::layoutVarDataPushesT(MtrxArr<T>& ma, std::string& vName)
{
  std::vector<T> fMV;
  std::vector<char> mode;

  // could return p_mode==0
  (void) get_FillValue(vName, fMV, &mode) ;
  std::string s("inf NaN");
  ma.setExceptionValue(fMV, &mode, &s);

  return;
}

void
NcAPI::layoutVarDataPushesVoid(nc_type type)
{
  // MtrxArr instances for potential record data
  if( type != NC_BYTE )
    rec_val_schar.push_back(0);

  if( type != NC_UBYTE )
    rec_val_uchar.push_back(0) ;

  if( type != NC_CHAR )
    rec_val_text.push_back(0) ;

  if( type != NC_SHORT )
    rec_val_short.push_back(0) ;

  if( type != NC_USHORT )
    rec_val_ushort.push_back(0) ;

  if( type != NC_INT )
    rec_val_int.push_back(0) ;

  if( type != NC_UINT )
    rec_val_uint.push_back(0) ;

  if( type != NC_UINT64 )
    rec_val_ulonglong.push_back(0) ;

  if( type != NC_INT64 )
    rec_val_longlong.push_back(0) ;

  if( type != NC_FLOAT )
    rec_val_float.push_back(0) ;

  if( type != NC_DOUBLE )
    rec_val_double.push_back(0) ;

  if( type != NC_STRING )
    rec_val_string.push_back(0) ;

   return;
}

bool
NcAPI::open(std::string f, std::string mode, bool isExitOnError)
{
  if( f.size() == 0 && !isNcFilenameChanged )
    return true ;  // filename is unchanged

  if( isThisOpen ) // only if previously opened
     clearLayout();

  if( f.size() > 0 )
    setFilename(f, mode);

  Split fam(fileAccessMode, "|");

  // read
  int omode=0; // default: NC_NOWRITE
  for( size_t i=0 ; i < fam.size() ; ++i )
  {
    if( fam[i] == "NC_WRITE" )
      omode |= NC_WRITE;
    if( fam[i] == "NC_SHARE" )
      omode |= NC_SHARE;
  }

  // open netCDF file
  status = nc_open(ncFilename.c_str(), omode, &ncid) ;

  if(status)
  {
    if( isExitOnError )
    {
      std::string key("NC_1_3");
      std::string capt("Could not open file.");
      std::string text ;

      exceptionHandling(key, capt, text);
      exit(1);
    }
    else
      return false;
  }

  isDefineMode=false;

  getLayout();  // interface to the entire layout

  // set flag for opened
  isThisOpen=true;

  // reset new-filename flag
  isNcFilenameChanged=false;

  return isThisOpen;
}

void
NcAPI::print_error(std::string method, std::string text)
{
  std::ostringstream ostr(std::ios::app);
  ostr << "\nFile: " << ncFilename ;
  ostr << "\n" << method << "\n" << text ;
  if(status)
    ostr << "\nstatus=" << status ;
  ostr <<  "\n";

  exceptionError( ostr.str() );

  return;
}

template <typename Type>
void
NcAPI::putData(size_t rec, int varid, Type *arr )
{
  disableDefMode();

  if( layout.rec_index[varid] < UINT_MAX )
    layout.rec_start[varid][ layout.rec_index[varid] ] = rec ;

  status = nc_put_vara(ncid, varid,
        layout.rec_start[varid], layout.rec_count[varid], (void*)arr);

  if(status)
  {
    std::string key("NC_8_1");
    std::string capt("Could not put data to NetCDF file.") ;

    std::string text("Variable <");
    text += getVarnameFromVarID(varid);
    text += ">";

    exceptionHandling(key, capt, text, getVarnameFromVarID(varid));
  }

  return ;
}

template <typename Type>
void
NcAPI::putData(size_t rec_beg, size_t rec_count, int varid, Type *arr )
{
  if( rec_count == 1 )
  {
    putDat1(rec_beg, varid, arr ) ;
    return;
  }

  // a range of records
  disableDefMode();

  // add record properties into the arrays of netCDF edges and corners
  if( layout.rec_index[varid] < UINT_MAX )
  {
    layout.rec_start[varid][ layout.rec_index[varid] ] = rec_beg ;
    layout.rec_count[varid][ layout.rec_index[varid] ] = rec_count ;
  }

  status = nc_put_vara(ncid, varid,
        layout.rec_start[varid], layout.rec_count[varid],
           (void*)arr);

  if(status)
  {
    std::string key("NC_8_2");
    std::string capt("Could not put data to NetCDF file.") ;

    std::string text("variable=");
    text += getVarnameFromVarID(varid);

    exceptionHandling(key, capt, text, getVarnameFromVarID(varid));
    return;
  }

  // restore the record counter
  layout.rec_count[varid][ layout.rec_index[varid] ] = 1 ;

  return ;
}

template <typename ToT, typename FromT>
void
NcAPI::putData(size_t rec, int varid, FromT *from,
   ToT x, ToT (*func)(double,int,size_t), size_t recOut )
{
  if( recOut == UINT_MAX )
    recOut=rec;

  size_t sz=getDataSize(varid);

  if( layout.rec_index[varid] < UINT_MAX )
    layout.rec_start[varid][ layout.rec_index[varid] ] = rec;

  ToT *to = new ToT [ sz ] ;
  for( size_t i=0 ; i < sz ; ++i)
  {
    if( func )
    {
      to[i]=static_cast<ToT>(
               func(static_cast<double>(
                  static_cast<double>(from[i])),rec,i  ) );
    }
    else
      to[i] = static_cast<ToT>(from[i]) ;
  }

  disableDefMode();

//  var->put_rec( to, static_cast<long>(recOut) ) ;
  status = nc_put_vara(ncid, varid,
        layout.rec_start[varid], layout.rec_count[varid], (void*)to);

  if(status)
  {
    std::string key("NC_8_3");
    std::string capt("Could not put data to NetCDF file.") ;

    std::string text("variable=");
    text += getVarnameFromVarID(varid);

    exceptionHandling(key, capt, text, getVarnameFromVarID(varid));
  }

  delete [] to ;

  return ;
}

template <typename Type>
void
NcAPI::putDat1(size_t rec_beg, int varid, Type *arr )
{
  // a range of records
  disableDefMode();

  if( layout.rec_index[varid] < UINT_MAX )
    layout.rec_start[varid][ layout.rec_index[varid] ] = rec_beg ;

  status = nc_put_var1(ncid, varid,
        layout.rec_start[varid], (void*)arr);

  if(status)
  {
    std::string key("NC_8_4");
    std::string capt("Could not put a single data value to NetCDF file.")  ;

    std::string text("variable=");
    text += getVarnameFromVarID(varid);

    exceptionHandling(key, capt, text, getVarnameFromVarID(varid));
  }

  return ;
}

// special
template <typename T>
void
NcAPI::setAtt(std::string vName, std::string aName,
       std::vector<std::string> &vstr, T x)
{
  // Specialisation for generateParseAtt():
  // convert strings to type

  if( vstr.size() == 0 )
  {
    status=0;
    std::string key("NC_5_4");
    std::string capt("Value-vector must have size > 0.");

    std::string text("variable=");
    text += vName;

    exceptionHandling(key, capt, text, vName);
  }

  // get varid
  int varid = getVarID( vName );
  if( varid < -1 )
    return;

  // get type
  nc_type type = convTypeID( x );

  //convert
  std::vector<T> vals;
  T val;

  if( type == NC_CHAR )
    setAtt(vName, aName, vstr[0].c_str() );
  else
  {
    for( size_t i=0 ; i < vstr.size() ; ++i )
    {
      val=static_cast<T>( hdhC::string2Double(vstr[i]) );
      vals.push_back( val );
    }

    setAtt(varid, aName, type, vals);
  }

  return ;
}

// vector argument
template <typename T>
void
NcAPI::setAtt(int varid, std::string aName, std::vector<T> &values)
{
   enableDefMode();

   // safe if aName was not defined
   size_t ix = delAtt(aName, varid);

   // inquire type
   nc_type ncType=convTypeID( values[0] );

   if( ncType == NC_STRING )
   {
      // call special method for strings converted to char *
      setAttString(varid, aName, values);
      return;
   }

   // new attribute in any case
   size_t len=values.size();
   T *arr=0;
   if( len > 0 )
     arr= new T [len] ;
   for( size_t i=0 ; i < len ; ++i)
     arr[i] = values[i];

   status=nc_put_att(ncid, varid, aName.c_str(), ncType, len, static_cast<void*>(arr) );

   if(status)
   {
    std::string key("NC_3_13");
    std::string capt("Could not put attribute value.") ;

    std::string text("variable=");
    text += getVarnameFromVarID(varid);
    text += ", attribute=";
    text += aName;

    exceptionHandling(key, capt, text, getVarnameFromVarID(varid));
   }

   // keep layout adjusted
   updateAtts(varid, aName, ix, ncType, len );

   delete [] arr;

   return ;
}

// with type conversion
template <typename To, typename From>
void
NcAPI::setAtt(int varid, std::string aName,
      std::vector<From> &from, To x)
{
   std::vector<To> to;

   for( size_t i=0 ; i < from.size() ; ++i)
     to.push_back( static_cast<To>(from[i]) );

   nc_type ncType=convTypeID( from[0] );

   setAtt(varid, aName, to);

   return ;
}


// array argument
template <typename T>
void
NcAPI::setAtt(int varid, std::string aName, T *arr, size_t len)
{
   enableDefMode();

   // safe if aName was not defined
   size_t ix = delAtt(aName, varid);

   // new attribute in any case

   nc_type ncType=convTypeID( arr[0] );

   status=nc_put_att(ncid, varid, aName.c_str(), ncType, len, static_cast<void*>(arr) );

   if(status)
   {
     std::string vName(getVarnameFromVarID(varid));

     std::string key("NC_3_2");
     std::string capt("Attribute <" + vName + ">:" + aName) ;
     capt += ": Could not put attribute value." ;

     std::string text("variable=");
     text += getVarnameFromVarID(varid);
     text += ", attribute=";
     text += aName;

     exceptionHandling(key, capt, text, vName);
   }

   // keep layout adjusted
   updateAtts(varid, aName, ix, ncType, len );

   return ;
}

// with type conversion
template <typename To, typename From>
void
NcAPI::setAtt(int varid, std::string aName,
      From *from, size_t sz, To x)
{
   To *to = new To [sz] ;

   for( size_t i=0 ; i < sz ; ++i)
     to[i]= static_cast<To>(from[i]) ;

   setAtt(varid, aName, to, sz);

   delete [] to;

   return ;
}


// plain variable
template <typename From>
void
NcAPI::setAtt(int varid, std::string aName, From val)
{
  size_t sz = 1;

  // use the also the array variant for a plain value
  setAtt(varid, aName, &val, sz );
  return;
}

// with type conversion
template <typename From, typename To>
void
NcAPI::setAtt(int varid, std::string aName, From val, To to)
{
  to=static_cast<To>(val);
  size_t sz = 1;

  // use the array variant
  setAtt(varid, aName, &to, sz );
  return;
}

// convert strings to char*
void
NcAPI::setAttString(int varid, std::string aName,
       std::string text)
{
   std::vector<std::string> v;
   v.push_back(text);
   setAttString( varid, aName, v);
}

void
NcAPI::setAttString(int varid, std::string aName,
       std::vector<std::string> &text)
{
   // put a new attribute or change an old one (delete and new)

   // although netcdf-4 has the capability to store strings,
   // we have to write '\0' delimited characters, because of netcdf-3.

   enableDefMode();

   size_t sz = text.size();  // space for the \0 terminations
   for( size_t i=0 ; i < text.size() ; ++i )
     sz += text[i].size();

   // new in any case
   char *p = new char [sz] ;

   size_t k=0;
   for( size_t i=0 ; i < text.size() ; ++i)
   {
     for( size_t j=0 ; j < text[i].size() ; ++j)
     {
       p[k++] = text[i][j];
     }

     p[k++] = '\0' ;
   }

   // call method for char-array
   setAtt(varid, aName, p, sz);

   delete [] p;

   return ;
}

void
NcAPI::setChunking(int varid, size_t storage, size_t *chunkSize, size_t sz)
{
  enableDefMode();

  layout.varStorage[varid] = storage ;

  for( size_t i=0 ; i < sz ; ++i )
    layout.varChunkSize[varid][i] = chunkSize[i];

  status=nc_def_var_chunking(ncid, varid, storage, chunkSize );

  if(status)
  {
    std::string key("NC_4_5");
    std::string capt("Could not define chunking.") ;

    std::string text("variable=");
    text += getVarnameFromVarID(varid);

    exceptionHandling(key, capt, text, getVarnameFromVarID(varid));
  }

  return;
}

void
NcAPI::setDeflate(int varid, int shuffle, int deflate, int deflate_level)
{
  layout.varShuffle[varid] = shuffle ;
  layout.varDeflate[varid] = deflate ;
  layout.varDeflateLevel[varid] = deflate_level ;

  enableDefMode();

  status=nc_def_var_deflate(ncid, varid,
        shuffle, deflate, deflate_level );

  if(status)
  {
    std::string key("NC_4_6");
    std::string capt("Could not define deflating.") ;

    std::string text("variable=");
    text += getVarnameFromVarID(varid);

    exceptionHandling(key, capt, text, getVarnameFromVarID(varid));
  }

  return;
}

void
NcAPI::setEndian(int varid, int endian)
{
  layout.varEndian[varid] = endian ;

  enableDefMode();

  status=nc_def_var_endian(ncid, varid, endian );

  if(status)
  {
    std::string key("NC_4_7");
    std::string capt("Could not define endianess.") ;

    std::string text("variable=");
    text += getVarnameFromVarID(varid);

    exceptionHandling(key, capt, text, getVarnameFromVarID(varid));
  }

  return;
}

void
NcAPI::setFileAccessMode(std::string fam)
{
  fileAccessMode="";

  // decompose string at '|'. Also remove surrounding
  // spaces from each item
  Split split(fam, "|");
  split.addSeparator(',');

  if( split.size() == 0 )
    return;

  for( size_t i=0 ; i < split.size()-1 ; ++i)
  {
    if( split[i].size() == 0 )
      continue;

    fileAccessMode += hdhC::stripSides( split[i] ) ;
    fileAccessMode += '|' ;
  }
  fileAccessMode
     += hdhC::stripSides( split[split.size()-1] ) ;

  return;
}

void
NcAPI::setFilename(std::string file, std::string mode)
{
  std::string tmpFile;

  // e.g.: ncPath=/myRootPath and file=prePath/Filename
  //       ==> ncCurrentPath=/myRootPath/prePath

  if( file.size() > 0 )
    tmpFile = file;
  else
    tmpFile = ncPath + "/" ;

  setFileAccessMode(mode);

  if( tmpFile == ncFilename )
    return;

  ncFilename = tmpFile ;

  isNcFilenameChanged=true;

  return;
}

void
NcAPI::setFletcher32(int varid, int f)
{
  enableDefMode();

  layout.varFletcher32[varid] = f ;

  status=nc_def_var_fletcher32(ncid, varid, f );

  if(status)
  {
    std::string key("NC_4_8");
    std::string capt("Could not define Fletcher32 properties.") ;

    std::string text("variable=");
    text += getVarnameFromVarID(varid);

    exceptionHandling(key, capt, text, getVarnameFromVarID(varid));
  }

  return;
}

void
NcAPI::updateAtts(int varid, std::string aName, size_t attIndex,
      nc_type ncType, size_t len)
{
   //keep sizes in sync
   if( varid < 0 )
   {
     if( attIndex < UINT_MAX )
     {
       layout.globalAttName[attIndex]= aName ;
       layout.globalAttType[attIndex] = ncType ;
       layout.globalAttValSize[attIndex]= len ;
       layout.globalAttMap[aName] = attIndex;
     }
     else
       addAttToLayout(varid, aName, ncType, len);
   }
   else
   {
     if( attIndex < UINT_MAX )
     {
       layout.varAttName[varid][attIndex]= aName ;
       layout.varAttType[varid][attIndex] = ncType ;
       layout.varAttValSize[varid][attIndex]= len ;
       layout.varAttMap[varid][aName] = attIndex;
     }
     else
      addAttToLayout(varid, aName, ncType, len);
   }

   return;
}
