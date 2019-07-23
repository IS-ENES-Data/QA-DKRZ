#include "parse.h"

Parse::Parse(int argc, const char *ccarg[] )
{
  for( int i=0 ; i < argc ; ++i)
    argv.push_back( ccarg[i] );

  parse();
}

Parse::Parse(std::vector<std::string> &a)
{
  argv=a;
  parse();
  a=argv;
}

void
Parse::finally(int errCode, std::string note)
{
  // print a message to std::cout. This will be further handled by
  // the calling entity.
  if( note.size() )
    std::cout << note << ";" << std::endl;

  // in case that just a message is issued, but the program
  // is going to continue.
  if( errCode < 0)
    return;

  if( errCode > 0)
    exit(errCode);

  return;
}

std::string
Parse::getListObj( std::string &s, std::string &name, int &id)
{
   std::string param;
   return getListObj(s, name, id, param);
}

std::string
Parse::getListObj( std::string &s, std::string &name, int &id, std::string &param)
{
  char sep='\0';
  size_t pos;

  for( pos=0 ; pos < s.size() ; ++pos)
  {
    if( ! ( hdhC::isAlphaNum(s[pos]) ||  s[pos] == '/' ||  s[pos] == '\\'
                ||  s[pos] == '{' ||  s[pos] == '}' ) )
    {
      if( pos < s.size() && s[pos+1] == s[pos] )
      {
        sep=s[pos];
        break;
      }
    }
  }

  Split x_s(s,sep);
  name=x_s[0];
  id=x_s.toInt(1);

  std::string t(name);
  t += sep ; t += sep ;
  t += x_s[1];

  param = x_s.range(2);
  return t ;
}

std::string
Parse::parseObj( std::string &s, std::string &name, int &id, std::string &param)
{
  // parameter (! indicates separator):
  //      return: obj-name!!id  ( the latter possibly completed)
  //           s: input string. When leaving, the reduced by the ob-name!!id
  //        name: obj-name
  //          id:
  //         sep: separator (by default ':')
  //  isEmbedded: true when an embedded obj was found

  // syntax: XX[!!][n!][param]
  // examples: XX!!n!param                --> ret: XX!!1, name=XX, id=n
  //           XX!!param  == XX!!1!param  --> ret: XX!!1, name=XX, id=n

  // note that id==0, if no number is provided

  // find the separator of the first obj
  // first occurrence of a non-alpha (upper as well as lower case)

  // look for a double-sep; note '/', '\', {, and } are not allowed
  char sep='\0';
  size_t pos=0;
  for( ; pos < s.size() ; ++pos)
  {
    if( ! ( hdhC::isAlphaNum(s[pos]) ||  s[pos] == '/' ||  s[pos] == '\\'
              ||  s[pos] == '{' ||  s[pos] == '}'  || s[pos] == '_' ) )
    {
      if( pos < s.size() && s[pos+1] == s[pos] )
      {
        sep=s[pos];
        break;
      }
    }
  }

  std::string t;
  name.clear();
  id=0;

  if( pos == s.size() )
     return t;  // no obj found

  Split x_s;
  x_s.enableEmptyItems();
  x_s.setSeparator(sep);
  x_s = s;

  // the start of an obj description
  name=x_s[0];
  size_t ix;

  // obj_name!!id
  t += name;
  t += sep; t += sep;

  if( x_s.size() > 2 && hdhC::isDigit(x_s[2]) )
  {
     // go for the number
     t += x_s[2];
     id=x_s.toInt(2);
     ix = 3;
  }
  else
  {
    // no number; this is legal
    t += '0';
    ix=2;
  }

  size_t sz = x_s.size();

  for( ; ix < sz ; ++ix)
  {
     std::string tmp;
     std::string n_name;
     int n_id;

     // recursion

     if( (ix+1) < sz && x_s[ix+1].size() == 0 )
     {
       // NOTE: An embedded full-text obj descriptor MUST have a different separator
       //       than the one of the object it is embedded.
       //       This cannot be tested.
       tmp=x_s[ix];
       tmp += sep ; tmp += sep;
       if( (ix+2) < x_s.size() && hdhC::isDigit(x_s[ix+2]) )
       {
         tmp += x_s[ix+2] ;
         ix += 2;
       }
       else
       {
         tmp += '0' ;
         ++ix;
       }
     }
     else
       tmp = parseObj(x_s[ix], n_name, n_id, param) ;

     if( tmp.size() )
     {
       // An embedded obj was found.
       // The complete obj description is appended to argv.
       // The full text; i.e. obj, index and parameters, are replaced by an obj!!id designator,
       // which is appended to the linkList vector of the current obj
       if( param.size() )
       {
          tmp += param;  // full obj description with a different separator

          // append the embedded obj+index+text to the list of argv.
          argv.push_back( x_s[ix] );
       }

       linkList.back().push_back( tmp );
     }
     else
     {
       param += sep;
       param += x_s[ix] ;
     }
  }

  return t;
}

