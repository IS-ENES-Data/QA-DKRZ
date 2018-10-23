#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "hdhC.h"
#include "brace_op.h"
#include "date.h"
#include "getopt_hdh.h"
#include "matrix_array.h"  // with sources
#include "nc_api.h"
#include "readline.h"
#include "annotation.h"

#include "hdhC.cpp"
#include "BraceOP.cpp"
#include "Date.cpp"
#include "GetOpt_hdh.cpp"
#include "NcAPI.cpp"
#include "ReadLine.cpp"
#include "Split.cpp"
#include "Annotation.cpp"

/*! \file syncFiles.cpp
\brief Get the next filename for the QA processing.

netCDF files provided on the command-line (exclusive) or on stdin
are synchronised to a command-line given target.\n
The next filename to process is printed.\n

Options: --help
\n
return:  output: \n
  0      Name of next file(s).\n
  1      "" , i.e. qa-file is up-to-date or invalid.\n
  2      Last filename if the date is older than.\n
         the end-date in the QA-result file.\n
  3      Unspecific error.\n
  4      No or invalid time properties; output filename.\n
  5      Invalid time data.\n
  6      Could not open a NetCDF data file.\n
  7      Could not open a NetCDF QA file.\n
>10      Ambiguity test failed (values accumulate):\n
  +1        misaligned dates in filenames \n
  +2        modification time check failed \n
  +4        identical start and/or end date across files \n
  +8        misaligned time-values across files \n
  +16       filenames with a mix of date formats \n
  +32       suspicion of a renewed ensemble. \n
         Output: file names sorted according to the modification time. */

class Member
{
  public:
  Member();

  void   enableModTimeNote(void){isModTimeNote=true;}
  void   enablePrintDateRange(void){isPrintDateRange=true;}
  void   enablePrintOnlyMarked(void){isPrintOnlyMarked=true;}
  void   enableNewLine(std::string &nl){newline=nl;}
  void   getModificationTime(void);
  std::string
         getFile(void){ return path + filename ; }
  std::string
         getOutput(bool printSeparationLine=false);
  bool   openNc(int ncid){ return openNc(ncid, ""); }
  bool   openNc(std::string file){ return openNc(-1, file); }
  bool   openNc(int ncid, std::string file);
  void   print(bool printSeparationLine=false);
  void   putState(std::string, bool printDateRange=true);
  void   setBegin(Date);
  void   setEnd(Date);
  void   setFile(std::string&); // decomposes
  void   setFilename(std::string f){ filename = f;}
  void   setPath(std::string &p){ path = p;}

  std::string  filename;
  std::string  path;
  std::string  state;
  std::string  newline;

  bool         isPrintOnlyMarked ;
  bool         isPrintDateRange;
  bool         isModTimeNote;

  Date        refDate;
  Date        begin;
  Date        end;
  long        modTime;

  NcAPI       nc;
} ;

class Ensemble
{
   public:
   Ensemble();

   void   addTarget( std::string &qa_target );
   void   enablePrintEnsemble(void) {isPrintEnsemble=true;}
   void   enablePrintOnlyMarked(void){isPrintOnlyMarked=true;}
   void   enablePrintDateRange(void){isPrintDateRange=true;}
   void   enableNewLine(std::string &nl){newline=nl;}
   std::string
          getDateSpan(void);
   std::string
          getOutput(void);
   int    getTimes(std::string &, bool is=false);
   int    getTimes_NC(Member &);
   int    getTimes_FName(Member &);
   int    print(void);
   void   printDateSpan(void);
//   void   setAnnotation( Annotation *n ) { notes = n ;}
   void   setPath( std::string &p){path=p;}
   void   sortDate(void);
   int    testAmbiguity(std::string &,
                bool isOnlyMarked=false, bool isMod=false, bool isNoMix=true);

   bool   isContinuousAlignment;
   bool   isInvalid;
   bool   isFormattedDate;
   bool   isNoRec;
   bool   isPrintEnsemble;
   bool   isPrintOnlyMarked ;
   bool   isPrintDateRange;
   bool   withTarget;

