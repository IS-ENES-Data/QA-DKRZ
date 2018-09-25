#ifndef _MATRIX_ARRAY_H
#define _MATRIX_ARRAY_H

#include <cmath>
#include <limits>
#include <fstream>
#include <sstream>
#include <map>
#include <string>
#include <typeinfo>
#include <vector>

#include "hdhC.h"

/*! \file matrix_array.h
 \brief Interface to memory data: array <--> matrix format.
*/

//! Handling of data exceptions within the MtrxArr class.
/*! User provided exception value(s), a.k.a. _FillValue, Inf and NaN.*/
template<typename T>
class ValueException
{
public:
  ValueException();
  ~ValueException();
  void clearExceptionValue(void);
  void copy( const ValueException<T> &g, bool clearCounts=false);
  void disableValueExceptionTest(std::string s);
  void enableValueExceptionTest(std::string s);
  void enableExceptionValue(T *argv, size_t argc,
          std::vector<char>* mode=0, std::string* s=0)
          { setExceptionValue(argv, argc, mode, s); }
template<typename T_in>
  void import( const ValueException<T_in> &g);

  bool testInfNaN(bool *is, size_t isSz, size_t isBeg, T* arr, size_t arr_sz);
  bool testInf(bool *is, size_t isBeg, size_t isEnd, T* arr, size_t arr_sz);
  bool testNaN(bool *is, size_t isBeg, size_t isEnd, T* arr, size_t arr_sz);
  void resetExceptionCounts(void);
  void setExceptionValue(T *argv, size_t argc,
         std::vector<char>* mode=0, std::string* s=0);
  void testValueException( T* arr, size_t arr_sz,
         std::vector<size_t> &validRangeBegin ,
         std::vector<size_t> &validRangeEnd );
  void testValueModesFull(T* arr, size_t arr_sz,
         std::vector<size_t> &validRangeBegin ,
         std::vector<size_t> &validRangeEnd);
  void testValueModesPoint(T* arr, size_t arr_sz,
         std::vector<size_t> &validRangeBegin ,
         std::vector<size_t> &validRangeEnd);
  void testValueModesPoints(T* arr, size_t arr_sz,
         std::vector<size_t> &validRangeBegin ,
         std::vector<size_t> &validRangeEnd);

  void updateValueException(bool b)
          { update = b; }

  // telling events. Note: inf and NaN are counted separately.
  std::vector<T> exceptionValue;  // user provided exception values
  std::vector<char> exceptionMode; // '=', '<', '>', 'R' (range R: two values)
  std::vector<size_t> exceptionCount;
  size_t countInf;
  size_t countNaN;
  int firstInf_ix;
  int firstNaN_ix;

  // permitted values: 'inf', 'NaN', or 'user'
  std::map<std::string, bool> isValueExceptionTest;

  bool   update;
private:      // prevent copying
  ValueException(const ValueException&);
  ValueException& operator=(const ValueException&);
};

template<typename T>
ValueException<T>::ValueException()
  {
    // init value exceptions
    isValueExceptionTest["inf"]=false;
    isValueExceptionTest["nan"]=false;
    isValueExceptionTest["user"]=false;
    isValueExceptionTest["none"]=true;

    firstInf_ix=-1;
    firstNaN_ix=-1;
    update = true;
  }

template<typename T>
ValueException<T>::~ValueException()
{
    clearExceptionValue();
}

template<typename T>
void
ValueException<T>::clearExceptionValue(void)
{
    exceptionValue.clear();
    exceptionMode.clear();
    exceptionCount.clear();

    return;
}

template<typename T>
void
ValueException<T>::copy( const ValueException<T> &g, bool is)
{
    exceptionValue  = g.exceptionValue ;
    exceptionMode   = g.exceptionMode;
    exceptionCount = g.exceptionCount ;
    isValueExceptionTest = g.isValueExceptionTest ;
    countInf  = g.countInf;
    countNaN  = g.countNaN;
    firstInf_ix = g.firstInf_ix;
    firstNaN_ix = g.firstNaN_ix;

    if( is )
    {
      for( size_t i=0 ; i < exceptionCount.size() ; ++i )
        exceptionCount[i] = 0 ;
      countInf  = 0;
      countNaN  = 0;
      firstInf_ix = -1;
      firstNaN_ix = -1;
    }

    return;
}

template<typename T>
void
ValueException<T>::disableValueExceptionTest(std::string s)
{
    s = hdhC::Lower()(s);

    size_t c=0;
    if( s.find("inf") < std::string::npos )
    {
      isValueExceptionTest["inf"]=false;
      ++c;
    }
    if( s.find("nan") < std::string::npos )
    {
      isValueExceptionTest["nan"]=false;
      ++c;
    }
    if( s.find("user") < std::string::npos )
    {
      isValueExceptionTest["user"]=false;
      ++c;
    }

    if( c == 3 )
      isValueExceptionTest["none"]=true;
    else
      isValueExceptionTest["none"]=false;

    return;
}

template<typename T>
void
ValueException<T>::enableValueExceptionTest(std::string s)
{
    if( s.size() )
      s = hdhC::Lower()(s);
    else
      s="inf nan user"; //enable all

    if( s.find("inf") < std::string::npos )
    {
      isValueExceptionTest["inf"]=true;
      isValueExceptionTest["none"]=false;
    }
    if( s.find("nan") < std::string::npos )
    {
      isValueExceptionTest["nan"]=true;
      isValueExceptionTest["none"]=false;
    }
    if( s.find("user") < std::string::npos )
    {
      isValueExceptionTest["user"]=true;
      isValueExceptionTest["none"]=false;
    }

    return;
}

template<typename T>
template<typename T_in>
void
ValueException<T>::import( const ValueException<T_in> &g)
{
    exceptionValue.clear();
    exceptionMode.clear();

    for( size_t i=0 ; i < g.exceptionValue.size() ; ++i )
      exceptionValue.push_back(static_cast<T>(g.exceptionValue[i]) ) ;
    for( size_t i=0 ; i < g.exceptionValue.size() ; ++i )
      exceptionMode.push_back(static_cast<T>(g.exceptionMode[i]) ) ;

    exceptionCount.clear();
    for( size_t i=0 ; i < g.exceptionCount.size() ; ++i )
      exceptionCount.push_back(g.exceptionCount[i]) ;

    isValueExceptionTest = g.isValueExceptionTest ;

    countInf  = g.countInf;
    countNaN  = g.countNaN;
    firstInf_ix  = g.firstInf_ix;
    firstNaN_ix  = g.firstNaN_ix;

    update = g.update;
    return;
}

template<typename T>
void
ValueException<T>::resetExceptionCounts(void)
{
  countInf=0;
  countNaN=0;

  for( size_t i=0 ; i < exceptionCount.size(); ++i)
    exceptionCount[i]=0;

  return;
}

template<typename T>
void
ValueException<T>::setExceptionValue(
  T *argv, size_t argc, std::vector<char>* mode, std::string* s )
{
  // mode: if one of the values in argv is not a single point, but
  //       means something below, then all items of mode must be given.
  //       Else by default.
  //   = exact value
  //   < all values smaller than value
  //   > all values greater than value
  //   R range (two successive items)

  // init value exceptions
  if( argc )
  {
    exceptionValue.clear() ;
    exceptionMode.clear();
    exceptionCount.clear();

    for( size_t i=0 ; i < argc ; ++i )
    {
      if( mode )
        exceptionMode.push_back( (*mode)[i] ) ;
      else
        exceptionMode.push_back( '=' ) ;

      exceptionValue.push_back( argv[i] ) ;
      exceptionCount.push_back( 0 );
    }

    *s += " user";
  }

  enableValueExceptionTest(*s);

  return;
}

