#include "date.h"

Date::Date( std::string iso, std::string cal)
{
  init();

  if( iso.size() )
  {
    setCalendar(cal);
    setDate( iso);
  }
}

Date::~Date()
{
  if( regularMonthDays )
    delete[] regularMonthDays ;
}

Date::Date(
  double y, double mo, double d, double h, double mi, double s,
     std::string cal)
{
  init();

  setCalendar(cal);
  setDate( y, mo, d, h, mi, s) ;
}

Date::Date( long double d, std::string cal )
{
  init();
  setCalendar(cal);

  jul.set(d);
  julian2Date(jul, &currYr, &currMon, &currDay, &currHr, &currMin, &currSec);
}

Date::Date( double d, std::string cal )
{
  init();
  setCalendar(cal);

  jul.set( static_cast<long double>(d) );
  julian2Date(jul, &currYr, &currMon, &currDay, &currHr, &currMin, &currSec);
}

Date::Date( const Date &d )
{
  init();

  unitStr     = d.unitStr;
  refUnitSign = d.refUnitSign;
  precision   = d.precision;
  isDateSet   = d.isDateSet ;
  isFormattedDate = d.isFormattedDate;
  lY_start    = d.lY_start;
  lY_month   = d.lY_month;
  monthLengths= d.monthLengths;

  copyCurr(d);
  setCalendar(d.currCalendar);

  jul.jdn  = d.jul.jdn ;
  jul.time = d.jul.time ;
}

Date&
Date::operator=( const Date &d )
{
  init();

  unitStr     = d.unitStr;
  refUnitSign = d.refUnitSign;
  precision   = d.precision;
  isDateSet   = d.isDateSet ;
  isFormattedDate = d.isFormattedDate;
  lY_start    = d.lY_start;
  lY_month   = d.lY_month;
  monthLengths= d.monthLengths;

  copyCurr(d);
  setCalendar(d.currCalendar);

  jul.jdn  = d.jul.jdn ;
  jul.time = d.jul.time ;

  return *this;
}

Date&
Date::operator=( std::string z)
{
  setDate( z );

  isDateSet = true ;
  return *this;
}

double
Date::operator-( const Date &d )
{
  return (jul.jdn + jul.time) - (d.jul.jdn + d.jul.time) ;
}

void
Date::addDays( long double d )
{
  jul += d ;
  //jul = date2Julian(currYr, currMon, currDay, currHr, currMin, currSec);
  julian2Date(jul, &currYr, &currMon, &currDay, &currHr, &currMin, &currSec);
  isDateSet=true;

  return;
}

void
Date::addHours( long double v )
{
  jul += v/24. ;
  //jul = date2Julian(currYr, currMon, currDay, currHr, currMin, currSec);
  julian2Date(jul, &currYr, &currMon, &currDay, &currHr, &currMin, &currSec);
  isDateSet=true;

  return;
}

void
Date::addMinutes( long double v )
{
  jul += v / (24.*60.) ;
  //jul = date2Julian(currYr, currMon, currDay, currHr, currMin, currSec);
  julian2Date(jul, &currYr, &currMon, &currDay, &currHr, &currMin, &currSec);
  isDateSet=true;

  return;
}

void
Date::addMonths( long double v )
{
  // Add months, but keep the current day, e.g. adding a
  // month to 2012-02-28 will result in 2012-03-28.
  // Note: adding months to a specific date is not very useful.

  if( currCalendarEnum == EQUAL_MONTHS )
  {
     jul += v*30. ;
     return;
  }

  // irregular calendars
  currMon += v ;
  jul = date2Julian(currYr, currMon, currDay, currHr, currMin, currSec);
  julian2Date(jul, &currYr, &currMon, &currDay, &currHr, &currMin, &currSec);
  isDateSet=true;

  return;
}

void
Date::addSeconds( long double v )
{
  jul += v / ( 24.*3600.) ;
  //jul = date2Julian(currYr, currMon, currDay, currHr, currMin, currSec);
  julian2Date(jul, &currYr, &currMon, &currDay, &currHr, &currMin, &currSec);
  isDateSet=true;

  return;
}

void
Date::addTime(double time, std::string unit)
{
  if( unit.size() == 0 )
    unit = unitStr;

  if( unit[0] == 'y' )
    addYears( time );
  else if( unit.substr(0,2) == "mo" )
    addMonths( time );
  else if( unit[0] == 'd' )
    addDays( time );
  else if( unit[0] == 'h' )
    addHours( time );
  else if( unit.substr(0,2) == "mi" )
    addMinutes( time );
  else if( unit[0] == 's' )
    addSeconds( time );

  else  // the default
    addDays( time );

  return;
}

void
Date::addTime(std::string time, std::string unit)
{
  Split splt;

  // default for units: inherent units
  if( unit.size() == 0 )
    unit = unitStr;

  // It is save to append units (even if empty) to time
  // Note: time-unit overrules parameter unit.
  time += unit;

  // Split string at positions where digits and characters alternate.
  splt.setSeparator(":alnum:");
  splt = time ;

  // If there is a mix of digits and non-digits, splt.size() >= 2:
  // isNonDigit && size == 1 is an error.
  // size == 0 is an error.
  // But, for !isNonDigit && size == 1 we got the default

  bool isNon = ! hdhC::isDigit(time);

  if( (splt.size() == 1 && isNon) || (splt.size() == 0) )
  {
     std::ostringstream ostr(std::ios::app);
     ostr << "Date::addTime()"
          << "\nerror in 2nd parameter (time=" << time << ")" ;
     exceptionError( ostr.str() );
  }

  // unitStr and int strings are empty.
  // Pure digits are always converted to days.
  if( splt.size() == 1 )
  {
    addDays( splt.toDouble(0) ) ;
    return;
  }

  std::string str(splt[1]);

  if( str[0] == 'y' )
    addYears( splt.toDouble(0) );
  else if( str.substr(0,2) == "mo" )
    addMonths( splt.toDouble(0) );
  else if( str[0] == 'd' )
    addDays( splt.toDouble(0) );
  else if( str[0] == 'h' )
    addHours( splt.toDouble(0) );
  else if( str.substr(0,2) == "mi" )
    addMinutes( splt.toDouble(0) );
  else if( str[0] == 's' )
    addSeconds( splt.toDouble(0) );

  return;
}

void
Date::addYears( long double v )
{
  jul=date2Julian( currYr+v, currMon, currDay, currHr, currMin, currSec );
  julian2Date(jul, &currYr, &currMon, &currDay, &currHr, &currMin, &currSec);
  isDateSet=true;
  return;
}

void
Date::clear(void)
{
  isDateSet=false;
  isFormattedDate=false;

  jul.jdn  = 0. ;
  jul.time = 0. ;

  return;
}