   size_t startIndex;  // default: 0, modifiable by a time-limit.
   size_t sz ;         // end of effective ensemble, which is modifiable.
   size_t last;        // the index of the target appended to the ensemble

   std::string           newline;
   std::string           path;
   std::string           frequency;
   std::vector<Member*>  member ;

   Date        refDate ;
// Annotation           *notes;
} ;

class SyncFiles
{
   public:
   SyncFiles( Ensemble &);

   void enableMixingRefused(void)       {isMixingRefused=true;}
   void enableModificationTimeTest(void){isModificationTest=true;}
   void enablePrintDateRange(void)      {ensemble->enablePrintDateRange();}
   void enablePrintEnsemble(void)       {ensemble->enablePrintEnsemble();}
   void enablePrintTotalTimeRange(void) {isPeriod=true;}
   void enableNewLine(std::string nl)   {ensemble->enableNewLine(nl);}
   void description(void);
   int  print(void);
   int  printTimelessFile(std::string &);
   void readInput(void);
   void readArgv(int optInd, int argc, char *argv[]);
   int  run(void);

   void setTarget(std::string q)        {qa_target=q;}
   void setPath(std::string &p);
   void setQA_target(std::string s)     {qa_target=s;}

   std::string path;
   std::string qa_target ;

   bool isAddTimes;
   bool isFNameAlignment;
   bool isInvalid;
   bool isMixingRefused;
   bool isModificationTest;
   bool isPeriod;
   bool isPrintOnlyMarked;

   int  returnValue;

   Ensemble   *ensemble;
//   Annotation *notes;
} ;

int main(int argc, char *argv[])
{
  Ensemble ensemble;

  // Note that ensemble is internally converted to pointer-type
  SyncFiles syncFiles(ensemble);

  std::string noteOpts;
//  Annotation *notes=0;

  std::string annotText;
  std::string outputText;

  std::string path;

  if( argc == 1 )
  {
    syncFiles.description();
    return 3;
  }

  GetOpt opt;
  opt.opterr=0;
  int copt;
  std::string str;
  std::string str0;
  std::string oStr("AEhl:mMP:p:ST");
  oStr += "<--calendar>:";
  oStr += "<--continuous>";
  oStr += "<--fname-alignment>";
  oStr += "<--freq>:";
  oStr += "<--only-marked>";
  oStr += "<--help>";
  oStr += "<--line-feed>:";
  oStr += "<--note>:";

  ensemble.refDate.setCalendar("proleptic_gregorian");

  while( (copt = opt.getopt(argc, argv, oStr.c_str() )) != -1 )
  {
    if( opt.longOption > 0 )
    {
      str0=opt.longOption;

      if( str0 == "--help" )
      {
        syncFiles.description();
        return 3;
      }

      if( str0 == "--calendar" )
        ensemble.refDate.setCalendar(opt.optarg);

      else if( str0 == "--continuous" )
        ensemble.isContinuousAlignment=true;

      else if( str0 == "--fname-alignment" )
        syncFiles.isFNameAlignment=true;

      else if( str0 == "--freq" )
        ensemble.frequency=opt.optarg;

      else if( str0 == "--line-feed" )
        syncFiles.enableNewLine(opt.optarg);

      else if( str0 == "--note" )
        noteOpts=opt.optarg;

      else if( str0 == "--only-marked" )
        syncFiles.isPrintOnlyMarked=true;

      else
      {
          std::cerr << "syncFiles.x: unknown option " + str0 << std::endl;
          exit(1);
      }

      continue;
    }

    switch ( copt )
    {
      case 'A':
        //obsolete
        break;
      case 'E':
        syncFiles.enablePrintEnsemble();
        break;
      case 'h':
        syncFiles.description();
        return 3;
        break;
      case 'm':
        syncFiles.enableMixingRefused();
        break;
      case 'M':
        syncFiles.enableModificationTimeTest();
        break;
      case 'P':
        path = opt.optarg ;
        syncFiles.setPath(path) ;
        break;
      case 'p':
        syncFiles.setQA_target(opt.optarg) ;
        break;
      case 'S':
        // printing date range of each file in output requested
        syncFiles.enablePrintDateRange();
        break;
      case 'T':
        syncFiles.enablePrintTotalTimeRange() ;
        break;
      default:
        std::ostringstream ostr(std::ios::app);
        ostr << "syncFiles:getopt() unknown option -" ;
        ostr << copt;
        ostr << "\nreturned option -" << copt ;
        std::cout << ostr.str() << std::endl ;
        break;
    }
  }

  // Note= argc counts from 1, but argv[0] == this executable
  if( opt.optind == argc )
    syncFiles.readInput();
  else
    syncFiles.readArgv(opt.optind, argc, argv);

  // this does all
  return syncFiles.run();
}


