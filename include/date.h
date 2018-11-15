#ifndef _DATUM_H
#define _DATUM_H

#include <cctype>
#include <ctime>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include <cmath>

#include "hdhC.h"
#include "split.h"

/*! \file date.h
 \brief Management of dates: relative and absolute.
*/

//! Absolute, relative date/time and the date/time of now.
/*! Assignment of absolute/reference date in different formats,
    addition of time, comparisons and retrieval of particular
    properties. Date expressed in certain time units to a reference
    date are returned as objects or strings. The German word Date is
    used instead of the English 'date' to avoid potential clashes.
    Note: The Julian day number starts at noon of a given day;
          civil day starts at midnight. Thus, the two
          must not be added to get the Julian date with decimals.
          Proleptic Gregorian calendar calculations are based on
          http://en.wikipedia.org/wiki/Julian_day.
    Note: the Gregorian calendar is mapped to the proleptic one. */

class Date
{
  friend class Julian;

  //! Class Julian: nested helper class and friend of Date.
  class Julian
  {
    friend class Date;
    Julian();
    Julian(long double d);
    Julian(long int d, double h);
    Julian( const Julian& );

    //! Assignment operator
    /*! The long (double) versions expect a Julian Day Number*/
    Julian  operator=  ( const  Julian &);
    Julian  operator=  ( long double );
    Julian  operator=  ( double d)
       { operator= (static_cast<long double>(d)) ; return *this;}

    //! Arith. operator
    /*! The long (double) versions expect a value in units of 'day'
        added/substracted from the current Julian Day Number.
        Decimals and fractions are assimilated.*/
    Julian& operator+=  ( const Julian &);
    Julian& operator+=  ( long double );
    Julian& operator+=  ( double d)
       { operator+= (static_cast<long double>(d)) ; return *this;}

    Julian& operator-=  ( const Julian &);
    Julian& operator-=  ( long double );
    Julian& operator-=  ( double d)
       { operator-= (static_cast<long double>(d)) ; return *this;}

    bool    operator==  ( const Julian &) const;
    bool    operator!=  ( const Julian &) const;
    bool    operator<  ( const Julian &) const;
    bool    operator<= ( const Julian &) const;
    bool    operator>  ( const Julian &) const;
    bool    operator>= ( const Julian &) const;

    void    add( long double );
    void    adjust(void);
    void    init(void);
    bool    isValid(long double);
    void    subtract(long double);

    //! Get a Julian Date object
    Julian  getJulianDate(void){ return *this ;}

    //! Get the current Julian Day Number
    /*! Note: jdn changes at noon; day represents time of the day.
        -0.5 corrects to the correct julian date*/
    long double
             getJulianDay(void) const
                { return jdn + time -0.5 ; }

    //! Set the Julian Day Number
    /*! The number is split into a Julian day part valid at 0h
        of the real day and a part for the hour of the real day.*/
    void     set(long double d=0.);
    void     set(long double jd, double h);

    //! Julian day number.
    /*! Represents 0h of the current real day*/
    long double jdn;

    // for the detection of wrong input: the end of the current julian cycle
    long double jdn_limit;

    //! Representation of the real day from 0h -24h in units of 'day'.
    long double time;
  } ;

  Julian jul;

  //! Helper for writing error/warning messages.
  /*! Each derived class streams warnings and error messages,
      respectively, to a file of its own.*/
  struct ExceptionStruct
  {
     std::ofstream *ofsError;
     std::string strError;
  } ;

public:
  //! Default constructor and for ISO-8601 formated string
  Date( std::string isoFormat="", std::string cal="");

/*
  Date( const char* mode, const char * geoLong);
  Date( const char* mode, double geoLong = 0.);
*/

  //!Year, month, day: mandatory; hour, min, sec by default zero
  /*! Calendar cal: proleptic_gregorian [default], equal_months,
      no-leap */
  Date( double yr, double mnth, double day,
        double hr=0., double min=0., double sec=0.,
        std::string cal="");

  //! Construction by a Julian date
  Date( long double jul, std::string cal="" );
  Date( double jul, std::string cal="");

  //! Copy constructor.
  Date( const Date&);

