#ifndef _NOTIFICATION_H
#define _NOTIFICATION_H

#include <map>
#include "iobj.h"
#include "brace_op.h"

/*! \file annotation.h
 \brief Annotate QA exceptions.
*/

//! Annotation QA exceptions.
/*! A file with user supplied directives is parsed to
    respond for any annotation/task. An example of such a file
    is given by file QA-0.4/tables/projects/CMIP5/CMIP5_check-list.conf.
    Flags within the file have to be specified in the QA class in order
    to identify conditions for annotation.\n
    The methods inq() and operate() are applied paired off: inq() inquires
    a task to be made, the method operate() performs it. In between any
    non-annotation program-unit may be performed in order to determine
    text transferred to operate().\n
    Issued directives distinguish different levels
    (L1-L4), cancelling and marking for email messages.
    This can be done for each variable. Specification of
    a certain level alone as well as of a common directive
    is optional.\n
    Components of a directive: L1, L2, L3, D, EM, VR, PT, flag, var, V=value\n
    L1        Notifiy pecularities. Continue unrestricted.\n
    L2        Continue checking to the end of the file. Then, lock
       processing of the current variable.\n
    L3        Stop immediately (but complete meta-data checks). Lock
       processing of current variable.\n
    L4        Stop immediately and shutdown the QA session (no locks).\n
    Table:    Standard Output Table (VR), Project Table (PT)\n
    Flag:     Given in column 1. Must match in the QA.cpp class.\n
    Task:     Email notification (EM), discard the check/test (D)\n
    Variable: A list of comma-separated acronyms of variables; directive is \n
              only applied to the variables (for all by default).\n
    Value:    Constraining value, e.g. {flag,D,V=0,var} discards a test for \n
              variable var only if value=0.\n
    Record:   Exclude record(s) from being checked. A range may be specified, e.g. 1-12.\n
              Multiple occurrences of R in a braced statement are possible.\n
              Note: counting of numbers starts with one.
    Default directives: {VR,L1}, {PT,L2,EM}, {L3,EM}.
   */
class Annotation : public IObj
{
  public:
  //! Default constructor.
  Annotation(Annotation *p=0);

  //! coresponding to virtual methods in IObj
    bool         closeEntryTime(void){return false;}
    bool         entry(void){return false;}
    bool         init(void) ;
    void         linkObject(IObj *){;}
    void         setFilename(hdhC::FileSplit& f) {file=f;}
    void         setFilename(std::string f){file.setFile(f);}
    void         setProject(std::string s){project=s;}
    void         setTablePath(std::string p){ tablePath=p; }

  //! Options are parsed
  void applyOptions(void);
  void config(void);
  void copyInit(Annotation *);
  void eraseAnnotation( std::string str, std::string name="") ;
  bool findAnnotation(std::string tag, std::string name="");
  bool findIndex(std::string &key, bool only=false);
  std::vector<std::string>
       getAnnotation(std::string tag);
  std::vector<std::string>
       getAnnotation(std::string tag, std::vector<std::string>& txt);
  std::string
       getCheckStatus(bool with_rank=false);
  int  getExitState(int e=-1);
  int  getFlag(void);
  void initDefaults(void);

  //! Inquiry of tasks.
  /*! Flag indicates a condition for a specific annotation. If no task
      was specified for a particular variable name,
      then a default is applied. 'mode' is composed of strings
      'INQ_ONLY' and 'NO_MT', separated by '|' if both are set. \n
      ACCUM    NO_MT with accumulated text messages \n
      INQ_ONLY for simulation without triggering an action \n
      NO_MT    disables multiple identical tags \n
      NO_TXT   disable the default text from the check-list table.
      */
  bool inq( std::string flag, std::string var_name="", std::string mode="");