template<typename T>
bool
ValueException<T>::testInfNaN(bool *is, size_t isSz, size_t isBeg,
    T* arr, size_t arr_sz)
{
  size_t isEnd= arr_sz - isBeg ;
  if( isEnd > isSz )
    isEnd=isSz ;

  for( size_t i=0 ; i < isEnd ; ++i )
    is[i] = false;  // default for is not Inf and not NaN

  bool entire=false;
  if( isValueExceptionTest["inf"] )
    if(testInf(is, isBeg, isEnd, arr, arr_sz))
       entire=true;

  if( isValueExceptionTest["nan"] )
    if(testNaN(is, isBeg, isEnd, arr, arr_sz))
       entire=true;

  return entire;
}

template<typename T>
bool
ValueException<T>::testInf(bool *is, size_t isBeg, size_t isEnd,
    T* arr, size_t arr_sz)
{
  size_t count=0;

  for( size_t i=0 ; i < isEnd ; ++i )
  {
#ifndef XLC
    if( std::isinf( arr[isBeg+i] ) )
#else
    if( isinf( arr[isBeg+i] ) )
#endif
    {
      is[i] = true ;
      if( firstInf_ix < 0 )
        firstInf_ix = isBeg+i;

      ++count;
    }
  }

  countInf += count;
  return count;
}

template<typename T>
bool
ValueException<T>::testNaN(bool *is, size_t isBeg, size_t isEnd,
    T* arr, size_t arr_sz)
{
  // NaN compared to itself equals to false

  // Hope this is sufficient to prevent aggressive
  // optimisation; volatile should be avoided, because
  // this would take values from the central storage.

  size_t count=0;

  for( size_t i=0 ; i < isEnd ; ++i )
  {
#ifndef XLC
    if( std::isnan( arr[isBeg + i]) )
#else
    if( isnan( arr[isBeg + i]) )
#endif
    {
      is[i] = true ;
      if( firstNaN_ix < 0 )
        firstNaN_ix = isBeg+i;
      ++count;
    }
  }

  countNaN += count;

  return count;
}

template<typename T>
void
ValueException<T>::testValueException( T* arr, size_t arr_sz,
         std::vector<size_t> &validRangeBegin ,
         std::vector<size_t> &validRangeEnd )
{
   // Test values in arr against inf and NaN.
   // The result of the test is stored in array 'is'
   // which must be allocated with the same size of 'arr'

   // compose pairs of indexes to be used as ranges
   // for valid values in a for-loop.

   // reset
   resetExceptionCounts();

   validRangeBegin.clear();
   validRangeEnd.clear();

   if( isValueExceptionTest["none"] )
   {
     validRangeBegin.push_back(0);
     validRangeEnd.push_back( arr_sz );
     return;
   }

   size_t evSz = exceptionValue.size() ;

   bool isPoints=true;
   size_t e;
   for( e=0 ; e < evSz ; ++e )
   {
     if( exceptionMode[e] != '=' )
     {
       isPoints=false;
       break;
     }
   }

  // note that there is always a default
  if( isPoints )
  {
    if( e == 1 )
      testValueModesPoint(arr, arr_sz, validRangeBegin, validRangeEnd) ;
    else
      testValueModesPoints(arr, arr_sz, validRangeBegin, validRangeEnd) ;
  }
  else
    testValueModesFull(arr, arr_sz, validRangeBegin, validRangeEnd) ;

   return;
}

template<typename T>
void
ValueException<T>::testValueModesFull(T* arr, size_t arr_sz,
         std::vector<size_t> &validRangeBegin ,
         std::vector<size_t> &validRangeEnd)
{
   // valid data points are those which are not INf, NaN, _FillValue
   bool isValid=true;

   // portions of arr are mapped
   size_t chunk=1024;
   if( chunk > arr_sz )
      chunk = arr_sz;

   bool *isInfNaN = new bool [chunk];

   size_t nextStop=0;
   size_t i=0 ;  //arr index

   size_t evSz=exceptionValue.size();

   bool isLeg=false;  // legs of valid data

   while ( nextStop < arr_sz )
   {
      nextStop += chunk;

      bool isAnyInvalid = testInfNaN(isInfNaN, chunk, i, arr, arr_sz) ;

      for( ; i < arr_sz ; ++i)
      {
        if( i == nextStop )
          break;

        // test Inf and NaN block-wise; return true for any
        if( isAnyInvalid )
        {
          // skip leading invalid data points
          if( isInfNaN[i] && isLeg )
          {
            validRangeEnd.push_back(i++);  // note for i==0: isLeg=false
            isLeg=false;
          }

          while( i < nextStop && isInfNaN[i] )
            ++i;

          if( i == nextStop )
            break;  // no data point found in the entire block, try next

          if(!isLeg)
          {
            validRangeBegin.push_back(i);
            isLeg=true;
            continue;
          }
        }

        isValid=true;

        // look for exception values, ranges and min/max lobes
        for( size_t j=0 ; j < evSz ; ++j )
        {
          if( exceptionMode[j] == '=' )
          {
            if( arr[i] == exceptionValue[j] )
            {
              ++exceptionCount[j];
              isValid=false ;
              break;
            }
          }
          else if( exceptionMode[j] == '<' )
          {
            if( arr[i] < exceptionValue[j] )
            {
              ++exceptionCount[j];
              isValid=false ;
              break;
            }
          }
          else if( exceptionMode[j] == '>' )
          {
            if( arr[i] > exceptionValue[j] )
            {
              ++exceptionCount[j];
              isValid=false ;
              break;
            }
          }
          else if( exceptionMode[j] == 'R' )
          {
            if( arr[i] < exceptionValue[j] || arr[i] > exceptionValue[j+1] )
            {
              ++exceptionCount[j];
              isValid=false ;
            }

            break;  // as long as R is the last to check
          }
        }

        if( isValid )
        {
          if( !isLeg )
          {
            validRangeBegin.push_back(i);
            isLeg=true;
          }
        }
        else if( isLeg )
        {
          validRangeEnd.push_back(i);
          isLeg=false;
        }
      }
   }

   delete [] isInfNaN ;

  // Special case: arr[last] is valid and arr[last-1] is not.
  // The 'close' condition below would be true, thus leading to a fault.
  if( validRangeBegin.size() == validRangeEnd.size() )
    return;

  // close the last range
  if( isValid && isLeg )
    validRangeEnd.push_back( arr_sz );

  return ;
}

