#include "split.h"

Split::Split()
{
  init();
  sep.push_back(std::string(" "));  // by default
  sep.push_back(std::string("\t"));  // by default
}

Split::Split(std::string s) : str(s)
{
  init();
  sep.push_back(std::string(" "));  // by default
  sep.push_back(std::string("\t"));  // by default
}

Split::Split(const char* s) : str(s)
{
  init();
  sep.push_back(std::string(" "));  // by default
  sep.push_back(std::string("\t"));  // by default
}

Split::Split(std::string s, std::string sp, bool eachCharIsSep) : str(s)
{
  init();

  addSeparator(sp, eachCharIsSep);
}

Split::Split(std::string s, char sp) : str(s)
{
  init();
  s=sp;
  sep.push_back(s);
}

Split::Split(const Split &t)
{
  xcptn.ofsError=0;
  str = t.str ;
  sep=t.sep;

  isAlNum = t.isAlNum;
  isDecomposed = t.isDecomposed;
  isFixedWidth=t.isFixedWidth;
  isEmptyItemsEnabled=t.isEmptyItemsEnabled;
  isItemsWithSep = t.isItemsWithSep;
  isStripSides=t.isStripSides;
  is_valid=t.is_valid;

  items = t.items ;
  itemPos = t.itemPos ;
}

Split::~Split()
{
  if( xcptn.ofsError )
    delete xcptn.ofsError ;
}

void
Split::operator=(  const char *s )
{
  std::string sstr(s);
  isDecomposed=false;
  items.clear();
  itemPos.clear();
  *this=sstr ;
  return ;
}

void
Split::operator=( std::string s )
{
  str=s;
  isDecomposed=false;
  items.clear();
  itemPos.clear();
  return ;// *this;
}

void
Split::operator+=( std::string s )
{
  str+=s;
  isDecomposed=false;
  return ;
}

std::string&
Split::operator[](size_t i)
{
  if( ! isDecomposed )
    decompose();

  if( i >= items.size() )
    return empty;

  return items[i];
}

void
Split::addIgnore( std::string s, bool isStr)
{
  if( isStr )
    ignore.push_back(s);
  else
  {
    std::string t;
    for( size_t i=0 ; i < s.size() ; ++i)
    {
      t=s[i];
      ignore.push_back(t);
    }
  }

  return ;
}

void
Split::addProtector( std::string s, bool eachCharIsSep)
{
  if( s == ":alnum:" )
  {
    isProtectAlNum=true;
    return;
  }
  else if(eachCharIsSep)
  {
    std::string t;
    for( size_t i=0 ; i < s.size() ; ++i)
    {
      t=s[i];
      protector.push_back(t);
    }
  }
  else
    protector.push_back(s);

  return ;
}

void
Split::addSeparator( std::string s, bool eachCharIsSep)
{
  if( s == ":alnum:" )
  {
    isAlNum=true;
    return;
  }
  else if(eachCharIsSep)
  {
    std::string t;
    for( size_t i=0 ; i < s.size() ; ++i)
    {
      t=s[i];
      sep.push_back(t);
    }
  }
  else
    sep.push_back(s);

  return ;
}

void
Split::append(std::string t)
{
  if( ! isDecomposed )
    decompose();

  items.push_back(t);

  return;
}

void
Split::assign(size_t i0, std::string t)
{
  if( ! isDecomposed )
    decompose();

  if( i0 < items.size() )
    items[i0] = t ;
  else
    items.push_back(t);

  return;
}

void
Split::clear(void)
{
  str = "";
  items.clear();
  itemPos.clear();

  return ;
}

const char*
Split::c_str(size_t i)
{
  if( ! isDecomposed )
  {
    if( str.size() == 0 )
    {
      std::ostringstream ostr(std::ios::app);
      ostr << "Split::c_str()"
           <<"\nno string specified.";
      exceptionError( ostr.str() );
    }

    decompose();
  }

  if( i>=items.size() )
  {
      std::ostringstream ostr(std::ios::app);
      ostr << "Split::c_str()"
           << "\nno such high token index.";
      exceptionError( ostr.str() );
  }

  return items[i].c_str();
}