std::string
Date::convertFormattedDate(std::string sdd)
{
  // be prepared for some short-cuts to be valid
  // (e.g. 1961 equiv. 1961-01 equiv. 1961-01-01)
  // bool parameter extends a date to the end of the given date (see below);
  // sharp by default.

  if( formattedRange.size() == 0 )
    if( sdd.find('-') < std::string::npos )
       return sdd;

  // --> YYYYMMDDhhmmss[.deci]
  //std::string iso("0000-01-01T00:00:00");

  std::string iso;
  if( ! getFullyFormattedDateStr(sdd, iso))
  {
     std::string text("Date::convertFormattedDate failed for ");
     text += sdd;
     exceptionError(text);
  }

  // append decimal figures
  std::string dec;
  size_t pos;
  if( (pos=iso.find('.')) < std::string::npos)
  {
    dec = iso.substr(pos) ;
    iso = iso.substr(0,pos);
  }

  // way back in the past
  bool isMinus(false);

  if( iso[0] == '-' )
  {
     isMinus=true;
     iso = iso.substr(1);
  }

  // Apply maximum date ranges when enabled; times remain sharp.
  // Take into account individual settings 'Yx-Mx-...' with
  // x=s for a sharp deadline and x=e for extended maximum period.
  // Defaults for x=s.

  if( formattedRange.size() > 0 )
  {
     // +e: before, e+: after, defautl: sharp
     std::string &extension = formattedRange ;

     // year
     if( extension.find("Y+") < std::string::npos )
        iso.replace(5, 14, "12-31T24:00:00");
     else if( extension.find("-Y") < std::string::npos )
        iso.replace(5, 14, "01-01T00:00:00");
     else if( extension.find("M+") < std::string::npos )
     {
          // months; iso with the pre-case; two M+ cases
          // find the end of the month specified
          // Date xD(sd, getCalendar());
          double md = getMonthDaysNum(
            hdhC::string2Double( iso.substr(5,2) ),    // month
            hdhC::string2Double( iso.substr(0,4) ) ) ; //year

         // convert to string with leading zero(s)
         std::string smd( hdhC::double2String(md, 0, "2_0") );

         // the number of days of the end month of the period
         iso.replace(8, 2, smd);
         iso.replace(11, 8, "24:00:00"); // hrs
     }
     else if( extension.find("-M") < std::string::npos )
        iso.replace(8, 11, "01T00:00:00");
     else if( extension.find("D+") < std::string::npos )
        // days
        iso.replace(11, 8, "24:00:00"); // hrs
     else if( extension.find("-D") < std::string::npos )
        // days
        iso.replace(11, 8, "00:00:00"); // hrs
     else if( extension.find("h+") < std::string::npos )
        // hours:
        iso.replace(11, 8, "24:00:00"); //min
     else if( extension.find("-h") < std::string::npos )
        iso.replace(11, 8, "00:00:00");// hr
     else if( extension.find("m+") < std::string::npos )
        // minutes
        iso.replace(14, 5, "60:60");
     else if( extension.find("-m") < std::string::npos )
        iso.replace(14, 5, "00:00");
     else if( extension.find("s+") < std::string::npos )
        // seconds
        iso.replace(17, 2, "60");
     else if( extension.find("-s") < std::string::npos )
        iso.replace(17, 2, "00");

     // reset to default
     formattedRange.clear();
  }

  if(isMinus)
    iso = "-" + iso;

  return iso + dec ;
}

std::string
Date::convertFormattedToISO_8601(double f)
{
  // convert YYYYMMDDhhmmss.f formatted str to ISO-8601 where
  // the decimal applies to the last item in the date
  std::string iso("0000-01-01T00:00:00");

  double fInt = static_cast<long int>(f);
  double decim = f - fInt ;

  std::string str( hdhC::double2String(fInt, "p=0|adj,float") );
  size_t sz = str.size();

  if( sz > 3 )
    iso.replace(0, 4, str, 0, 4);
  if( sz > 5 )
    iso.replace(5, 2, str, 4, 2);
  if( sz > 7 )
    iso.replace(8, 2, str, 6, 2);

  if( decim == 0. )
    return iso;

  // convert remainder to hour
  double hour=0.;

  if( sz > 12 )
  {
    double f = hdhC::string2Double(str.substr(12,2)) + decim ;
    decim=0.;
    hour += f/3600.;
  }

  if( sz > 10 )
  {
    double f = hdhC::string2Double(str.substr(10,2)) + decim ;
    decim=0.;
    hour += f/60.;
  }

  if( sz > 8 )
  {
    hour += hdhC::string2Double(str.substr(8,2)) + decim ;
    decim=0.;
  }

  double h, m, s;
  Date::getDayTime(hour, &h, &m, &s);

  str = hdhC::double2String(h, "w2,f=0,p=0|adj");
  iso.replace(11, 2, str, 0, 2);  // hour

  str = hdhC::double2String(m, "w2,f=0,p=0|adj");
  iso.replace(14, 2, str, 0, 2); // minute

  fInt = static_cast<int>(s);
  str = hdhC::double2String(fInt, "w2,f=0,p=0|adj");
  iso.replace(17, 2, str, 0, 2);  // second

  s -= fInt;

  if( s > 0 )
  {
    str = hdhC::double2String(s,"p=3|adj,float") ;
    if( str[0] == '0' && str[1] == '.' )
      iso += str.substr(1);
    else
      iso += str;
  }

  return iso;
}

std::string
Date::convertFormattedToISO_8601(std::string sdd )
{
  // Convention: Date ISO 8601 Format: yyyy-mm-ddThh:mm:ss
  // different compositions of str0 are converted.
  // Note: yy means really yy A.D., not the century + yy

  if( hdhC::isNumber(sdd) )
  {
     double f = hdhC::string2Double(sdd);
     return convertFormattedToISO_8601(f);
  }

  size_t pos;

  std::string str ;
  getFullyFormattedDateStr(sdd, str) ;
  /*
  if( ! getFullyFormattedDateStr(sdd, str))
  {
     std::string txt("Date::convertFormattedToIS_8601 failed for " + sdd);
     exceptionError(txt);
  }
  */

  std::string iso;

  // convert 'T' into a blank
  if( (pos=str.find('T')) < std::string::npos )
     str[pos] = ' ';

  Split x_str(str);
  size_t offset;

  for(offset=0 ; offset < x_str.size() ; ++offset)
    // parameter true: also sign and decimal
    if( hdhC::isDigit(x_str[offset][0], true) )
       break;

  // different formats: date [time[[ ]Zone]]
  std::string t0;
  bool isSign=false;
  for( size_t j=offset ; j < x_str.size() ; ++j )
  {
    // the date
    if( j == offset )
    {
      if( x_str[j][0] == '-' )
      {
        t0 = x_str[j].substr(1);
        isSign=true;
      }
      else
        t0 = x_str[j];

      if( t0.find('-') < std::string::npos )
      {
         Split x_t0(t0,'-') ;

         // also accepts yyyy-m-d or even yyy-m
         iso += x_t0[0];

         for(size_t i=1 ; i < 3 ; ++i)
         {
           iso += '-';
           if( x_t0[i].size() == 1 )
             iso += '0';
           iso += x_t0[i];
         }
      }
      else if( t0.size() > 3 && hdhC::isDigit(t0) )
      {
         iso += t0.substr(0,4) ;

         if( t0.size() > 4 )
         {
           iso += '-' ;
           iso += t0.substr(4,2) ;
         }
         if( t0.size() > 6 )
         {
           iso += '-' ;
           iso += t0.substr(6,2) ;
         }
      }
    }
    else if( j == (offset +1) )
    {
      //the time
      t0 = x_str[j];
      iso += 'T';

      if( t0.find(':') < std::string::npos )
      {
         Split x_t0(t0,':') ;

         for( size_t i=0 ; i < x_t0.size() ; ++i )
         {
            // also accepts h:m:s.f or even h:m
            if( i )
              iso += ':';

            if( x_t0[i].size() == 1 )
              iso += '0';
            iso += x_t0[i];
         }
      }
      else if( t0.size() && hdhC::isDigit(t0.substr(0,6)) )
      {
         if( t0.size() == 1 )
            iso += '0';
         iso += t0.substr(0,2) ;

         if( t0.size() > 2 )
         {
           iso += ':' ;
           iso += t0.substr(2,2) ;
         }

         if( t0.size() > 4 )
         {
           iso += ':' ;
           iso += t0.substr(4) ;
         }
      }
    }
    else if( j == (offset +2) )
      iso += x_str[j] ;  // add the time-zone
  }

  std::string def("0000-00-00T00:00:00");
  if( iso.size() < def.size() )
     iso += def.substr(iso.size());

  if( isSign )
     iso = "-" + iso ;

  return iso ;
}

void
Date::copyCurr(double y, double mo, double d, double h, double mi, double s)
{
    currYr = y;
    currMon = mo;
    currDay = d;
    currHr = h;
    currMin = mi;
    currSec = s;

    return;
}

void
Date::copyCurr(const Date &d)
{
    currYr = d.currYr;
    currMon = d.currMon;
    currDay = d.currDay;
    currHr = d.currHr;
    currMin = d.currMin;
    currSec = d.currSec;

    return;
}


Date::Julian
Date::date2Julian(double yr, double mo,
           double d, double hour )
{
  if( currCalendarEnum == PROLEPTIC_GREGORIAN )
    return prolGreg2Julian( yr, mo, d, hour);
  else if( currCalendarEnum == GREGORIAN )
    return gregorian2Julian( yr, mo, d, hour);
  else
    return modelDate2Julian( yr, mo, d, hour);
}

Date::Julian
Date::date2Julian(double yr, double mo,
           double d, double h, double mi, double s )
{
  double hour = h + mi / 60. + s / 3600. ;

  return date2Julian(yr, mo, d, hour);
}

