#ifndef _VARIABLE_H
#define _VARIABLE_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "hdhC.h"

#include "geo_meta.h"
#include "nc_api.h"

#include "annotation.h"

/*! \file variable.h
 \brief Variable.
*/

  class GeoMetaBase ;
  class DataStatisticsBase ;
  class MtrxArrB ;
  class Base ;
  class InFile ;
  class NcAPI ;

//! Meta-data of a netCDF file for a particular variable.
class VariableMeta
{
  public:
  VariableMeta();

  void clearCoord(void);

  double      addOffset;
  uint32_t    checksum;  //fletcher-32
  double      doubleFillValue;
  double      doubleMissingValue;
  void       *fillValue;
  bool        isUnitsDefined;
  void       *missingValue;
  std::string name;
  double      range[2];
  double      scaleFactor;
  std::string std_name;
  nc_type     type;
  std::string units;

  std::vector<std::string>               attName;
  std::map<std::string, int>             attNameMap;
  std::vector<nc_type>                   attType;
  std::vector<std::vector<std::string> > attValue;

  struct SN_TableEntry
  {
     std::string std_name;
     std::string remainder;
     bool        found={false};
     bool        hasBlanks={false};
     std::string alias;
     std::string canonical_units;
     std::string amip;
     std::string grib;
  };
  SN_TableEntry snTableEntry[2];

  struct Coordinates
  {
     bool isAny;
     bool isCoordVar;   // coordinate variable, e.g. time(time) with units
     bool isZ_p;
     bool isZ_DL;  //dimensionless coord

     std::vector<char> cType;  // 0:X, 1:Y, 2:Z, 3: T
     std::vector<bool> isBasicType;  // longitue, latitude, z, time
     std::vector<bool> isC;
     std::vector<int>  weight;
  };
  Coordinates coord;

  int  countData;
  int  countAux;
  int  weight_DV;
  int  isUnlimited_;  // access by isUnlimited() method

  bool isArithmeticMean; // externally set
  bool isAUX;
  bool isDATA;
  bool isChecked;
  bool isClimatology;
  bool isCompress;
  bool isDescreteSamplingGeom;
  bool isExcluded;
  bool isFixed;  // isTime==false && isDataVar==true
  bool isFillValue;
  bool isFT_paramVar;
  bool isFT_pvc_var;
  bool isInstant;
  bool isLabel;
  bool isMapVar;
  bool isMissingValue;
  bool isNoData_;
  bool isScalar;
  bool isVoid;  // variables which may/must have no data

  bool is_1st_X;  // one-time switches in units_lon_lat()
  bool is_1st_Y;
  bool is_1st_rotX;
  bool is_1st_rotY;

  std::vector<std::string> dimName;
  std::vector<size_t>      dimSize;
  std::vector<size_t>      dim_ix;

//  std::string associatedTo;
  std::vector<std::string> aux;
  std::vector<std::string> auxOf;
  std::string boundsOf;    // also for climatological statistics
  std::string bounds ;  // -"-
};

//! Container class for meta-data of a given variable.
/*! Provision of access to the meta-data, methods of the Base class
 by inheritance, MtrxArr instances and data statistics of read data.
 The Infile* points to the corresponding object where the opened
 nc-file resides and the NcAPI* to the nc-file itself.
 The GeoMeta object holds 2D and 3D geo-located fields of the
 variable (also giving access to the grid-cell areas, weights, and
 coordinates). Coordinates for multi-dimensionally expressed
 longitudes/latitudes, e.g. tripolar ocean data are recognised.*/
class Variable : public VariableMeta
{
  public:

  void addWeight(int) ; // int--> index 0: X, 1: Y, 2: Z, 3: T, -1: decrease all
  void addDataCount(int i=1) ;
  void addAuxCount(int i=1) ;
  void clear(void);

  template<typename T>
  void setDefaultException(T, void *);

  template<typename T>
  void setExceptions( T*, MtrxArr<T>*) ;

  int  getAttIndex(std::string, bool tryLowerCase=true) ;
  // forceLowerCase==true will return the value always as lower-case
  std::string
       getAttValue(std::string, bool forceLowerCase=false);
  int  getCoordinateType(void);  // X: 0, Y: 1, Z: 2, T: 3, any: 4, none: -1
//  template<typename T>
//  std::pair<int,int>
//       getData(MtrxArr<T>&, int rec, int leg=0);
  bool getData(int rec);
  std::string
       getDimNameStr(bool isWithVar=false, char sep=',');
  int  getVarIndex(){ return id ;}
  bool isAuxiliary(void) { return (countAux > countData ) ? true : false ;}
  bool isCoordinate(void);
  bool isDataVar(void);
  bool isNoData(void) ;
  bool isUnlimited(void) ;
  bool isValidAtt(std::string s, bool tryLowerCase=true);
  bool isValidAtt(std::string s, std::string sub_str);
  void makeObj(bool is);
  void push_aux(std::string&);
  void setID(int i){id=i;}

  MtrxArr<char>               *mvCHAR;
  MtrxArr<signed char>        *mvBYTE;
  MtrxArr<unsigned char>      *mvUBYTE;
  MtrxArr<short>              *mvSHORT;
  MtrxArr<unsigned short>     *mvUSHORT;
  MtrxArr<int>                *mvINT;
  MtrxArr<unsigned int>       *mvUINT;
  MtrxArr<unsigned long long> *mvUINT64;
  MtrxArr<long long>          *mvINT64;
  MtrxArr<float>              *mvFLOAT;
  MtrxArr<double>             *mvDOUBLE;

  int                id;
  bool               isInfNan;

  GeoMetaBase        *pGM;
  DataStatisticsBase *pDS;
  MtrxArrB           *pMA;
  Base               *pSrcBase;
  InFile             *pIn;
  NcAPI              *pNc;
};

#endif