void
Split::decompose(void)
{
  if( protector.size() )
    return decomposeWithProtect();

  items.clear();
  itemPos.clear();

  if( ignore.size() )
    doIgnore( str ) ;  // will perhaps change object wide 'str'

  if( isAlNum )
  {
    decomposeAlNum();
    return;
  }

  std::vector<size_t> items_beg;
  std::vector<size_t> items_len;
  std::vector<int> sep_ix;
  int sepIndex;

  if( isFixedWidth )
     getFixedFormat();
  else
  {
    std::string::size_type pos, pos0  ;
    pos0=0;

    size_t sz = str.size();
    size_t len=0;

    do
    {
      sepIndex=-1;

      // Works for multiple separators.
      // Find position of a next separator
      pos=std::string::npos;
      for( size_t i=0 ; i < sep.size() ; ++i)
      {
        size_t p;
        if( ( p = str.find(sep[i], pos0)) < std::string::npos )
        {
          if( pos > p )
          {
            pos=p;
            sepIndex= static_cast<int>(i);
          }
        }
      }

      if( sepIndex == -1 )
      {
        // last regular item or no separator at all
        items_beg.push_back(pos0);
        sep_ix.push_back(-1);
        items_len.push_back( sz - pos0 );
        break;
      }
      else
      {
        len = sep[sepIndex].size() ;

        if( pos > pos0 )
        {
          // a regular item
          items_beg.push_back(pos0);
          sep_ix.push_back(-1);
          items_len.push_back( pos-pos0 );
        }

        items_beg.push_back(pos);
        sep_ix.push_back(sepIndex);
        items_len.push_back(len);
      }

      pos0 = pos + len ;
    } while ( pos0 < sz ) ;

    // context change
    sz = items_beg.size();

    // regular case: avoid trailing separators
    if( ! (isItemsWithSep || isEmptyItemsEnabled) )
    {
      for( int j=sz-1 ; j >= 0 ; --j)
      {
        if( sep_ix[j] > -1 )
          --sz;
        else
          break;
      }
    }

    for( size_t i=0 ; i < sz ; ++i )
    {
      if( sep_ix[i] == -1 || isItemsWithSep )
      {
        std::string t0(str.substr(items_beg[i], items_len[i]));

        if( isStripSides )
          t0 = hdhC::stripSides(t0, stripSides) ;

        if( t0.size() )  // empty item if !(sz == 1 && t0 == stripSides[0])
        {
          items.push_back(t0);
          itemPos.push_back(items_beg[i]);
        }
      }
      else if( isEmptyItemsEnabled && sep_ix.size() && sep_ix[i-1] > -1 )
      {
        items.push_back( empty );
        itemPos.push_back(items_beg[i]);
      }
    }
  }

  isDecomposed=true;

  return ;
}

