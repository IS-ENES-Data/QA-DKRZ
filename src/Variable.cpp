#include "variable.h"

VariableMeta::VariableMeta()
{
  // preset some members
  isAUX=false;
  isDATA=false;

  checksum = 0;
  countAux = countData = 0;

  isArithmeticMean=false;
  isChecked=false;
  isClimatology=false;
  isCompress=false;
  isDescreteSamplingGeom=false;
  isExcluded=false;
  isFixed=false;
  isFillValue=false;
  isFT_paramVar=false;
  isFT_pvc_var=false;
  isInstant=false;
  isLabel=false;
  isMapVar=false;
  isMissingValue=false;
  isNoData_=true;
  isScalar=false;
  isUnitsDefined=false;
  isVoid=false;

  is_1st_X=true;
  is_1st_Y=true;
  is_1st_rotX=true;
  is_1st_rotY=true;

  weight_DV=0;

  isUnlimited_=-1;

  range[0]=MAXDOUBLE;
  range[1]=-MAXDOUBLE;

  coord.cType.push_back('X');
  coord.cType.push_back('Y');
  coord.cType.push_back('Z');
  coord.cType.push_back('T');

  coord.isBasicType.push_back(false);
  coord.isBasicType.push_back(false);
  coord.isBasicType.push_back(false);
  coord.isBasicType.push_back(false);

  clearCoord();
}

void
VariableMeta::clearCoord(void)
{
  coord.isAny=false;
  coord.isCoordVar=false;

  coord.isZ_p=false;
  coord.isZ_DL=false;

  coord.isC.clear();
  for( size_t c=0 ; c < 4 ; ++c )
  {
    coord.isC.push_back(false);
    coord.weight.push_back(0);
  }

  return;
}

void
Variable::addDataCount(int i)
{
  countData += i ;
  addWeight(-1);

  return;
}

void
Variable::addAuxCount(int i)
{
  countAux += i ;
  return;
}

void
Variable::addWeight(int ix )
{
  if( ix < 0 )
  {
    for(size_t c=0 ; c < 4 ; ++c )
      --coord.weight[c];
  }
  else
    ++coord.weight[ix];

  return;
}

void
Variable::clear(void)
{
    name.clear();
    id=-1;
    pGM->clear();
    pDS->clear();
    pSrcBase=0;
    pIn=0;
    pNc=0;
    return;
}

int
Variable::getAttIndex(std::string aName, bool tryLowerCase)
{
  int ix;

  if( (ix=attNameMap.count(aName)) )
     return attNameMap[aName] ;
  else if( tryLowerCase )
  {
    std::vector<std::string> attNames( pNc->getAttName(name) ) ;
    int sz = static_cast<int>(attNames.size());

    for( int i=0 ; i < sz ; ++i )
       if( aName == hdhC::Lower()(attNames[i]) )
         return i;
  }

  return -1 ;
}

std::string
Variable::getAttValue(std::string aName, bool forceLowerCase)
{
  std::string s;
  int i = getAttIndex(aName);

  if( i > -1 )
  {
     s = attValue[i][0] ;

     for( size_t j=1 ; j < attValue[i].size() ; ++j )
       s += " " + attValue[i][j] ;

     if( forceLowerCase )
       s = hdhC::Lower()(s);
  }

  return s ;
}

int
Variable::getCoordinateType(void)
{
  // return when only one type exists: X: 0, Y: 1, Z: 2, T: 3, none: -1
  // return sum(i*10


  int i=0 ;
  int fact=1;

  for( size_t c=0 ; c < coord.isC.size() ; ++c )
  {
    if( coord.isC[c] )
    {
      if(i)
        fact *= 10 ;
      i += c*fact;
    }
  }

  if( i < 5 )  // ordinary case
    return i-1 ;

  return i ;
}

bool
Variable::getData(int rec)
{
  // prevent unlimited variables to get data beyond
  // available records; also if the data section is totally empty

  if( isNoData() )
    return true;

  bool is=false;

  switch ( type )
  {
    case NC_CHAR:   // signed char
    {
      is=pNc->getData(*mvCHAR, name, rec);
    }
    break;
    case NC_BYTE:   // signed char
    {
      is=pNc->getData(*mvBYTE, name, rec);
    }
    break;
    case NC_SHORT:  // short
    {
      is=pNc->getData(*mvSHORT, name, rec);
    }
    break;
    case NC_INT:    // int
    {
      is=pNc->getData(*mvINT, name, rec);
    }
    break;
    case NC_FLOAT:  // float
    {
      is=pNc->getData(*mvFLOAT, name, rec);
    }
    break;
    case NC_DOUBLE: // double
    {
      is=pNc->getData(*mvDOUBLE, name, rec);
    }
    break;
#ifdef NC4
    case NC_UBYTE:  // unsigned char
    {
      is=pNc->getData(*mvUBYTE, name, rec);
    }
    break;
    case NC_USHORT: // unsigned short
    {
      is=pNc->getData(*mvUSHORT, name, rec);
    }
    break;
    case NC_INT64:  // long long
    {
      is=pNc->getData(*mvINT64, name, rec);
    }
    break;
    case NC_UINT:   // unsigned int
    {
      is=pNc->getData(*mvUINT, name, rec);
    }
    break;
    case NC_UINT64: // unsigned long long
    {
      is=pNc->getData(*mvUINT64, name, rec);
    }
    break;
//    case NC_STRING:
//      print_error(0,
//          "NcAPI::getData: no rules to convert a string to MtrxArr");
//    break;
#endif
  }

  return is ;
}