template<typename T>
void
ValueException<T>::testValueModesPoint(T* arr, size_t arr_sz,
         std::vector<size_t> &validRangeBegin ,
         std::vector<size_t> &validRangeEnd)
{
   // valid data points are those which are not INf, NaN, _FillValue

  // portions of arr are mapped
   size_t chunk=1024;
   if( chunk > arr_sz )
      chunk = arr_sz;

   bool *isInfNaN = new bool [chunk];

   size_t nextStop=0;
   size_t i=0 ;  //arr index

   bool isLeg=false;  // legs of valid data

   while ( nextStop < arr_sz )
   {
      nextStop += chunk;

      bool isAnyInvalid = testInfNaN(isInfNaN, chunk, i, arr, arr_sz) ;

      for( ; i < arr_sz ; ++i)
      {
        if( i == nextStop )
          break;

        // test Inf and NaN block-wise; return true for any
        if( isAnyInvalid )
        {
          // skip leading invalid data points
          if( isInfNaN[i] && isLeg )
          {
            validRangeEnd.push_back(i++);  // note for i==0: isLeg=false
            isLeg=false;
          }

          while( i < nextStop && isInfNaN[i] )
            ++i;

          if( i == nextStop )
            break;  // no data point found in the entire block, try next

          if(!isLeg)
          {
            validRangeBegin.push_back(i);
            isLeg=true;
            continue;
          }
        }

        // the one and only exception value must be of type '='
        if( arr[i] == exceptionValue[0] )
        {
          ++exceptionCount[0];

          if( isLeg )
          {
            validRangeEnd.push_back(i);
            isLeg=false;
          }
        }
        else if( !isLeg )
        {
          validRangeBegin.push_back(i);
          isLeg=true;
        }
      }
   }

   delete [] isInfNaN ;

  // Special case: arr[last] is valid and arr[last-1] is not.
  // The 'close' condition below would be true, thus leading to a fault.
  if( validRangeBegin.size() == validRangeEnd.size() )
    return;

  // close the last range
  if( isLeg )
    validRangeEnd.push_back( arr_sz );

  return ;
}

template<typename T>
void
ValueException<T>::testValueModesPoints(T* arr, size_t arr_sz,
         std::vector<size_t> &validRangeBegin ,
         std::vector<size_t> &validRangeEnd)
{
   // valid data points are those which are not INf, NaN, _FillValue
   bool isValid=true;

   // portions of arr are mapped
   size_t chunk=1024;
   if( chunk > arr_sz )
      chunk = arr_sz;

   bool *isInfNaN = new bool [chunk];

   size_t nextStop=0;
   size_t i=0 ;  //arr index

   size_t evSz=exceptionValue.size();

   bool isLeg=false;  // legs of valid data

   while ( i < arr_sz )
   {
      nextStop += chunk;

      bool isAnyInvalid = testInfNaN(isInfNaN, chunk, i, arr, arr_sz) ;

      for( ; i < arr_sz ; ++i)
      {
        if( i == nextStop )
          break;

        // test Inf and NaN block-wise; return true for any
        if( isAnyInvalid )
        {
          // skip leading invalid data points
          if( isInfNaN[i] && isLeg )
          {
            validRangeEnd.push_back(i++);  // note for i==0: isLeg=false
            isLeg=false;
          }

          while( i < nextStop && isInfNaN[i] )
            ++i;

          if( i == nextStop )
            break;  // no data point found in the entire block, try next

          if(!isLeg)
          {
            validRangeBegin.push_back(i);
            isLeg=true;
            continue;
          }
        }

        isValid=true;

        // all exception values must be of type '='
        for( size_t j=0 ; j < evSz ; ++j )
        {
          if( arr[i] == exceptionValue[j] )
          {
            ++exceptionCount[j];
            if( isLeg )
            {
              validRangeEnd.push_back(i);
              isLeg=false;
            }

            isValid=false ;
            break;
          }
        }

        if( isValid )
        {
          if( !isLeg )
          {
            validRangeBegin.push_back(i);
            isLeg=true;
          }
        }
      }
   }

   delete [] isInfNaN ;

  // Special case: arr[last] is valid and arr[last-1] is not.
  // The 'close' condition below would be true, thus leading to a fault.
  if( validRangeBegin.size() == validRangeEnd.size() )
    return;

  // close the last range
  if( isValid && isLeg )
    validRangeEnd.push_back( arr_sz );

  return ;
}

// the MArep class
//! The core of the MtrtAtt class.
template<typename T>
class MArep
{
public:
  struct ExceptionStruct
  {
     std::ofstream *ofsError;
     std::string strError;
  } ;

  MArep(const T* p, std::vector<size_t> &d );
  ~MArep();

  void assign( const T *p, std::vector<size_t> *d=0);
  void clearM(void);
  void getDim(std::vector<size_t> &d_in);
  T** const  getM(void);
  size_t getSize(void);
  MArep* makeRoot(void);
  void resize(size_t sz);
  void setM(void);

  // messaging in case of exceptions.
  struct ExceptionStruct xcptn;

  T* arr;       // pointer to data
  size_t counter; // number of references
  std::vector<size_t> dim; // dimensions' sizes
  std::vector<size_t> dimProduct;
  size_t arr_sz;

  T** m2D;    //matrix access; initialised at 1st call
  size_t m2D_dim0;

private:      // prevent copying
  MArep(const MArep&);
  MArep& operator=(const MArep&);
};


template<typename T>
MArep<T>::MArep(const T* p, std::vector<size_t> &d )
{
  // constructor with assignment for p>0
    m2D=0;
    m2D_dim0=0;

    counter=1;
    getDim(d);
    arr=0 ;
    arr_sz=getSize();

    resize(arr_sz);

    if( p )
      for(size_t i=0 ; i < arr_sz ; ++i)
        arr[i]=p[i];
    else
    {
      T zero = static_cast<T>( 0 ) ;
      for(size_t i=0 ; i < arr_sz ; ++i)
        arr[i]=zero;
    }

    // Note: there is no value exception test at construction time
//    validRangeBegin.push_back(0);
//    validRangeEnd.push_back(arr_sz);
  }

template<typename T>
MArep<T>::~MArep()
  {
    clearM();
    if( static_cast<bool>(arr) )
      delete [] arr;
  }

template<typename T>
void
MArep<T>::assign( const T *p, std::vector<size_t> *d)
{
    // this method imports plain arrays pointed to.
    if( ! p && ! d )
    {
      resize(0) ;  // reset all
      return;
    }

    if( d->size() )  // dimensions are passed
    {
      getDim(*d);
      size_t tmp_sz=getSize();

      // previous layout doesn't hold anymore
      if( arr_sz != tmp_sz)
      {
        arr_sz = tmp_sz;
        resize(arr_sz);
        clearM();
      }
    }
//    else:  dimensions of *this must fit

    if( p )
      for(size_t i=0 ; i < arr_sz ; ++i)
        arr[i]=p[i];

    return ;
}

template<typename T>
void
MArep<T>::clearM(void)
{
    if( static_cast<bool>(m2D) )
    {
      delete [] m2D;
      m2D=0;
      m2D_dim0=0;
    }

    return;
}

template<typename T>
void
MArep<T>::getDim(std::vector<size_t> &d_in)
{
    if( d_in == dim )
      return  ;

    // Test dimensions.
    dim = d_in;

    // Product of dimensions. Purpose: conversion of
    // array index to matrix indices and vice versa.
    // for 0-based indices:
    // index = i0 +i1*D0 + i2*D0*D1 + i3*D0*D1*D2 + ...
    dimProduct.clear();

    for( size_t i=0 ; i < dim.size() ; ++i )
    {
      dimProduct.push_back(1);

      for( size_t j=i+1 ; j < dim.size() ; ++j )
        dimProduct[i] *= dim[j] ;
    }

    return ;
}