void
Split::decomposeWithProtect(void)
{
  items.clear();
  itemPos.clear();

  if( ignore.size() )
    doIgnore( str ) ;  // will perhaps change object wide 'str'

  if( isAlNum )
  {
    decomposeAlNum();
    return;
  }

  std::vector<size_t> items_beg;
  std::vector<size_t> items_len;
  std::vector<int> sep_ix;
  int sepIndex;

  if( isFixedWidth )
     getFixedFormat();
  else
  {
    std::string::size_type pos, pos0  ;
    pos0=0;

    size_t sz = str.size();
    size_t len=0;

    std::vector<std::pair<size_t,size_t> > vs_protector;

    // find positions with protectors. No nesting.
    bool isFindProtectors=true;
    if( protector.size() == 1
            && (pos0=str.find(protector[0])) < std::string::npos )
    {
      vs_protector.push_back( std::pair<size_t,size_t>(0,pos0) );
      isFindProtectors=false;
    }
    else if( (str.find(protector[0])) > (pos=str.find(protector[1])) )
    {
      vs_protector.push_back( std::pair<size_t,size_t>(0,pos0) );
      pos0=pos+1;
    }

    while( isFindProtectors )
    {
      size_t p1st=std::string::npos;
      size_t ix_1st=0;
      size_t p2nd;

      for( size_t ix=0 ; ix < protector.size() ; ix+=2 )
      {
        if( (pos=str.find(protector[ix], pos0)) < std::string::npos )
        {
          if( p1st > pos )
          {
            p1st=pos;
            ix_1st=ix;
          }
        }
      }

      if( p1st == std::string::npos )
        break;

      p2nd=str.find(protector[ix_1st+1], p1st) ;
      if( p2nd < std::string::npos )
        ++p2nd;

      vs_protector.push_back( std::pair<size_t,size_t>(p1st,p2nd) );

      pos0=p1st+1;
    }

    pos0=0;

    do
    {
      sepIndex=-1;

      // Works for multiple separators.
      // Find position of a next separator
      size_t pos=std::string::npos;
      bool isProtect=false;
      size_t p_try=pos0;

      do
      {
        size_t pMin=std::string::npos;
        size_t sep_ix=-1;
        for( size_t i=0 ; i < sep.size() ; ++i)
        {
          size_t p;
          if( ( p = str.find(sep[i], p_try)) < std::string::npos )
          {
            if( pMin > p )
            {
                pMin=p;
                sep_ix= static_cast<int>(i);
            }
          }
        }

        isProtect=false;
        for( size_t b=0 ; b < vs_protector.size() ; ++b )
        {
          if( vs_protector[b].first < pMin && pMin < vs_protector[b].second )
          {
            isProtect=true;
            p_try=pMin+1;
            break;
          }
        }

        if( !isProtect )
        {
          pos=pMin;
          sepIndex= static_cast<int>(sep_ix);
        }
      } while(isProtect) ;

      if( sepIndex == -1 )
      {
        // last regular item or no separator at all
        items_beg.push_back(pos0);
        sep_ix.push_back(-1);
        items_len.push_back( sz - pos0 );
        break;
      }
      else
      {
        len = sep[sepIndex].size() ;

        if( pos > pos0 )
        {
          // a regular item
          items_beg.push_back(pos0);
          sep_ix.push_back(-1);
          items_len.push_back( pos-pos0 );
        }

        items_beg.push_back(pos);
        sep_ix.push_back(sepIndex);
        items_len.push_back(len);
      }

      pos0 = pos + len ;
    } while ( pos0 < sz ) ;

    // context change
    sz = items_beg.size();

    // regular case: avoid trailing separators
    if( ! (isItemsWithSep || isEmptyItemsEnabled) )
    {
      for( int j=sz-1 ; j >= 0 ; --j)
      {
        if( sep_ix[j] > -1 )
          --sz;
        else
          break;
      }
    }

    for( size_t i=0 ; i < sz ; ++i )
    {
      if( sep_ix[i] == -1 || isItemsWithSep )
      {
        std::string t0(str.substr(items_beg[i], items_len[i]));

        if( isStripSides )
          t0 = hdhC::stripSides(t0, stripSides) ;

        if( t0.size() )  // empty item if !(sz == 1 && t0 == stripSides[0])
        {
          items.push_back(t0);
          itemPos.push_back(items_beg[i]);
        }
      }
      else if( isEmptyItemsEnabled && sep_ix.size() && sep_ix[i-1] > -1 )
      {
        items.push_back( empty );
        itemPos.push_back(items_beg[i]);
      }
    }
  }

  isDecomposed=true;

  return ;
}

void
Split::decomposeAlNum(void)
{
  bool isA=false;
  bool isN=false;

  for( size_t i=0 ; i < str.size() ; ++i)
  {
    if( isA && ! hdhC::isDigit(str[i], true) )
      items.back() += str[i];
    else if( isN && hdhC::isDigit(str[i], true) )
      items.back() += str[i];

    else if( hdhC::isDigit(str[i], true) )
    {
      isN=true;
      items.push_back("");
      items.back() += str[i] ;
      isA=false;
    }
    else
    {
      isA=true;
      items.push_back("");
      items.back() += str[i] ;
      isN=false;
    }
  }

  if( items.size() == 0 )
  {
    items.push_back("");
    itemPos.push_back(0);
  }

  if( isStripSides )
  {
    for( size_t l=0 ; l < items.size() ; ++l )
      items.back() = hdhC::stripSides(items.back(), stripSides) ;
  }

  isDecomposed=true;

  return ;
}

void
Split::doIgnore( std::string x )
{
  std::string xx;

  for( size_t j=0 ; j < ignore.size() ; ++j )
  {
    xx.clear();
    size_t sz = ignore[j].size() ;

    if( sz == 1 )
    {
      // ignore plain character
      for( size_t i=0 ; i < x.size() ; ++i )
      {
         if( x[i] == ignore[j][0] )
           continue;
         xx += x[i];
      }
    }
    else
    {
      // ignore strings
      for( size_t i=0 ; i < x.size() ; ++i )
      {
         if( x[i] == ignore[j][0] )
         {
           if( x.substr(i,sz) == ignore[j] )
           {
             i += (sz - 1) ;
             continue;
           }
           else
             xx += x[i];
         }
         else
          xx += x[i];
      }
    }

    x = xx ;
  }

  str = x ;
}