  //! Conduction of an annotation.
  /*! Strings caption, text, and where supply a brief description,
      a detailed explanation of the condition rising an annotation,
      and the C++ method, where it happened; mail_subject and
      mail_body overrule the default procedure for email notification.
      A leading ! may suppress the upper-case conversion of the initial character
      of caption */
  bool operate(std::string caption, std::string text,
          std::string mail_subject="", std::string mail_body="") ;
  bool operate(std::string caption, std::vector<std::string>* text=0,
          std::string mail_subject="", std::string mail_body="") ;
  //! Parse directives.
  void parse();
  //! Output check results.
  /*! Triggers internally print methods.*/
  void print(void);
  //! Print a brief check result.
  /*! Meta-data, time data and variable data are given a PASS or FAIL.*/
  void printCheckResult(void);
  void printEMail(void) ;
  //!  Print a brief description for each condition raising an annotation.
  void printFlags(void);
  void printHeader(std::ofstream*);
  //! Detailed description of a condition raising an annotation.
  void printNotes(std::string&, std::string&, std::vector<std::string>*,
                  bool is=false );

  //! Insert a directive internally aside a check-list file.
  void push_back(std::string pf_code, std::string pf_var,
          std::vector<std::string> &pf_frq, std::string pf_level,
          std::string pf_task, std::string pf_text,
          std::vector<std::string> &pf_value,
          std::vector<size_t> &xRec_0, std::vector<size_t> &xRec_1 );
  void push_front(std::string pf_code, std::string pf_var,
          std::vector<std::string> &pf_frq, std::string pf_level,
          std::string pf_task, std::string pf_text,
          std::vector<std::string> &pf_value,
          std::vector<size_t> &xRec_0, std::vector<size_t> &xRec_1 );

  //! Read a check-list file.
  void readConf(void);

  //int: rank; -1 not provided
  void setCheckStatus(std::string id, std::string s="PASS", bool add=false);

  void setConfVector(std::string txt, std::string brace);
  void setConstraintValue(std::string s){ constraintValue=s;}
  void setConstraintFreq(std::string s){ constraintFrq=s;}
  void setCurrentRecord(size_t i){ currentRecord=i;}

  void setUseAlways(std::string s, std::string mode="always");
  std::vector<std::string> options;
  std::vector<std::string> descript;

  std::string mail_out_subject;
  std::string mail_out_body;
  int         mail_level;  // for worst-case subject

  std::string constraintFrq;
  std::string constraintValue;
  size_t      currentRecord;

  bool   isExit[5];
  size_t numOfExits;

//  private:
  std::string checkList;

  // map flags, caption and text
  std::map<std::string, std::string> mp;
  std::map<std::string, std::string> mp_capt;
  std::map<std::string, std::string> mp_lev;
  std::map<std::string, int>         mp_count;
  std::map<std::string, std::string> mp_txt;

  std::ofstream *ofsNotes;
  std::string ofs_text;

  size_t recErrCountLimit;

  hdhC::FileSplit file;

  std::string project;
  std::string tablePath;

  std::string currTable;
  // acronym corresponding to the current table
  std::string currTableAcronym;

  std::vector<std::string> code;   // from check_list.txt
  std::vector<size_t>      count;  // number of of occurences
  std::vector<std::string> level;  // L1, ... ,L4
  std::vector<std::string> task;   // D/E/S/W: disable/error/warning
  std::vector<std::string> text;   // user-provided headlines
  std::vector<std::string> var;    // acronym; name

  std::vector<std::vector<std::string> > frq;  // specified constraining frequencies
  std::vector<std::vector<std::string> > value;  // specified constraining value(s)
  std::vector<std::string>  permittedFlagBegin;

  // First and last+1 index of a range excluded from checks.
  std::vector<std::vector<size_t> > xRecord_0;
  std::vector<std::vector<size_t> > xRecord_1;

  std::vector<size_t>      effIndex;   // store all found indexes
  std::vector<std::string> effVar;     // store corresponding var-name
  size_t                   currIndex;

  size_t countMultipleTags;

  std::string currName;
  std::string useAlways;

  std::map<std::string, std::string> checkID;
  std::map<std::string, int>         checkRank;

  bool isAccumText;
  bool isCheckResultsWasPrinted;
  bool isInit;
  bool isDescriptionFromTable;
  bool isMultipleTags;
  bool isDisplay;
  bool isOutputPASS;
  bool isApply_MORE;
  bool isUseDefault;

  size_t      levelLimit;
  std::string levelLimitStr;
};

#endif