  ~Date();

  //! Assignment by formatted string.
  Date& operator=  ( std::string );

  //! Format by a float or int, i.e. yyyymmdd[.f]
  Date& operator=  ( double );

  //! Assignment by const Date reference.
  Date& operator=  ( const  Date &);

  double operator-  ( const  Date &);

  //! Comparison operators.
  bool   operator== ( const Date &d)
              { return jul.operator==(d.jul) ; }
  bool   operator!= ( const Date &d) const
              { return jul.operator!=(d.jul) ; }
  bool   operator<  ( const Date &d) const
              { return jul.operator<(d.jul) ; }
  bool   operator<= ( const Date &d) const
              { return jul.operator<=(d.jul) ; }
  bool   operator>  ( const Date &d) const
              { return jul.operator>(d.jul) ; }
  bool   operator>= ( const Date &d) const
              { return jul.operator>=(d.jul) ; }

  //! Add a period to the currently set Julian date.
  void   addYears( long double v );
  void   addMonths( long double v);
  void   addDays( long double v );
  void   addHours( long double v );
  void   addMinutes( long double v );
  void   addSeconds( long double v );

  void   addYears( double v )
            {addYears(static_cast<long double>(v));}
  void   addMonths( double v)
            {addMonths(static_cast<long double>(v));}
  void   addDays( double v )
            {addDays(static_cast<long double>(v));}
  void   addHours( double v )
            {addHours(static_cast<long double>(v));}
  void   addMinutes( double v )
            {addMinutes(static_cast<long double>(v));}
  void   addSeconds( double v )
            {addSeconds(static_cast<long double>(v));}

  //! Add time to the currently set Julian date.
  /*! A sub-string (year, month, day, hour, minute, second) may be
    appended to the time string or be given as separate parameter.
    If both are omitted, the represented number has the unit 'day'.
    Note: A distinctive number of characters is sufficient.
    Note: If both appended and separate units are given, then
    the appended one wins.*/
  void   addTime(std::string time, std::string unit="");

  //! Add time value to the currently set julian date.
  /*! String unit overwrites inherent unitStr. If none is set,
      applied unit is day.*/
  void   addTime(double val, std::string unit="");

  //! convert yyymmdd[.f] to ISO-8601
  static std::string
         convertFormattedToISO_8601(double f) ;
  static std::string
         convertFormattedToISO_8601(std::string) ;
  //! not static, because of dependencies
  std::string
         convertFormattedDate(std::string sd);

  //! A usage description and examples
  static void
         description(void);

  //! Disable a reference date.
  void   disableDate(void){isDateSet=false; return;}

  //! Return the calendar type in use.
  std::string
         getCalendar(void){ return currCalendar;}

  static std::string
         getCurrentDate(void){return Date::getTodayStr();}

  //! Convert val to ISO-8601.
  /*! If val is relative to a reference date, then this is returned
      in ISO-8601 format. If reference was not set before, then
      val is considered a Julian day. The bool set true states
      that val is a Julian day anyway.*/
      // val must be a Julian Date number if isRel.==false
  Date   getDate(double val, bool isJD=false) ;

  //! Get relative or absolute date.
  /*! Provide time units relative to date and return ISO-8601 string.
      If 'enableSetDate=true', then a reference date is extracted and
      set (see setDate(string) from the std::string parameter before
      a relative part is considered. In case of failure,
      'not-a-valid-date' string is returned.
      Bool isFormatted=true for formatted value like 20010102.5*/
  Date   getDate(std::string, bool isFormatted=false);

  //! Get the day of the year and month.
  double getDay( void );

  //! Get the day of the year (starts counting from 1.1. 00:00h)
  double getDayOfTheYear(void);

  double getDayOfTheYear( double y, double mo, double d,
                    double h=0, double mi=0, double s=0 );

  //! Get the decimal hour of the day
  double getDeciHour( void );

  //! Convert hour, minute and second to decimal hour
  double getDeciHour( double h, double m, double s )
            {return h + m/60. + s/3600. ; }

  //! Get the decimal month of the year.
  double getDeciMonth(void );