void
Split::exceptionError(std::string str)
{
  // Occurrence of an error stops the run immediately.
  xcptn.strError = "error_SPLIT.txt" ;

  // open file for appending data
  if( ! xcptn.ofsError )
    xcptn.ofsError
       = new std::ofstream(xcptn.strError.c_str(), std::ios::app);

  *xcptn.ofsError << str << std::endl;

  exit(1);
  return ;
}

std::vector<std::string>&
Split::getItems(void)
{
  if( ! isDecomposed )
    decompose();

  return items;
}

std::string
Split::getItemsStr(std::string sp)
{
  if( ! isDecomposed )
    decompose();

  std::string s;
  for( size_t i=0 ; i < items.size() ; ++i )
  {
      if( s.size() )
          s += sp ;
      s += items[i] ;
  }

  return s;
}

std::string
Split::getLine(void)
{
  std::string s;
  for( size_t i=0 ; i < items.size() ; ++i)
    s += items[i] ;
  return s;
}

void
Split::init(void)
{
  xcptn.ofsError=0;

  isAlNum=false;
  isDecomposed=false;
  isEmptyItemsEnabled=false;
  isFixedWidth=false;
  isItemsWithSep=false;
  is_valid=false;
  isStripSides=false;

  return;
}

void
Split::insert(size_t i0, std::string t)
{
  if( ! isDecomposed )
    decompose();

  if( i0 < items.size() )
    items.insert(items.begin()+i0, t);
  else
    items.push_back(t);

  return;
}

bool
Split::isDigit(unsigned char c)
{
//  if(  c == '.' || c == '-' || c == '+'  )
//    return true;

  int i = c ;

  i -= 47 ;

  if( i > 0 && i < 11 )
    return true;
  else
    return false;
}

bool
Split::isNumber( size_t index )
{
  if ( index >= size() )
    return false;

  double val;

  return toDouble( index, val);
}

void
Split::getFixedFormat(void)
{
  size_t pos=0;

  while( pos < str.size() )
  {
    items.push_back( str.substr(pos,fixedWidth) );
    itemPos.push_back(pos);
    pos += fixedWidth;
  }

  return ;
}

std::string
Split::range(size_t i0, size_t i1 )
{
  // Get the substring between indicated separator positions.
  // The default for the second position is the end of the input string.
  // This holds also for a too high value of i1.
  if( i0 >= itemPos.size() )
    return "";
  else if( i1 == static_cast<size_t>(-1) || itemPos[ itemPos.size() - 1 ] < (i1+1) )
    return str.substr(itemPos[i0]);
  else
    return str.substr(itemPos[i0], itemPos[i1+1]);
}

void
Split::replace(size_t i0, std::string t)
{
  if( ! isDecomposed )
    decompose();

  if( i0 < items.size() )
    items[i0] = t;

  return;
}

void
Split::setIgnore( std::string s, bool isStr)
{
  isDecomposed=false;
  ignore.clear();

  addIgnore(s, isStr);

  return ;
}

void
Split::setProtector( std::string s, bool eachCharIsSep)
{
  isDecomposed=false;
  protector.clear();

  addProtector(s, eachCharIsSep);

  return ;
}

void
Split::setSeparator( std::string s, bool eachCharIsSep)
{
  isDecomposed=false;
  sep.clear();

  addSeparator(s, eachCharIsSep);

  return ;
}

size_t
Split::size(void)
{
  if( str.size() > 0 && ! isDecomposed )
    decompose();

  return items.size();
}

void
Split::swap(size_t i0, size_t i1)
{
  if( ! isDecomposed )
    decompose();

  if( i0 < items.size() && i1 < items.size() )
  {
    std::string t(items[i0]);
    items[i0] = items[i1] ;
    items[i1] = t;
  }

  return;
}

bool
Split::toDouble( size_t i, double &retVal, size_t nr)
{
  if( i >= size() )
    return false;

  size_t pos0, pos1;
  pos0 = pos1 = 0;
  return toDouble( items[i], pos0, pos1, retVal, nr) ;
}

