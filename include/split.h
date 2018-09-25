#ifndef _SPLIT_H
#define _SPLIT_H

#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <cmath>

#include <values.h>

/*! \file split.h
 \brief Partitioning of a string.
*/

//! Partitioning of a string.
/*! Split a string at given separation point. Separators are by
    default spaces. Leading separators are ignored and adjacent
    are treated as one. Multiple different separators,
    also strings, can be used simultaneously. Return references
    to single sub-strings or values of various formats.*/

class Split
{
  public:
  struct ExceptionStruct
  {
     std::ofstream *ofsError;
     std::string strError;
  } ;

  //! Default consturctor.
  Split(); // separator is by default a blank

  //! Copy constructor.
  Split(const Split &);

  //! Construct and split a string at white-spaces.
  Split(std::string s);

  //! Construct and split a const char* at white-spaces.
  Split(const char* s);

  //! Construct and split a string at string sep.
  /*! By default, the string acts an individual separator;
   true for each char in string is a separator.
   Note: no white-spaces by default with this constructor.*/
  Split(std::string s, std::string sep, bool eachCharIsSep=false);

  //! Construct and split a string at char sep.
  Split(std::string s, char sep);

  //! Destructor
  ~Split();

//  ~Split() { ;}
  //! Assignment operator of a string to be split.
  void operator=( std::string s );

  //! Assignment operator of a const char* to be split.
  void operator=(  const char *s );

  //! Incremental assignment operator of a string to be split.
  void operator+=( std::string s );

  //! Subscription operator. Returns i-th sub-string.
  std::string&  operator[](size_t i);

  //! Add another character to the list of those to ignore.
  void   addIgnore( char s){addIgnore(std::string(1,s));}

  //! Add another character to the list of those to ignore.
  void   addIgnore( std::string s, bool isStr=false);

  //! Add another separator to those already set.
  void   addProtector( std::string s, bool eachCharIsSep=false);

  //! Add another separator to those already set.
  void   addProtector( char s){addProtector(std::string(1,s));}

  //! Add another separator to those already set.
  void   addSeparator( char s){addSeparator(std::string(1,s));}

  //! Add another separator to those already set.
  void   addSeparator( std::string s, bool eachCharIsSep=false);

  //! Strip leading and trailing characters
  /*! Ignored, when identical to a separation character */
  void   addStripSides(std::string s){ stripSides.push_back(s); isStripSides=true;}

  void   append(std::string);

  void   assign(size_t i, std::string);

  //! Clear previous assignment, but leaving the setting alone.
  void clear(void);

  //! Get i-th sub-string as const char*.
  const char*
         c_str(size_t i) ;

  //! Enable every adjacent separator to be active.
  /*! The class merges adjacent separators to a single one by default.
   This method enables to keep each and every separator active.
   A purpose could be reading a comma-separated table with
   empty items. Parameter b=false disables again. E.g. reading
   a table if  fixed comma-sep-columns: 'item1,item2,,,item4' */
  void   disableEmptyItems(void){isEmptyItemsEnabled=false;}
  void   enableEmptyItems(void){isEmptyItemsEnabled=true;}

  //! Erase i-th vector element
  /*! The original line stored in mem is not affected.*/
  void   erase(size_t i){items.erase(items.begin()+i);}

  //! Erase j-th char from the i-th vector element
  /*! The original line stored in mem is not affected.*/
  void   erase(size_t i, size_t j, size_t n=1)
            {items[i].erase(j, n);}

//! Error messages are written to a file. Exits immediately.
  void   exceptionError(std::string);

  //! Get a vector containing all sub-strings
  std::vector<std::string>&
         getItems(void);

  //! Get a vector containing all sub-strings
  std::string
         getItemsStr(std::string sep=" ");

  //! Get the originally input-string cleared from separators.
  std::string
         getLine(void);

  //! Get the originally input-string
  std::string
         getStr(void){ return str;}

  void   insert(size_t i, std::string);

         //! Is the i-th substring a number?
  bool   isNumber( size_t i );