template<typename T>
T** const
MArep<T>::getM(void)
{
     if( m2D == 0 )
       setM();
     return m2D;
}

template<typename T>
size_t
MArep<T>::getSize(void)
{
    if( dim.size() == 0 )
      return 0;

    size_t sz=1;
    for(size_t i=0 ; i < dim.size() ; ++i)
      sz *= dim[i];

    return sz;
}

template<typename T>
MArep<T>*
MArep<T>::makeRoot(void)
{
    // This method checks whether the MArep obj allocated by
    // *this will be re-used or whether a new obj has to be
    // created due to changing data.
    // This method does not change exception properties.
    // Even  exception properties are passed to a new MArep obj,
    // because changing data is going to be an the basis
    // of arithmetric operations (but, counting is reset).

    // *this obj is owner of MArep, i.e. allocated memory.
    if( counter == 1 )
      return this;

    --counter;

    // *this is not owner, thus it must allocate a new
    // MArep obj to store modified data.
    MArep *nrep = new MArep(arr, dim) ;

    return  nrep;
}

template<typename T>
void
MArep<T>::resize(size_t sz)
{
    if( static_cast<bool>(arr) )
    {
      delete [] arr;
      arr=0;
    }

    if( sz )
      arr = new T [sz] ;

    return;
}

template<typename T>
void
MArep<T>::setM(void)
{
    if( m2D && m2D_dim0 != dim[0] )
       clearM();

    // allocate new mem for the 2D shape
    if( dim.size() == 2 )
    { // m x n matrix
      if( m2D == 0 )
      {
        m2D = new T* [dim[0]];
        m2D_dim0=dim[0];
      }

      for(size_t i0=0; i0 < dim[0]; ++i0)
        m2D[i0] = arr + i0 * dim[1] ;
    }

    return;
}

//! Abstract class for the MtrxArr class.
/*! Permit run-time decisions for the various MtrxArr templates.*/
class MtrxArrB
{
public:
  virtual ~MtrxArrB(){}

  virtual void addCounter(int) =0;

  virtual void disableValueExceptionUpdate(bool b=false) =0;
  virtual void enableValueExceptionUpdate(bool b=true) =0;

  virtual size_t  getExceptionCount(size_t) =0;
  virtual size_t  getExceptionCount(std::string="all") =0;

  virtual void getExceptionValue(void* p, size_t i=0) =0;

  virtual bool isValid(void) =0;  // for the entire data set
  virtual bool isValid(size_t i0) =0;
  virtual bool isValid(size_t i0, size_t i1) =0;
  virtual bool isValid(size_t i0, size_t i1, size_t i2) =0;

  virtual void setExceptionValue(void* p=0, size_t sz=0,
                  std::vector<char>* m=0, std::string* s=0) =0;

};

//! Matrix-alike (2D, 3D) and/or array acccess of data of type T.

/*! Memory is only allocated by a constructor. Reference counting
 takes place and manages also modification of data by operators.
 If assignment methods are called, then allocated memory is
 re-used when the size of the array is unchanged. Otherwise the
 assigned pointer overwrites former values.
 Memory may also be allocated by multiple dimensional notation
 for a given shape (1D, 2D, or 3D).\n
 Special: storage of 'foreign' data may be conducted by the 'link()'
 method with no allocation, but full access.\n
 Read access is always given by the array 'arr'.\n
 Access methods: arr[index], members m[][], m3D[][][], and
 generally by get(i,[j],[k]), where j and/or k may be omitted.
 Checking of data for 'Inf', 'NaN', and user-defined exception values
 may be enabled. If so, then found value exceptions can be eliminated
 by using vectors validRangeBegin and validRangeEnd indicating ranges
 of valid data. Notice the difference between 'exception value'
 and 'value exception'. If a figure matches an 'exception value'
 (a.k.a. FillValue or missing_value), then a 'value exception'
 is given.
*/

template<typename T>
class MtrxArr : public MtrxArrB
{
  struct ExceptionStruct
  {
     std::ofstream *ofsError;
     std::string strError;
  } ;

  // messaging in case of exceptions.
  struct ExceptionStruct xcptn;

public:
//! Default constructor.
/*! Note: no test for exception values in any constructor. */
  MtrxArr();

//! Copy constructor.
  MtrxArr(const MtrxArr<T>&);

  MtrxArr(size_t nrow, size_t ncol=0, size_t nz=0);
  MtrxArr(const T*, size_t nrow, size_t ncol=0, size_t nz=0);
  MtrxArr(std::vector<size_t> &dim);
  MtrxArr(const T*, std::vector<size_t> dim);
  ~MtrxArr() {clear();}

  // representation
  MArep<T> *rep;

//  struct ValueException;  // check validity of data
  ValueException<T> *valExcp;

//! Subscription operator.
/*! Note: This is for a 1D array.*/
  T& operator[](size_t i) { return arr[i] ;}
  T const &operator[](size_t i) const { return arr[i] ;}

  //! overloaded math operators
  /*! special feature: operation with an invalid value does
      not change the validity of this object; invalid
      array values are skipped.*/
  MtrxArr<T>& operator=( const T *p);
  MtrxArr<T>& operator=( const MtrxArr<T> &);
  MtrxArr<T>& operator=( const T );

  template<typename T_in> // special:
  MtrxArr<T>& operator=( const MtrxArr<T_in> &);

  void       operator+( const T );
  void       operator-( const T );
  void       operator*( const T );
  void       operator/( const T );
  MtrxArr<T>& operator+=( const T );
  MtrxArr<T>& operator-=( const T );
  MtrxArr<T>& operator*=( const T );
  MtrxArr<T>& operator/=( const T );

  void       operator+( MtrxArr<T> &g);
  void       operator-( MtrxArr<T> &g);
  void       operator*( MtrxArr<T> &g);
  void       operator/( MtrxArr<T> &g);
  MtrxArr<T>& operator+=( MtrxArr<T> &g);
  MtrxArr<T>& operator-=( MtrxArr<T> &g);
  MtrxArr<T>& operator*=( MtrxArr<T> &g);
  MtrxArr<T>& operator/=( MtrxArr<T> &g);

  //! Indirect indexing.
  MtrxArr<T> operator[]( const MtrxArr<size_t> &p );

  void       addCounter(int i) { rep->counter += i; }

  //! Assignment of an array.
  template<typename T_in> // with type conversion
  void    assign( const T_in *p, std::vector<size_t> dim);
  void    assign( const T *p,  std::vector<size_t> dim);
  void    assign( const T *p,  size_t nx=0, size_t ny=0, size_t nz=0);
  template<typename T_in> // with type conversion
  void    assign( const T_in *p, size_t nx=0, size_t ny=0, size_t nz=0);

  //! get the starting address [with off set].
  T*      begin(size_t offSet=0) { return arr + offSet; }

  //! clear local allocations (not the for the reference).
  void    clear(void);

  //! Disable/enable testing on Inf, NaN, and user-defined values.
  /*! If parameter s is empty, then all types of exceptions will
      be tested. Values of s are "inf", "NaN, or "user" and any
      combination. Testing is enabled by default for all.*/
  void    disableValueExceptionTest(std::string s="");
  void    disableValueExceptionUpdate(bool b=false)
             { valExcp->update = b; }

  void    enableValueExceptionTest(std::string s="", std::string add="")
             { valExcp->enableValueExceptionTest(s); }

