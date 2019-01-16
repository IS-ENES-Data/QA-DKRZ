// // epsiolon#include "hdhC.h"

namespace hdhC
{

FileSplit&
FileSplit::operator=(const FileSplit& fs)
{
  if( this != &fs )
    copy(fs);

  return *this;
}

void
FileSplit::clear()
{
   filename.clear();
   basename.clear();
   extension.clear();
   path.clear();
   return;
}

void
FileSplit::copy(const FileSplit &fs)
{
  filename = fs.filename;
  basename = fs.basename;
  extension = fs.extension;
  path = fs.path;
  return;
}
std::string
FileSplit::getFile(void)
{
  std::string f;

  if( path.size() )
    f = path + "/";

  f += basename;

  if( extension.size() )
    f += "." + extension;

  return f;
}

std::string
FileSplit::getFilename(void)
{
  if( extension.size() )
    return basename + "." + extension ;
  else
    return basename;
}

bool
FileSplit::exist(std::string f)
{
  std::string testFile("/bin/bash -c \'test -e ") ;

  if( f.size() )
    testFile += f ;
  else
    testFile += getFile() ;

  testFile += '\'' ;

  // see 'man system' for the return value, here we expect 0,
  // if directory exists.

  if( system( testFile.c_str()) )
    return false;

  return true;
}

void
FileSplit::setExtension(std::string f)
{
  extension=f;
  filename = basename + "." + extension ;
  return ;
}

// ----------------------------------------------------

void
FileSplit::setFile(hdhC::FileSplit& f)
{
  filename =f.filename;
  basename =f.basename;
  extension=f.extension;
  path     =f.path;
  return;
}

void
FileSplit::setFile(std::string f)
{
   // A file is split into path, filename, basename, and extension.
   // Successive calls with different components are merged where
   // a path must have a trailing / and an extension a leading '.' .

   if( f.size() == 0 )
     return;

   size_t pos ;

   // remove multiple /
   unique(f, '/') ;

   // path
   if( (pos = f.rfind('/') ) < std::string::npos )
   {
     if( pos == 0 )
     {
       path = "/";
       if( ++pos == f.size() )
         return; // only a path

       filename = f;
     }
     else
     {
       path = f.substr( 0, pos++ ) ;
       if( pos == f.size() )
         return; // only a path

       filename = f.substr(pos);
     }
   }
   else
     filename = f;

   // file components
   if( (pos = filename.rfind('.') ) < std::string::npos )
   {
     if( pos == 0 )
       extension = filename;  // no basename
     else
     {
       basename = filename.substr( 0, pos++ ) ;

       if( pos == filename.size() )
         return; // only a basename

       extension  = filename.substr(pos);
     }
   }
   else
     basename = filename ;

   return ;
}

void
FileSplit::setFilename(std::string f)
{
  size_t pos;

  if( (pos=f.rfind('/')) < std::string::npos )
    f = f.substr(++pos);  // remove a leading path

  if( (pos=f.rfind('.')) < std::string::npos )
  {
    basename = f.substr(0,pos++);
    extension = f.substr(pos);
  }
  else
    basename = f;

  filename=f;

  return;
}


template<typename T>
T
clearLeastBits( T v, size_t shft)
{
  switch( sizeof( T ) )
  {
    case 8:
    {
      void *p = &v;
      uint64_t *r=reinterpret_cast<uint64_t*>(p);
      *r >>= shft;
      *r <<= shft;
    }
    break;
    case 4:
    {
      void *p = &v;
      uint32_t *r=reinterpret_cast<uint32_t*>(p);
      *r >>= shft;
      *r <<= shft;
    }
    break;
    case 2:
    {
      void *p = &v;
      uint16_t *r=reinterpret_cast<uint16_t*>(p);
      *r >>= shft;
      *r <<= shft;
    }
    break;
    case 1:
    {
      void *p = &v;
      uint8_t *r=reinterpret_cast<uint8_t*>(p);
      *r >>= shft;
      *r <<= shft;
    }
    break;
  }

  return v;
}

/*
bool compare(double x, char op, double y, double epsilon)
{
  // compare x and y within uncertainty ranges e
  // modes: op: '=' --> x == y
  //        op: '<'  --> x < y
  //        op: '>'  --> x > y

  if( op == '=' )
  {
    if( x == y )
       return true;

    if( fabs(x - y) < epsilon )
      return true;
    else
      return false;
  }

  if( op == '<' )
  {
    if( (x + epsilon) < y )
      return true;
    else
      return false;
  }

  if( op == '>' )
  {
    if( x > (y + epsilon) )
      return true;
    else
      return false;
  }

  return false;
}
*/

bool
compare(double x, std::string op, double y, int decimals)
{
  double epsilon=1;
  for( int i=0 ; i < decimals ; ++i )
     epsilon /= 10.;
  epsilon *= 2. ;

  return compare(x, op, y, epsilon);
}

bool
compare(double x, std::string op, double y, double epsilon)
{
// compare x and y within uncertainty ranges e
  // A factor of 2 is substituted in epsilon
  // modes: op: "=" --> x == y
  //        op: "<"  --> x < y
  //        op: ">"  --> x > y
  //        op: ">=" and "<=", too

  op += ' ';

  if( op[1] == '=' )
  {
      char swp = op[0];
      op[0]=op[1];
      op[1]=swp;
  }

  bool is;

  if( op[0] == '=' )
  {
    double xy= fabs(x - y);

    is = xy < epsilon ? true : false;
    if(is)
        return is;
  }

  is = x > y ? true : false ;

  if( op[1] == '<' )
    is = !is ;

  return is;
}

double convertTime(std::string targetUnit, std::string time,
            std::string unit)
{
  // Conversion between different time formats.
  // Note: time-unit appended at string 'time' overrules unit.
  // Time-units: y,mo,d,h,mi,s .
  // Note: This function does not work, if the conversion
  // crosses the border between year and months on the one hand
  // and days or smaller units on the other. In such a case, please,
  // use combination of class Date.addTime() and Date.get..., where
  // ... denotes the target unit.

  // If there is a mix of digits and non-digits, size is >= 2:
  // If !isDigit && size == 1, then there is an error.
  // If size == 0, error
  // But, for isDigit && size == 1 we got the default

  // It is save to append units (even if empty) to time
  // Note: time-unit overrules parameter unit.
  time += unit;

  Split splt;
  splt.setSeparator(":alnum:");
  splt = time;

  bool isNon = ! isDigit(time);
  if( (splt.size() == 1 && isNon) || (splt.size() == 0) )
  {
     std::cerr << "convertTime(): error in 2nd parameter (time="
               << time << ")" << std::endl;
     exit(1);
  }

  char c ;
  std::string sc;

  if( isNon )
  {
    c = tolower(splt[1][0]) ;
    sc = Lower()(splt[1]);

    if( c == 'y' )
       unit="y";
    else if( sc.substr(0,2) == "mo" )
       unit="mo";
    else if( c == 'd' )
       unit="d";
    else if( c == 'h' )
       unit="h";
    else if( sc.substr(0,2) == "mi" )
       unit="mi";
    else if( c == 's' )
       unit="s";
  }
  else if( unit.size() == 0 )
    return splt.toDouble(0) ;

  c = tolower(targetUnit[0]) ;
  sc = Lower()(targetUnit);

  if( unit[0] == 'y' )
  {
    if( c == 'y')
      return splt.toDouble(0);
    else if( sc.substr(0,2) == "mo" )
      return 12. * splt.toDouble(0);

    return 0. ;
  }

  if( unit.substr(0,2) == "mo" )
  {
    if( c == 'y')
      return splt.toDouble(0) / 12.;
    else if( sc.substr(0,2) == "mo" )
      return splt.toDouble(0);

    return 0. ;
  }

  if( unit[0] == 'd' )
  {
    if( c == 'd')
      return  splt.toDouble(0);
    else if( c == 'h')
      return  24. * splt.toDouble(0);
    else if( sc.substr(0,2) == "mi")
      return  1440. * splt.toDouble(0);
    else if( c == 's')
      return  86400. * splt.toDouble(0);

    return 0. ;
  }

  if( unit[0] == 'h' )
  {
    if( c == 'd')
      return  splt.toDouble(0) / 24. ;
    else if( c == 'h')
      return  splt.toDouble(0);
    else if( sc.substr(0,2) == "mi")
      return  60. * splt.toDouble(0);
    else if( c == 's')
      return  3600. * splt.toDouble(0);

    return 0. ;
  }

  if( unit.substr(0,2) == "mi" )
  {
    if( c == 'd')
      return  splt.toDouble(0) / 1440. ;
    else if( c == 'h')
      return  splt.toDouble(0) / 60.;
    else if( sc.substr(0,2) == "mi")
      return  splt.toDouble(0);
    else if( c == 's')
      return  60. * splt.toDouble(0);

    return 0. ;
  }

  if( unit[0] == 's' )
  {
    if( c == 'd')
      return  splt.toDouble(0) / 86400. ;
    else if( c == 'h')
      return  splt.toDouble(0) / 3600.;
    else if( sc.substr(0,2) == "mi")
      return  splt.toDouble(0) / 60. ;
    else if( c == 's')
      return  splt.toDouble(0);
  }

  // crosses border
  return 0. ;
}

double
deci2dgr(double grad, double min, double sec)
{
  double sign=1.;

  if( grad <0. )
    sign = -1. ;
  else if( grad == 0. && min  < 0. )
    sign = -1. ;
  else if( grad == 0. && min == 0. && sec < 0. )
    sign = -1. ;

  return sign * (fabs(grad) + fabs(min)/60. + fabs(sec)/3600.) ;
}

double
dgr2deci(std::string s)
{
  double sign=1.;
  double konvFakt=0.;
  char c='\0';
  double grad=0.;
  double min=0.;
  double sec=0.;

  if( s.empty() )
  {
    std::cerr << "dgr2deci: string is empty" << std::endl ;
    exit(1);
  }

  for( size_t i=0 ; i < s.size() ; ++i )
  {
    /*Konvertion: Stunde in Grad: suche h,H bzw.  g,G,n,N,s,S,O,o,e,E,w,W */
    c = s[i] ;

    if( c == 'h' || c == 'H' )
    {
      konvFakt = 360./24. ;
      break;
    }
    else if( c == 'g' || c == 'G' || c == 'o' || c == 'O'
               || c == 'e' || c == 'E' || c == 'n' || c == 'N'  )
    {
      konvFakt = 1. ;
      break;
    }
    else if( c == 'w' || c == 'W' || c == 's' || c == 'S' )
    {
      konvFakt = -1. ;
      break;
    }
    else if( i == ( s.size() - 1 ) )
      return string2Double(s,1);
  }

  grad  = string2Double(s,1);
  if ( ! string2Double(s,2, &min) )
    min  = 0. ;
  if ( ! string2Double(s,3, &sec) )
    sec  = 0. ;

  if( grad < 0. )
    sign = -1. ;
  else if( grad == 0. && min  < 0. )
    sign = -1. ;
  else if( grad == 0. && min == 0. && sec < 0. )
    sign = -1. ;

  return sign * (fabs(grad) + fabs(min)/60. + fabs(sec)/3600.)*konvFakt ;
}

// ----------------------------------------------------

std::string
double2String( double z, int d, std::string flag)
{
  // Note: for downward compatibility.


  /* a floating number is converted to a string. The number of
     decimals is prescribed by 'd'. Rounding takes place.
     Format: 'flag'="n_X" with width 'n'
     and the char 'X' used to fill left-side.
     If '_' is omitted, then the width is filled with blanks.
     Default no 'lead': no left-side filling.
     Negative value of d indicate rounding with additional
     elimination of trailing zeros and a trailing decimal point.*/

  std::string str;
  size_t pos;

  if( isDigit(flag) )
  {
    str  = "w=";
    str += flag;
  }
  else if( (pos=flag.find('_')) < std::string::npos )
  {
    str  = "w=";
    str += flag.substr(0,pos) ;
    str += ",f=";
    str += flag[flag.size()-1];
  }
  else if( flag.size() )
    str = flag;

  if( str.size() )
    str += ",";

  str  += "p=";
  if( d < 0 )
  {
    str += itoa( -1*d );
    str += "|adjusted";
  }
  else
    str += itoa(d);

  return double2String(z, str);

}

std::string double2String( double z, std::string flag)
{
  /* a floating number is converted to a string.
     Format: [??][w=int?][p=int[|adj]?][f=int?][s[ci]|float]

     Separator ?: by default ',', The pre- and post-fix may
     be omitted for the default. If changed, then the prefix
     has to be formed by twice a character (not: w, p,f,s).
     w: width of the field, p: precision, f: filling character,
     sci: scientific notation.
     If the value of precision contains substring '|adj', then
     trailing zeros and decimal point are discarded..
  */

  bool isSci=false;
  bool isFloat=false;
  bool isAdjusted=false;
  std::string c;
  int width = -1;
  int precision = -5;

  Split splt;

  if( flag.size() > 2 && ! isDigit(flag) && flag[0] == flag[1] )
  {
    splt.setSeparator(flag[0]);
    flag=flag.substr(2,flag.size()-2);
  }
  else
    splt.setSeparator(',');

  splt = flag;
  for(size_t i=0 ; i < splt.size() ; ++i )
  {
     if( splt[i][0] == 's' )
     {
       isSci=true;
       continue;
     }

     if( splt[i] == "float" )
     {
       isFloat=true;
       continue;
     }

     if( splt[i][0] == 'w' )
     {
       width = static_cast<int>( hdhC::string2Double(splt[i]) ) ;
       continue;
     }
     if( splt[i][0] == 'f' )
     {
       c = splt[i][splt[i].size()-1];
       continue;
     }
     if( splt[i][0] == 'p' )
     {
       size_t pos=1;
       if( splt[i][1] == '=' )
         ++pos;

       std::string t0(splt[i].substr(pos));

       Split splt_p(t0,"|");
       for( size_t i=0 ; i < splt_p.size() ; ++i )
       {
         if( splt_p[i].substr(0,3) == "adj" )
           isAdjusted=true;
         else
           precision = static_cast<int>( hdhC::string2Double(splt_p[i]) ) ;
       }
       continue;
     }
  }

  std::ostringstream ostr(std::ios::app);

  double z_abs(fabs(z));

  if( !isFloat && (isSci || z_abs > 100000.
                   || (z_abs > 0.0 && z_abs < 0.000001)) )
  {
    ostr.setf(std::ios::scientific, std::ios::floatfield);
    isSci=true;
  }
  else
    ostr.setf(std::ios::fixed, std::ios::floatfield);

  if( flag.size() )
  {
    ostr.setf(std::ios::adjustfield);
    if( c.size() )
      ostr.fill(c[0]);

    if( width > -1 )
    {
      if( precision )
        ostr << std::setw(width+precision+1);
      else
        ostr << std::setw(width);
    }
  }

  ostr.precision( precision );

  ostr << z ;

  if( isAdjusted )
  {
    std::string str( ostr.str() );
    std::string sci_rep;

    if( isSci )
    {
       size_t pos;
       if( (pos=str.find('e')) < std::string::npos )
          ;
       else if( (pos=str.find('E')) < std::string::npos )
         ;

       if( pos < std::string::npos )
       {
          sci_rep = str.substr(pos);
          str = str.substr(0, pos);
       }
    }

    if( str.find('.') < std::string::npos )
    {
      int pos=str.size()-1;
      for(  ; pos > -1 ; --pos)
        if( str[pos] != '0' )
          break ;

      if( str[pos] == '.' )
        --pos;

      str = str.substr(0, ++pos);
    }

    str += sci_rep;

    return str;
  }

  return ostr.str();
}

// ----------------------------------------------------

void envelope( std::vector<double> &x, std::vector<double> &y,
        std::vector<double> &wEnv, std::vector<double> &env, bool upper )
{
  int index;
  bool isTst;

  env.clear();
  wEnv.clear();

  if( upper )  // upper envelope of the curve
    index = 1;
  else
    index = 0;

  for( size_t i=1 ; i<x.size()-1 ; i++)
  {
    if( index == 0 )  //Minimum
      isTst = ( y[i-1] > y[i] && y[i+1] > y[i]  ) ;
    else              //Maximum
      isTst = ( y[i-1] < y[i] && y[i+1] < y[i]  ) ;

    if( isTst )
    {
      // Extremum was found
      Point scheitel = parabolaSummit(x[i-1], x[i], x[i+1], y[i-1],
                       y[i], y[i+1]);

      env.push_back(scheitel.y);
      wEnv.push_back(scheitel.x);
    }
  }

  return ;
}

// ----------------------------------------------------

template<typename T>
bool equal(T x1, T x2, double epsilon)
{
  // Is (x1-x2) == 0 under the constraint of computer inaccuracies?
  // Get the magnitude and compare values to an adjusted epsilon.

  // return: true if fabs(x1-x2) < magnitude*epsilon
  //         where magnitude is the one of the larger absolute value

  // simple equality
  if( x1 == x2 )
    return true;

  // Note: it is assumed the typenames are restricted to
  // a kind of float

  // test for the same sign
  if( x1 > 0. )
  {
    if( x2 < 0. )
      return false;
  }
  else
  {
    if( x2 > 0. )
      return false;
  }

  x1=fabs(x1);
  x2=fabs(x2);

  // get the magnitude of the larger value
  double x = (x1 < x2) ? x2 : x1 ;
  double scale;

  if( x > 1. )
  {
    // find the magnitude; large value case
    scale=0.1;
    do
    {
       scale *= 10.;  // increase
       x /= 10.;      // decrease
    } while( x > 1. );
  }
  else
  {
    // find the magnitude; small value case
    scale=10.;
    do
    {
       scale /= 10.;  // increase
       x *= 10.;      // decrease
    } while( x < 1. );
  }

  if( fabs( x1-x2 ) < (scale*epsilon ) )
    return true;  //
  else
    return false;
}

//! Find position of a value in a vector
int
findPos(std::string& item, std::vector<std::string>& set)
{
  for(size_t i=0 ; i < set.size() ; ++i )
    if( item == set[i] )
      return static_cast<int>(i) ;

  return -1;
}

template <typename T>
int
findPos(T item, std::vector<T>& set)
{
  for(size_t i=0 ; i < set.size() ; ++i )
    if( item == set[i] )
      return static_cast<int>(i) ;

  return -1;
}

// ----------------------------------------------------
// Note: *_cmip5 versions do not calculate the Fletcher32
// checksum as expected for data arrays that are
// sub-divided and processed in sequential calls,
// because the second reduction step is done at the
// end of each call.
// The versions are kept for CMIP5 application in order
// to get backward compatibility.

uint32_t fletcher32_cmip5( std::string &str, bool *reset)
{
   return fletcher32_cmip5(str.c_str(), str.size(), reset);
}

uint32_t fletcher32_cmip5( double data, bool *reset)
{
//   std::string str = double2String(data) ;
#if __cplusplus > 201100L
   std::string str = std::to_string(data) ;
#else
   std::string str = double2String(data) ;
#endif

   return fletcher32_cmip5( str.c_str(), str.size(), reset) ;
}

uint32_t fletcher32_cmip5( double *data, size_t sz, bool *reset)
{
   std::string str ;

#if __cplusplus > 201100L
   for( size_t i=0 ; i < sz ; ++i )
     str += std::to_string(data[i]) ;
# else
   for( size_t i=0 ; i < sz ; ++i )
     str += double2String(data[i]) ;
#endif

   return fletcher32_cmip5( str.c_str(), str.size(), reset) ;
}

uint32_t fletcher32_cmip5( const char *data, size_t byte_sz, bool *reset )
{
  bool thisReset=true;
  if( reset )
    thisReset = *reset;

  // adapted from:
  // http://en.wikipedia.org/wiki/Fletcher%27s_checksum
  // Ref.: Fletcher, J. G., An Arithmetic Checksum for Serial
  // Transmissions, IEEE Trans. on Comm., Vol. COM-30, pp 247-252.

   // Note: this version does not calculate the Fletcher32
   // checksum as expected for data arrays that are
   // sub-divided and processed in sequential calls,
   // because the second reduction step is done at the
   // end of each call.
   // The version is kept for CMIP5 application in order
   // to get backward compatibility.

   // Length in 16-bit words; even
   size_t len = byte_sz / 2;

   static uint32_t sum1 = 0xffff;
   static uint32_t sum2 = 0xffff;

   if( thisReset )
   {
     sum1 = 0xffff;
     sum2 = 0xffff;

     // the boolean is set to false after the first call. It
     // keeps that value until changed outside
     thisReset = false;
   }

   while (len)
   {
     size_t tlen = len > 360 ? 360 : len;
     len -= tlen;

     do
     {
        // invariant against big or little endianess
        sum1 += static_cast<uint32_t>(
          (static_cast<uint16_t>(data[0]) << 8)
            | static_cast<uint16_t>(data[1])  );

        data += 2; //shift data by two bytes
        sum2 += sum1;
     } while (--tlen);

     sum1 = (sum1 & 0xffff) + (sum1 >> 16);
     sum2 = (sum2 & 0xffff) + (sum2 >> 16);
   }

   // last (odd) byte
   if(byte_sz % 2)
   {
     sum1 += (uint32_t)(((uint16_t)*data) << 8);
     sum2 += sum1;
     sum1 = (sum1 & 0xffff) + (sum1 >> 16);
     sum2 = (sum2 & 0xffff) + (sum2 >> 16);
   }

   // Second reduction step to reduce sums to 16 bits
   sum1 = (sum1 & 0xffff) + (sum1 >> 16);
   sum2 = (sum2 & 0xffff) + (sum2 >> 16);

   if( reset )
     *reset = thisReset ;


   return sum2 << 16 | sum1;
}

// ----------------------------------------------------

uint32_t fletcher32( std::string &str, bool *reset)
{
  size_t sz = str.size();
  return fletcher32(str.c_str(), sz, reset);
}

template<typename T>
uint32_t fletcher32( std::vector<T> &v, bool *reset, size_t nShift)
{
  uint32_t cs;
  for( size_t i=0 ; i < v.size() ; ++i )
    cs = fletcher32(v[i], reset, nShift);
  return cs ;
}

template<typename T>
uint32_t fletcher32( T data, bool *reset, size_t nShift)
{
  return fletcher32( &data, 1, reset, nShift) ;
}

template<typename T>
uint32_t fletcher32( T *data, size_t sz, bool *reset, size_t nShift)
{
  int i = 1;
  char *p = (char *)&i;

  size_t nBytes=sizeof(T);

  if( nShift )
  {
    if(nBytes > 1 && p[0] == 1)
    {
      // little endian
      return fletcher32_LE_clear( data, sz, reset, nShift) ;
    }
    else
    {
      // big endian
      uint8_t *c=reinterpret_cast<uint8_t*>(data);
      return fletcher32_BE_clear( c, sz*nBytes, reset, nShift) ;
    }
  }
  else
  {
    if(nBytes > 1 && p[0] == 1)
    {
      // little endian
      return fletcher32_LE( data, sz, reset) ;
    }
    else
    {
      // big endian
      uint8_t *c=reinterpret_cast<uint8_t*>(data);
      return fletcher32_BE(c, sz*nBytes, reset) ;
    }
  }
}

template<typename T>
uint32_t fletcher32( const T *data, size_t sz, bool *reset, size_t nShift)
{
  T *c;
  c=const_cast<T*>(data);

  int i = 1;
  char *p = (char *)&i;

  size_t nBytes=sizeof(T);

  if( nShift )
  {
    if( nBytes > 1 && p[0] == 1)
    {
      // little endian
      return fletcher32_LE_clear( c, sz, reset, nShift) ;
    }
    else
    {
      // big endian
      uint8_t *d=reinterpret_cast<uint8_t*>(c);
      return fletcher32_BE_clear( d, sz*nBytes, reset, nShift) ;
    }
  }
  else
  {
    if( nBytes > 1 && p[0] == 1)
    {
      // little endian
      return fletcher32_LE( c, sz, reset) ;
    }
    else
    {
      // big endian
      uint8_t *d=reinterpret_cast<uint8_t*>(c);
      return fletcher32_BE( d, sz*nBytes, reset) ;
    }
  }

}

uint32_t fletcher32_BE( uint8_t *data, size_t len, bool *reset)
{
  bool thisReset=true;
  if( reset )
    thisReset = *reset;

  // adapted from:
  // http://en.wikipedia.org/wiki/Fletcher%27s_checksum
  // Ref.: Fletcher, J. G., An Arithmetic Checksum for Serial
  // Transmissions, IEEE Trans. on Comm., Vol. COM-30, pp 247-252.

  // first call: reset==true
  // following calls: reset==false from this function
  // last call: reset==true externally; finalisation

  // len: the number of bytes

   static uint32_t sum1     = 0xffff;
   static uint32_t sum2     = 0xffff;

   if( thisReset )
   {
     sum1 = 0xffff;
     sum2 = 0xffff;

     // the boolean is set to false after the first call. It
     // keeps that value until changed outside
     thisReset = false;
   }

   while (len)
   {
     size_t llen = len > 5056 ? 5056 : len;
     len -= llen;

     do
     {
        sum1 += *data++;
        sum2 += sum1;
     } while (--llen);

     sum1 = (sum1 & 0xffff) + (sum1 >> 16);
     sum2 = (sum2 & 0xffff) + (sum2 >> 16);
   }

   // Second reduction step to reduce sums to 32 bits
   sum1 = (sum1 & 0xffff) + (sum1 >> 16);
   sum2 = (sum2 & 0xffff) + (sum2 >> 16);

  if( reset )
    *reset = thisReset;

   return sum2 << 16 | sum1;
}

template<typename T>
uint32_t fletcher32_LE( T *data, size_t len, bool *reset )
{
  bool thisReset=true;
  if( reset )
    thisReset = *reset;

  // adapted from:
  // http://en.wikipedia.org/wiki/Fletcher%27s_checksum
  // Ref.: Fletcher, J. G., An Arithmetic Checksum for Serial
  // Transmissions, IEEE Trans. on Comm., Vol. COM-30, pp 247-252.

  // first call: reset==true
  // following calls: reset==false from this function
  // last call: reset==true externally; finalisation

   static uint32_t sum1     = 0xffff;
   static uint32_t sum2     = 0xffff;

   if( thisReset )
   {
     sum1 = 0xffff;
     sum2 = 0xffff;

     // the boolean is set to false after the first call. It
     // keeps that value until changed outside
     thisReset = false;
   }

   size_t numBytes = sizeof(T);
   size_t threshold = static_cast<size_t>(5056)/numBytes ;

   while (len)
   {
     size_t llen = len > threshold ? threshold : len;
     len -= llen;

     do
     {
       size_t j=numBytes-1;
       uint8_t *d=reinterpret_cast<uint8_t*>(data);
       do
       {
          sum1 += d[j];
          sum2 += sum1;
       } while ( j-- > 0 ) ;
       data++ ;
     } while (--llen);

     sum1 = (sum1 & 0xffff) + (sum1 >> 16);
     sum2 = (sum2 & 0xffff) + (sum2 >> 16);
   }

   // Second reduction step to reduce sums to 32 bits
   sum1 = (sum1 & 0xffff) + (sum1 >> 16);
   sum2 = (sum2 & 0xffff) + (sum2 >> 16);

  if( reset )
    *reset = thisReset;

   return sum2 << 16 | sum1;
}

template<typename T>
uint32_t fletcher32_BE_clear( T *data, size_t len, bool *reset, size_t nShift )
{
  bool thisReset=true;
  if( reset )
    thisReset = *reset;

  // adapted from:
  // http://en.wikipedia.org/wiki/Fletcher%27s_checksum
  // Ref.: Fletcher, J. G., An Arithmetic Checksum for Serial
  // Transmissions, IEEE Trans. on Comm., Vol. COM-30, pp 247-252.

  // first call: reset==true
  // following calls: reset==false from this function
  // last call: reset==true externally; finalisation

   static uint32_t sum1     = 0xffff;
   static uint32_t sum2     = 0xffff;

   if( thisReset )
   {
     sum1 = 0xffff;
     sum2 = 0xffff;

     // the boolean is set to false after the first call. It
     // keeps that value until changed outside
     thisReset = false;
   }

   size_t numBytes = sizeof(T);
   size_t threshold = static_cast<size_t>(5056)/numBytes ;

   while (len)
   {
     size_t llen = len > threshold ? threshold : len;
     len -= llen;

     do
     {
       size_t j=0;

       T tmp = clearLeastBits( *data, nShift );
       uint8_t *d=reinterpret_cast<uint8_t*>(&tmp);
//printBits(*data);
//printBits(tmp);
//std::cout << std::endl;
//exit(0);
       do
       {
          sum1 += d[j];
          sum2 += sum1;
       } while ( j++ < numBytes ) ;
       data++ ;
     } while (--llen);

     sum1 = (sum1 & 0xffff) + (sum1 >> 16);
     sum2 = (sum2 & 0xffff) + (sum2 >> 16);
   }

   // Second reduction step to reduce sums to 32 bits
   sum1 = (sum1 & 0xffff) + (sum1 >> 16);
   sum2 = (sum2 & 0xffff) + (sum2 >> 16);

  if( reset )
    *reset = thisReset;

   return sum2 << 16 | sum1;
}

template<typename T>
uint32_t fletcher32_LE_clear( T *data, size_t len, bool *reset, size_t nShift )
{
  bool thisReset=true;
  if( reset )
    thisReset = *reset;

  // adapted from:
  // http://en.wikipedia.org/wiki/Fletcher%27s_checksum
  // Ref.: Fletcher, J. G., An Arithmetic Checksum for Serial
  // Transmissions, IEEE Trans. on Comm., Vol. COM-30, pp 247-252.

  // first call: reset==true
  // following calls: reset==false from this function
  // last call: reset==true externally; finalisation

   static uint32_t sum1     = 0xffff;
   static uint32_t sum2     = 0xffff;

   if( thisReset )
   {
     sum1 = 0xffff;
     sum2 = 0xffff;

     // the boolean is set to false after the first call. It
     // keeps that value until changed outside
     thisReset = false;
   }

   size_t numBytes = sizeof(T);
   size_t threshold = static_cast<size_t>(5056)/numBytes ;

   while (len)
   {
     size_t llen = len > threshold ? threshold : len;
     len -= llen;

     do
     {
       size_t j=numBytes-1;

       T tmp = clearLeastBits( *data, nShift );
       uint8_t *d=reinterpret_cast<uint8_t*>(&tmp);
//printBits(*data);
//printBits(tmp);
//std::cout << std::endl;
//exit(0);
       do
       {
          sum1 += d[j];
          sum2 += sum1;
       } while ( j-- > 0 ) ;
       data++ ;
     } while (--llen);

     sum1 = (sum1 & 0xffff) + (sum1 >> 16);
     sum2 = (sum2 & 0xffff) + (sum2 >> 16);
   }

   // Second reduction step to reduce sums to 32 bits
   sum1 = (sum1 & 0xffff) + (sum1 >> 16);
   sum2 = (sum2 & 0xffff) + (sum2 >> 16);

  if( reset )
    *reset = thisReset;

   return sum2 << 16 | sum1;
}

// ----------------------------------------------------

//! get index
/*!  index of a corresponding vector v with size()==sz. If -sz < index < 0, then
     the reverse index is used. If index > -sz or index > sz, then index==0 */
int
getIndex(int index, int sz)
{
  if( index == INT_MAX )
      return 0;

  if( index < 0 && index > (-sz))
     index = sz + index;

  return index; // would raise an exception  outside the valid range
}

std::string
getVector2Str(std::vector<std::string>& vs, bool isSpace, char sep)
{
  std::string s;

  if( vs.size() )
  {
    s = vs[0] ;

    for( size_t i=1 ; i < vs.size() ; ++i )
    {
      s += sep;
      if(isSpace)
        s += ' ';

      s += vs[i] ;
    }
  }

  return s;
}


template <typename T>
T invertBits ( T val )
{
  // invert the ordering of bits

  size_t sz = sizeof(val);
  uint8_t *r=reinterpret_cast<uint8_t*>(&val);

  T inverse = 0;
  uint8_t *inv=reinterpret_cast<uint8_t*>(&inverse);

  for ( size_t j=0; j < sz; ++j )
  {
    for ( size_t i=0; i < 8; ++i )
    {
      // shift left and then append the last bit from val
      inv[7-i] = ( inv[7-i] << 1 ) | ( r[j] & 1 );
      r[j] >>= 1;
    }
  }

  return inverse;
}

bool isWhiteString( std::string &str )
{
  // a string only of spaces, tabs or empty
  size_t pE=str.size();

  if( pE == 0 )
    return true;

  for( size_t p=0 ; p < pE ; ++p )
  {
    if( str[p] == ' ' || str[p] == '\t' )
      continue ;
    else
      return false ;
  }

  return true ;
}

template <typename T>
void printBits ( T val )
{
  // note: the ordering of printed bits is inversed!
  T ival = invertBits(val);

  size_t sz = sizeof(val);
  uint8_t *r=reinterpret_cast<uint8_t*>(&ival);

  for ( size_t j=0; j < sz; ++j )
  {
    for ( size_t i=0; i < 8; ++i )
    {
      std::cout << ( r[i] & 1 );
      r[i] >>= 1;
    }
    std::cout << " " ;
  }

  std::cout << std::endl;
  return;
}

// ----------------------------------------------------
std::string
getNonAlpha(std::string s)
{
  std::string t;

  for( size_t i=0; i < s.size(); ++i )
  {
    if( ! isAlpha(s[i]) )
    {
      size_t j=0;
      for( ; j < t.size() ; ++j )
         if( t[j] == s[i] )
            break;

      if( j == t.size() )
        t += s[i];
    }
  }

  return t;
}

std::string
getNonAlphaNum(std::string s)
{
  std::string t;

  for( size_t i=0; i < s.size(); ++i )
  {
    if( ! isAlphaNum(s[i]) )
    {
      size_t j=0;
      for( ; j < t.size() ; ++j )
         if( t[j] == s[i] )
            break;

      if( j == t.size() )
        t += s[i];
    }
  }

  return t;
}

std::string
getUniqueString(std::string& s, char sep)
{
  Split x_s(s, sep) ;

  std::vector<std::string>vs( getUniqueVector( x_s.getItems() ));

  std::string us;
  for( size_t j=0 ; j < vs.size() ; ++j)
  {
    if(j)
      us += sep ;
    us += vs[j];
  }

  return us;
}

std::string
getUniqueString(std::vector<std::string>& vs, char sep)
{
  std::vector<std::string> vs_unique(getUniqueVector(vs));
  std::string s;

  for( size_t j=0 ; j < vs_unique.size() ; ++j )
  {
     if(j)
       s += sep;

     s += vs_unique[j] ;
  }

  return getUniqueString(s, sep);
}

std::vector<std::string>
getUniqueVector(std::string& s, char sep)
{
  Split x_s(s,sep) ;
  return getUniqueVector( x_s.getItems() );
}

std::vector<std::string>
getUniqueVector(std::vector<std::string>& vs)
{
  std::vector<std::string> vs_uni;

  for( size_t j=0 ; j < vs.size() ; ++j )
  {
     size_t u;
     for( u=0 ; u < vs_uni.size() ; ++u )
       if( vs_uni[u] == vs[j] )
          break;

     if( u == vs_uni.size() )
       vs_uni.push_back(vs[j]);
   }

  return vs_uni;
}

bool isAlpha(unsigned char c)
{
  int i = c ;

  bool is=false;

  if( ((i-64) > 0 && (i-64) < 27 ) || ((i-96) > 0 && (i-96) < 27 ) )
    is=true;

  return is;
}

bool isAlpha(std::string s, bool isContains)
{
  if( ! s.size() )
    return false;

  if( isContains )
  {
    for(size_t k=0; k < s.size(); ++k)
      if( isAlpha(s[k]) )
        return true;
  }
  else
  {
    for(size_t k=0; k < s.size(); ++k)
      if( !isAlpha(s[k]) )
        return false;
  }

  return true;
}

bool isAlphaNum(unsigned char c)
{
  int i = c ;

  bool is=false;

  if( (i-64) > 0 && (i-64) < 27 )
    is=true;
  else if( (i-96) > 0 && (i-96) < 27 )
    is=true;
  else if( (i-47) > 0 && (i-47) < 11 )
    is=true;

  return is;
}

bool
isAmong(std::string& item, std::vector<std::vector<std::string> >& set,
  std::string mode)
{
  size_t i, j;
  return isAmong(item, set, i, j, mode);
}

bool
isAmong(std::string& item, std::vector<std::vector<std::string*> >& set,
  std::string mode)
{
  size_t i, j;
  return isAmong(item, set, i, j, mode);
}

bool
isAmong(std::string& item, std::vector<std::vector<std::string> >& set,
  size_t& i, size_t& j, std::string mode)
{
  for(i=0 ; i < set.size() ; ++i )
    if( isAmong(item, set[i], j, mode ) )
      return true ;

  return false;
}

bool
isAmong(std::string& item, std::vector<std::vector<std::string*> >& set,
  size_t& i, size_t& j, std::string mode)
{
  for(i=0 ; i < set.size() ; ++i )
    if( isAmong(item, set[i], j, mode ) )
      return true ;

  return false;
}

bool
isAmong(std::string& item, std::vector<std::string>& set, std::string mode)
{
   size_t i;
   return isAmong(item, set, i, mode);
}

bool
isAmong(std::string& item, std::vector<std::string*>& set, std::string mode)
{
   size_t i;
   return isAmong(item, set, i, mode);
}

bool
isAmong(std::string& item, std::vector<std::string>& set,
        size_t& i,std::string mode)
{
  //mode: ""     --> 0: exact
  //      "beg"  --> 1: sub-str to the left of string
  //      "end"  --> 2: sub-string to the right
  //      "find" --> 3: find anywhere
  int imode=-1;

  if( ! mode.size() )
    imode=0;
  else if( mode == "beg" )
    imode=1;
  else if( mode == "end" )
    imode=2;
  else if( mode == "find" )
    imode=3;

  if(!imode)
  {
    for(i=0 ; i < set.size() ; ++i )
      if( item == set[i] )
        return true ;
  }
  else if( imode == 1 )
  {
    for(i=0 ; i < set.size() ; ++i )
      if( item == set[i].substr(0,item.size()) )
        return true ;
  }
  else if( imode == 2 )
  {
    for(i=0 ; i < set.size() ; ++i )
      if( item == set[i].substr(set[i].size() - item.size()) )
        return true ;
  }
  else if( imode == 3 )
  {
    for(i=0 ; i < set.size() ; ++i )
      if( set[i].find(item) < std::string::npos )
        return true ;
  }

  return false;
}

bool
isAmong(std::string& item, std::vector<std::string*>& set,
        size_t& i,std::string mode)
{
  //mode: ""     --> 0: exact
  //      "beg"  --> 1: sub-str to the left of string
  //      "end"  --> 2: sub-string to the right
  //      "find" --> 3: find anywhere
  int imode=-1;

  if( ! mode.size() )
    imode=0;
  else if( mode == "beg" )
    imode=1;
  else if( mode == "end" )
    imode=2;
  else if( mode == "find" )
    imode=3;

  if(!imode)
  {
    for(i=0 ; i < set.size() ; ++i )
      if( item == *(set[i]) )
        return true ;
  }
  else if( imode == 1 )
  {
    for(i=0 ; i < set.size() ; ++i )
      if( item == set[i]->substr(0,item.size()) )
        return true ;
  }
  else if( imode == 2 )
  {
    for(i=0 ; i < set.size() ; ++i )
      if( item == set[i]->substr(set[i]->size() - item.size()) )
        return true ;
  }
  else if( imode == 3 )
  {
    for(i=0 ; i < set.size() ; ++i )
      if( set[i]->find(item) < std::string::npos )
        return true ;
  }

  return false;
}

template <typename T>
bool
isAmong(T item, std::vector<T>& set)
{
  size_t i;
  return isAmong(item, set, i);
}

template <typename T>
bool
isAmong(T item, std::vector<T>& set, size_t& i)
{
  for(i=0 ; i < set.size() ; ++i )
    if( item == set[i] )
      return true ;

  return false;
}

template <typename T>
bool
isAmong(T item, std::vector<T>& set, bool is)
{
  size_t i;
  return isAmong(item, set, i, is);
}

template <typename T>
bool
isAmong(T item, std::vector<T*>& set, size_t& i, bool is)
{
  // note that the bool ist just for the compiler
  for( i=0 ; i < set.size() ; ++i )
    if( item == *(set[i]) )
      return true ;

  return false;
}

template <typename T>
bool
isAmong(T item, std::vector<T>& set, size_t& i, bool is)
{
  // note that the bool ist just for the compiler
  for( i=0 ; i < set.size() ; ++i )
    if( item == set[i] )
      return true ;

  return false;
}

template <typename T>
bool
isAmong(std::vector<T>& sub, std::vector<T*>& set, bool all)
{
  if( set.size() == 0 )
     return false;

  size_t count=0;
  for( size_t i=0 ; i < sub.size() ; ++i )
  {
    if( isAmong(sub[i], set) )
    {
      if( !all )
        return true;

      ++count;
    }
    else if( all )
      return false;
  }

  if( count == sub.size() )
    return true;  // for all==true

  return false ;
}

template <typename T>
bool
isAmong(std::vector<T>& sub, std::vector<T>& set, bool all)
{
  if( set.size() == 0 )
     return false;

  size_t count=0;
  for( size_t i=0 ; i < sub.size() ; ++i )
  {
    if( isAmong(sub[i], set) )
    {
      if( !all )
        return true;

      ++count;
    }
    else if( all )
      return false;
  }

  if( count == sub.size() )
    return true;  // for all==true

  return false ;
}

bool isDigit(unsigned char c, bool isNumber)
{
  if( isNumber )
    if(  c == '.' || c == '-' || c == '+'  )
      return true;

  int i = c ;

  i -= 47 ;

  if( i > 0 && i < 11 )
    return true;
  else
    return false;
}

bool isDigit(std::string s, bool isContains)
{
  if( ! s.size() )
    return false;

  if( isContains )
  {
    for(size_t k=0; k < s.size(); ++k)
      if( isdigit(s[k]) )
        return true;

    return false;
  }
  else
  {
    for(size_t k=0; k < s.size(); ++k)
      if( !isdigit(s[k]) )
        return false;
  }

  return true;
}

bool isNumber( std::string &str )
{
  const char *s = str.c_str();

  /*  s  ist der gesamte String
   return: true, falls string nur Zahl, sonst false */

  /* der folgende Teil entspricht im wesentlichen der Fkt 'atof' */
  for( ; ( isdigit(*s)  ||  *s == '-'  || *s == '+'
          ||  *s == '.' ||  *s == 'E' ||  *s == 'e' ) ; s++ )
  {
    if( *s== 'E' ||  *s == 'e' )
    {
      if( *(s+1) == '+' ||  *(s+1) == '-' || isdigit(*(s+1)) )
        continue;
    }
  }

  if( *s == '\0' )
    return true ;    /* nur Zahl */

  return false ;
}

// ----------------------------------------------------
//! formatting of attribute|variable name in combination with value etc.

std::string tf_assign(std::string right)
{
  if( right.size() > 10 )
    return "= <" + right + ">" ;
  else
    return "=<" + right + ">" ;
}

std::string tf_assign(std::string left, std::string right)
{
  if( right.size() > 10 )
    return left + " = <" + right + ">" ;
  else
    return left + "=<" + right + ">" ;
}

std::string
tf_att(std::string p1, std::string p2, std::string p3,
    std::string p4, std::string p5, std::string p6)
{
  bool isColon=false;
  bool isBlank=true;
  bool isUpper=false;

  std::vector<std::string*> ps;
  ps.push_back(&p1);
  ps.push_back(&p2);
  ps.push_back(&p3);
  ps.push_back(&p4);
  ps.push_back(&p5);
  ps.push_back(&p6);

  std::vector<std::string*> vs_str;

  for( size_t i=0 ; i < ps.size() ; ++i )
  {
    if( *ps[i] == colon )
      isColon=true;
    else if( *ps[i] == no_blank )
      isBlank=false;
    else if( *ps[i] == upper )
      isUpper=true;
    else if( *ps[i] != s_void )
      vs_str.push_back(ps[i]) ;
  }

  return tf_att(vs_str, isColon, isBlank, isUpper );
}

std::string
tf_att(std::vector<std::string*>& p,
       bool isColon, bool isBlank, bool isUpper)
{
  std::string s;

  if( p.size() == 0 )
    return s;

  s = "attribute ";

  if( p.size() == 1 )
    s += *(p[0]);
  else if( p.size() > 1 )
  {
    if( p[0]->size() )
      s += "<" + *(p[0]) + ">:" + *(p[1]);
    else
      s += *(p[1]) ;
  }

  if( p.size() == 3 )
    s += " = <" + *(p[2]) + ">";

  if(isUpper )
    s[0] = 'A' ;

  if(isColon)
    s += colon;

  if(isBlank)
    s += blank;

  return s ;
}

std::string
tf_range(std::string v0, std::string v1, std::string blnk)
{
  std::string s(" <");
  s += v0 ;
  s += " - " ;
  s += v1 ;
  s += ">" ;

  if(blnk.size())
    s += blank;

  return s ;
}

std::string
tf_val(std::string v, std::string blnk)
{
  std::string s(" <" + v + ">" );
  if(blnk.size())
    s += blank;

  return s ;
}

std::string
tf_var(std::string p1, std::string p2, std::string p3, std::string p4)
{
  bool isColon=false;
  bool isBlank=true;
  bool isUpper=false;

  std::vector<std::string*> ps;
  ps.push_back(&p2);
  ps.push_back(&p3);
  ps.push_back(&p4);

  for( size_t i=0 ; i < ps.size() ; ++i )
  {
    if( *ps[i] == colon || *ps[i] == ":" )
      isColon=true;
    else if( *ps[i] == no_blank || *ps[i] == "" )
      isBlank=false;
    else if( *ps[i] == upper )
      isUpper=true;
  }

  return tf_var(p1, isColon, isBlank, isUpper );
}

std::string
tf_var(std::string& v,
       bool isColon, bool isBlank, bool isUpper)
{
  std::string s;

  if( v.size() == 0 )
    return s;

  s = "variable <" + v + ">";

  if(isUpper )
    s[0] = 'A' ;

  if(isColon)
    s += colon;

  if(isBlank)
    s += blank;

  return s ;
}


// ----------------------------------------------------
std::vector<std::string> itoa(std::vector<int>& vi_in )
{
  std::vector<std::string> vs_out;

  for(size_t i=0 ; i < vi_in.size() ; ++i )
    vs_out.push_back( itoa(vi_in[i]) );

  return vs_out;
}

std::string itoa(int n )
{
  //for downward compability only
  int sign=1;
  if( n < 0 )
  {
    n *= -1 ;
    sign=-1;
  }

  return itoa( static_cast<long> (n), sign ) ;
}

std::string itoa(unsigned long long int n, int sign )
{
  // returns string representation of n

  std::string t;

  // the inverse of n is stored in t
  do
  {
    t += n % 10 + '0';
  } while((n/=10)>0);

  if(sign < 0)
    t += '-';

  // reverse
  for( size_t j=0 ; j < t.size()/2 ; ++j)
  {
    char c = t[j];
    size_t l= t.size() - j - 1 ;
    t[j] = t[l] ;
    t[l] = c    ;
  }

  return t ;
}

// ----------------------------------------------------

Point parabolaSummit(double xm, double x, double xp, double ym,
                        double y, double yp)
{
  Point punkt;
  double epsilon, a1,a2 ;

  a1 = (yp-y)*(xm*xm - x*x) -(ym - y)*(xp*xp -x*x) ;
  a2 = (ym-y)*(x-xp) - (yp-y)*(x-xm) ;

  if( fabs(a2) > 0. )
    epsilon = a1 / (2.*a2) ;
  else  // ist eine Gerade
    epsilon = x ;

  punkt.x = epsilon ;

  a2 = ( (x-epsilon)*(x-epsilon) - (xm-epsilon)*(xm-epsilon) );

  if( fabs(a2) > 0. )
    punkt.y = yp - (xp-epsilon)*(xp-epsilon) *(y - ym) / a2 ;
  else  // entartet zum Pkt
    punkt.y = yp ;

  return punkt ;
}

// ----------------------------------------------------

double planck(double anue,double t)
{
  double a,b,pl,aa;
  double nk_c ,nk_h ,nk_k;

/*
  c=2.9979 * 1.E+08 M/S
  h=6.6256 * 1.E-34 J*S
  k=1.3805 * 1.E-23 J/K
  WELLENZAHL:ANUE=FREQ. * C ; [CM-1] =: 100 [M-1]
  SIGMA=5.6693*1.E-08 [W M-2 K-4]
*/

  if( t == 0. )
    return 0.;

  nk_c=2.99792458 ;
  nk_h=6.62606876 ;
  nk_k=1.3806503 ;
  a=2.*nk_h*nk_c*nk_c*1.e-10;
  b=nk_h*nk_c/nk_k*.1 / t;

  aa=a*anue*anue*anue ;
  pl=aa/(exp( b*anue) - 1.);

  return  pl;
}

// ----------------------------------------------------

double
rounding(double v, int dNum)
{
   double fact=1.;
   for(int i=0 ; i < dNum ; ++i)
     fact *= 10.;

   v *= fact;
   v += 0.5;

   int iv = static_cast<int>(v);

   return static_cast<double>(iv) / fact;
}

// private function
double string2DoubleFct( std::string &, size_t & );

//public access
bool string2Double( const char *s0, size_t nr, double* retVal)
{
  std::string t(s0) ;
  t += '\0';
  return string2Double( t, nr, retVal) ;
}

bool string2Double( std::string s0, size_t nr, double* retVal)
{
  /*  s  entrire string
    val  eventualy the found number
     nr  the number-position of anumber to be searched in the string;
         i.e. nr=2 looks for the second real number in the string.
   return: true, if number was detected, else: false */

  std::string s(s0) ;
  s += '\0';

  size_t index=0 ;
  double val=0.  ;

  for( size_t n=0 ; n<nr ; n++ )
  {
    /* mainly function 'atof' */
    for( ; ! ( isDigit(s[index])  ||  s[index] == '-'  || s[index] == '+'
            ||  s[index] == '.' ) ; index++  )
    {
      if( s[index] == '\0' )
        return false ;    // no number found
    }

    // where all the work is done
    val = string2DoubleFct( s, index );
   }

  *retVal = val ;

  return true ;
}
double string2Double( const char *s0, size_t nr, bool isReturnValNull)
{
  std::string s(s0);
  s += '\0' ;
  return string2Double( s, nr, isReturnValNull) ;
}

double string2Double( std::string s0, size_t nr, bool isReturnValNull)
{
  /*  s  entrire string
    val  eventualy the found number
     nr  the number-position of anumber to be searched in the string;
         i.e. nr=2 looks for the second real number in the string.

    If isReturnValNull set true: return 0. in case of not found.
    Default:  false ==> return MAXDOUBLE  */

  double val=0.;

  std::string s( s0 );
  s += '\0' ;

  size_t index=0 ;

  for( size_t n=0 ; n<nr ; n++ )
  {
    //mainly fct 'atof'
    for( ; ! ( isDigit(s[index])  ||  s[index] == '-'  || s[index] == '+'
            ||  s[index] == '.' ) ; index++  )
    {
      if( s[index] == '\0' )
        return isReturnValNull ? 0. : MAXDOUBLE ; //no number in string
    }

    // where work takes place
    val = string2DoubleFct( s, index );
  }

  return val ;
}

double string2Double( const char *s0, size_t nr, int &width, int &precision, bool isReturnValNull)
{
  std::string s(s0) ;
  s += '\0' ;

  return string2Double( s, nr, width, precision, isReturnValNull) ;
}

double string2Double( std::string s0, size_t nr, int &width, int &precision, bool isReturnValNull)
{
  /*  s  entrire string
    val  eventualy the found number
     nr  the number-position of anumber to be searched in the string;
         i.e. nr=2 looks for the second real number in the string.
    width      return the length of the substring containing the number
    precision  number of decimals

    If isReturnValNull set true: return 0. in case of not found.
    Default:  false ==> return MAXDOUBLE  */

  std::string s( s0 );
  s += '\0' ;

  size_t index=0 ;
  size_t beg=0;
  double val=0. ;

  width = 0;
  precision=0;

  for( size_t n=0 ; n<nr ; n++ )
  {
    for( ; ! ( isDigit(s[index])  ||  s[index] == '-'  || s[index] == '+'
            ||  s[index] == '.' ) ; ++index )
    {
      if( s[index] == '\0' )
        return isReturnValNull ? 0. : MAXDOUBLE ;    // keine Zahl vorhanden
    }

    beg=index;

    val = string2DoubleFct( s, index );
  }

  // Bestimmung der Feldbreite und Praezision
  size_t front=s.find('.',beg);

  std::string frontStr(s.substr(beg, front-beg) );
  std::string decStr(s.substr(front+1, index-front-1) );

  precision = decStr.size();
  width     = frontStr.size() + precision +1 ;

  return val ;
}

double string2Double( const char *s0, size_t nr ,int *restIndex, bool isReturnValNull )
{
   std::string s(s0);
   s += '\0' ;

   return string2Double( s, nr, restIndex, isReturnValNull ) ;
}

double string2Double( std::string s0, size_t nr ,int *restIndex, bool isReturnValNull )
{
  //restIndex: index where the remmaining non-digit string continues

  std::string s( s0 );
  s += '\0' ;

  size_t index=0 ;
  double val=0.  ;

  for( size_t n=0 ; n<nr ; n++ )
  {
    for( ; ! ( isDigit(s[index])  ||  s[index] == '-'  || s[index] == '+'
            || s[index] == '.' ) ; ++index )
    {
      if( s[index] == '\0' )
      {
        *restIndex = index ;
        return isReturnValNull ? 0. : MAXDOUBLE ; //no number
      }
    }

    val = string2DoubleFct( s, index );
  }

  *restIndex = index ;

  return val ;
}

double string2DoubleFct( std::string &s, size_t &index )
{
  int sign ,power_count=0 ;
  double power, val ;

  sign = (s[index] == '-') ? -1 : 1 ;

  if( s[index] == '+' || s[index] == '-')
    ++index ;

  for( val=0.0; isDigit(s[index]); ++index )
    val = 10.0 * val + (s[index] - '0');

  if( s[index] == '.' )
    ++index ;

  for( power = 1.0; isDigit( s[index]); ++index )
  {
    val = 10.0 * val + (s[index] - '0' );
    power *= 10.0;
    power_count++;
  }

  // scientific notation?
  if( s[index]=='e' || s[index]=='E' )
  {
    ++index;
    val = sign * val / power ;

    if( ! (s[index] == '+' || s[index] == '-' || isDigit(s[index]) ) )
      return val ;

    bool isSign = s[index] == '+' || s[index] == '-';

    if( ((index+1) == s.size()) || (isSign && !isDigit(s[index+1])) )
    {
      // the detected 'e' or 'E' is not part of the number
      // although a sign follows, but without any digit
      return val;
    }

    if( ! ( isSign || isDigit(s[index]) ) )
    {
      // the detected 'e' or 'E' is not part of the number
      return val;
    }

    std::string t(s.substr(index));
    val *= pow( 10. , string2Double(t,1) );

    // index must be adjusted for the next looping
    while( (s[index] == '+' || s[index] == '-' || isDigit(s[index]) ) )
      ++index ;

    return val ;
  }

  val = sign * val / power ;

  return val ;
}

// ----------------------------------------------------

//! concatenate a string-vector to a comma-separated (with blanks) string
std::string
catStringVector(std::vector<std::string>& vs)
{
  std::string s;
  for( size_t i=0 ; i < vs.size() ; ++i)
  {
    if(i)
      s += ", ";

    s += vs[i];
  }

  return s;
}

//! Remove characters from a str.
std::string
clearChars(std::string str, std::string s, bool isStr )
{
  // By default, each character in 's' is removed.
  // If isStr==true, then only every occurrence of
  // the string 's' is removed.
  size_t sz = s.size();
  std::string t;

  if( isStr )
  {
    size_t pos0=0;
    size_t pos1;

    while( (pos1=str.find(s,pos0)) < std::string::npos )
    {
      t += str.substr(pos0, pos1-pos0) ;
      pos0=pos1+sz;
    };
    t += str.substr(pos0) ;
  }
  else
  {
    // each character
    for( size_t j=0 ; j < str.size() ; ++j)
    {
      bool is=true;
      for( size_t i=0 ; i < s.size() ; ++i)
      {
        if( str[j] == s[i] )
        {
          is=false;
          break;
        }
      }

      if( is )
        t += str[j] ;
    }
  }

  return t ;
}

std::string
clearEnclosures(std::string& str, char a, char b )
{
   clearEnclosures_unpaired = false;

   // example: str="one(two(three)four)five" --> "onefive
   if( str.size() == 0 )
     return str;

   size_t p0=0;

   std::vector<size_t> pA;
   while( (p0=str.find(a, p0)) < std::string::npos )
      pA.push_back(p0++);

   p0=0;
   std::vector<size_t> pB;
   while( (p0=str.find(b, p0)) < std::string::npos )
      pB.push_back(p0++);

   std::string s;

   if( pA.size() == pB.size() )
   {
     if( pA.size() == 0 )
        return str ;

     pA.push_back(str.size()+1);
     pB.push_back(str.size());
     size_t c=0 ;
     size_t pBeg=0 ;
     size_t ixA=0;
     size_t ixB=0;

     while( ixA < pA.size() )
     {
        if( pA[ixA+c] < pB[ixB+c] && pA[ixA+c+1] > pB[ixB+c] )
        {
           if( pA[ixA] > pBeg )
             s += str.substr(pBeg, pA[ixA] - pBeg);

           c=0;
           pBeg = pB[ixB+c] +1 ;
           ixA += c+1;
           ixB += c+1;
        }
        else if( str.size() < pA[ixA] )
           break;
        else
        {
           ++c;
        }
     }

     if( pBeg < str.size() )
        s+=str.substr(pBeg, str.size() - pBeg);
   }
   else
      clearEnclosures_unpaired=true;

   return s;
}

//! Clear all spaces from a string
std::string clearSpaces(std::string str )
{
  std::string s;

  for( size_t p=0 ; p < str.size() ; ++p )
    if( str[p] != ' ' && str[p] != '\t' && str[p] != '\n')
      s += str[p] ;

  return s ;
}

std::vector<std::string>
itemise(std::string& s, std::string key, std::string mode)
{
  std::vector<std::string> vs_item;

  Split x_s(s);

  if( mode.size() )
  {
    size_t kSz = key.size();
    if( mode == "first" )
    {
      for( size_t i=0 ; i < x_s.size() ; ++ i )
      {
        if(x_s[i].substr(0,kSz) == key )
          vs_item.push_back(x_s[i]);
        else
        {
          if( vs_item.size() == 0 )
            return vs_item ;  // fault

          vs_item.back() += blank ;
          vs_item.back() += x_s[i] ;
        }
      }
    }
    else if( mode == "last" )
    {
      for( size_t i=0 ; i < x_s.size() ; ++ i )
      {
        if(x_s[i].substr(x_s[i].size()-kSz) == key )
          vs_item.push_back(x_s[i]);
        else
        {
          if( vs_item.size() == 0 )
            return vs_item ;  // fault

          vs_item.back() += blank ;
          vs_item.back() += x_s[i] ;
        }
      }
    }
  }
  else
  {
    for( size_t i=0 ; i < x_s.size() ; ++ i )
    {
      if(x_s[i].find(key) < std::string::npos )
        vs_item.push_back(x_s[i]);
      else
      {
        if( vs_item.size() == 0 )
          return vs_item ;  // fault

        vs_item.back() += blank ;
        vs_item.back() += x_s[i] ;
      }
    }
  }

  return vs_item;
}

// Replace characters of a str.
std::string
replaceChars(std::string str, std::string pat, char rep, bool isStr )
{
  size_t patSz = pat.size();

  if( str.size() == 0 || patSz == 0 )
    return str;

  if( !isStr )
  {
    for( size_t i=0 ; i < patSz ; ++i )
      str = replaceChars(str, pat[i], rep );

    return str;
  }

  std::string t;

  size_t pos0=0;
  size_t pos1;

  while( (pos1=str.find(pat,pos0)) < std::string::npos )
  {
    t += str.substr(pos0, pos1-pos0) ;
    t += rep;

    pos0=pos1+patSz;
  };

  return t + str.substr(pos0) ;
}

std::string
replaceChars(std::string str, char s, char rep)
{
  // each character of 's' in str is replaced by rep.
  for( size_t j=0 ; j < str.size() ; ++j)
  {
    if( str[j] == s )
      str[j] = rep;
  }

  return str ;
}

std::string
stripSides(std::string str, std::string strip, std::string mode )
{
   std::vector<std::string> vs;

   if( strip.size() )
     vs.push_back(strip);

   return stripSides(str, vs, mode) ;
}

std::string
stripSides(std::string str, std::vector<std::string>& strp, std::string mode )
{
  // strip off leading and trailing blanks and tabs
  size_t p0=0;
  size_t p1=str.size()-1;
  bool isEmpty=true;

  std::string side;

  std::vector<std::string> strip;

  if( strp.size() )
    strip = strp ;
  else
  {
    strip.push_back(" ");
    strip.push_back("\t");
  }

  for( size_t i=0 ; i < mode.size() ; ++i )
  {
    if( mode[i] == 'r' )
    {
      if( mode.substr(i, 5) == "right" )
      {
        side = "right";
        i += 4;
      }
    }
    else if( mode.substr(i, 4) == "left" )
    {
      side = "left";
      i += 3;
    }
  }

  if( side != "right" )
  {
    for( size_t p=0 ; p < str.size() ; ++p )
    {
      bool isCont=false;
      for( size_t i=0 ; i < strip.size() ; ++i )
      {
        size_t sz = strip[i].size() ;
        if( str.substr(p, sz) == strip[i] )
        {
          p += (sz -1) ; // because of ++p
          p0=p+1;  // applies only to an item equal to strip[i]
          isCont=true ;
        }
      }

      if( isCont )
        continue;

      p0=p;
      isEmpty=false;

      break;
    }
  }

  if( side != "left" )
  {
    for( int p=str.size()-1 ; p > -1 ; --p )
    {
      bool isCont=false;
      for( size_t i=0 ; i < strip.size() ; ++i )
      {
        int sz = strip[i].size() ;
        if( p >= sz && str.substr(p-sz+1, sz) == strip[i] )
        {
          p -= (sz - 1) ;  // becaus of --p
          p1=p-1;  // applies only to an item equal to strip[i]
          isCont=true ;
        }
      }

      if( isCont )
        continue;

      p1=p;
      isEmpty=false;
      break;
    }
  }

  if( isEmpty )
    return "" ;

  return str.substr(p0, p1 - p0 +1 ) ;
}

std::string unique(std::string str, char c)
{
  if( !str.size() )
     return str;

  // replace multiple identical characters by a single one
  Split splt(str, c);

  std::string s;
  if( str[0] == c )
    s += c;
  s += splt[0] ;

  for( size_t p=1 ; p < splt.size() ; ++p )
  {
    s += c ;
    s += splt[p] ;
  }

  if( str.size() && str[str.size()-1] == c )
    s += c;

  return s ;
}

std::vector<std::string>
unique(std::vector<std::string> &vs, std::string pat, bool isStr)
{
  std::vector<std::string> vs_t;
  for(size_t i=0 ; i < vs.size() ; ++i)
    vs_t.push_back( unique(vs[i], pat, isStr ) );

  return vs_t;
}

std::string unique(std::string str, std::string pat, bool isStr)
{
  if( !str.size() )
     return str;

  if( pat.size() == 1 )
    return unique(str, pat[0]);

  // unique for every char
  if( pat.size() == 0 )
  {
    std::string t;
    for( size_t i=0 ; i < str.size() ; ++i)
    {
      char& c=str[i++];
      t += c;

      for( ; i < str.size() ; ++i)
      {
        if( c != str[i] )
        {
          --i;
          break;
        }
      }
    }

    return t;
  }
  else if(!isStr)
  {
    // each char of pat
    for( size_t i=0 ; i < pat.size() ; ++i )
      str=unique(str, pat[i]);

    return str;
  }

  // unique for the entire pat-string
  Split splt(str, pat, true);
  std::string s;

  if( str.substr(0, pat.size()) == pat )
    s = pat;

  s += splt[0];

  for( size_t p=1 ; p < splt.size() ; ++p )
  {
    s += pat ;
    s += splt[p] ;
  }

  if( str.size() > pat.size() && str.substr(str.size()-pat.size()) == pat )
    s += pat;

  return s ;
}

std::string
unsplit(std::vector<std::string>& vs, std::string sep, size_t num)
{
  std::string s;
  if( ! num )
    num = vs.size();

  if( num )
  {
    s = vs[0];
    for(size_t i=1 ; i < num; ++i )
      s += sep + vs[i];
  }

  return s;
}

std::string
unsplit(Split& x_s, std::string sep, size_t num)
{
  std::string s;
  if( ! num )
    num = x_s.size();

  if( num )
  {
    s = x_s[0];
    for(size_t i=1 ; i < num; ++i )
      s += sep + x_s[i];
  }

  return s;
}

// ----------------------------------------------------

void upperCase(char &c)
{
  c = toupper(c);
  return;
}

void lowerCase(char &c)
{
  c = tolower(c);
  return;
}


// ----------------------------------------------------

} // end of namespace