void
Date::description(void)
{
   std::cout << "\nclass Date.cpp\n";
   std::cout << "Set, modify and get date and time.\n";
   std::cout << "The constructors and some methods accept various forms\n";
   std::cout << "of setting the date. The constructors accepts absolute\n";
   std::cout << "dates, whereas methods exist which set a date relatively\n";
   std::cout << "to a basic date. Most forms of the ISO-8601 standard are\n";
   std::cout << "valid, except those including a weekly format (i.e. not such\n";
   std::cout << "as 2000-W28), a period, and excluded is also implicit century\n";
   std::cout << "(e.g. 980203 does not mean a date in 1998).\n\n";
   std::cout << "List of valid formats (ddd:= day of the year):\n";
   std::cout << "Format      Example     Format       Example\n";
   std::cout << "yyyy-mm-dd 2004-07-11  -yyyy-mm-dd     -0333-07-11\n";
   std::cout << "yyyymmdd   20040711    -yyyymmdd       -03330711\n";
   std::cout << "yy-mm-dd   04-07-11     Notice: This is 4 A.D.\n";
   std::cout << "yymmdd     040711       Notice: This is 4 A.D.\n";
   std::cout << "-yymmdd   -010711       Notice: This is 2 B.C.\n";
   std::cout << "yyyy-mm    2004-07     -yyyy-mm        -0333-07\n";
   std::cout << "yyyy       2004        -yyyy           -0333\n";
   std::cout << "yyyy-ddd   2004-193    -yyyy-ddd       -0333-193\n";
   std::cout << "yyyyddd    2004193     -yyyyddd        -0333193\n";
   std::cout << "Special: yymmdd_HHMMSSf  with 20yy for yy<50 and\n";
   std::cout << "19yy for yy>50. Here, the delimiter _ is mandatory.\n\n";
   std::cout << "Formats of time (f means the fraction of a second):\n";
   std::cout << "HH:MM:SS.f, HHMMSSf, HH:MM, HHMM, HH\n\n";
   std::cout << "The date and time format may be used as separate strings\n";
   std::cout << "or concatenated (Usually with the delimiter T). Any\n";
   std::cout << "non-digit character can be used as delimiter between items.\n";
   std::cout << "i.e. also the example 1999-12-31 01h02m03s is valid\n";
   std::cout << "\nRules for times related to a reference time.\n";
   std::cout << "A date set by method setDate(string) is a reference time.\n";
   std::cout << "A string argument to method getDate(string) is a reference,\n";
   std::cout << "if it contains at least one non-digit.\n";
   std::cout << "A reference date overwrites a previous one.\n";
   std::cout << "Before any related time can be evaluated, the unit of the\n";
   std::cout << "relation must be stated (string: year, month, day, hour,\n";
   std::cout << "minute, second). Such a unit may be concatenated with\n";
   std::cout << "a related time.\n";
   std::cout << "Reference date, unit, and related time may be given in a\n";
   std::cout << "call of method getDate() unsorted; for multiple related\n";
   std::cout << "time, the last is evaluated.\n";
   std::cout << std::endl;

  return;
}

void
Date::exceptionError(std::string str)
{
  // Occurrence of an error usually stops the run at once.
  // But, the calling program unit is due to exit.
  static bool doInit=true;
  if( doInit )
  {
    xcptn.strError += "error_Date.txt";

    doInit = false;
  }

  // open file for writing
  if( ! xcptn.ofsError )
    xcptn.ofsError = new std::ofstream(xcptn.strError.c_str());

  *xcptn.ofsError << str << std::endl;

  return ;
}

Date
Date::getDate( double val, bool isJulDay)
{
  // If val is relative to a reference date set before, then
  // the ISO-8601 format of the realative date is returned.

  // If a reference is not set, then val is considered a Julian day.

  // The bool set true states that val is a Julian day.
  Date myDate(*this);

  if( isFormattedDate )
    myDate.setDate(val);
  else if( isJulDay )
    // representation of the current julian day
    myDate.jul=val;
  else
    myDate.addTime(val);

  return myDate;
}

Date
Date::getDate(std::string arg, bool isFormatted)
{
  // isFormatted=true for formatted value like 20010102.5*/
  Date myDate(*this);

  if( ! myDate.setDate(arg) )
  {
    if( isFormatted )
      myDate.setFormattedDate();

    if( myDate.isFormattedDate )
      myDate.setDate( hdhC::string2Double(arg) );
    else if( ! isDateSet )
      myDate.jul=hdhC::string2Double(arg);
    else
      myDate.addTime(hdhC::string2Double(arg));
  }

  return myDate ;
}

double
Date::getDay( void )
{
  //julian2Date(jul, &currYr, &currMon, &currDay, &currHr, &currMin, &currSec);
  return currDay;
}

double
Date::getDayOfTheYear(void)
{
  julian2Date(jul, &currYr, &currMon, &currDay, &currHr, &currMin, &currSec);

  Julian j0 = date2Julian( currYr, 1., 1., 0.);

  // the 1st of Jan, etc.!
  return ( static_cast<double>( jul.jdn - j0.jdn ) + 1. ) ;
}

double
Date::getDayOfTheYear(double y, double mo, double d,
                      double h, double mi, double s)
{
  double deciH = h + mi/60. + s/3600. ;
  Julian j0(date2Julian( y, 1., 1., 0.) );
  Julian j1(date2Julian( y, mo, d, deciH) );

  // the 1st of Jan, etc.!
  return ( static_cast<double>( j1.jdn - j0.jdn ) + 1. ) ;
}

void
Date::getDayTime(double d, double *h, double *m, double *s)
{
  // day time
  d *= 24.;

  *h = static_cast<double>( static_cast<int>(d) ) ;
  d -= *h ;

  d *= 60. ;
  *m = static_cast<double>( static_cast<int>(d) );
  d -= *m ;

  *s = d * 60. ;

  return ;
}

double
Date::getDeciHour(void)
{
  return static_cast<double>( jul.time ) ;
}

double
Date::getDeciMonth(void)
{
  julian2Date(jul, &currYr, &currMon, &currDay, &currHr, &currMin, &currSec);
  return getDeciMonth( currYr, currMon, currDay, currHr, currMin, currSec);
}

double
Date::getDeciMonth(double y, double mo, double d,
       double h, double mi, double s )
{
  double monthDays = getMonthDaysNum( mo, y ) ;

  return mo +( d - 1. + jul.time) /monthDays ;
}

double
Date::getDeciYear(void)
{
  julian2Date(jul, &currYr, &currMon, &currDay, &currHr, &currMin, &currSec);

  return Date::getDeciYear(currYr, currMon, currDay, currHr, currMin, currSec) ;
}

double
Date::getDeciYear(double y, double mo, double d,
                      double h, double mi, double s)
{
  double daysY = getYearDaysNum( y );
  double currD = getDayOfTheYear(y, mo, d, h, mi, s) ;
  double currTime = getDeciHour();

  // -1: Jan 1st starts with 0.0 dYear
  double dYear = y + (currD -1. + currTime)/daysY ;
  return dYear ;
}