  void    enableValueExceptionUpdate(bool b=true)
             { valExcp->update = b; }

  void    frame( const MtrxArr<T> &g, size_t beg, size_t sz=0);

  //! Get value at given index(es).
  /*! For external arry and matrix notation (2D, and 3-D).*/
  T       get(std::vector<size_t>& vs) { return arr[ index(vs) ] ;}
  T       get(size_t i0, size_t i1, size_t i2)
            {return *(arr +i0+i1*rep->dimProduct[1]+i2*rep->dimProduct[2] ) ;}
  T       get(size_t i0, size_t i1)
            {return *(arr + i0 + i1*rep->dim[0]);}
  T       get(size_t i0)
            {return arr[i0];}

  //! Get number of columns.
  size_t  getColSize(void)
         { return rep->dim.size() > 1 ? rep->dim[1] : 0 ;}

  std::vector<size_t>
          getDimensions(void)const { return rep->dim;}
  size_t  getDimSize(){ return rep->dim.size(); }

  size_t  getExceptionCount(size_t i);
  size_t  getExceptionCount(std::string s="all");

  void    getExceptionValue(void* p, size_t i=0);
  void    getExceptionValue(std::vector<T> &, size_t);

  T ** const
          getM(void) { return rep->getM(); }

  //! Get number of rows.
  size_t  getRowSize(void) { return rep->dim.size() > 0 ? rep->dim[0] : 0 ;}

  std::vector<std::string>
          getStatus(bool clear=true);

  //! Get value matrix indices at given array index.
  std::vector<size_t>
         indices(size_t);

  // String representation of indices
  std::string
         indicesStr(size_t);

  //! Get value at given index(es).
  size_t index(std::vector<size_t>&);
  size_t index(size_t i0, size_t i1, size_t i2)
            {return i0 + i1*rep->dim[0]+i2*rep->dimProduct[2] ;}
  size_t index(size_t i0, size_t i1)
            {return i0 + i1*rep->dim[0];}
  size_t index(size_t i0)
            {return i0;}

  bool   initValExceptUpdate(void);

  //! Get the state of the data representation
  /*! Return value is false if a data value pointed to by index(es)
      is Inf, NaN, or a user-defined exception value (filling data).
      The method called without parameters returns false, if at least
      one value of the array is invalid.*/
  bool   isValid(void);  // for the entire data set
  bool   isValid(size_t i0);
  bool   isValid(size_t i0, size_t i1)
            {return isValid(i0 * rep->dim[1] + i1) ;}
  bool   isValid(size_t i0, size_t i1, size_t i2)
            {return isValid((i0*rep->dim[1]+i1) * rep->dim[2] + i2); }

  //! Link a pointer to data while keeping the shape.
  /*! Purpose: Layering of 3D data*/
  void    link( const T *p, bool keepShape=false);

  //! Put value at given index(es).
  /*! For external arry and matrix notation (2D, and 3-D).*/
  void    put(T val, std::vector<size_t>&);
  void    put(T val, size_t i0, size_t i1=0, size_t i2=0);

  //! Resize matrix.
  void    resize(size_t nx, size_t ny=0, size_t nz=0);
  void    resize(std::vector<size_t>&d);

  //! Set exception values.
  /*! String enable value exception test for "inf" and/or "NaN",
      e.g. string="inf NaN" enables both. Number sz user-defined
      exception values (a.k.a. missingValue, _FillValue)
      are stored in array vE.*/
  void    setExceptionValue(void* p=0, size_t sz=0,
            std::vector<char>* m=0, std::string* s=0)
            { valExcp->setExceptionValue(reinterpret_cast<T*>(p), sz, m, s);}

  void    setExceptionValue(T *vE, size_t sz,
            std::vector<char>* m=0, std::string* s=0)
            { valExcp->setExceptionValue(vE, sz, m, s);}

  void    setExceptionValue(std::vector<T> vE,
            std::vector<char>* mode=0, std::string* s=0);

  //! Get size of data.
  size_t  size(void) const;

  void    testValueException(void)
             { valExcp->testValueException(rep->arr, rep->arr_sz,
               validRangeBegin, validRangeEnd); }

  T*  arr ;  // points to &rep->(*pva)[0]
  T zero ;  // nomen est omen

  std::vector<std::string> status;

  // combine to index ranges of valid values
  std::vector<size_t> validRangeBegin ;
  std::vector<size_t> validRangeEnd ;
};

// Default constructor.
template<typename T>
MtrxArr<T>::MtrxArr()
{
  std::vector<size_t> t;
  zero = static_cast<T>( 0 );
  valExcp = new ValueException<T> ;

  rep = new MArep<T>(0, t );  // t with size()==0

  if( rep->arr )
    arr=rep->arr;
  else
    arr=0;
}

// Copy constructor.
template<typename T>
MtrxArr<T>::MtrxArr(const MtrxArr<T> &ma)
{
  zero = static_cast<T>( 0 );
  valExcp = new ValueException<T> ;

  ma.rep->counter++;
  rep=ma.rep;

  rep->dim=ma.rep->dim;
  arr=rep->arr;

  valExcp->copy( *ma.valExcp );
}

// Construct for a given shape without assignment
template<typename T>
MtrxArr<T>::MtrxArr(size_t nx, size_t ny, size_t nz)
{
  zero = static_cast<T>( 0 );
  valExcp = new ValueException<T> ;
  rep=0;

  std::vector<size_t> t;
  t.push_back(nx);
  t.push_back(ny);
  t.push_back(nz);

  rep = new MArep<T>(0, t );
  arr=rep->arr;
}

// Construct for a given shape without assignment
template<typename T>
MtrxArr<T>::MtrxArr(const T *p, size_t nx, size_t ny, size_t nz)
{
  zero = static_cast<T>( 0 );
  valExcp = new ValueException<T> ;
  rep=0;

  std::vector<size_t> t;
  t.push_back(nx);
  t.push_back(ny);
  t.push_back(nz);

  rep = new MArep<T>(p, t );
  arr=rep->arr;

  // test and get exception values
  testValueException();
}

// Construct for a given shape without assignment
template<typename T>
MtrxArr<T>::MtrxArr(std::vector<size_t> &d)
{
  zero = static_cast<T>( 0 );
  valExcp = new ValueException<T> ;
  rep=0;
  rep = new MArep<T>(0, d );
  arr=rep->arr;
}

template<typename T>
MtrxArr<T>::MtrxArr(const T *p, std::vector<size_t> t)
{
  zero = static_cast<T>( 0 );
  valExcp = new ValueException<T> ;
  rep=0;
  rep = new MArep<T>(p, t );
  arr=rep->arr;

  // test and get exception values
  testValueException();
}

//! Assignment operator of const value to each array element.
template<typename T>
MtrxArr<T>&
MtrxArr<T>::operator=( const T *p)
{
  // Sizes of p and rep must be identical.
  // This cannot be checked.

  if( rep->counter == 1)  // reuse the current MArep obj
    rep->assign(p) ;
  else
  {
    rep->counter--;
    rep=new MArep<T>(p, rep->dim);
  }

  arr=rep->arr;

  // test and get exception values
  testValueException();

  return *this;
}