Ensemble::Ensemble()
{
  isContinuousAlignment  = false;
  isFormattedDate  = false;
  isInvalid        = false;
  isNoRec          = false;
  isPrintEnsemble  = false;
  isPrintOnlyMarked= false;
  isPrintDateRange = false;
  withTarget       = false;

  startIndex  = 0;
  newline="\n";
}

void
Ensemble::addTarget( std::string &qa_target )
{
  // test for existance
  std::string testFile("/bin/bash -c \'test -e ") ;
  testFile += qa_target ;
  testFile += '\'' ;

  // see 'man system' for the return value, here we expect 0,
  if( system( testFile.c_str()) )
  {
     // we assume that we start a series; so there is no target
     return;
  }

  member.push_back( new Member );
  member.back()->setFile(qa_target) ;
  withTarget = true;
  last=sz ;

  return ;
}

std::string
Ensemble::getDateSpan(void)
{
   std::string span("PERIOD=");

   span += member[startIndex]->begin.str();
   span += '_' ;
   span += member[sz-1]->end.str();
   span +="\n";

   return span ;
}

std::string
Ensemble::getOutput(void)
{
  if( isPrintEnsemble )
  {
    startIndex=0;
    sz=member.size();
    enablePrintDateRange();
//    isPrintOnlyMarked=false;
  }

  std::string s;
  bool printSepLine=false;

  for( size_t i=startIndex ; i < sz ; ++i)
  {
    if( isPrintOnlyMarked )
      member[i]->enablePrintOnlyMarked();
    if( isPrintDateRange )
      member[i]->enablePrintDateRange();
    if( newline.size() )
      member[i]->enableNewLine(newline);

    if( withTarget && i == (member.size()-1) )
      printSepLine=true;

    s += member[i]->getOutput(printSepLine) ;
  }

  return s ;
}

int
Ensemble::getTimes(std::string &str, bool isFNameAlignment)
{
  // reading netcdf files or use only filename info

  int retVal=0;

  std::string begStr;
  std::string endStr;

  size_t ens_sz = member.size();
  if( withTarget )
      --ens_sz;

  for( size_t i=0 ; i < member.size() ; ++i )
  {
    Member &mmb = *(member[i]);

    if( isFNameAlignment )
    {
        if( i < ens_sz )
        {
          // this mode doesn't need a target, even if there was one.
          retVal = getTimes_FName(mmb) ;

          if( retVal == -1 )
             return 0;  // fixed file
        }
    }
    else
    {
        int retVal = getTimes_NC(mmb);
        mmb.nc.close();

        if( retVal == -1 )
        {
           isNoRec=true;
           return 0;  // fixed file
        }
        else if( retVal == 4 )
           isNoRec=true;
    }

    if( mmb.state.size() )
        isInvalid=true;
  }

  // any serious conditions
  if( isInvalid )
  {
     // only files with annotations
     enablePrintOnlyMarked();
     str = getOutput();

    if( retVal )
      return retVal;
    else
      return 3 ;  // unspecific error
  }

  // get modification times of files
  for(size_t i=0 ; i < member.size() ; ++i )
    member[i]->getModificationTime() ;

  // sorted vector of pointers of dates
  sortDate();

  return 0;
}

