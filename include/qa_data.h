#ifndef _QA_DATA_H
#define _QA_DATA_H

#include "hdhC.h"
#include "annotation.h"

//! Quality Control Program Unit for checking the data section.
/*! Outlier test and such for replicated records
are performed. Annotations are supplied via the Annotation class
linked by a pointer.
*/

class QA;
class QA_Data;
class VariableMetaData ;

class DataOutputBuffer
{
  //! Buffering of record-based results.
  /*! Due to efficiency, chunks of records are investigated before
   writing any data. A vector of objects of collects the results.*/

   public:
   DataOutputBuffer();
   ~DataOutputBuffer(){ clear();}

   void clear(void);
   void    flush(void);

   //! Change the flush counter; 1500 by default
   void    initBuffer(QA*, size_t next=0, size_t max=1500);
   void    setName(std::string n){name=n;}
   void    setNextFlushBeg(size_t n){nextFlushBeg=n;};
   void    store(hdhC::FieldData &fA);

   std::string name;

   size_t  bufferCount;
   size_t  maxBufferSize;
   size_t  nextFlushBeg;

   double *min;
   double *max;
   double *ave;
   double *std_dev;
   int    *fill_count;
   int    *checksum;

   QA* pQA;
};

class Outlier
{
  public:
  Outlier( QA*, VariableMetaData*);
  ~Outlier(){;}

  static bool
        isSelected(Variable&, std::vector<std::string> &opts,
            std::string& unlimName );

  //! Evaluate outlier test
  /*! When of checking sub-temp files or in post-processing mode.
      Acknowledgement: Dr. Andreas Chlond, MPI-M Hamburg, suggested to
      exploit the function N(sigma) as criterion.*/
  void  parseOption(std::vector<std::string>&);
  void  setAnnotation(Annotation *p){notes=p;}
  void  setVMD(VariableMetaData* p){vMD=p;}
  bool  test(QA_Data*);

  std::vector<std::string> options;

  size_t vMD_ix;
  std::string name;
  MtrxArr<double> ma_d;

  Annotation *notes;
  QA *pQA;
  VariableMetaData* vMD;
} ;

class ReplicatedRecord
{
  //! Test for replicated records.
  /*! Before an array of values (e.g. ave, max ,min)
      is flushed to the qa_<filename>.nc file, the values
      in the array are tested for replicated records in
      the priviously written qa_<filename>.nc as well as
      in the array itself.*/

  public:
  ReplicatedRecord( QA*, VariableMetaData*);
  ~ReplicatedRecord(){;}

  void   getRange(size_t ix, size_t sz, size_t recNum,
            size_t *arr_rep_ix, size_t *arr_1st_ix,
            bool *arr_rep_bool, bool *arr_1st_bool,
            std::vector<std::string>& range) ;
  static bool
         isSelected(Variable&, std::vector<std::string> &options,
           std::string& unlimName );

  void   parseOption( std::vector<std::string> &opts ) ;
  void   report( std::vector<std::string> &note, size_t bufCount,
            bool isMultiple );
  void   setAnnotation(Annotation *p){notes=p;}
  void   setGroupSize(size_t i){groupSize=i;}
  void   setVMD(VariableMetaData* p){vMD=p;}
  void   test( int nRecs, size_t bufferCount, size_t nextFlushBeg,
            bool isMultiple );

  bool   enableReplicationOnlyGroups;

  std::string name;
  std::vector<std::string> options;
  size_t groupSize;

  size_t vMD_ix;
  Annotation *notes;
  QA* pQA;
  VariableMetaData* vMD;
} ;

class SharedRecordFlag
{
  public:
  SharedRecordFlag();
 ~SharedRecordFlag();

  void   adjustFlag(int num, size_t rec, int erase=0);
  void   flush(void);

  //! Change the flush counter; 1500 by default
  void   initBuffer(QA*, size_t next=0, size_t max=1500);
  void   setAnnotation(Annotation *p){notes=p;}
  void   setName(std::string n){name=n;}
  void   setNextFlushBeg(size_t n){nextFlushBeg=n;};
  void   store(void);

  size_t bufferCount;
  size_t maxBufferSize;
  size_t nextFlushBeg;

  int   *buffer;
  int    currFlag;

  std::string name;

  Annotation *notes;
  QA *pQA;
};

class QA_Data
{
  public:
  QA_Data();
  ~QA_Data();

  void   applyOptions(std::vector<std::string>& optStr);

  //! Collects the results of checked records.
  /*! Does not really print, but store in the 'qaNcOutData' object.*/
  void   checkFinally(Variable *);

  void   disableTests(std::string);

  //! The final operations.
  /*! An exit code is returned.*/
  int    finally(int errCode=0);

  void   forkAnnotation(Annotation *p);

  //! Flush results to the qa-netCDF file.
  void   flush(void);

  void   init(QA*, std::string);

  //! Change the flush counter; 1500 by default
  void   initBuffer(QA*, size_t next=0, size_t max=1500);

  //! Initialisiation of a resumed session.
  void   initResumeSession(std::string& name);

  void   openQA_NcContrib(NcAPI*, Variable *var);

  void   setAnnotation(Annotation *p);
  void   setInFile(InFile *p){ pIn=p;}

  void   setName(std::string);
  void   setNextFlushBeg(size_t n);
  void   setStatisticsAttribute(NcAPI*);
  void   setValidRangeAtt(NcAPI*, std::string&, double* arr, size_t sz, nc_type);
  void   setVar(Variable* p){var=p;}
  void   store(hdhC::FieldData &);

  void   test( Variable*, hdhC::FieldData &);
  void   testAnyFillValue(hdhC::FieldData &);
  void   testConst(hdhC::FieldData &);
  void   testInfNaN(hdhC::FieldData &);
  void   testNegativeVal(hdhC::FieldData &);
  bool   testValidity(hdhC::FieldData &);

  std::string name;
  MtrxArr<double> tmp_mv;

  size_t bufferCount;
  size_t maxBufferSize;
  size_t nextFlushBeg;

  double currMin;
  double currMax;

  double validMin;
  double validMax;

  Statistics<double> statAve;
  Statistics<double> statMin;
  Statistics<double> statMax;

  bool   allRecordsAreIdentical;

  bool   enableConstValueTest;
  bool   enableFillValueTest;
  bool   enableOutlierTest;
  bool   enableReplicationTest;
  bool   enableReplicationOnlyGroups;

  bool   isEntirelyConst;
  bool   isEntirelyFillValue;
  bool   isForkedAnnotation;

  bool                  constValueRecordState;
  std::vector<std::string>   constValueRecord;
  std::vector<double>   constValueRecordStartTime;
  std::vector<double>   constValueRecordEndTime;
  std::vector<size_t>   constValueRecordStartRec;
  std::vector<size_t>   constValueRecordEndRec;

  bool                  fillValueRecordState;
  std::vector<double>   fillValueRecordStartTime;
  std::vector<double>   fillValueRecordEndTime;
  std::vector<size_t>   fillValueRecordStartRec;
  std::vector<size_t>   fillValueRecordEndRec;

  std::string   ANNOT_ACCUM;
  std::string   ANNOT_NO_MT;
  size_t        numOfClearedBitsInChecksum;

  //! results to qa.nc
  DataOutputBuffer dataOutputBuffer;
  SharedRecordFlag sharedRecordFlag;

  Annotation *notes;
  InFile *pIn;
  Outlier *outlier;
  QA *pQA;
  ReplicatedRecord *replicated;
  Variable* var;
};

#endif