bool
Date::getFullyFormattedDateStr(std::string sd, std::string& str)
{
    // [[[y]y]y]ymmddhhmmss.*  --> yyyy-mm-ddThh:mm:ss.*

    // yymmdd_hh[mmss.f local] --> yymmddhh[mmss.f local]
    size_t pos;
    if( (pos=sd.find('_')) < std::string::npos )
    {
       std::string t(sd.substr(pos+1,sd.size()) );
       sd = sd.substr(0,pos) + t ;
    }

    bool isMinus=false;
    if(sd[0] == '-')
    {
      isMinus=true;
      sd=sd.substr(1);
    }

    std::string as_d[3] ;
    as_d[1]="01";
    as_d[2]="01";

    std::string as_t[3] ;
    as_t[0]="00";
    as_t[1]="00";
    as_t[2]="00";

    if( hdhC::isDigit(sd) )
    {
        // Y[Y[Y[YMM[DD[hh[mm[ss[.f]]]]]]]]
        // this kind of coding is generally ambiguous;
        // only cases YYYY, i.e. the common case of CMIP, also YMM.
        int sz=sd.size();

        if( sd.size() < 3 )
          as_d[0] = sd ;
        else
        {
          // now, yyyy is supposed
          as_d[0] = sd.substr(0,4) ;   // <- yr

          if( sd.size() > 4 )
          {
            as_d[1] = sd.substr(4, 2) ;  // <- mon

            if( sd.size() > 6 )
            {
              as_d[2] = sd.substr(6,2) ;  // <- day

              if( sd.size() > 8 )
              {
                as_t[0] = sd.substr(8,2) ;  // <- hr

                if( sd.size() > 10 )
                {
                  as_t[1] = sd.substr(10,2) ;  // <- min

                  if( sd.size() > 12 )
                    as_t[2] = sd.substr(12,sz) ;  // <- sec.f
                }
              }
            }
          }
        }
    }
    else
    {
      size_t pos;
      if( (pos=sd.find(' ')) < std::string::npos )
          sd[pos]='T';

      Split x_T(sd,'T');
      Split x_dash(x_T[0], '-');

      if( x_dash.size() == 0 || x_dash.size() > 3 )
          return false;

      // year
      as_d[0] = x_dash[0] ;

      // month
      if( x_dash.size() > 1 )
      {
         if( x_dash[1].size() == 1 )
           as_d[1] = "0"  + x_dash[1];
         else if( x_dash[1].size() > 2 )
            return false;
         else
           as_d[1] = x_dash[1];
      }

      // day
      if( x_dash.size() > 2 )
      {
         if( x_dash[2].size() == 1 )
           as_d[2] = "0"  + x_dash[2];
         else if( x_dash[2].size() > 2 )
            return false;
         else
           as_d[2] = x_dash[2];
      }

      if( x_T.size() > 1 )
      {
         Split x_colon(x_T[1], ':');

         if( x_colon.size() > 3 )
            return false;

         // hr
         if( x_colon[0].size() == 1 )
            as_t[0] = "0" + x_colon[0];
         else if( x_colon[0].size() > 2 )
            return false;
         else
            as_t[0] = x_colon[0];

        // min
         if( x_colon[1].size() == 1 )
            as_t[1] = "0"  + x_colon[1];
         else if( x_colon[1].size() > 2 )
            return false;
         else
            as_t[1] = x_colon[1];

        // sec
        if( x_colon.size() > 2 )
        {
           if( (pos=x_colon[2].find('.')) < std::string::npos )
           {
              if( pos == 0 )
                 as_t[2] = "00" + x_colon[2];
              else if( pos == 1 )
                 as_t[2] = "0" + x_colon[2];
              else if( pos == 2 )
                 as_t[2] = x_colon[2];
              else
                 return false;
           }
           else
           {
              if( x_colon[2].size() == 1 )
                 as_t[2] = "0" + x_colon[2];
              else if( x_colon[2].size() > 2 )
                 return false;
              else
                 as_t[2] = x_colon[2];
           }
        }
      }
    }

    // right-fill with "0"
    if( as_d[0].size() < 4 )
    {
      std::string s;
      for( size_t i=as_d[0].size() ; i < 4 ; ++i )
         s += '0' ;
      as_d[0] = s + as_d[0] ;
    }

    if( isMinus )
        as_d[0] = "-" + as_d[0] ;

    str = as_d[0] + "-" + as_d[1] + "-" + as_d[2] ;
    str += "T";
    str += as_t[0] + ":" + as_t[1] + ":" + as_t[2] ;

    return true;
}

double
Date::getHour( void )
{
  int h = static_cast<int>(jul.time*24.) ;
  return static_cast<double>(h);
}

std::string
Date::getISO_8601(void)
{
  julian2Date(jul, &currYr, &currMon, &currDay, &currHr, &currMin, &currSec);

  // Date ISO 8601 Format: yyyy-mm-ddThh:mm:ss
  return getISO_8601(currYr, currMon, currDay, currHr, currMin, currSec);
}

std::string
Date::getISO_8601(double zy, double zmo, double zd,
           double zh, double zmi, double zs, bool isErr)
{
  //static method
  std::string t;

  if( isErr )
  {
    t = "not-a-valid-date" ;
    return t;
  }

  // Date ISO 8601 Format: yyyy-mm-ddThh:mm:ss

  double s2=static_cast<double>( static_cast<int>(zs) );

  double threshold=1.;
  for( int i=0 ; i < precision ; ++i)
    threshold /=10.;

  if( (zs - s2 ) < threshold )
    zs=s2;

  if ( zy < 0. )
  {
    t = "-";
    t += hdhC::double2String(-1.*zy,0,"4_0") ;
  }
  else
    t = hdhC::double2String(zy,0,"4_0") ;

  t += "-";
  t += hdhC::double2String(zmo,0,"2_0") ;
  t += "-";
  t += hdhC::double2String(zd,0,"2_0") ;
  t += "T" ;
  t += hdhC::double2String(zh,0,"2_0") ;
  t += ":" ;
  t += hdhC::double2String(zmi,0,"2_0") ;
  t += ":" ;

  std::string ts;
  if( (zs - static_cast<double>( static_cast<int>(zs) ) ) > 0. )
  {
    ts = hdhC::double2String(zs,precision,"2_0") ;
  }
  else
    ts = hdhC::double2String(zs,0,"2_0") ;

  //It is possible that ts was set ts 00.000 due to rounding
  if( ts.find('.') < std::string::npos )
  {
    size_t i=ts.size()-1;
    while ( ts[i] == '0' )
    {
      --i;  //used as index
      if( ts[i] == '.' )
      {
        --i;
        break;
      }
    }
    ++i; // indicates a size
    t += ts.substr(0,i);
  }
  else
    t += ts;

  return t;
}

double
Date::getMinute( void )
{
  double th, tm, ts ;
  getDayTime(static_cast<double>(jul.time), &th, &tm, &ts);
  return tm;
}

double
Date::getMonth( void )
{
  //julian2Date(jul, &currYr, &currMon, &currDay, &currHr, &currMin, &currSec);
  return currMon;
}

double
Date::getMonthDaysNum( void )
{
  julian2Date(jul, &currYr, &currMon, &currDay, &currHr, &currMin, &currSec);

  return getMonthDaysNum(currMon, currYr) ;
}

double
Date::getMonthDaysNum( double myMon, double myY )
{
  if( currCalendarEnum == EQUAL_MONTHS )
    return 30. ;

  int  mon = static_cast<int>( myMon );
  double t = regularMonthDays[mon-1] ;

  bool is = currCalendarEnum == GREGORIAN
              || currCalendarEnum == PROLEPTIC_GREGORIAN ;

  if( mon == 2 && is )
  {
    Julian j1(date2Julian( myY, myMon +1., 1., 0.) );
    Julian j0(date2Julian( myY, myMon    , 1., 0.) );

    long double tld= j1.jdn - j0.jdn  ;
    tld += (j1.time -j0.time) ;
    t = static_cast<double>( static_cast<int>(tld+0.5) );
  }

  return t ;
}

double
Date::getSecond( void )
{
  double th, tm, ts ;
  getDayTime(static_cast<double>(jul.time), &th, &tm, &ts);
  return ts;
}

double
Date::getSince(Date &lag, const char *ref)
{
  // this wrapping method returns the time lag since date.
  // valid expr for desc: seconds, minutes, hours, days, months year
  // If date is omitted, a reference date must have been
  // set previously.

  std::string sRef(ref);

  if( sRef.size() > 0 )
    setDate(sRef);

  double dJul=lag.getJulianDay() - getJulianDay();

  if( unitStr.substr(0,1) == "y" )
  { // ToDo
    return 0.;
  }
  else if( unitStr.substr(0,2) == "mo" )
  { // ToDo
    return 0.;
  }
  else if( unitStr.substr(0,4) == "day" )
    return static_cast<double>(dJul);
  else if( unitStr.substr(0,1) == "h" )
    return static_cast<double>(dJul*24.);
  else if( unitStr.substr(0,2) == "mi" )
    return static_cast<double>(dJul*24.*60.);
  else if( unitStr.substr(0,1) == "s" )
    return static_cast<double>(dJul*24.*60.*60.);

  return 0.;  // a return statement here is required for the xlc++
}

double
Date::getSince(const char *curr, const char *ref)
{
  std::string sCurr(curr);

 // the date for the lag
  Date lag( sCurr, currCalendar );

  return getSince(lag, ref);
}

double
Date::getSince(std::string &curr, const char *ref)
{
  // the date for the lag
  Date lag( curr, currCalendar );

  return getSince(lag, ref);
}