double
Split::toDouble( size_t i, size_t nr, bool isRetZero)
{
  if( i >= size() )
    return isRetZero ? 0. : MAXDOUBLE ;

  size_t pos0, pos1;
  pos0 = pos1 = 0;
  return toDouble( items[i], pos0, pos1, nr, isRetZero);
}

bool
Split::toDouble( std::string s,
     size_t &pos0, size_t &pos1, double &retVal, size_t nr)
{
  /* s   input string
    val  number (float or int); to be found and to be returned
     nr  index of the scanned number, counting from position 1.

  If no number was found:
    isReturnValNull:      true  ==> return 0.
                Default:  false ==> return MAXDOUBLE */

  double val=0.  ;

  for( size_t n=0 ; n<nr ; n++ )
  {
    //Code is based on a snippet from K&R: The C Programming Language

    for( ; ! ( isDigit(s[pos0])  ||  s[pos0] == '-'  || s[pos0] == '+'
            ||  s[pos0] == '.' ) ; pos0++  )
    {
      if( pos0 == s.size() )
        return false ;    // nothing found
    }

    pos1=pos0;

    // where the work is done
    val = toDoubleFct( s, pos1 );
  }

  retVal = val ;

  return true ;
}
double
Split::toDouble( std::string s,
  size_t &pos0, size_t &pos1, size_t nr, bool isReturnValNull)
{
  /*  s  input string
    val  number (float or int); to be found and to be returned
     nr  index of the scanned number, counting from position 1.

  If no number was found:
    isReturnValNull:      true  ==> return 0.
                Default:  false ==> return MAXDOUBLE */

  double val=0. ;

  for( size_t n=0 ; n<nr ; n++ )
  {
    for( ; ! ( isDigit(s[pos0])  ||  s[pos0] == '-'  || s[pos0] == '+'
            ||  s[pos0] == '.' ) ; pos0++  )
    {
      if( pos0 == s.size() )
        return isReturnValNull ? 0. : MAXDOUBLE ;  // not found
    }

    // where the work is done
    pos1=pos0;
    val = toDoubleFct( s, pos1 );
  }

  return val ;
}

double
Split::toDoubleFct( std::string s, size_t &pos )
{
  int sign ,power_count=0 ;
  double power , val;

  sign = (s[pos] == '-') ? -1 : 1 ;

  if( s[pos] == '+' || s[pos] == '-')
    ++pos ;

  for( val=0.0; isDigit(s[pos]); ++pos )
    val = 10.0 * val + (s[pos] - '0');

  if( s[pos] == '.' )
    ++pos ;

  for( power = 1. ; isDigit( s[pos] ); ++pos )
  {
    val = 10.0 * val + (s[pos] - '0' );
    power *= 10.0;
    power_count++;
  }

  // any E format?
  if( s[pos]=='e' || s[pos]=='E' )
  {
    ++pos;
    val = sign * val / power ;

    bool isSign = s[pos] == '+' || s[pos] == '-';

    if( ((pos+1) == s.size()) || (isSign && !isDigit(s[pos+1])) )
    {
      --pos ; // the detected 'e' or 'E' is not part of the number
              // although a sign follows, but without any digit
      return val;
    }

    if( ! ( isSign || isDigit(s[pos]) ) )
    {
      --pos ; // the detected 'e' or 'E' is not part of the number
      return val;
    }

    std::string t(s.substr(pos));

    // get power by a new query
    size_t hpos=0;
    val *= pow( 10. , toDoubleFct(t, hpos) );

    pos += hpos ;
/*
    ++pos1;
    while( (s[pos1] == '+' || s[pos1] == '-' || isDigit(s[pos1]) ) )
      ++pos1 ;   // clear the positions of th E format
*/

    return val ;
  }

  val = sign * val / power ;

  return val ;
}

bool
Split::toInt( size_t i,  int &retVal, size_t nr)
{
  double x;
  bool b=toDouble(i, x, nr);
  if( b )
  {
     retVal = static_cast<int>(x);
     return true;
  }
  else
    return false;
}

int
Split::toInt( size_t i, bool isRetZero, size_t nr)
{
//  double x=toDouble( i, isRetZero, nr=1);
  return static_cast<int>( toDouble( i, nr, isRetZero) ) ;
}