template<typename T>
MtrxArr<T>&
MtrxArr<T>::operator=( const MtrxArr<T> &g)
{
  if( this == &g )
    return *this;

  // update counter
  g.rep->counter++;

  if( (--rep->counter) == 0 )
    delete rep;  //delete the obsolete instance, if owner

  rep=g.rep;
  arr=rep->arr;

  // get exception values
//  if( initValExceptUpdate() )
//  {
    valExcp->copy( *g.valExcp );

    validRangeBegin=g.validRangeBegin;
    validRangeEnd=g.validRangeEnd;
//  }
//  else
//    testValueException();

  return *this;
}

template<typename T>
MtrxArr<T>&
MtrxArr<T>::operator=(const T v)
{
  // Note: no check for exception values
  if( v == zero )
    return *this;

  rep=rep->makeRoot();
  arr=rep->arr;

  for( size_t i=0 ; i < rep->arr_sz ; ++i)
     arr[i]=v;

  testValueException();

  validRangeBegin.clear();
  validRangeEnd.clear();

  validRangeBegin.push_back(0);
  validRangeEnd.push_back( rep->arr_sz );

  return *this;
}

// assign a MtrxArr obj of different type
template<typename T>
template<typename T_in>
MtrxArr<T>&
MtrxArr<T>::operator=( const MtrxArr<T_in> &g)
{
  if( reinterpret_cast<void*>(this)
       == reinterpret_cast<void*>(const_cast<MtrxArr<T_in>*> (&g)) )
    return *this;

  assign(g.arr, g.getDimensions() );

  if( valExcp && valExcp->update )
  {
    valExcp->import( *g.valExcp );

    validRangeBegin=g.validRangeBegin;
    validRangeEnd=g.validRangeEnd;
  }

  return *this;
}

//! const value to each element of array
template<typename T>
void
MtrxArr<T>::operator+(const T v)
{
  // Note: no check for exception values
  if( v == zero )
    return;

  rep=rep->makeRoot();
  arr=rep->arr;

  for( size_t i=0 ; i < rep->arr_sz ; ++i)
     arr[i] += v;

  // Note: testValueException(): nothing has changed

  return ;
}

//! const MtrxArr object is added
template<typename T>
void
MtrxArr<T>::operator+(MtrxArr<T> &g)
{
  if( size() != g.size() )
    status.push_back("MtrxArr objects of different size.");
  else
  {
    rep=rep->makeRoot();
    arr=rep->arr;

    for( size_t i=0 ; i < rep->arr_sz ; ++i)
       arr[i] += g.arr[i] ;

    if( ! g.isValid )
      testValueException();
  }

  return ;
}

template<typename T>
void
MtrxArr<T>::operator-(const T v)
{
  // Note: no check for exception values

  if( v == zero )
    return;

  rep=rep->makeRoot();
  arr=rep->arr;

  for( size_t i=0 ; i < rep->arr_sz ; ++i)
     arr[i] -= v ;

  // Note: testValueException(): nothing has changed

  return ;
}

//! const MtrxArr object is added
template<typename T>
void
MtrxArr<T>::operator-( MtrxArr<T> &g)
{
  // Note: no check for exception values

  if( size() != g.size() )
  {
    status.push_back("MtrxArr objects of different size.");

    return ;
  }

  rep=rep->makeRoot();
  arr=rep->arr;

  for( size_t i=0 ; i < rep->arr_sz ; ++i)
     arr[i] -= g.arr[i] ;

  if( ! g.isValid )
    testValueException();

  return ;
}

template<typename T>
void
MtrxArr<T>::operator*(const T v)
{
  // Note: no check for exception values

  if( v == static_cast<T>( 1 ) )
    return;

  rep=rep->makeRoot();
  arr=rep->arr;

  for( size_t i=0 ; i < rep->arr_sz ; ++i)
     arr[i] *= v ;

  // Note: testValueException(): nothing has changed

  return ;
}

//! const MtrxArr object is added
template<typename T>
void
MtrxArr<T>::operator*( MtrxArr<T> &g)
{
  if( size() != g.size() )
  {
    status.push_back("MtrxArr objects of different size.");

    return ;
  }

  rep=rep->makeRoot();
  arr=rep->arr;

  for( size_t i=0 ; i < rep->arr_sz ; ++i)
     arr[i] *= g.arr[i] ;

  if( ! g.isValid )
    testValueException();

  return ;
}

template<typename T>
void
MtrxArr<T>::operator/(const T v)
{
  // Note: no check for exception values

  // ignore division by zero
  if( v == static_cast<T>(1) )
    return;

  rep=rep->makeRoot();
  arr=rep->arr;

  for( size_t i=0 ; i < rep->arr_sz ; ++i)
     arr[i] /= v ;

  if( v == zero )
  {
    status.push_back("division by zero.");

    testValueException();
  }
  // else: nothing has changed

  return ;
}

//! const MtrxArr object is added
template<typename T>
void
MtrxArr<T>::operator/( MtrxArr<T> &g)
{
  if( size() != g.size() )
  {
    status.push_back("MtrxArr objects of different size.");

    return ;
  }

  rep=rep->makeRoot();
  arr=rep->arr;

  for( size_t i=0 ; i < rep->arr_sz ; ++i)
     arr[i] /= g.arr[i] ;

  if( ! g.isValid )
    testValueException();

  return ;
}

//! const MtrxArr object is added
template<typename T>
MtrxArr<T>&
MtrxArr<T>::operator+=( MtrxArr<T> &g)
{
  if( size() != g.size() )
  {
    status.push_back("MtrxArr objects of different size");

    return *this;
  }

  rep=rep->makeRoot();
  arr=rep->arr;
  size_t sz = size();

  bool isSync=true;

  if( validRangeBegin.size() == g.validRangeBegin.size() )
  {
     for(size_t i=0 ; i < validRangeBegin.size() ; ++i )
     {
         if( i != g.validRangeBegin[i] )
         {
             isSync = false;
             break;
         }
     }
  }
  else
    isSync = false;

  if(isSync)
  {
     for(size_t i=0 ; i < validRangeBegin.size() ; ++i )
        for(size_t j=validRangeBegin[i] ; j < validRangeEnd[i] ; ++j )
           arr[j] += g.rep->arr[j] ;

     return *this;
  }

  // different invalid values; solve my masks
  status.push_back("different patterns of exception values");

  T g_mask[sz];

  for( size_t i=0 ; i < sz ; ++i)
     g_mask[i] = zero ;

  for(size_t i=0 ; i < g.validRangeBegin.size() ; ++i )
     for(size_t j=g.validRangeBegin[i] ; j < g.validRangeEnd[i] ; ++j )
        g_mask[j] = 1 ;

  isSync = true;
  if( valExcp->exceptionValue.size() < g.valExcp->exceptionValue.size() )
  {
     for(size_t i=0 ; i < g.valExcp->exceptionValue.size() ; ++i )
     {
       size_t j;
       for(j=0 ; j < valExcp->exceptionValue.size() ; ++j )
         if( valExcp->exceptionValue[j] == g.valExcp->exceptionValue[i] )
             break;

       if( j == valExcp->exceptionValue.size() )
       {
          valExcp->exceptionValue.push_back( g.valExcp->exceptionValue[i] );
          valExcp->exceptionMode.push_back( g.valExcp->exceptionMode[i] );
          valExcp->enableValueExceptionTest("user");
          isSync=false;
       }
     }
  }

  if( !isSync)
     status.push_back("objects with different exception values");
  else
     isSync=true;

  for(size_t i=0 ; i < validRangeBegin.size() ; ++i )
  {
     for(size_t j=validRangeBegin[i] ; j < validRangeEnd[i] ; ++j )
     {
        if( g_mask[i] > g.zero )
          arr[i] += g.arr[i] ;
        else
        {
          arr[i] = g.arr[i] ;
          isSync=false;
        }
     }
  }

  if( ! isSync )
    testValueException();

  return *this;
}