std::string
Date::getTodayStr(void)
{
  // return the current date in ISO-8601 format. Based on
  // the example of the man-page of strftime(3).
  std::string s;

  char outstr[25];
  time_t t;
  struct tm *tmp;

  t = time(0);
  tmp = localtime(&t);
  if( tmp == 0 )
    return s; // failure: return empty string

  // the format string
  s="%Y-%m-%dT%T%Z" ;
  if( strftime(outstr, 25, s.c_str(), tmp ) == 0 )
  {
     s.clear();
     return s;  // failure: return empty string
  }

  s = outstr ;
  return s;
}

double
Date::getYear( void )
{
  //julian2Date(jul, &currYr, &currMon, &currDay, &currHr, &currMin, &currSec);
  return currYr;
}

double
Date::getYearDaysNum( double myY )
{
  Julian j1(date2Julian( myY+1., 1., 1., 0.) );
  Julian j0(date2Julian( myY, 1., 1., 0.) );

  return static_cast<double>( j1.jdn - j0.jdn ) ;
}

Date::Julian
Date::gregorian2Julian( double y, double mo, double d, double dh )
{
  return prolGreg2Julian(y, mo, d, dh);
}

void
Date::init(void)
{
  xcptn.ofsError=0;

  refUnitSign=1.;
  precision=3; //default
  lY_start=-1;
  lY_month = 1;  // Feb by default
  lY_is=false;

  isDateSet=false;
  isFormattedDate=false;

  currYr = 0.;
  currMon = 0.;
  currDay = 0.;
  currHr = 0.;
  currMin = 0.;
  currSec = 0.;

  regularMonthDays = new double [12] ;
}

bool
Date::isNotASingleDigit(std::string s)
{
  for(size_t k=0; k < s.size(); ++k)
    if( isdigit(s[k]) )
      return false;

  return true;
}

bool
Date::isDigit(std::string s)
{
  for(size_t k=0; k < s.size(); ++k)
    if( !isdigit(s[k]) )
      return false;

  return true;
}

bool
Date::isValidDate(std::string s)
{
  std::string mn("january february march april may june july");
  mn += " august september october november december" ;
  Split x_mn(mn);

  s = hdhC::Lower()(s);
  s = hdhC::replaceChars(s, ',', ' ');
  Split x_s(s);

  if( x_s.size() != 3 )
    return false;

  int i_d, i_m, i_y;
  i_d = i_m = i_y = 0;

  for(size_t i=0 ; i < x_s.size() ; ++i )
  {
    if( hdhC::isAlpha(x_s[i]) )
    {
      for(size_t m=0 ; m < x_mn.size() ; ++m )
      {
        if( x_s[i] == x_mn[m].substr(0, x_s[i].size()) )
        {
          i_m = hdhC::string2Double(x_s[i]) ;
          break;
        }
      }
    }
    else
    {
      if( hdhC::isDigit(x_s[i]) )
      {
        if( x_s[i].size() > 2 )
          i_y = hdhC::string2Double(x_s[i]) ;
        else
          i_d = hdhC::string2Double(x_s[i]) ;
      }
      else
        i_d = hdhC::string2Double(x_s[i]) ;
    }
  }

  bool is=true;
  if( i_d == 0 )
    is=false ;
  if( i_m == 0 )
    is=false ;
  if( i_y == 0 )
    is=false ;

  if(is)
    return true;

  return false;
}

void
Date::julian2Date( long double jD,
  double *yr, double *mo, double *dy, double *hr, double *mi, double *se )
{
  Julian j;
  j.set(jD);

  if( currCalendarEnum == PROLEPTIC_GREGORIAN )
    julian2ProlGreg( j, yr, mo, dy, hr, mi, se );
  else if( currCalendarEnum == GREGORIAN )
    julian2Gregorian( j, yr, mo, dy, hr, mi, se );
  else
    julian2ModelDate( j, yr, mo, dy, hr, mi, se );

  return;
}

void
Date::julian2Date( const Date::Julian &myJul,
  double *yr, double *mo, double *day, double *hr, double *mi, double *se )
{
  if( currCalendarEnum == PROLEPTIC_GREGORIAN )
    julian2ProlGreg( myJul, yr, mo, day, hr, mi, se );
  else if( currCalendarEnum == GREGORIAN )
    julian2Gregorian( myJul, yr, mo, day, hr, mi, se );
  else
    julian2ModelDate( myJul, yr, mo, day, hr, mi, se );

  return;
}

void
Date::julian2Gregorian( const Date::Julian &j,
  double *yr, double *mo, double *dy, double *hr, double *mi, double *se )
{
   julian2ProlGreg(j, yr, mo, dy, hr, mi, se);
   return ;
}

void
Date::julian2ModelDate( const Date::Julian &j,
  double *y, double *mo, double *d, double *h, double *mi, double *s )
{
  double resid;
  long double jd=j.jdn + j.time ;

  *mo = *d = 0. ;
  *h = *mi = *s = 0.;

  if( currCalendarEnum == EQUAL_MONTHS )
  {
     *y = static_cast<double>
          (  static_cast<long long int> (jd / 360. ) )  ;

     resid = static_cast<double>(jd - *y*360.) ;

     if( resid != 0. )
     {
       *mo = static_cast<double>( static_cast<int>(resid / 30.) )  ;
       resid -= *mo*30. ;

       *d = static_cast<double>( static_cast<int>(resid) ) ;
       resid -= *d ;
     }
  }
  else
  {
    // explicitly: months and/or leap year and/or leap_month
    // For calender: NO_LEAP, ALL_LEAP, UNDEF and not provided
    long double yrDays;

    if( currCalendarEnum == ALL_LEAP )
    {
      yrDays = 366. ;
      lY_is=false ;
    }
    else if( currCalendarEnum == NO_LEAP )
    {
      yrDays = 365. ;
      lY_is=false ;
    }
    else if( lY_is )
      yrDays = lY_yrDays + 0.25;
    else
    {
      yrDays = 365. ;
      lY_is=false ;
    }

    *y = static_cast<double>
        (  static_cast<long long int> (jd / yrDays ) ) ;

    resid = static_cast<double>( jd - *y*yrDays ) ;

    if( resid != 0. )
    {
      bool isCurrLY = false;
      if( lY_is )
        if( (static_cast<int>(*y) - lY_start) % 4  == 0 )
          isCurrLY = true;

      // we are looking for the current month
      double tmp=0. ;
      for(int imon=0 ; imon < 12 ; ++imon)
      {
        double cMon = regularMonthDays[imon];
        if( isCurrLY && imon == lY_month )
          cMon += 1.;

        tmp += cMon;

        if( tmp > resid )
        {
           *mo = static_cast<double>(imon) ;  //the previous month is indicated by imon
           resid -= (tmp - cMon);
           *d=static_cast<double>( static_cast<int>(resid) ) ;
           resid -= *d;
           break;
        }
      }
    }
  }

  // day time
  if( resid > 0. )
     getDayTime(resid, h, mi, s);

  // convert from numerical to date representation
  // y=1900, mo=11, d=30, h=24 <--> 1901-01-01 00:00:00
  //*y  += 1. ;
  *mo += 1. ;
  *d  += 1. ;

  return ;
}

void
Date::julian2ProlGreg( const Date::Julian &myJul,
  double *yr, double *mo, double *day, double *hr, double *mi, double *se )
{
 /*
  uint32_t J = static_cast<uint32_t>(myJul.jdn)  ;
  uint32_t j = J + static_cast<uint32_t>(32044) ;

  uint32_t g  = j / 146097;
  uint32_t dg = j % 146097;

  uint32_t c  = (dg / 36524 + 1) * 3 / 4;
  uint32_t dc = dg - c * 36524;

  uint32_t b  = dc / 1461;
  uint32_t db = dc % 1461;

  uint32_t a  = (db / 365 + 1) * 3 / 4;
  uint32_t da = db - a * 365;

  uint32_t y = g * 400 + c * 100 + b * 4 + a;

  uint32_t m = (da * 5 + 308) / 153 - 2;

  uint32_t d = da - (m + 4) * 153 / 5 + 122;

  uint32_t Y = y - 4800 + (m + 2) / 12;
  uint32_t M = (m + 2) % 12 + 1;

  *yr = Y ;
  *mo = M ;
  *day = d + 1;
*/

  uint32_t f = static_cast<uint32_t>(myJul.jdn) ;
           f = f + 1401 + ((( 4*f + 274277)/146097) * 3)/4 -38 ;
  uint32_t e = 4*f + 3 ;
  uint32_t g = ( e % 1461 ) / 4 ;
  uint32_t h = 5 * g + 2 ;
  uint32_t D = (h % 153) / 5 + 1 ;
  uint32_t M = (h/153 + 2) % 12 + 1 ;
  uint32_t Y = e/1461 - 4716 + (12 + 2 - M)/12 ;

  *yr = Y ;
  *mo = M ;
  *day = D ;

  // get the civil time
  getDayTime(static_cast<double>(myJul.time), hr, mi, se);

  return;
}