  //! Get the decimal month for the given parameters.
  double getDeciMonth( double y, double mo, double d,
                    double h, double mi, double s );

  //! Get decimal year.
  double getDeciYear( void );

  //! Get decimal year for the parameters.
  double getDeciYear( double y, double mo, double d,
                    double h=0, double mi=0, double s=0 );

  //! Get double representation of the integer hour.
  double getHour( void );

  std::string
         getISO_8601(void);

  std::string
         getISO_8601( double y, double mo, double d,
            double h=0, double mi=0, double s=0,
            bool isError=false);

         //! Get the julian date.
  long double
         getJulianDay(long double add=0.) const
            { return jul.getJulianDay() + add ;}
  long double
         getJulianDate(long double add=0.) const
            { return jul.getJulianDay() + add ;}

  //! Get double representation of the integer minute.
  double getMinute( void );

  //! Get double representation of the integer month.
  double getMonth( void );

  //! Get the number of days of provided year and month.
  double getMonthDaysNum( double month, double year );

  //! Get the number of days of the month.
  double getMonthDaysNum( void );

  //! Get double representation of the integer second.
  double getSecond( void );

  //! Get time lag to and in units of the relative time step.
  /*! Return time lag between 'd' minus date stored in the obj.
      The reference date may be given in the string additionally
      to time unit and a relative date. If 'ref' is omitted, a
      reference date must have been set previously.*/
  double getSince(const char *d, const char *ref="");

  //! Get time lag to and in units of the relative time step.
  double getSince(std::string &d, const char *ref="");

  //! Get time lag to and in units of the relative time step.
  double getSince(Date &d, const char *ref="");

  //! Return the real current date.
  static std::string
         getTodayStr(void);

  //! Get the unit of the reference date
  std::string
         getUnits(void){ return unitStr; }

  //! Get double representation of the current integer year.
  double getYear( void );

  //! Get number of days of the year specified as parameter.
  double getYearDaysNum( double myY );

  //! Formatted date ( only %Y%m%d[.f] )
  bool   isDateFormatted(void){ return isFormattedDate;}

  //! Inquire whether a reference date was set.
  bool   isSet( void ){ return isDateSet ;}

  bool   isValid(long double d){ return jul.isValid(d);}

  static bool
         isValidDate(std::string);

  //! Set the calendar type
  /*! Valid types: proleptic_georgian|georgian, noleap,
      equal_months|360_day. The case of the strings doesn't
      matter and the default is proleptic_georgian.
      The default of the julian date for the non-georgian types
      is zero.*/
  void   setCalendar(std::string, std::string monLengths="");

  //! Copy a date object.
  bool   setDate( const Date & );

  //! Set date from a Julian date object.
  bool   setDate( const Date::Julian & );

  //! Set date by date-encoded float or a Julian Day
  /*! Encoding:
        at present: (%Y%m%d.%f); requires previous
        setting of setFormattedDate().
      For no encoding:
        if a date was previously set, then add f to the date,
        else: f is taken into account a Julian Date.*/
  bool   setDate( double f, std::string cal="", std::string monLen="" );

  //! Set reference date.
  /*! String could be "7 minutes since 1955-01-01 00:00", i.e.
      contain a reference date and the unit of relative time.*/
  bool   setDate( std::string, std::string cal="", std::string mLen="");

  //! Set date from parameters.
  bool   setDate( double y, double mnth, double d,
                    double h=0, double min=0, double s=0 );

  void   setFormattedDate(void){isFormattedDate=true;}
  void   setFormattedRange(std::string s){formattedRange=s;}

  //! Set the occurrences of a leap month.
  /*! The number of the leap month (2 by default). Note that this is only
      effective together with leap_year. Range: 1-12
      Mostly in conjunction of NetCDF time:attribute = leap_month.*/
  void   setLeapMonth(std::string lM)
            {lY_month=static_cast<size_t>(hdhC::string2Double(lM));}

  //! Set the occurrences of a leap year.
  /*! All years that differ from this year by a multiple of four are also
      leap years; mostly in conjunction of NetCDF
      time:attribute = leap_year.*/
  void   setLeapYear(std::string lY)
            {lY_start=static_cast<int>(hdhC::string2Double(lY));}