int
Ensemble::getTimes_FName(Member &mmb)
{
    int retVal=0;

    std::string fName;
    size_t pos;
    if( (pos=mmb.filename.rfind('.')) < std::string::npos )
        fName = mmb.filename.substr(0,pos) ;
    else
        fName = mmb.filename ;

    Split y_fName(fName,'_');
    Split x_fName(y_fName[y_fName.size()-1],'-');

    size_t ix_0, ix_1;
    if( x_fName.size() > 1 )
    {
        ix_1=x_fName.size()-1 ;
        ix_0=ix_1-1 ;

        if( ! hdhC::isDigit(x_fName[ix_1]) )
            return -1;
        else if( ! hdhC::isDigit(x_fName[ix_0]) )
            ix_0 = ix_1 ;
    }
    else if( x_fName.size() > 0 )
    {
        ix_1=x_fName.size()-1 ;

        if( ! hdhC::isDigit(x_fName[ix_1]) )
            return -1;
        ix_0=ix_1 ;
    }
    else
      return -1;

    std::string tBegin(x_fName[ix_0]);

    mmb.begin = refDate;
    mmb.begin.setFormattedDate();
    // extended when specified down to sub-day
    mmb.begin.setFormattedRange("");

    if( ! mmb.begin.setDate(tBegin) )
    {
       mmb.putState("invalid data, found " + tBegin);
       return 5;
    }

    retVal=0;
    if( ix_0 == ix_1 )
      mmb.end = mmb.begin ;
    else
    {
      std::string tEnd(x_fName[ix_1]);

      mmb.end = refDate;
      mmb.end.setFormattedDate();

      if( frequency.substr(0,2) == "yr" )
        mmb.end.setFormattedRange("Y+"); // extended
      else if( frequency.find("mon") < std::string::npos )
        mmb.end.setFormattedRange("M+"); // extended
      else if( frequency.find("day") < std::string::npos )
        mmb.end.setFormattedRange("D+"); // extended
      else if( frequency == "6hr")
        mmb.end.setFormattedRange("h+"); // extended

      if( ! mmb.end.setDate(tEnd) )
      {
         mmb.putState("invalid data, found " + tEnd, false);
         retVal = 5;
      }
    }

    return retVal;
}