std::string
Variable::getDimNameStr(bool isWithVar, char sep)
{
   std::string s;

   if(isWithVar)
      s = name + "(" ;

   for( size_t i=0 ; i < dimName.size() ; ++i )
   {
     if( i )
        s += sep;

     s += dimName[i] ;
   }

   if(isWithVar)
     s += ")" ;

   return s;
}

bool
Variable::isCoordinate(void)
{
  if(coord.isAny)
    return true;

  for( size_t c=0 ; c < 4 ; ++c)
    if( coord.isC[c] )
      return true;

  return false;
}

bool
Variable::isNoData(void)
{
   if( ! isNoData_ )
      return isNoData_ ;

   return ( isNoData_ = pNc->isEmptyData(name) );
}

bool
Variable::isUnlimited(void)
{
   if( isUnlimited_ > -1 )
      return isUnlimited_ ;

   isUnlimited_=0;

   if( pNc->isVarUnlimited(name) )
      isUnlimited_ = 1;
   else
   {
     for( size_t i=0 ; i < dimName.size() ; ++i )
        if( dimName[i] == "time" )
           isUnlimited_ = 1;
   }

   return isUnlimited_  ;
}

bool
Variable::isValidAtt(std::string s, bool tryLowerCase)
{

  bool is=attNameMap.count(s);

  if( !is && tryLowerCase )
  {
    std::vector<std::string> aNames( pNc->getAttName(name) ) ;

    for( size_t i=0 ; i < aNames.size() ; ++i )
       if( s ==  hdhC::Lower()(aNames[i]) )
         return true;
  }

  return is;
}

bool
Variable::isValidAtt(std::string aName, std::string sub)
{
  int j;
  if( (j=getAttIndex(aName)) > -1 )
  {
    Split x_aV(attValue[j][0]);

    for( size_t j=0 ; j < x_aV.size() ; ++j )
      if( x_aV[j] == sub )
        return true;
  }

  return false;
}

void
Variable::makeObj(bool is)
{
  isInfNan = is;

  // making of GeoMeta object is postponed
  fillValue=0;     // void*
  missingValue=0;  // void*

  switch ( type )
  {
    case NC_CHAR:   // signed char
    {
      mvCHAR = new MtrxArr<char> ;
      pMA = mvCHAR ;
//      pGM = new GeoMeta<signed char> ;
//      pDS = new DataStatistics<signed char> ;

//      static signed char fvBYTE;
//      setExceptions( &fvBYTE, mvBYTE );
    }
    break;
    case NC_BYTE:   // signed char
    {
      mvBYTE = new MtrxArr<signed char> ;
      pMA = mvBYTE ;
      pGM = new GeoMeta<signed char> ;
      pDS = new DataStatistics<signed char> ;

      static signed char fvBYTE;
      setExceptions( &fvBYTE, mvBYTE );
    }
    break;
    case NC_SHORT:  // short
    {
      mvSHORT = new MtrxArr<short> ;
      pMA = mvSHORT ;
      pGM = new GeoMeta<short> ;
      pDS = new DataStatistics<short> ;

      static short fvSHORT;
      setExceptions( &fvSHORT, mvSHORT );
    }
    break;
    case NC_INT:    // int
    {
      mvINT = new MtrxArr<int> ;
      pMA = mvINT ;
      pGM = new GeoMeta<int> ;
      pDS = new DataStatistics<int> ;

      static int fvINT;
      setExceptions( &fvINT, mvINT );
    }
    break;
    case NC_FLOAT:  // float
    {
      mvFLOAT = new MtrxArr<float> ;
      pMA = mvFLOAT ;
      pGM = new GeoMeta<float> ;
      pDS = new DataStatistics<float> ;

      static float fvFLOAT;
      setExceptions( &fvFLOAT, mvFLOAT );
    }
    break;
    case NC_DOUBLE: // double
    {
      mvDOUBLE = new MtrxArr<double> ;
      pMA = mvDOUBLE ;
      pGM = new GeoMeta<double> ;
      pDS = new DataStatistics<double> ;

      static double fvDOUBLE;
      setExceptions( &fvDOUBLE, mvDOUBLE );
    }
    break;
#ifdef NC4
    case NC_UBYTE:  // unsigned char
    {
      mvUBYTE = new MtrxArr<unsigned char> ;
      pMA = mvUBYTE ;
      pGM = new GeoMeta<unsigned char> ;
      pDS = new DataStatistics<unsigned char> ;

      static unsigned char fvUBYTE;
      setExceptions( &fvUBYTE, mvUBYTE );
    }
    break;
    case NC_USHORT: // unsigned short
    {
      mvUSHORT = new MtrxArr<unsigned short> ;
      pMA = mvUSHORT ;
      pGM = new GeoMeta<unsigned short> ;
      pDS = new DataStatistics<unsigned short> ;

      static unsigned short fvUSHORT;
      setExceptions( &fvUSHORT, mvUSHORT );
    }
    break;
    case NC_INT64:  // long long
    {
      mvINT64 = new MtrxArr<long long> ;
      pMA = mvINT64 ;
      pGM = new GeoMeta<long long> ;
      pDS = new DataStatistics<long long> ;

      static long long fvINT64;
      setExceptions( &fvINT64, mvINT64 );
    }
    break;
    case NC_UINT:   // unsigned int
    {
      mvUINT = new MtrxArr<unsigned int> ;
      pMA = mvUINT ;
      pGM = new GeoMeta<unsigned int> ;
      pDS = new DataStatistics<unsigned int> ;

      static unsigned int fvUINT;
      setExceptions( &fvUINT, mvUINT );
    }
    break;
    case NC_UINT64: // unsigned long long
    {
      mvUINT64 = new MtrxArr<unsigned long long> ;
      pMA = mvUINT64 ;
      pGM = new GeoMeta<unsigned long long> ;
      pDS = new DataStatistics<unsigned long long> ;

      static unsigned long long fvUINT64;
      setExceptions( &fvUINT64, mvUINT64 );
    }
    break;
    case NC_STRING: // unsigned long long
    {
       return;
    }
#endif
  }

  pMA->disableValueExceptionUpdate();

  return;
}