Date::Julian
Date::modelDate2Julian( double y, double mo, double d, double h,
   double mi, double s )
{
   Julian j;

   // convert from calendar to numerical representation
   d  -= 1. ;  // cal: [1-31] + h' --> num.: [0-30] + h'
   mo -= 1.;
   //y  -= 1.;

   // note: reference date is 0000-01-01 00:00:00,
   // decimal numbers are supported.
   if( currCalendarEnum == EQUAL_MONTHS )
   {
     j.jdn += y*360. ; // y begins at 0.
     j.jdn += mo*30. ; // mo begins at 0.
   }

   else if( currCalendarEnum == NO_LEAP || currCalendarEnum == ALL_LEAP )
   {
     double mdays = currCalendarEnum == NO_LEAP ? 365. : 366. ;

     j.jdn += y*mdays ;

     // add the number of days within entire months
     int imon=static_cast<int>(mo) ;
     for( int i=0 ; i < imon ; ++i )
       j.jdn += regularMonthDays[i] ;
   }


   double fd = ( h + mi/60. + s/3600.) / 24. ;
   int id = static_cast<int>(fd);
   d += static_cast<double>(id) ;
   fd -= static_cast<double>(id);

   j.jdn += d ;
   j.time = fd ;

   isDateSet=true;

   return j ;
}

bool
Date::parseISO_8601(std::string str0)
{
  // ISO 8601 Format: yyyy-mm-ddThh:mm:ss
  double year, month, day, hour;

  // defaults:
  year = 0.;
  month = 1.;
  day = 1.;
  hour=0.;

  Split x_str0(str0, " T", true);

  // negative year?
  double negYr=1.;
  if( x_str0[0][0] == '-' )
      negYr=-1.;

  Split x_d(x_str0[0],"-");

  if( x_d.size() > 2 )
  {
    year  = hdhC::string2Double( x_d[0] );
    month = hdhC::string2Double( x_d[1] );
    day   = hdhC::string2Double( x_d[2] );
  }
  else if( x_d.size() > 1 )
  {
    year  = hdhC::string2Double( x_d[0] );
    month = hdhC::string2Double( x_d[1] );
  }
  else
    year  = hdhC::string2Double( x_d[0] );

  if( x_str0.size() > 1 )
  {
    Split x_t(x_str0[1], ':') ;

    if( x_t.size() > 2 )
    {
      hour  = hdhC::string2Double( x_t[0] );
      hour += hdhC::string2Double( x_t[1] ) / 60.;
      hour += hdhC::string2Double( x_t[2] ) / 3600.;
    }
    else if( x_t.size() > 1 )
    {
      hour  = hdhC::string2Double( x_t[0] );
      hour += hdhC::string2Double( x_t[1] ) / 60.;
    }
    else
      hour  = hdhC::string2Double( x_t[0] );
  }

  if( x_str0.size() == 3 )
  {
    // local time zone
    Split x_t(x_str0[2], ':') ;
    if( x_t.size() > 1 )
    {
      hour += hdhC::string2Double(x_t[0]) ;
      hour += hdhC::string2Double(x_t[1]) / 60. ;  // minutes
    }
    else
      hour += hdhC::string2Double(x_t[0]) ;

  }

  year *= negYr;
  currYr = year;
  currMon = month;
  currDay = day;

  if( hour > 0. )
     getDayTime(hour, &currHr, &currMin, &currSec);

  jul = date2Julian( year, month, day, hour);
  julian2Date(jul, &currYr, &currMon, &currDay, &currHr, &currMin, &currSec);
  isDateSet=true;

  return false ;
}

Date::Julian
Date::prolGreg2Julian( double yr, double mo, double d, double hour )
{
  long int iday = static_cast<long int>( d );
  double fday = static_cast<double>(iday) ;
  if( fday != d )
    hour += (d-fday)*24. ;

  long int a = ( 14 - static_cast<long int>(mo) ) / 12 ;
  long int y= static_cast<long int>(yr) +4800 - a ;
  long int m = static_cast<long int>(mo) + 12*a - 3 ;

  long int JDN = static_cast<long int>(d) + (153*m + 2)/5
                 +365*y + y/4 - y/100 + y/400 -32045;

  Julian j( static_cast<long int>(JDN), hour );

  return j ;
}

void
Date::setCalendar(std::string cal, std::string monLen)
{
  if( monLen.size() )
     monthLengths=monLen;

  // convert to upper case
  (void) hdhC::Lower()(cal, true);

  if( cal == "gregorian" || cal == "standard" )
  {
    // the Greogorian calendar is actually the proleptic one.
    currCalendarEnum = PROLEPTIC_GREGORIAN ;
    currCalendar="proleptic_gregorian";
  }
  else if( cal == "proleptic_gregorian" )
  {
    // Is used as astronomical calendar, i.e. there is a year 0.
    // Begin: -4713-11-24, End: 3268-01-23T00:00:00
    // The length of a Scaliger cycle: 2914695 days
    // julian day 2440000 <==> 1968-05-23T00:00:00
    currCalendarEnum = PROLEPTIC_GREGORIAN ;
    currCalendar="proleptic_gregorian";
  }
  else if( cal.substr(0,3) == "360")
  {
    currCalendarEnum = EQUAL_MONTHS ;
    currCalendar=cal.substr(0,3);
    jul.set(0.);
  }
  else if( cal == "julian" )
  {
    currCalendarEnum = JULIAN ;
    currCalendar=cal;
    jul.set(0.);
  }
  else if( cal == "noleap" || cal.substr(0,3) == "365" )
  {
    currCalendarEnum = NO_LEAP ;
    currCalendar="365";
    jul.set(0.);
  }
  else if( cal == "all_leap" || cal.substr(0,3) == "366" )
  {
    currCalendarEnum = ALL_LEAP ;
    currCalendar="366";
    jul.set(0.);
  }
  else
  {
    currCalendarEnum = UNDEF ;
    currCalendar=cal;
    jul.set(0.);
  }

  // the number of days of regular months
  if( currCalendarEnum == EQUAL_MONTHS)
  {
    for( size_t i=0 ; i < 12 ; ++i )
      regularMonthDays[i]=30.;
  }
  else if( monthLengths.size() )
    setMonthLengths( monLen );
  else
  {
    int i=0;
    regularMonthDays[i++]=31.; regularMonthDays[i++]=28.;
    regularMonthDays[i++]=31.; regularMonthDays[i++]=30.;
    regularMonthDays[i++]=31.; regularMonthDays[i++]=30.;
    regularMonthDays[i++]=31.; regularMonthDays[i++]=31.;
    regularMonthDays[i++]=30.; regularMonthDays[i++]=31.;
    regularMonthDays[i++]=30.; regularMonthDays[i++]=31.;
  }

  if( currCalendar == "366" )
     regularMonthDays[1]=29.;

  if( currCalendarEnum == UNDEF )
  {
    // a leap year is defined
    lY_is=true;

    if( lY_start == -1 )
      lY_start=0;

    if( lY_month == -1 )
      lY_month=1;
    else
      --lY_month;  // adjust to C-array counting

    // num of days between consecutive leap years + leap day
    double num=0.;

    for(size_t i=0 ; i < 12 ; ++i)
      num += regularMonthDays[i] ;

    lY_yrDays=static_cast<int>(num) ;
    lY_cycleDays = static_cast<int>( 4*num +1. ) ;
  }

  return;
}