int
Ensemble::getTimes_NC(Member &mmb)
{
    if( ! mmb.openNc(mmb.getFile()) )
    {
       mmb.state = "could not open file";

       std::string fName( mmb.filename );

       if(withTarget)
           return 7;
       else
           return 6;

       return 76;
    }

    // no unlimited variable found; this is not
    // neccessarily an error.
    std::string ulName(mmb.nc.getUnlimitedDimName());
    if( ulName.size() == 0 )
    {
       if( sz == 1 )
       	 return -1; // a single file

       mmb.state += "missing time dimension";
       return 4;
    }

    if( mmb.nc.getNumOfRecords() == 0 )
    {
       // this could be wrong
       // or just a fixed variable with time dimension defined.
       if( sz == 1 )
      	  return -1;

       mmb.state = "no time values available";
       return 4;
    }

    std::string vName(mmb.nc.getUnlimitedDimRepName() );
    if( vName.size() == 0 )
    {
       mmb.state = "no time variable";
       return 4;
    }

    // get refdate (each member could have its own)

    // calendar; None by default
    std::string att_cal(mmb.nc.getAttString("calendar", vName));

    if( att_cal.size() )
        mmb.refDate.setCalendar(att_cal);

    // units; a missing one is a fault
    std::string att_units(mmb.nc.getAttString("units", vName));
    if( att_units.size() )
    {
        if( ! mmb.refDate.setDate(att_units) )
        {
          mmb.state = "invalid units";
          mmb.nc.close();
          return 4;
        }
    }
    else
    {
        mmb.state = "no units";
        mmb.nc.close();
        return 4;
    }

    // time values: fist and last
    MtrxArr<double> ma_t;
    double tBegin, tEnd ;

    if( (tBegin=mmb.nc.getData(ma_t, vName,0)) == MAXDOUBLE )
    {
        if( sz > 1 )
        {
           mmb.state = "could not read time values";
           return 5;
        }
    }

    if( ma_t.validRangeBegin.size() == 1  )
    {
        if( ma_t.validRangeBegin[0] != 0 )
        {
           mmb.putState("first time value equals _FillValue");
           return 5;
        }

        // note validRangeEnd points to memory after the last element
        if( ma_t.validRangeEnd[0] != ma_t.size() )
        {
           mmb.putState("last time value equals _FillValue");
           return 5;
        }
    }
    else
    {
        mmb.putState("all time values equal _FillValue");
        return 5;
    }

    if( ! mmb.refDate.isValid(tBegin))
    {
       mmb.putState("invalid data, found " + hdhC::double2String(tBegin), false);
       return 5;
    }

    mmb.setBegin( mmb.refDate.getDate(tBegin) );

    size_t sz=mmb.nc.getNumOfRows(vName) ;
    if(sz)
        --sz;

    tEnd=mmb.nc.getData(ma_t, vName, -1);

    if( ! mmb.refDate.isValid(tEnd))
    {
       mmb.putState("invalid data, found " + hdhC::double2String(tEnd), false);
       return 5;
    }

    mmb.setEnd( mmb.refDate.getDate(tEnd) );

    return 0;
}

int
Ensemble::print()
{
  std::string s( getOutput() );

  std::cout << s ;

  if( s.size() == 0 )
    return 1;

  return 0;
}

void
Ensemble::printDateSpan(void)
{
   std::string span;
   span  = member[startIndex]->begin.str();
   span += '_' ;
   span += member[sz-1]->end.str();

   std::cout << "PERIOD=" << span << std::endl;
   return ;
}

void
Ensemble::sortDate(void)
{
  // sort member coresponding to dates (without target)
  // sort dates from old to young
  for( size_t i=0 ; i < sz-1 ; ++i)
  {
    for( size_t j=i+1 ; j < sz ; ++j)
    {
      // 'greater than' means later
      if( member[i]->begin > member[j]->begin )
      {
        Member *tmp=member[i];
        member[i]=member[j];
        member[j]=tmp;
      }
    }
  }

  // synchronise with target; note that last points to the target
  if( withTarget )
  {
    if( member[sz-1]->end  ==  member[last]->end )
       startIndex=sz;  // endo of the sequence was already reached
    else
    {
      for( size_t i=0 ; i < sz ; ++i )
      {
        if( member[i]->begin  >  member[last]->end )
        {
          startIndex = i;
          break;
        }
      }
    }
  }

  return;
}

