#ifndef HDH_C_H
#define HDH_C_H

#include <algorithm>
#include <cmath>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <typeinfo>
#include <vector>

#include <stddef.h>
#include <values.h>
#include <unistd.h>
#include <stdint.h>

#include "split.h"

/*! \file hdhC.h
 \brief Helper functions.
*/

//class Upper ;
//class Lower;
//! Namespace for a collection of functions.

/*! Note: many of these functions could be obsolete.\n
  Requires an instance of class Split.*/

namespace hdhC
{

//! Obsolete.
const double EQUALITY_TOLERANCE=1.e-12 ;

static std::string blank(" ");
static std::string colon(":");
static std::string empty("");
static std::string NA("N/A");
static std::string no_blank("no_blank");
static std::string no_colon("no_colon");
static std::string lower("lower");
static std::string s_void("void_bool");
static std::string upper("upper");

//! Epsilon tolerance in comparisons.
/*! Facilitates equality comparisons of floating numbers,
 e.g.: double x=1./10. ; if(fabs(x-0.1) < NUMERICAL_TOLERANCE)...*/
const double NUMERICAL_TOLERANCE=1.e-10 ;

//! Struct: simple statistics of a geographical area
struct FieldData
{
//! Is any valid average defined?
  bool   isValid;
  bool   isStndDevValid;

//! Minimum
  double      min;

//! Maximum
  double      max;

//! The number of grid-cells marked as missing|filling value.
  size_t fillingValueNum;
  size_t infCount;
  size_t nanCount;

//! Weighted average over the effective area.
/*! Cells marked as missing value are exclude from area.*/
  double areaWeightedAve;

//! Standard deviation (weighted)
  double stndDev;

//! Fraction of the effective area, i.e. excluding fillingValue grids.
  double areaFract;

//! Checksum of the data (inclusively filling values)
  uint32_t checksum;

//! Ensemble size
  size_t size;
};

//! Struct: simple statistics of a geographical area
struct FieldDataMeta
{
//! Is any valid average defined?
  bool   isValid;

//! Minimum
  double      min;

//! Maximum
  double      max;

//! The number of grid-cells marked as missing|filling value.
  size_t fillingValueNum;
  size_t infCount;
  size_t nanCount;

//! Ensemble size
  size_t size;
};

class FileSplit
{
   public:

   FileSplit(){;}
   FileSplit(const FileSplit& fs) {copy(fs);}

   FileSplit& operator=(const FileSplit&);

   void clear(void);
   void copy(const FileSplit&);

   std::string
        getBasename(void){ return basename ; }
   std::string
        getExtension(void){ return extension ; }
   std::string
        getFile(void);
   std::string
        getFilename(void);
   std::string
        getPath(void){ return path ; }

   bool exist(std::string f=empty);
   void setExtension(std::string);
   void setFile(std::string f );
   void setFile(struct FileSplit& );

   void setFilename(std::string);
   void setPath(std::string f){ path=f; }