  //! Set the lengths of the months
  /*! 12 space separated values; mostly in conjunction of NetCDF
      time:attribute = month_lengths.*/
  void   setMonthLengths(std::string monLengths);

  //! Set the precision of seconds in returned dates (def.: ss.ppp )
  void   setPrecision(int p){precision=p; return; }

  //! Set the unit of the reference date
  /*! Purpose: for a reference date without units */
  void   setUnits(std::string s, int unitSign=1);

  bool   size(void){ return isDateSet; }

  bool   shift(std::string s="");
  bool   shiftShortest(std::string s="");

  void   shiftSecondBeg(void);
  void   shiftSecondEnd(void);
  void   shiftMinuteBeg(void);
  void   shiftMinuteEnd(void);
  void   shiftHourBeg(void);
  void   shiftHourEnd(void);
  void   shiftDayBeg(void);
  void   shiftDayEnd(void);
  void   shiftMonthBeg(void);
  void   shiftMonthEnd(void);
  void   shiftYearBeg(void);
  void   shiftYearEnd(void);

  std::string  // ISO-8601 representation of the date
         str(void) ;

  void   clear(void);

  double *regularMonthDays ;

private:
  enum Calendar { GREGORIAN, PROLEPTIC_GREGORIAN, JULIAN,
                  EQUAL_MONTHS, ALL_LEAP, NO_LEAP, UNDEF } ;
  enum Calendar currCalendarEnum ;
  std::string currCalendar;

  std::string monthLengths;
  int         lY_start;
  int         lY_month;
  int         lY_yrDays;  // num of days of a non-leap year
  int         lY_cycleDays;  // num of days between consecutive leap years
  bool        lY_is;

  int precision ;

  bool         isDateSet ;
  bool         isFormattedDate;
  std::string  formattedRange;

  std::string  unitStr;
  std::string  dateCode ;  // prescribed formats
  double       refUnitSign;

  double       currYr;
  double       currMon;
  double       currDay;
  double       currHr;
  double       currMin;
  double       currSec;

  // messaging in case of exceptions.
  struct ExceptionStruct xcptn;

  Julian date2Julian( double yr, double mo,
           double d, double h, double mi, double s );

  Julian date2Julian( double y, double mo, double d, double deziH ) ;

  void   copyCurr(double y, double mo, double d, double h=0., double mi=0., double s=0.);
  void   copyCurr(const Date &);

  void   exceptionError(std::string );

  /*  double getDayLightST( void );*/
  static bool
         getFullyFormattedDateStr(std::string, std::string&);

  static void
         getDayTime(double d, double *h, double *m, double *s);
  double getLocalTimeZone( void ) ;

  Julian gregorian2Julian( double y, double mo, double d, double hour ) ;

  void   init();
  bool   isDigit(std::string);
  bool   isNotASingleDigit(std::string);
  // wrapper function to all types of calendars
  void   julian2Date( long double jul, double *yr, double *mo,
           double *d, double *h, double *mi, double *s );
  void   julian2Date( const Julian &, double *yr, double *mo,
           double *d, double *h, double *mi, double *s );

  void   julian2Gregorian( long double jul, double *yr, double *mo,
           double *d, double *h, double *mi, double *s );
  void   julian2Gregorian( const Julian &, double *yr, double *mo,
           double *d, double *h, double *mi, double *s );

  void   julian2ProlGreg( long double jul, double *yr, double *mo,
           double *d, double *h, double *mi, double *s );
  void   julian2ProlGreg( const Julian &, double *yr, double *mo,
           double *d, double *h, double *mi, double *s );

  void   julian2ModelDate( const Julian &, double *yr, double *mo,
           double *d, double *h, double *mi, double *s );
  void   localToUtc( void );

  Julian modelDate2Julian( double y, double mo=0., double d=0.,
           double h=0., double mi=0., double s=0.) ;

  bool   parseISO_8601(std::string );

  Julian prolGreg2Julian( double y, double mo, double d, double deziH ) ;

  void   setUnitsAndClear(std::string& s);
  void   string2Date(std::string &);
  void   string2ModelDate(std::string &);
};

#endif