int
Ensemble::testAmbiguity( std::string &str,
   bool is_only_marked,
   bool isModificationTest,
   bool isMixingRefused )
{
  // this function operates on the time-sorted files

  //detect a renewed data set testing internal time values
  if( withTarget )
  {
    // Note that last points to the target
    if( member[last]->begin  >  member[sz-1]->end )
    {
      startIndex = sz-1;
      sz = member.size();
      enablePrintOnlyMarked();

      str = getOutput();
      return 32;
    }
  }

  // may have added 10 eventually, depending on the type of error
  int returnVal=0;

  // test modification time against the target
  if( isModificationTest && withTarget )
  {
    int rV=0;
    for( size_t i=0 ; i < sz ; ++i)
    {
       // note: 'later than' is equivalent to 'greater than'
       if( member[last]->modTime  <  member[i]->modTime )
       {
         enablePrintEnsemble();
         member[i]->enableModTimeNote();

         member[i]->putState("modification time") ;
         if(!rV)
         {
           rV=2;
           returnVal += rV;
         }
       }
    }
  }

  // check the correct sequence of dates in the filename
  std::string t;
  std::string t0_b;
  std::string t0_e;
  std::string t1_b;
  std::string t1_e;
  Split splt;
  splt.setSeparator('-');

  bool isRange=false;
  bool isSingular=false;

  // check ascending begin and end dates from filenames, respectively.
  for( size_t i=0 ; i < sz ; ++i)
  {
     t1_b.clear();
     t1_e.clear();

     size_t pos=member[i]->filename.rfind('_') + 1  ;
     t=member[i]->filename.substr( pos ) ;

     if( (pos=t.rfind(".nc")) < std::string::npos )
       t=t.substr(0,pos);

     splt=t;  // separate the dates

     // ensures that filenames without dates work
     t1_b='X';
     t1_e='X';

     if( splt.size() > 0 )
     {
       if( hdhC::isDigit( splt[0] ) )
          t1_b = splt[0] ;

       if( splt.size() == 2 )
       {
         if( hdhC::isDigit( splt[1] ) )
            t1_e = splt[1] ;
         isRange=true;
       }
       else  // singular date
       {
          t1_e = t1_b;
          isSingular=true;
       }
     }

     if( i > 0 && t0_e > t1_b )
     {
       member[i-1]->putState("misaligned filename dates");
       member[i]->putState("misaligned filename dates");

       returnVal += 1;
       break;
     }

     t0_b=t1_b;
     t0_e=t1_e;
  }

  // Detection of a mix of ranges of dates and singular dates in filenames
  if( isMixingRefused && isSingular && isRange )
  {
       enablePrintEnsemble();
       str = getOutput();

       returnVal += 16;
  }

  // check temporal coverage; any identical begin or end?
  for( size_t i=1 ; i < sz ; ++i)
  {
     int rV=0;
     if( member[i-1]->begin == member[i]->begin )
     {
       enablePrintEnsemble();

       member[i-1]->putState("identical begin");
       member[i]->putState("identical begin");

         if(!rV)
         {
           rV=4;
           returnVal += rV;
         }
     }
     if( member[i-1]->end == member[i]->end )
     {
       enablePrintEnsemble();

       member[i-1]->putState("identical end");
       member[i]->putState("identical end");

       if(!rV)
       {
         rV=4;
         returnVal += rV;
       }
     }
  }

  // test for overlapping temporal coverage
  for( size_t i=1 ; i < sz ; ++i)
  {
     int rV=0;
     if( member[i-1]->end > member[i]->begin )
     {
       enablePrintEnsemble();

       member[i-1]->putState("misaligned time across files");

       if(!rV)
       {
         rV=8;
         returnVal += rV;
       }
     }
  }

  if( isContinuousAlignment )
  {
    for( size_t i=1 ; i < sz ; ++i)
    {
       int rV=0;
       if( member[i-1]->end != member[i]->begin )
       {
         enablePrintEnsemble();

         member[i-1]->putState("misaligned across files");

         if(!rV)
         {
           rV=8;
           returnVal += rV;
         }
       }
    }
  }

  if( returnVal )
  {
    if( is_only_marked )
      enablePrintOnlyMarked();

    returnVal += 10;
    str = getOutput();
  }

  return returnVal ;
}

Member::Member()
{
  isPrintDateRange=false;
  isModTimeNote=false;
  isPrintOnlyMarked=false ;
  newline="\n";
}

void
Member::getModificationTime(void)
{
  struct stat buffer;
  int         status;

  status = stat(getFile().c_str(), &buffer);

  if( status == 0 )
  {
    time_t t=buffer.st_mtime ;
    modTime = t ;
  }
  else
    modTime=0L;

  return  ;
}