   bool is(){ return getFile().size(); }
   std::string filename;
   std::string basename;
   std::string extension;
   std::string path;
};

//! Struct defining a point coordinate
struct Point
{
  double x;
  double y;
};

template<typename T>
T clearLeastBits( T v, size_t shft=2);

//! Compare x and y within uncertainty ranges e(psilon)
/*! A factor of 2 is substituted in epsilon
 modes: op: "==" --> fabs(x - y) < e \n
        op: "<"  --> x + e < y \n
        op: ">"  --> x > y + e
*/
/*
bool
compare(double x, std::string op, double y,
           double epsilon=NUMERICAL_TOLERANCE);
*/

/*! epsilon is 10^-DECS of the smaller value of the magnitude of x and y*/
bool
compare(double x, std::string op, double y, int DECS=10);

bool
compare(double x, std::string op, double y, double epsilon);

//! Convert time
/*! Time given in a certain unit is converted to a target unit.
 A sub-string (year, month, day, hour, minute, second) may be
 appended to the time string or be given as separate parameter.
 Note: A distinctive number of characters is sufficient.
 Note: If both appended units and such in the parameter list are given,
 then the appended one prevails.
 Note: This function does not work, if the conversion
 crosses the border between year, months on the on hand and
 the smaller units on the other.
 Use in these cases the class-method Date.addTime().*/
double
convertTime(std::string targetUnit, std::string time,
            std::string unit=empty);

//! Convert a floating number into a string.
/*!Format: ?[w=int?][p=int[|][adj]?][f=char?][s[ci]|float]?\n
   ?: separator (by default ','). The pre- and post-fix may
     be omitted for the default. If changed, then the prefix
     has to be formed by twice a character (not: w, p,f,s).
   w: width of the field,
   p: precision with 'adj' discarding trailing zeros and dec. point,
   f: filling character
   sci: scientific notation.
   float: floating notation.
   Note: assignment '=' may be omitted. By default sci is used for abs-values
   greater than 100000. and smaller than 0.000001.
*/
std::string
double2String( double z, std::string flag);

//! Convert a floating number into a string.
/*! Number 'z' is represented with precision of 'd' with rounding.
 Format of 'flag' is 'n_X with n leading positions filled by
 character X. The _-char may be omitted if X is not a number.
 Negative value of d indicates rounding with additional
 elimination of trailing zeros and a trailing decimal point.
*/
std::string
double2String( double z, int d=-5, std::string flag=empty);

//! Convert decimal degree to degree-minute-second.
/*! Decimal degrees are converted to traditional coordinates
 (degree, minute, seconds).*/
double
deci2dgr(double degree, double minute=0., double second=0. );

//! Convert degree-minute-second to decimal degrees.
/*! Traditional coordinates (degree, minute, seconds) are converted
 to decimal degree.*/
double
dgr2deci(std::string );

//! Obsolete.
void
envelope( std::vector<double> &w, std::vector<double> &fct,
          std::vector<double> &wE, std::vector<double> &E,
          bool upper=true );

//! Test of equality even of extreme small/large values
template<typename T>
bool
equal(T x1, T x2, double epsilon);

//! Checksum of a vector of data applying the Fletcher 32-bit algorithm.
/*! Reference: Fletcher32 algorithm \n
 http://en.wikipedia.org/wiki/Fletcher%27s_checksum
 Ref.: Fletcher, J. G., ?An Arithmetic Checksum for Serial
 Transmissions?, IEEE Trans. on Comm., Vol. COM-30, pp 247-252.\n
The start for a new sequence is indicated by boolean reset=true,
otherwise continued. String data ends with '\0'. If the size of the string is even, then '\0' is discarded, otherwise included. The boolean pointer is set to false after the first call.\n
Note: *_cmip5 versions do not calculate the Fletcher32
checksum as expected for data arrays that are
sub-divided and processed in sequential calls,
because the second reduction step is done at the
end of each call.
The versions are kept for CMIP5 application in order
to get backward compatibility.*/

uint32_t
fletcher32_cmip5( std::string &data, bool *reset=0);

//! Building the [accumulated] checksum from single doubles.
/*! Each double is converted to a string representation with default
 rounding of the function hdhC::double2String and then
 back-converted; exept isConvert is set false.*/
uint32_t
fletcher32_cmip5( double, bool *reset=0);

uint32_t
fletcher32_cmip5( double*, size_t sz=1, bool *reset=0);

uint32_t
fletcher32_cmip5( const char *data, size_t byte_sz=1, bool *reset=0 );

uint32_t
fletcher32( std::string &data, bool *reset=0);

template<typename T>
uint32_t
fletcher32( std::vector<T> &data, bool *reset=0, size_t clear=0);

template<typename T>
uint32_t
fletcher32( T, bool *reset=0, size_t shift=0);

template<typename T>
uint32_t
fletcher32( T*, size_t sz=1, bool *reset=0, size_t clear=0);

template<typename T>
uint32_t
fletcher32( const T*, size_t sz=1, bool *reset=0, size_t clear=0);

uint32_t
fletcher32_BE( uint8_t *data, size_t sz, bool *reset);

template<typename T>
uint32_t
fletcher32_LE( T *data, size_t sz, bool *reset);

template<typename T>
uint32_t
fletcher32_BE_clear( T *data, size_t sz, bool *reset, size_t clear=0);

template<typename T>
uint32_t
fletcher32_LE_clear( T *data, size_t sz, bool *reset, size_t clear=0);

//! Extract path, discard the filename.
std::string
getPath(std::string&, bool isWithSlash=false );

//! Extract filename, discard the path.
std::string
getFilename(std::string& );

//! Get filename without extension
std::string
getBasename(std::string& );

//! Get extension.
std::string
getExtension(std::string& );

//! get index
/*!  index of a corresponding vector v with size()==sz. If -sz < index < 0, then
     the reverse index is used. If index > -sz or index > sz, then index==0 */
int
getIndex(int index, int sz);

//! Get all path components
void
getPathComponents(std::string &str, std::string &path,
                  std::string &fName, std::string &base,
                  std::string &ext);

//! Invert the ordering of bits
/*! For any type of T*/
template <typename T>
T invertBits ( T );

//! Print bits of any type
template <typename T>
void printBits ( T val );

//! Find position of a value in a vector
/*! Return -1 if not found*/
int
findPos(std::string&, std::vector<std::string>&);

template <typename T>
int
findPos(T, std::vector<T>&);

//! return vector composed to a char-separated string
/*! A space follows 'sep', if isSpace=true*/
std::string
getVector2Str(std::vector<std::string>& vs, bool isSpace=false, char sep=',');

//! Get non-alpha characters.
/*! Return empty if no one was found.*/
std::string
getNonAlpha(std::string);

//! Get the non-alpha-numeric characters.
std::string
getNonAlphaNum(std::string);

//! convert vector of strings into a single string
/*! Remove space-separated duplicates*/
std::string
getUniqueString(std::string&, char sep=' ');

std::string
getUniqueString(std::vector<std::string>&, char sep=' ');

//! split a strings at sep into a vector of strings
std::vector<std::string>
getUniqueVector(std::string&, char sep=' ');

std::vector<std::string>
getUniqueVector(std::vector<std::string>&);

//! Is unsigned char a alphabetic?
bool
isAlpha(unsigned char);

//! Is string composed of alphas?
/*! If 'isContains' is true, then a single alpha is sufficient.*/
bool
isAlpha(std::string, bool isContains=false);

bool
isAlphaNum(unsigned char);

// "mode": exact, "beg"|"end": match from begin|end, "find": anywhere
bool
isAmong(std::string&, std::vector<std::vector<std::string> >& set,
        std::string mode="");

bool
isAmong(std::string&, std::vector<std::vector<std::string*> >& set,
        std::string mode="");

// size_t& outer, size_t inner fetch also the indeces of vector and position
// of a first match
bool
isAmong(std::string&, std::vector<std::vector<std::string> >& set,
        size_t& outer, size_t& inner,  std::string mode="");
bool
isAmong(std::string&, std::vector<std::vector<std::string*> >& set,
        size_t& outer, size_t& inner,  std::string mode="");

bool
isAmong(std::string&, std::vector<std::string>& set, std::string mode="");
bool
isAmong(std::string&, std::vector<std::string*>& set, std::string mode="");

// get also the index of a first match
bool
isAmong(std::string&, std::vector<std::string>& set, size_t&, std::string mode="");
bool
isAmong(std::string&, std::vector<std::string*>& set, size_t&, std::string mode="");

template <typename T>
bool
isAmong(T, std::vector<T>& set);

template <typename T>
bool
isAmong(T, std::vector<T>& set, size_t&);

template <typename T>
bool
isAmong(T, std::vector<T>& set, bool);  // bool for the compiler

template <typename T>
bool
isAmong(T, std::vector<T>& set, size_t&, bool);  // bool for the compiler

// all==true: the entire sub-set must be found in the set
template <typename T>
bool
isAmong(std::vector<T>& sub, std::vector<T>& set, bool all=false);

//! Is unsigned char a digit?
/*! If isNumber is true, then also sign and decimal point are accepted.*/
bool
isDigit(unsigned char, bool isNumber=false);

//! Is string composed of digits?
/*! If 'isContains' is true, then a single digit is sufficient.*/
bool
isDigit(std::string, bool isContains=false);

//! Is string a number?
/*! The string may be e- or E-format*/
bool
isNumber( std::string & );

//! Is string only of white-spaces
bool
isWhiteString( std::string &str );

//double
//interpol(int ma, std::vector<double> &xV, std::vector<double> &yV, double x);

/*
void
interpolArray2Array( int idxa, int idxn , int idya, int idyn,
                     double spv, double **fa, double **fn);
*/

//! Itemise a string,  e.g. "a: b c d: e f"  --> "a: b c", "d: e f"
/*! pos indicates the position of key. empty --> key must macth,
    first --> at the beginning, last --> at the end */
std::vector<std::string>
itemise(std::string&, std::string key, std::string pos=empty);

//! Convert int to string (based on K&R).
std::vector<std::string>
itoa(std::vector<int>& );

std::string
itoa(int n );

//! Convert int to string (K&R).
/*! For very long numbers, e.g. the year 2100 expressed in
 seconds since 1956.*/
std::string
itoa(unsigned long long int n, int sign);

//! Approximate the vertex of a curve by a parabola.
Point
parabolaSummit(double xm, double x, double xp, double ym,
               double y, double yp);

//! Planck function.
double
planck(double anue,double t);

double
rounding(double, int dNum=0);

//! Convert string to double (K&R).
/*! Extract a number from string. Multiple numbers in a string
 separated by any char that is not part of a number (i.e. not
 e, E, -,+) may be skipped ny parameter 'nr' counting from 1*/
bool
string2Double( std::string s, size_t nr, double* retVal) ;

//! Convert string to double
double
string2Double( std::string s, size_t nr=1,bool isRetNull=false) ;

//! Convert string to double
double
string2Double( std::string s, size_t nr, int &width,
               int &precision,  bool isRetNull=false) ;

//! Convert string to double
double
string2Double( std::string s, size_t nr ,int *restIndex,
               bool isRetNull=false) ;

std::string
tf_assign(std::string lvalue, std::string rvalue) ;

std::string
tf_assign(std::string rvalue) ;

std::string
tf_att(std::string v=s_void, std::string a=s_void, std::string val=s_void,
        std::string b1=s_void, std::string b2=s_void, std::string b3=s_void);

std::string
tf_att(std::vector<std::string*>& vs,  bool colon, bool blank, bool isUpper);

std::string
tf_range(std::string v0, std::string v1, std::string blnk=empty);

std::string
tf_val(std::string v, std::string blnk=empty);

std::string
tf_var(std::string v,
        std::string b1=s_void, std::string b2=s_void, std::string b3=s_void);

std::string
tf_var(std::string& v,  bool colon, bool blank, bool isUpper);

//! concatenate a string-vector to a comma-separated (with blanks) string
std::string
catStringVector(std::vector<std::string>&);

//! Remove all characters from a str.
/*! By default, each character in 's' is removed. If isStr==true, then only
    every occurrence of the string 's'.*/
std::string
clearChars(std::string str, std::string s, bool isStr=false );

std::string
clearEnclosures(std::string& str, char a='(', char b=')' );

static bool clearEnclosures_unpaired(false);

//! remove white spaces and newlines
std::string
clearSpaces(std::string str);

//! Replace all characters from a str.
/*! By default, each character of 's' is replaced in str by char r; default blank.
    If isStr==true, then only every occurrence of the string 's'.*/
std::string
replaceChars(std::string str, char, char r=' ');

std::string
replaceChars(std::string str, std::string s, char r=' ', bool isStr=false );

//! strip off surrounding characters.
/*! Surrounding strings 'strip are removed; default: ' ' and '\t'.
    Additionally, keyword 'left' and 'right' may be given
    causing stripping only from the left or right, respectively. */
//! enhanced function: strip (vector of) string(s); default whitespaces
std::string
stripSides(std::string, std::vector<std::string>& strip, std::string key=empty );

//! enhanced function: strip (vector of) string(s); default whitespaces
std::string
stripSides(std::string, std::string strip=empty, std::string key=empty );

//! replace multiple characters by a single one
/*! if char is specified, then only for this specific one.*/
std::string
unique(std::string str, char);

std::vector<std::string>
unique(std::vector<std::string> &vs, std::string=empty, bool isStr=false);

std::string
unique(std::string str, std::string=empty, bool isStr=false);

//! concatenate strings; num==0 means full size of the vector
std::string
unsplit(std::vector<std::string>&, std::string sep=empty, size_t num=0);

std::string
unsplit(Split&, std::string sep=empty, size_t num=0);

//! Convert char to upper case
//! Convert char to lower case
void
lowerCase(char &c);

void
upperCase(char &c);

//! Convert string to upper case
class Upper
{
  public:
  std::string operator()(std::string s, bool self=false)
  {
    if( self )
    {
      std::for_each(s.begin(), s.end(), hdhC::upperCase);
      return s;
    }
    else
    {
      std::string t(s) ;
      std::for_each(t.begin(), t.end(), hdhC::upperCase);
      return t;
    }
  }
};

//! Convert string to lower case
class Lower
{
  public:
  std::string operator()(std::string s, bool self=false)
  {
    if( self )
    {
      std::for_each(s.begin(), s.end(), hdhC::lowerCase);
      return s;
    }
    else
    {
      std::string t(s) ;
      std::for_each(t.begin(), t.end(), hdhC::lowerCase);
      return t;
    }
  }
};

} // end of namespace


#endif