void
Date::setMonthLengths(std::string monLen)
{
   if( monLen.size() )
      monthLengths = monLen;
   else
   {
      if( monthLengths.size() == 0 )
        return;
   }

   Split x_ml(monthLengths);

   if( x_ml.size() != 12 )
   {
      std::ostringstream ostr(std::ios::app);
      ostr << "Date::setMonthLengths()"
           << "\n12 values reuqired; found: " << monthLengths;

      exceptionError( ostr.str() );
   }

   for( size_t i=0 ; i < x_ml.size() ; ++i)
      regularMonthDays[i]=hdhC::string2Double(x_ml[i]);

   return;
}

bool
Date::setDate( const Date &d )
{
  init();

  unitStr     = d.unitStr;
  refUnitSign = d.refUnitSign;
  precision   = d.precision;
  isDateSet   = d.isDateSet ;
  lY_start    = d.lY_start;
  lY_month   = d.lY_month;
  monthLengths= d.monthLengths;

  currYr = d.currYr;
  currMon = d.currMon;
  currDay = d.currDay;
  currHr = d.currHr;
  currMin = d.currMin;
  currSec = d.currSec;

  setCalendar(d.currCalendar, d.monthLengths);

  jul.jdn  = d.jul.jdn ;
  jul.time = d.jul.time ;

  return false;
}

bool
Date::setDate( double y, double mo, double d, double h, double mi, double s)
{
  currYr = y;
  currMon = mo;
  currDay = d;
  currHr = h;
  currMin = mi;
  currSec = s;

  jul = date2Julian(y, mo, d, h, mi, s);
  julian2Date(jul, &currYr, &currMon, &currDay, &currHr, &currMin, &currSec);
  isDateSet=true;

  return false;
}

bool
Date::setDate( const Date::Julian &j )
{
  jul = j ;
  isDateSet=true;

  return false;
}

bool
Date::setDate(double f, std::string cal, std::string monLen)
{
  if( cal.size() )
    setCalendar(cal, monLen);

  std::string str;

  if( isFormattedDate )
  {
    // convert %Y%m%d.f formatted str
    str = convertFormattedDate(hdhC::double2String(f) ) ;

    if( ! parseISO_8601(str) )
      return true;
  }
  else if( isDateSet )
    addTime(f);
  else
    jul = f ;

  return false;
}

bool
Date::setDate( std::string str, std::string cal, std::string monLen)
{
  // str could contain more information than just the date.
  // Return true, if no valid date string was found.

  // Possible input: str="minutes since 1955-01-01 00:00"
  //                 str="1955-1-1 00:00"

  if( str.size() == 0 )
  {
     std::string text("Date::setDate(): empty string");
     exceptionError(text);
  }

  if( cal.size() )
    setCalendar(cal, monLen);

  // key-words "since" and "before" indicate that a date
  // serves as a basic date and
  // key-words: years, months, days, hours, minutes, seconds
  // indicate the unit of the amount X in getDate(X) relative
  // to the basic date. The method clears the string from any
  // key-word.
  setUnitsAndClear(str) ;

  if( isFormattedDate || str.find('-') == std::string::npos )
    str = convertFormattedDate(str) ;

  if( ! parseISO_8601(str) )
    return true;
  else if( isDateSet && hdhC::isNumber(str) )
  {
    // try the string as float
    if( setDate(hdhC::string2Double(str)) )
      return true;
  }

  return false;
}

void
Date::setUnits(std::string str, int uSign)
{
  refUnitSign=uSign;
  unitStr = str;
  return;
}

void
Date::setUnitsAndClear(std::string& str)
{
  // key-words "since" and "before" indicate that a date
  // serves as a basic date and
  // key-words: years, months, days, hours, minutes, seconds
  // indicate the unit of the amount X in getDate(X) relative
  // to the basic date. The method clears the string from any
  // key-word.
  Split x_str(str);
  str.clear();

  for( size_t n=0 ; n < x_str.size() ; ++n)
  {
    std::string s(x_str[n]);

    // accept singular and plural forms
    if( s[s.size()-1] == 's' )
       s = s.substr(0,s.size()-1);

    if( s == "since" )
      continue;  // refUnitSign already set by default
    else if( s == "before" )
      refUnitSign=-1.;
    else if( s.find("year") < std::string::npos)
      unitStr = "year";
    else if( s.find("month") < std::string::npos)
      unitStr = "month";
    else if( s.find("day") < std::string::npos)
      unitStr = "day";
    else if( s.find("hour") < std::string::npos)
      unitStr = "hour";
    else if( s.find("minute") < std::string::npos)
      unitStr = "minute";
    else if( s.find("second") < std::string::npos )
      unitStr = "second";
    else
    {
      if( str.size() )
        str += " " ;
      str += x_str[n];
    }
  }

  return ;
}

bool
Date::shift(std::string s)
{
    // return true if shifting was done.
    // string: beg, end, f, y,mo,d,h,mi,s. csv separated.
    // beg|end: shift to the begin|end
    // f: force to shift correponding to the one and only frequency given.
    // default: shift according to the frequency that could be shifted (from
    //          smallest to highest frequency, i.e. from sec to year.

    Split x_s(s,',') ;
    std::string beg=("beg");
    std::string end=("end");
    size_t i;

    bool isBeg=false;

    if( s.find(beg) < std::string::npos || s.find(end) < std::string::npos )
    {
      if( s.find(beg) < std::string::npos )
          isBeg=true;

      if( x_s.size() != 2 )
          return false;

      if( x_s[0] == beg || x_s[0] == end )
        i=1;
    }
    else
      i=0;

    // get a match for frequency
    if( x_s[i] == "y" )
    {
       if( isBeg )
         shiftYearBeg();
       else
         shiftYearEnd();
    }
    else if( x_s[i] == "mo" )
    {
       if( isBeg )
         shiftMonthBeg();
       else
         shiftMonthEnd();
    }
    else if( x_s[i] == "d" )
    {
       if( isBeg )
         shiftDayBeg();
       else
         shiftDayEnd();
    }
    else if( x_s[i] == "h" )
    {
       if( isBeg )
         shiftHourBeg();
       else
         shiftHourEnd();
    }
    else if( x_s[i] == "mi" )
    {
       if( isBeg )
         shiftMinuteBeg();
       else
         shiftMinuteEnd();
    }
    else if( x_s[i] == "s" )
    {
       if( isBeg )
         shiftSecondBeg();
       else
         shiftSecondEnd();
    }
    else
        return false;


    return true;
}

bool
Date::shiftShortest(std::string s)
{
    // return true if shifting was done.
    // string: beg, end, f, y,mo,d,h,mi,s. csv separated.
    // beg|end: shift to the begin|end;
    // shift to the end by default,i.e. complete the frequency.
    // shift according to the frequency that could be shifted (from
    // smallest to highest frequency, i.e. from sec to year.

    Split x_s(s,',') ;
    std::string beg=("beg");
    std::string end=("end");
    size_t ix;

    bool isBeg=false;

    if( s.find(beg) < std::string::npos || s.find(end) < std::string::npos )
    {
      if( s.find(beg) < std::string::npos )
          isBeg=true;

      if( x_s.size() != 2 )
          return false;

      if( x_s[0] == beg || x_s[0] == end )
        ix=1;
    }
    else
      ix=0;

    // find the shortest item smaller than left
    if( currSec > 0. )
    {
      for( size_t i=0 ; i < x_s.size() ; ++i )
      {
         if( x_s[ix] == "s" )
         {
            if( isBeg )
                shiftSecondBeg();
            else
                shiftSecondEnd();

            return true ;
         }
      }
      return false;
    }
    else if( currMin > 0. )
    {
      for( size_t i=0 ; i < x_s.size() ; ++i )
      {
         if( x_s[ix] == "mi" )
         {
            if( isBeg )
                shiftMinuteBeg();
            else
                shiftMinuteEnd();
           return true ;
         }
      }
      return false;
    }
    else if( currHr > 0. )
    {
      for( size_t i=0 ; i < x_s.size() ; ++i )
      {
         if( x_s[ix] == "h" )
         {
            if( isBeg )
                shiftHourBeg();
            else
                shiftHourEnd();
           return true ;
         }
      }
      return false;
    }
    else if( currDay > 1. )
    {
      for( size_t i=0 ; i < x_s.size() ; ++i )
      {
         if( x_s[ix] == "d" )
         {
            if( isBeg )
                shiftDayBeg();
            else
                shiftDayEnd();
           return true ;
         }
      }
      return false;
    }
    else if( currMon > 1. )
    {
      for( size_t i=0 ; i < x_s.size() ; ++i )
      {
         if( x_s[ix] == "mo" )
         {
            if( isBeg )
                shiftMonthBeg();
            else
                shiftMonthEnd();
           return true ;
         }
      }
      return false;
    }
    else
    {
      for( size_t i=0 ; i < x_s.size() ; ++i )
      {
         if( x_s[ix] == "y" )
         {
            if( isBeg )
                shiftYearBeg();
            else
                shiftYearEnd();
           return true ;
         }
      }
      return false;
    }

    return true;
}