std::string
Member::getOutput(bool printSepLine)
{
  std::string str ;

  if( isPrintOnlyMarked && state.size() == 0 )
    return str;

  // construct the output string
  if( printSepLine )
  {
    str = "------------------------------------------------";
    str += newline;
  }

  str += filename ;

  if( isPrintDateRange )
  {
   std::string t0( begin.str() );
   if( t0.substr(0,10) != "4294962583" )
   {
     str += "  ";
     str += t0 ;
     str += " - " ;
     str += end.str() ;
   }
  }

  if( state.size() )
  {
     str += ": " ;
     str += state ;
  }

/*
  if( isModTimeNote )
  {
     double v = static_cast<double>( modTime );
     str += hdhC::double2String(v);
  }
*/

  str += newline;

  return str ;
}

bool
Member::openNc(int ncid, std::string file)
{
  if( ncid == -1 )
  {
    try
    {
      if( ! nc.open(file.c_str()) )
        throw "Exception";
    }
    catch (char const*)
    {
      // it is a feature for files built by pattern, if the
      // time frame extends the range of files.
      // Caller is responsible for trapping errors
      return false;
    }
  }

  return true;
}

void
Member::print(bool printSeparationLine)
{
  std::cout << getOutput(printSeparationLine) ;

  return ;
}

void
Member::putState(std::string s, bool is)
{
  if(state.size())
    state += ",";

  state += s;

  if( is )
      isPrintDateRange = is;
  return;
}

void
Member::setBegin(Date d)
{
 begin = d;

 return;
}

void
Member::setEnd(Date d)
{
 end = d;

 return;
}

void
Member::setFile( std::string &f)
{
  size_t pos;
  if( (pos=f.rfind('/') ) < std::string::npos )
  {
    path = f.substr(0,++pos);
    filename = f.substr(pos);
  }
  else
    filename = f;

  return ;
}

SyncFiles::SyncFiles( Ensemble &e)
{
  ensemble = &e;
//  notes=0;

  isAddTimes         = false;
  isFNameAlignment   = false;
  isInvalid          = false;
  isMixingRefused    = true;
  isModificationTest = false;
  isPeriod           = false;

  returnValue=0;
}

void
SyncFiles::description(void)
{
  std::cout << "usage: synxFiles.x [Opts] file[s]\n";
  std::cout << "netCDF files provided on the command-line\n";
  std::cout << "are synchronised to a command-line given target.\n";
  std::cout << "No option: print name of file with first date.\n";
  std::cout << "-E       Show info about all files (purpose: debugging).\n";
  std::cout << "-P path  Common path of all data files, but the target.\n";
  std::cout << "-p qa_<variable>.nc-file\n";
  std::cout << "         QA result file with path.\n";
  std::cout << "-s       Output the sequence of all remaining files.\n";
  std::cout << "-t begin_date-end_date \n";
  std::cout << "         Requires separator '-'; no spaces\n";
  std::cout << "-t [t0/]t1  As for -d, but referring to the time values.\n";
  std::cout << "         Time attributes of files must be identical.\n";

  std::cout << "--calendar    Calendar type; by defauöt proleptic-gregorian\n" ;
  std::cout << "--continuous  Continuous, formatted dates from filenames\n" ;
  std::cout << "--line-feed   Newline indicated by provided string.\n" ;
  //std::cout << "--note" ;
  std::cout << "--only-marked Issue only filnames with failed properties.\n" ;
  std::cout << "--only-fname-alignment\n" ;
  std::cout << "              Use only times from filenames.\n\n";
  std::cout << "Output: one  or more filenames.\n";
  std::cout << "return:  0  File(s) synchronised to the target.\n";
  std::cout << "         1  qa-file is up-to-date.\n";
  std::cout << "         2  File with latest date; but, \n";
  std::cout << "            the date is older than in the qa-file.\n";
  std::cout << "         3  Unspecific error.\n";
  std::cout << "         4  No unlimited dimension found.\n";
  std::cout << "        10  Ambiguity check failed (return values accumulate):\n";
  std::cout << "            +1  misaligned dates in filenames.\n";
  std::cout << "            +2  modification time check failed.\n";
  std::cout << "            +4  identical start and/or end date across files.\n";
  std::cout << "            +8  misaligned time-values across files.\n";
  std::cout << "            +16 filenames with a mix of date formats.\n";
  std::cout << "            +32 suspicion of a renewed ensemble.\n";
  std::cout << "       +50 the last file of ensemble is next or up-to-date.\n";

  std::cout << std::endl;

  return;
}

