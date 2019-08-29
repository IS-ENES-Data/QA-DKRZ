#ifndef _QA_TIME_H
#define _QA_TIME_H

#include "hdhC.h"
#include "date.h"
#include "annotation.h"

//! Quality Control Program Unit for time management and checks.

class QA;
class SharedRecordFlag;

class TimeOutputBuffer
{
  public:
  TimeOutputBuffer();
  ~TimeOutputBuffer();

  void    clear(void);
  void    flush(void);

  //! Change the flush counter; 1500 by default
  void    initBuffer(QA*, size_t next=0, size_t max=1500);
  void    setBufferType(nc_type t){bufferType=t;}
  void    setName(std::string n);
  void    setNextFlushBeg(size_t n){nextFlushBeg=n;};
  void    store(double val, double step);

  std::string name;
  std::string name_step;

  size_t  bufferCount;
  size_t  maxBufferSize;
  size_t  nextFlushBeg;

  QA* pQA;

  nc_type bufferType;
  double *buffer;
  float  *f_buffer;

  //nc_type buffer_stepType;
  double *buffer_step;
};


enum TimeTableMode
{
    INIT, UNDEF, REGULAR, ORPHAN, DISABLE, CYCLE
};
TimeTableMode timeTableMode;

class QA_Time
{
  public:

  //! Default constructor.
  QA_Time();
//  ~QA_Time();

  //! get only options from QA options relevant to class QA_Time
  void   applyOptions(std::vector<std::string> &os);

  /*! Close records for time only.*/
  bool   closeEntry(void);
  bool   entry(void);

  bool isClimatology;
  bool isFormattedDate;
  bool isMaxDateRange;
  bool isNoCalendar;
  bool isNoData;
  bool isNoProgress;
  bool isNoTBData;
  bool isRegularTimeSteps;
  bool isPrintTimeBoundDates;
  bool isReferenceDate;
  bool isReferenceDateAcrossExp;
  bool isSingleTimeValue;
  bool isTimeBounds;

  std::string name;  // the name
  std::string boundsName;
  int time_ix;    // the var-index
  int timeBounds_ix;
  bool isTime;
  nc_type timeType;
  nc_type timeBoundsType;

  double lastTimeStep;  // is set in flush()
  double refTimeOffset;
  double refTimeStep;

  Date refDate;

  double prevTimeValue ;
  double firstTimeValue;
  double currTimeValue;
  double lastTimeValue;
  double currTimeStep ;

  double prevTimeBoundsValue[2];
  double firstTimeBoundsValue[2];
  double currTimeBoundsValue[2];
  double lastTimeBoundsValue[2];

  // proleptic Gregorian, no_leap, 30day-month
  std::string calendar;  //default is empty for Gregorian
  std::string currDateStr;
  std::string dateFormat ; // only for absolute dates as time values
  std::string maxDateRange;

  hdhC::FileSplit timeTable;

  std::string fail;
  std::string notAvailable;

  void   clear(void);

  //! Check time properties.
  void   disableTimeBoundsTest(void){isTimeBounds=false;}
  void   enableTimeBoundsTest(void){isTimeBounds=true;}

  void   finally(NcAPI *);

  void   getDate(Date& , double t);
  void   getDRSformattedDateRange(std::vector<Date> &,
                   std::vector<std::string> &);
  std::string
         getName(void){ return name; }
  std::string
         getBoundsName(void){ return boundsName; }
  double getTimeValue(int rec);
  void   getTimeBoundsValues(double *pair, size_t rec);

  bool   init(std::vector<std::string>& optStr);

  /*! Absolute date encoded by a format given in time:units */
  bool   initAbsoluteTime(std::string &units);

  void   initDefaults(void);

  /*! Time relative to a reference date specified by time attributes */
  bool   initRelativeTime(std::string &units);

  //! Initialisiation of a resumed session.
  void   initResumeSession(void);

  bool   initTimeBounds(void) ;
  void   initTimeTable(void);

  void   openQA_NcContrib(NcAPI*);

  //! Parse the time_table file
  bool   parseTimeTable(size_t rec);

  void   setNextFlushBeg(size_t);
  void   setParent(QA*);

  //! Synchronise the in-file and the qa-netCDF file.
  /*! Return value==true for isNoProgress.*/
  bool   sync(void);

  //! Test time value.
  /*! This member is just the entry to different tests.*/
  void   testDate(NcAPI &);

  //! Test the time-period of the input file.
  /*! If the end-date in the filename and the last time value
      match within the uncertainty of 0.75% of the time-step, then
      the file is assumed to be completely qa-processed.
      Syntax of date ranges as given in CORDEX  DRS Syntax.*/
  bool   testPeriod(Split&);
  bool   testPeriod_regularBounds(std::vector<std::string> &sd, Date** pDates,
             std::vector<std::string> &key, std::vector<std::string> &capt,
             std::vector<std::string> &text );
  bool   testPeriod_FN_with_centre(std::vector<std::string> &sd, Date** pDates,
             std::vector<std::string> &key, std::vector<std::string> &capt,
             std::vector<std::string> &text );
  bool   testPeriodAlignment(std::vector<std::string> &sd, Date** pDates,
             std::vector<std::string> &key, std::vector<std::string> &capt,
             std::vector<std::string> &text );
  void   testPeriodPrecision(std::vector<std::string> &sd,
             std::vector<std::string> &key, std::vector<std::string> &capt,
             std::vector<std::string> &text );
  void   testPeriodPrecision_CORDEX(std::vector<std::string> &sd,
             std::vector<std::string> &key, std::vector<std::string> &capt,
             std::vector<std::string> &text );
  bool   testPeriodDatesFormat(std::vector<std::string> &sd,
             std::vector<std::string> &key, std::vector<std::string> &capt,
             std::vector<std::string> &text );

  bool   testPeriodFormat(Split&, std::vector<std::string> &sd) ;
  bool   testPeriodFormat_C56(Split&, std::vector<std::string> &sd) ;
  bool   testPeriodFormat_CORDEX(Split&, std::vector<std::string> &sd) ;

  //! Check for valid time bounds
  /*! No overlap with previous time bounds, reasonable span etc.. */
  void   testTimeBounds(NcAPI &);

  //! Check time steps
  bool   testTimeStep(void);

  // Time Table: hold the state over a record
  size_t tt_block_rec ;
  bool   tt_isBlock;
  size_t tt_index ;
  size_t tt_count_recs;
  std::string tt_id;

  Split  tt_xmode;
  std::vector<Date> tt_dateRange;

  size_t  bufferCount;
  size_t  maxBufferSize;
  size_t  nextFlushBeg;
  nc_type bufferType;

  std::string ANNOT_ACCUM;

  SharedRecordFlag sharedRecordFlag;
  TimeOutputBuffer timeOutputBuffer;

  Annotation *notes;
  InFile *pIn;
  QA *pQA;
};
#endif