  bool   isValid(void){ return is_valid;}

  //! Get the substring between indicated separator positions.
  /*! The default for the second position or, if i1 exceeds the number of items,
      is the end of the input string. If i0 exceeds the number of items, then
      an empty string is returned.*/
  std::string
         range(size_t i0, size_t i1=-1);

  //! Replace i-th sub-string by string 's'.
  void   replace(size_t, std::string);

  //! Set a fixed field width for items.
  /*! Purpose: read formatted tables without explicit separator.*/
  void   setFixedFormat( size_t w ){fixedWidth=w; isFixedWidth=true;}

  //! Discard specific character; replace former setting.
  void   setIgnore(char s){setIgnore(std::string(1,s));}

  //! Discard specific characters/strings.
  /*! By default, each character in the string is set as individual
   separator. If the entire string is a separator, isStr=true.*/
  void   setIgnore( std::string s, bool isStr=false);

  //! Each separator is contained in the separated items.
  void   setItemsWithSeparator(void){ isItemsWithSep=true;}

  //! Protect tokenising until the next protector
  void   setProtector( std::string, bool eachCharIsSep=false);

  //! Set a new separator; replace former setting.
  void   setSeparator(char s){setSeparator(std::string(1,s));}

  //! Set a new separator; discard old ones.
  /*! By default, each character in the string is set as individual
   sepaerator. If the entire string is a separator, isStr=true.
   If additionally s==:alnum:, the str and nums are separated.*/
  void   setSeparator( std::string s, bool eachCharIsSep=false);

  void   setStripSides(std::string s){stripSides.clear(); addStripSides(s);}
  void   setStripSides(void)         {stripSides.clear(); isStripSides=true;}

  //! Get number of split sub-strings.
  size_t size(void);

  //! swap items by using zero-based indeces
  void   swap(size_t, size_t);

  //! Get i-th sub-string converted to int.
  /*! Set i-th sub-string converted to int to reference 'retVal'.
   Success or failure is returned. The number 'nr' counting from 1
   inidicates the nr-th isolated number in the i-th sub-string,
   i.e. nr=2 gets 0 from string '12234_7E-10asdf18', because
   the extracted double is 7.E-10 cast to int equals 0.*/
  bool   toInt( size_t i,  int &retVal, size_t nr=1);

  //! Get i-th sub-string converted to int which is returned.
  /*! Boolean parameter inidcates whether zero or 987654321
   is returned in case of failure.*/
  int    toInt( size_t i, bool isRetZero=false, size_t nr=1);

  //! Get i-th sub-string converted to double.
  bool   toDouble( size_t i, double &retVal, size_t nr=1) ;

  //! Get i-th sub-string converted to double.
  double toDouble( size_t i, size_t nr=1, bool isRetZero=false) ;

private:
  void   decompose(void);
  void   decomposeWithProtect(void);
  void   decomposeAlNum(void);
  void   doIgnore(std::string);
  void   getFixedFormat(void);
  void   init(void);
  bool   isDigit(unsigned char );
  double toDoubleFct( std::string, size_t &pos );
  bool   toDouble( std::string ,
             size_t &pos0, size_t &pos1, double &retVal, size_t nr=1) ;
  double toDouble( std::string , size_t &pos0, size_t &pos1,
                   size_t nr=1, bool isRetZero=false) ;

  // messaging in case of exceptions.
  struct ExceptionStruct xcptn;

  bool   isAlNum;
  bool   isDecomposed;
  bool   isEmptyItemsEnabled;
  bool   isFixedWidth ;
  bool   isProtectAlNum;
  bool   isItemsWithSep;

  size_t fixedWidth;
  bool is_valid;
  bool isStripSides;

  std::string str;
  std::string empty;

  std::vector<std::string> sep;
  std::vector<std::string> ignore;
  std::vector<std::string> items;
  std::vector<size_t>      itemPos;
  std::vector<std::string> protector;
  std::vector<std::string> stripSides;
};

#endif