int
SyncFiles::print(void)
{
  int retVal = ensemble->print();

  if( isPeriod )
    ensemble->printDateSpan();

  return retVal;
}

int
SyncFiles::printTimelessFile(std::string &str)
{
   // true below means: print only marked entries
   int retVal;

   if( ensemble->sz > 1 )
   {  // occurence within an ensemble of files: error
     std::string key("17_1");
     std::string capt("Determination of temporal sequence of  files failed.");

     // More than a single file is an error; output filenames
     ensemble->enablePrintOnlyMarked();
     str = ensemble->getOutput() ;
     retVal=10;
   }
   else if( ensemble->sz == 1 )
   {
      if( ensemble->member[0]->state.size() )
      {
        ensemble->enablePrintOnlyMarked();
        retVal=3;  // a fixed field file, but with error
      }
      else
        retVal=4;  // a fixed field file

      str = ensemble->getOutput() ;
   }
   else
   {
      ensemble->enablePrintOnlyMarked();
      str = ensemble->getOutput() ;
      retVal=3 ;  // unspecific error
   }

   return retVal ;
}

void
SyncFiles::readInput(void)
{
  ReadLine rL;

  // now switch to std::cin
  rL.connect_cin();  // std::cin

  // read instructions
  while( ! rL.readLine() )
  {
     for( size_t i=0 ; i < rL.size() ; ++i )
     {
       ensemble->member.push_back( new Member );

       if( path.size() )
         ensemble->member.back()->setPath(path);

       ensemble->member.back()->setFilename(rL.getItem(i));
     }
  }

  // sz will be changed corresponding to the effective range
  ensemble->sz = ensemble->member.size() ;
  ensemble->last = ensemble->sz -1 ; // the real size
  return;
}

void
SyncFiles::readArgv
(int i, int argc, char *argv[])
{
  for( ; i < argc ; ++i)
  {
    ensemble->member.push_back( new Member );

    if( path.size() )
      ensemble->member.back()->setPath(path);

    ensemble->member.back()->setFilename(argv[i]);
  }

  // sz will be changed corresponding to the effective range
  ensemble->sz = ensemble->member.size() ;
  ensemble->last = ensemble->sz -1; // the real size

  return;
}

int
SyncFiles::run(void)
{
  returnValue=0;

  // we append the qa_target, if available
  if( qa_target.size()  )
    ensemble->addTarget(qa_target);

  // read dates from nc-files,
  // sort dates in ascending order (old --> young)
  // and get modification times of files.
  // Any annotation would be done there.
  std::string str;

  // isFNameAlignment is usually false
  returnValue = ensemble->getTimes(str, isFNameAlignment) ;

  if( str.size() )  // exit condition found
  {
    std::cout << str ;
    return returnValue ;
  }

  // Did any 'no-record' occur? Error cases are trapped.
  if( ensemble->isNoRec )
  {
    returnValue = printTimelessFile(str) ;
    if( str.size() )
      std::cout << str ;

    return returnValue ;
  }

  // Check for ambiguities, return 0 for PASS.
  // Safe for a single file, because this was processed before
  if( (returnValue = ensemble->testAmbiguity
        (str, isPrintOnlyMarked, isModificationTest, isMixingRefused )) )
  {
    if( str.size() )
    {
      std::cout << str ;
      return returnValue;
    }
  }

  // successful run
  if( print() == 1 )
      return 1;

  return returnValue ;
}

void
SyncFiles::setPath(std::string &p)
{
  path=p;

  if( path.size() && path[ path.size()-1 ] != '/' )
    path += '/' ;

  ensemble->setPath(path);

  return;
}