template<typename T>
MtrxArr<T>&
MtrxArr<T>::operator+=(const T v)
{
  // Note: no check for exception values

  if( v == zero )
    return *this;

  rep=rep->makeRoot();
  arr=rep->arr;

  for(size_t i=0 ; i < validRangeBegin.size() ; ++i )
    for(size_t j=validRangeBegin[i] ; j < validRangeEnd[i] ; ++j )
        arr[j] += v ;

  // Note: nothing has changed as to testValueException()

  return *this;
}

//! const MtrxArr object is subtracted
template<typename T>
MtrxArr<T>&
MtrxArr<T>::operator-=( MtrxArr<T> &g)
{
  if( size() != g.size() )
  {
    status.push_back("MtrxArr objects of different size.");

    return *this;
  }

  rep=rep->makeRoot();
  arr=rep->arr;

  for( size_t i=0 ; i < rep->arr_sz ; ++i)
     arr[i] -= g.arr[i] ;

  if( ! g.isValid )
    testValueException();

  return *this;
}

template<typename T>
MtrxArr<T>&
MtrxArr<T>::operator-=(const T v)
{
  if( v == zero )
    return *this;

  rep=rep->makeRoot();
  arr=rep->arr;

  for(size_t i=0 ; i < validRangeBegin.size() ; ++i )
    for(size_t j=validRangeBegin[i] ; j < validRangeEnd[i] ; ++j )
        arr[j] -= v ;

  // Note: nothing has changed as to testValueException()

  return *this;
}

//! const MtrxArr object is multiplicated
template<typename T>
MtrxArr<T>&
MtrxArr<T>::operator*=( MtrxArr<T> &g)
{
  if( size() != g.size() )
  {
    status.push_back("MtrxArr objects of different size.");

    return *this;
  }

  rep=rep->makeRoot();
  arr=rep->arr;

  for( size_t i=0 ; i < rep->arr_sz ; ++i)
     arr[i] *= g.arr[i] ;

  if( ! g.isValid )
    testValueException();

  return *this;
}

template<typename T>
MtrxArr<T>&
MtrxArr<T>::operator*=(const T v)
{
  // Note: no check for exception values

  if( v == static_cast<T>( 1 ) )
    return *this;

  rep=rep->makeRoot();
  arr=rep->arr;

  for( size_t i=0 ; i < rep->arr_sz ; ++i)
     arr[i] *= v ;

  // Note: testValueException(): nothing has changed

  return *this;
}

//! const MtrxArr object is divided
template<typename T>
MtrxArr<T>&
MtrxArr<T>::operator/=( MtrxArr<T> &g)
{
  if( size() != g.size() )
  {
    status.push_back("MtrxArr objects of different size.");

    return *this;
  }

  rep=rep->makeRoot();
  arr=rep->arr;

  for( size_t i=0 ; i < rep->arr_sz ; ++i)
     arr[i] /= g.arr[i] ;

  if( ! g.isValid )
    testValueException();

  return *this;
}

template<typename T>
MtrxArr<T>&
MtrxArr<T>::operator/=(const T v)
{
  // Note: no check for exception values

  if( v == static_cast<T>( 1 ) )
    return *this;

  if( v == zero )
  {
    status.push_back("division by zero.");

    return *this;  // do nothing
  }

  rep=rep->makeRoot();
  arr=rep->arr;

  for(size_t i=0 ; i < validRangeBegin.size() ; ++i )
    for(size_t j=validRangeBegin[i] ; j < validRangeEnd[i] ; ++j )
        arr[j] /= v ;

  return *this;
}

//! Assignment of a MtrxArr obj for indirect indexing.
template<typename T>
MtrxArr<T>
MtrxArr<T>::operator[]( const MtrxArr<size_t> &in )
{
  MtrxArr<T> tma(in.size());

  for(size_t i=0 ; i<in.size() ; ++i)
    tma.arr[i]=arr[ in.arr[i] ];

  if( !isValid() )
    tma.testValueException();

  return tma;
}

//! Assignment of an array.
template<typename T>
template<typename T_in>
void
MtrxArr<T>::assign( const T_in *p, std::vector<size_t> d )
{
  if( rep->counter == 1)  //reuse the current MArep obj
    rep->assign(0, &d) ;
  else
  {
    rep->counter--;
    rep=new MArep<T>(0, d);
  }

  arr=rep->arr;

  for( size_t i=0 ; i < rep->arr_sz ; ++i )
    arr[i] = static_cast<T>(p[i]);

  return;
}

template<typename T>
void
MtrxArr<T>::assign( const T *p, std::vector<size_t> d )
{
  if( rep->counter == 1)  //reuse own array
    rep->assign(p, &d ) ;
  else
  {
    rep->counter--;
    rep=new MArep<T>(p, d );
  }

  arr=rep->arr;

  testValueException();

  return;
}

template<typename T>
void
MtrxArr<T>::assign( const T *p, size_t nx, size_t ny, size_t nz )
{
  std::vector<size_t> t;
  if( nx )
    t.push_back(nx);
  if( ny )
    t.push_back(ny);
  if( nz )
    t.push_back(nz);

  assign(p, t );
  return;
}

template<typename T>
template<typename T_in>
void
MtrxArr<T>::assign( const T_in *p, size_t nx, size_t ny, size_t nz )
{
  std::vector<size_t> t;
  if( nx )
    t.push_back(nx);
  if( ny )
    t.push_back(ny);
  if( nz )
    t.push_back(nz);

  assign(p, t);
  return;
}

template<typename T>
void
MtrxArr<T>::clear(void)
{
  if( (--rep->counter) == 0 )
    delete rep; // clears if no longer referenced

  if( static_cast<bool>(valExcp) )
    delete valExcp ;
}

template<typename T>
void
MtrxArr<T>::frame( const MtrxArr<T> &g, size_t beg, size_t sz)
{
  if( sz )
  {  // check available space
     if( rep->arr_sz != sz )
       ;
  }

  // note: validRangeEnd points to the last index+1
  size_t end=beg+sz;

  rep->assign(g.arr + beg) ;

  arr=rep->arr;

  validRangeBegin.clear();
  validRangeEnd.clear();

  size_t lim=g.validRangeBegin.size();
  size_t k=0;

  // find start
  for( ; k < lim ; k++ )
     if( beg < g.validRangeEnd[k] )
       break;

  for( ; k < lim ; k++ )
  {

     if( end <= g.validRangeBegin[k] )
       break;

     // somewhere in between
     if( beg <= g.validRangeBegin[k] && end > g.validRangeEnd[k])
     {
       validRangeBegin.push_back( g.validRangeBegin[k] - beg ) ;
       validRangeEnd.push_back( g.validRangeEnd[k] - beg ) ;
       continue;
     }

     // the first interval from the left
     if( beg >= g.validRangeBegin[k] && beg < g.validRangeEnd[k])
     {
       validRangeBegin.push_back( 0) ;
       if( end > g.validRangeEnd[k] )
         validRangeEnd.push_back( g.validRangeEnd[k] - beg ) ;
       else
         validRangeEnd.push_back( sz ) ;
       continue;
     }

     validRangeBegin.push_back( g.validRangeBegin[k] - beg ) ;
     validRangeEnd.push_back( sz ) ;
     break;
  }

  // determine exception value counts of the frame
  if( g.valExcp  )
  {
    // no exception value provided
    if( ! g.valExcp->exceptionValue.size() )
      return;

    lim=validRangeBegin.size();
    for( k=0 ; k < lim ; k++ )
      sz -= (validRangeEnd[k] - validRangeBegin[k]) ;

    if( sz )
    {
      if( ! valExcp->exceptionCount.size() )
      {
         if( g.valExcp->exceptionCount.size() )
            valExcp->copy( *g.valExcp, true );
      }

      valExcp->exceptionCount[0] = sz ;
    }
    else
      valExcp->exceptionCount[0] = 0 ;
  }

  return ;
}