void
Parse::getRank(void)
{
  // a) IN obj come first
  // b) Linked obj proceed the calling obj
  std::vector<std::string> swap;
  bool hasSwapped;

  do
  {
    hasSwapped=false;

    for( size_t i=0 ; i < linkList.size()-1 ; ++i )
    {
      for( size_t k=1 ; k < linkList[i].size() ; ++k )
      {
        for( size_t j=i+1 ; j < linkList.size() ; ++j )
        {
           if( linkList[j][0].find( linkList[i][k] ) < std::string::npos )
           {
              swap = linkList[i] ;
              linkList[i] = linkList[j] ;
              linkList[j] = swap;
              hasSwapped=true;
           }
        }
      }
    }

  } while (hasSwapped) ;

  return ;
}

void
Parse::mkLinkList(void)
{
  std::string name;
  int id;
  std::string param;
  std::string s;

  // Convert embedded obj assignment to separated objects linked to
  // the owning one. Syntax of embbedded obj:
  //  'XX!![num!]=param_0[!param_1!param_2[, ... !param_n]]'
  // Character '!' denotes an optional separator which must be
  // different from the separator of
  // the owning object when the object is embedded. Nesting of objects is ok.

  for( size_t i=0 ; i < argv.size() ; ++i)
  {
    linkList.push_back( std::vector<std::string>() );
    linkList[i].push_back("");

    // get the obj to be instantiated
    param.clear();
    s = parseObj( argv[i], name, id, param);

    linkList[i][0] = s + param ;
  }

  return;
}

size_t
Parse::nextFreeObjID(std::string oStr)
{
  // If a new obj has to be generated, we need to know
  // the next free ID for the object indicated by oStr
  int objID=0;
  for( size_t i=0 ; i < argv.size() ; ++i)
  {
     std::string name;
     std::string param;
     int id;

     // take into account formally linked objects.
     Split split( argv[i],"<-");
     for( size_t i=0 ; i < split.size() ; ++i )
     {
       (void) parseObj(split[i], name, id, param) ;

       if( name == oStr )
         if( (id+1) > objID )
           objID=id+1;
     }
  }

  return static_cast<size_t>(objID);
}

void
Parse::parse(void)
{
  // define a linkList where the first string contains the instruction
  // to generate an object and the other space-separated items give
  // names of object to bve linked.
  // OBJ::n:parameter obj_0::x [obj_y::] ...
  mkLinkList();

  // replace all separators by ':' unique to all.
  unique();

  // 1-D obj sequence(s) in vector rankList
//  getRank();

  return;
}

void
Parse::printList(void)
{
  for( size_t i=0 ; i < argv.size() ; ++i)
    std::cerr << argv[i] << std::endl;
  std::cerr << std::endl;

  for( size_t i=0 ; i < linkList.size() ; ++i)
  {
    for( size_t j=0 ; j < linkList[i].size() ; ++j)
      std::cerr << linkList[i][j] << " " ;
    std::cerr <<  std::endl;
  }

  exit(1);
}

void
Parse::unique(void)
{
  // transform all separators to ':'
  char sep='\0';

  for( size_t i=0 ; i < linkList.size() ; ++i )
  {
     // get sep of the current obj-list
     for( size_t j=0 ; j < linkList[i][0].size() ; ++j )
     {
       if( ! ( hdhC::isAlphaNum( linkList[i][0][j] )
              ||  linkList[i][0][j] == '/' ||  linkList[i][0][j] == '\\') )
       {
          sep = linkList[i][0][j] ;
          break;
       }
     }

     if( sep == ':' )
        continue;

     for( size_t j=0 ; j < linkList[i].size() ; ++j )
       for( size_t k=0 ; k < linkList[i][j].size() ; ++k )
         if( sep == linkList[i][j][k] )
            linkList[i][j][k] = ':' ;
  }

  return;
}