void
Variable::push_aux(std::string& vName)
{
   size_t i;
   for( i=0 ; i < aux.size() ; ++i )
     if( aux[i] == vName )
       break;

   if( i == aux.size() )
     aux.push_back(vName);

   return;
}

template<typename T>
void
Variable::setDefaultException(T v, void *vmv)
{
   MtrxArr<T>* mv = reinterpret_cast< MtrxArr<T>* >(vmv) ;

   fillValue =(void*) &v;

   if( isInfNan )
   {
     std::string s("inf nan");
     pGM->enableExceptionValue( (void*) &v, 1, 0, &s) ;
     mv->valExcp->enableExceptionValue( &v, 1, 0, &s) ;
   }
   else
   {
     pGM->enableExceptionValue( (void*) &v, 1) ;
     mv->valExcp->enableExceptionValue( &v, 1) ;
   }

   return;
}

template<typename T>
void
Variable::setExceptions( T* v, MtrxArr<T> *mv)
{
  std::vector<T> vs;

  pNc->getAttValues(vs, "_FillValue", name) ;
  if( vs.size() )
  {
    *v = vs[0];
    doubleFillValue = static_cast<double>(v[0]);
    fillValue =(void*) v;

    if( isInfNan )
    {
      std::string s("inf nan");
      mv->valExcp->enableExceptionValue( v, 1, 0, &s) ;
      pGM->enableExceptionValue( (void*)v, 1, 0, &s) ;
    }
    else
    {
      mv->valExcp->enableExceptionValue( v, 1) ;
      pGM->enableExceptionValue( (void*)v, 1) ;
    }

    isFillValue=true;
  }
  else if( isInfNan )
  {
    std::string s("inf nan");
    mv->valExcp->enableExceptionValue( 0, 0, 0, &s) ;
    pGM->enableExceptionValue( (void*)0, 0, 0, &s) ;
  }

/*
  // CMIP5 has these default values for omitted _FillValue
  else
  {
     // CF default
     char fill_CHAR         = 0x00 ;
     signed char fill_BYTE  = -127 ;
     short fill_SHORT       = -32767 ;
     int fill_INT           = -2147483647;
     float fill_FLOAT       = 9.9692099683868690e+36 ;
     double fill_DOUBLE     = 9.9692099683868690e+36 ;

     T x;

     if( typeid(x).name() ==  typeid(fill_FLOAT).name() )
        setDefaultException(fill_FLOAT, (void*) mv) ;
     else if( typeid(x).name() ==  typeid(fill_DOUBLE).name() )
        setDefaultException(fill_DOUBLE, (void*) mv) ;
     else if( typeid(x).name() ==  typeid(fill_INT).name() )
        setDefaultException(fill_INT, (void*) mv) ;
     else if( typeid(x).name() ==  typeid(fill_SHORT).name() )
        setDefaultException(fill_SHORT, (void*) mv) ;
     else if( typeid(x).name() ==  typeid(fill_BYTE).name() )
        setDefaultException(fill_BYTE, (void*) mv) ;
     else if( typeid(x).name() ==  typeid(fill_CHAR).name() )
        setDefaultException(fill_CHAR, (void*) mv) ;

     isFillValue=true;
  }
*/

  return;
}