template<typename T>
size_t
MtrxArr<T>::getExceptionCount(size_t i)
{
  // special: indicates the index of user exception counts

  if( valExcp->exceptionCount.size() <= i )
    return 0;

  return valExcp->exceptionCount[i] ;
}

template<typename T>
size_t
MtrxArr<T>::getExceptionCount(std::string s)
{
  if( valExcp->exceptionCount.size() == 0 )
    return 0;

  size_t count=0;

  if( s == "all" )
  {
    for( size_t i=0 ; i < valExcp->exceptionCount.size() ; ++i )
      count += valExcp->exceptionCount[i];

    count += valExcp->countInf;
    count += valExcp->countNaN;

    return count;
  }

  if( s == "user" )
  {
    for( size_t i=0 ; i < valExcp->exceptionCount.size() ; ++i )
      count += valExcp->exceptionCount[i];
    return count;
  }

  if( s == "inf" )
    return valExcp->countInf ;

  if( s == "nan" )
    return valExcp->countNaN ;

  return count;
}

template<typename T>
void
MtrxArr<T>::getExceptionValue(void* p, size_t i)
{
  if( i )
  {
    getExceptionValue(p, i);
    return;
  }

  for( i=0 ; i < valExcp->exceptionValue.size() ; ++i )
    getExceptionValue(p, i);

  return ;
}

template<typename T>
void
MtrxArr<T>::getExceptionValue(std::vector<T> &g, size_t i)
{
  g[i] = valExcp->exceptionValue[i];

  return ;
}

template<typename T>
std::vector<std::string>
MtrxArr<T>::getStatus(bool clear)
{
    if( ! clear)
        return status;

    std::vector<std::string> s(status);
    status.clear();
    return s;
}

template<typename T>
std::vector<size_t>
MtrxArr<T>::indices(size_t index)
{
  size_t sz = rep->dim.size() ;

  // initiate with a size appropriate to the dimensions
  std::vector<size_t> ixs(sz);

  size_t mx = 1 ;
  for( size_t i=0 ; i < rep->dim.size() ; ++i )
    mx *= rep->dim[i] ;

  if( sz == 0 || index >= mx )
  {
    ixs.clear();
    return ixs;  // no error handling
  }

  if( index < rep->dim[0] )
    ixs[0] = index ;
  else
  {
    for( size_t l=0 ; l < rep->dim.size() ; ++l )
    {
      ixs[l] = index / rep->dimProduct[l] ;
      index -= ixs[l] * rep->dimProduct[l] ;
    }
  }

  return ixs ;
}

template<typename T>
std::string
MtrxArr<T>::indicesStr(size_t index)
{
  std::string str("(");

  std::vector<size_t> ix( indices(index) );

  for( size_t i=0 ; i < ix.size() ; ++i )
  {
    if(i)
      str += ", " ;
    str += hdhC::itoa(ix[i]) ;
  }

  str += ")" ;

  return str;
}

template<typename T>
size_t
MtrxArr<T>::index(std::vector<size_t>& indices)
{
  if( indices.size() == 0 )
    return 0;

  size_t index=indices[0];
  for( size_t i=1 ; i < indices.size() ; ++i)
    index += indices[i] * rep->dimProduct[i] ;

  return index;
}

template<typename T>
bool
MtrxArr<T>::initValExceptUpdate(void)
{
  bool is=valExcp->update;
  valExcp->update=false;
  return is;
}

template<typename T>
bool
MtrxArr<T>::isValid(void)
{
  // Note: the entire data set is invalid if vRE[0] == 0.
  if( validRangeBegin.size() == 1 && validRangeEnd[0] )
    return true;
  else
    return false;
}

template<typename T>
bool
MtrxArr<T>::isValid(size_t i0)
{
  for( size_t j=0 ; j < validRangeBegin.size() ; ++j )
    if( i0 >= validRangeBegin[j] && i0 < validRangeEnd[j] )
      return true;

  return false;
}

template<typename T>
void
MtrxArr<T>::link( const T *p, bool keepShape)
{
  // *this with incomplete initialisation
  if( ! arr )
    keepShape=false;

  if( keepShape )
  {
    // Only the adress rep.arr is pointing to has changed.
    // So, the shape must fit.

    rep->assign(p) ;
  }
  else
  {
    // sizes of p and rep must be identical
    if( rep->counter == 1)  //reuse own array
      rep->assign(p,&rep->dim) ;
    else
    {
      rep->counter--;
      rep=new MArep<T>(p, rep->dim);
    }
  }

  arr=rep->arr;
  testValueException();

  return ;
}

template<typename T>
void
MtrxArr<T>::put(T val, size_t i0, size_t i1, size_t i2)
{
  // Put value at given index(es).
  // For internal and external array and matrix notation (2D, and 3-D).

  if( ! arr )  // no data
    return ;  // ToDo: error handling

  size_t ii = index(i0, i1, i2);

  bool is=false;

  // test previous value
  if( arr[ii] == std::numeric_limits<T>::infinity()
      || arr[ii] != arr[ii] )
    is = true;

  // now, set the value
  arr[ii]=val;

  if( is )
    testValueException();

  return;
}

template<typename T>
void
MtrxArr<T>::resize(size_t nx, size_t ny, size_t nz)
{
  std::vector<size_t> t;
  t.push_back(nx);
  t.push_back(ny);
  t.push_back(nz);

  resize(t);
  return;
}

template<typename T>
size_t
MtrxArr<T>::size(void) const
{
  if( !validRangeEnd.size() || !validRangeEnd[0] )
    return 0;

  return rep->arr_sz;
}

template<typename T>
void
MtrxArr<T>::resize(std::vector<size_t> &d)
{
  // resize
  rep->assign(0, &d );
  arr=rep->arr;

  return;
}

template<typename T>
void
MtrxArr<T>::setExceptionValue(std::vector<T> e, std::vector<char>* m,
    std::string* s)
{
  size_t sz = e.size();

  if( sz )
  {
    T* tmpArr = new T [sz] ;
    for( size_t i=0 ; i < sz ; ++i )
      tmpArr[i] = e[i];

    setExceptionValue(tmpArr, sz, m, s);
    delete [] tmpArr;
  }
  else
  {
    T* p=0;
    setExceptionValue(p, 0, m, s);
  }

  return;
}

#endif