void
Date::shiftSecondBeg(void)
{
    // take fraction of the second away
    int is = static_cast<int>(currSec) ;
    double resid = currSec - static_cast<double>(is);

    if( resid > 0. )
      setDate( currYr, currMon, currDay, currHr, currMin, is);

    return;
}

void
Date::shiftSecondEnd(void)
{
    int is = static_cast<int>(currSec);
    double resid = currSec - static_cast<double>(is);

    if( resid > 0. )
      setDate( currYr, currMon, currDay, currHr, currMin, static_cast<double>(is+1));

    return;
}

void
Date::shiftMinuteBeg(void)
{
    if( currSec > 0. )
      setDate( currYr, currMon, currDay, currHr, currMin, 0.);

    return;
}

void
Date::shiftMinuteEnd(void)
{
    if( currSec < 60. )
       setDate( currYr, currMon, currDay, currHr, currMin, 60.);

    return;
}

void
Date::shiftHourBeg(void)
{
    if( currMin > 0. || currSec > 0. )
       setDate( currYr, currMon, currDay, currHr, 0., 0.);

    return;
}

void
Date::shiftHourEnd(void)
{
    if( currMin < 60. || currSec < 60. )
       setDate( currYr, currMon, currDay, currHr, 59., 60.);

    return;
}

void
Date::shiftDayBeg(void)
{
    if( currHr > 0. || currMin > 0. || currSec > 0. )
       setDate( currYr, currMon, currDay, 0., 0., 0.);

    return;
}

void
Date::shiftDayEnd(void)
{
    bool isA = currHr == 23. && currMin == 60. && currSec == 0. ;
    bool isB = currHr == 23. && currMin == 59. && currSec == 60. ;
    bool isC = currHr == 24. && currMin ==  0. && currSec ==  0. ;

    if( ! (isA || isB || isC) )
       setDate( currYr, currMon, currDay, 24., 0., 0.);

    return;
}

void
Date::shiftMonthBeg(void)
{
    if( currDay > 1.
              || currHr > 0. || currMin > 0. || currSec > 0. )
      setDate( currYr, currMon, 1., 0., 0., 0.);

    return;
}

void
Date::shiftMonthEnd(void)
{
    double nd = getMonthDaysNum();

    bool isLastDay = nd == currDay ;
    bool isA = isLastDay && currHr == 24. && currMin == 0. && currSec == 0. ;
    bool isB = isLastDay && currHr == 23. && currMin == 59. && currSec == 60. ;
    bool isC = isLastDay && currHr == 23. && currMin == 60. && currSec == 0. ;

    if( ! (isA || isB || isC) )
       setDate( currYr, currMon, nd, 24., 0., 0.);
    return;
}

void
Date::shiftYearBeg(void)
{
    if( ! ( currMon == 1. && currDay == 1.
              && currHr == 0. && currMin == 0. && currSec == 0. ) )
       setDate( currYr, 1., 1., 0., 0., 0.);
    return;
}

void
Date::shiftYearEnd(void)
{
    bool isLastDay = currMon == 12. && currDay == 31.;
    bool isA = isLastDay && currHr == 24. && currMin == 0. && currSec == 0. ;
    bool isB = isLastDay && currHr == 23. && currMin == 59. && currSec == 60. ;
    bool isC = isLastDay && currHr == 23. && currMin == 60. && currSec == 0. ;

    if( ! ( isA || isB || isC) )
       setDate( currYr, 12., 31., 24., 0., 0.);
    return;
}

std::string
Date::str(void)
{
  return getISO_8601();
}

/*
void
Date::utcToLocal( void )
{
    jul += getLocalTimeZone() / 24. ;
//    jul += getDayLightST() / 24. ;
    return  ;
}
*/

// ########### nested class Julian

Date::Julian::Julian()
{
  init();
  jdn=0;
  time=0.;
}

Date::Julian::Julian(long double d)
{
  init();
  set( d );
}

Date::Julian::Julian(long int d, double h)
{
  init();

  set( d, h );
}

Date::Julian::Julian(const Date::Julian &j)
{
  init();
  jdn=j.jdn ;
  time=j.time;
}

Date::Julian
Date::Julian::operator=( const Julian &j )
{
  init();

  jdn=j.jdn ;
  time=j.time;

  return *this;
}

Date::Julian
Date::Julian::operator=( long double d )
{
  set(d);

  return *this;
}

bool
Date::Julian::operator<( const Julian &d) const
{
  return ( (jdn + time) < (d.jdn + d.time) )
              ? true : false ;
}

bool
Date::Julian::operator==( const Julian &d) const
{
  return ( (jdn + time) == (d.jdn + d.time) )
              ? true : false ;
}

bool
Date::Julian::operator!=( const Julian &d) const
{
  return ( (jdn + time) != (d.jdn + d.time) )
              ? true : false ;
}

bool
Date::Julian::operator>( const Julian &d) const
{
  return ( (jdn + time) > (d.jdn + d.time) )
              ? true : false ;
}

bool
Date::Julian::operator>=( const Julian &d) const
{
  return ( (jdn + time) >= (d.jdn + d.time) )
              ? true : false ;
}

bool
Date::Julian::operator<=( const Julian &d) const
{
  return ( (jdn + time) <= (d.jdn + d.time) )
              ? true : false ;
}

Date::Julian&
Date::Julian::operator += ( const Date::Julian &j)
{
  jdn += j.jdn ;
  time += j.time ;

  adjust();

  return *this;
}

Date::Julian&
Date::Julian::operator += ( long double d)
{
  add( d ) ;
  return *this;
}

Date::Julian&
Date::Julian::operator -= ( const Date::Julian &j)
{
  jdn -= j.jdn ;
  time -= j.time ;

  adjust();

  return *this;
}

Date::Julian&
Date::Julian::operator -= ( long double d)
{
  subtract(d);

  return *this;
}

void
Date::Julian::add(long double d)
{
  long double d_num = static_cast<long double>(
     static_cast<long int>(d) ) ;

  long double hr = d - d_num ;

  jdn  += d_num ;
  time += hr ;

  adjust();

  return;
}

void
Date::Julian::adjust(void)
{
  while( time > 1. )
  {
    jdn += 1. ;
    time -= 1. ;
  }

  while( time < 0. )
  {
    jdn -= 1. ;
    time += 1. ;
  }

  return;
}

void
Date::Julian::init(void)
{
    jdn_limit = 291469500291.;
    return;
}

bool
Date::Julian::isValid(long double d)
{
  long double dif = jdn + d;

  // d > 0           || d < 0
  if(dif > jdn_limit || (jdn + d) < 0.)
     return false;

  return true;
}

void
Date::Julian::set(long double d)
{
  // The Julian Day Number is stored, which changes at noon.
  // The civil day is represented by time, which must not be
  // added with JDN
  if( d == 0. )
  {
    jdn=0;
    time=0.;
    return;
  }

  // cast to noon
  jdn =static_cast<long double>( static_cast<long int>(d) ) ;

  // decimal fraction
  time = d - jdn ;

  // shift fraction to civil time
  if( time > 0.5 )
  {
    jdn += 1. ;
    time -= 0.5;
  }
  else
    time += 0.5;

  return;
}

void
Date::Julian::set(long double j, double h)
{
  // The Julian Day Number is stored, which changes at noon.
  // The civil day is represented by time, which must not be
  // added with JDN

  jdn = j ;

  // decimal fraction
  time = h / 24. ;

  return;
}

void
Date::Julian::subtract(long double d)
{
  long double d_num = static_cast<long double>(
     static_cast<long int>(d) ) ;

  long double hr = d - d_num ;

  jdn  -= d_num ;
  time -= hr ;

  adjust();

  return;
}
