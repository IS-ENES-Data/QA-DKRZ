QA_Time::QA_Time()
{
  initDefaults();
}

/*
QA_Time::~QA_Time()
{
 ;
}
*/

void
QA_Time::applyOptions(std::vector<std::string> &optStr)
{
  for( size_t i=0 ; i < optStr.size() ; ++i)
  {
     Split split(optStr[i], "=");

     if( split[0] == "aMDR"
            || split[0] == "applyMaximumDateRange"
                 || split[0] == "apply_maximum_date_range" )
     {
          isMaxDateRange=true;

          if( split.size() == 2 )
             maxDateRange=split[1];

          continue;
     }

     if( split[0] == "dTB" || split[0] == "disableTimeBoundsCheck"
         || split[0] == "disable_time_bounds_check" )
     {
          disableTimeBoundsTest() ;
          continue;
     }

     if( split[0] == "iRD" || split[0] == "ignoreReferenceDate"
         || split[0] == "ignore_reference_date" )
     {
          isReferenceDate=false ;
          continue;
     }

     if( split[0] == "iRDAE" || split[0] == "ignoreRefDateAcrossExp"
         || split[0] == "ignore_ref_date_across_exp" )
     {
          isReferenceDateAcrossExp=false ;
          continue;
     }

     if( split[0] == "calendar" )
     {
          calendar=split[1];
          isNoCalendar=false ;
          continue;
     }

     if( split[0] == "nRTS" || split[0] == "nonRegularTimeSteps"
       || split[0] == "non_regular_time_steps" )
     {
          isRegularTimeSteps=false ;
          continue;
     }

     if( split[0] == "tTR"
         || split[0] == "tableTimeRanges"
            || split[0] == "table_time_ranges" )
     {
          timeTable.setFile(split[1]) ;
          timeTableMode=INIT;
          continue;
     }
   }

   if( timeTable.path.size() == 0 )
      timeTable.setPath(pQA->tablePath);

   return;
}

void
QA_Time::finally(NcAPI *nc)
{
  if( ! (isTime || pQA->isCheckTime ) )
    return;

  timeOutputBuffer.flush();
  sharedRecordFlag.flush();

  if( refDate.getDate(firstTimeValue) <= refDate.getDate(currTimeValue) )
  {
    // not true for a gap across a file, because then currDate
    // represents the last date from the previous file.
    std::string out( "PERIOD-BEG " );

    // if bounds are available, then use them
    if( isTimeBounds )
    {
      out += refDate.getDate(firstTimeBoundsValue[0]).str();
      out += " - " ;
      out += refDate.getDate(lastTimeBoundsValue[1]).str();
    }
    else
    {
      out += refDate.getDate(firstTimeValue).str() ;
      out += " - " ;
      out += refDate.getDate(lastTimeValue).str() ;
    }

    out += "PERIOD-END";  // mark of the end of an output line
    std::cout << out << std::endl;
  }

  // write internal data to variable time
  nc->setAtt( name, "last_time", lastTimeValue);
  nc->setAtt( name, "last_date", refDate.getDate(lastTimeValue).str() );
  nc->setAtt( name, "isTimeBoundsTest", static_cast<double>(isTimeBounds));

  if( pIn->currRec )
  {
    // catch a very special one: the initial file got only a
    // single record and the period is longer than a day, then
    // the time step is twice as long.
    double tmp;

    if( pQA->currQARec == 1 )  // due to the final loop increment
    {
      refTimeStep *= 2. ;
      tmp=2.*currTimeStep;
    }
    else
      tmp=currTimeStep;

    nc->setAtt( name+"_step", "last_time_step", tmp);
  }

  nc->setAtt( name, "last_time_bnd_0", prevTimeBoundsValue[0]);
  nc->setAtt( name, "last_time_bnd_1", prevTimeBoundsValue[1]);

  return;
}

void
QA_Time::getDate(Date& d, double t)
{
/*
  if( isFormattedDate )
  {
     d.setDate( Date::getIso8601(t), calendar ) ;
  }
  else
  {
*/
    d=refDate;
    d.addTime(t) ;
//  }

  return ;
}

void
QA_Time::getDRSformattedDateRange(std::vector<Date> &period,
       std::vector<std::string> &sd)
{
  std::vector<std::string> iso;

  // be prepared for some short-cuts to be valid
  // (e.g. 1961 equiv. 1961-01 equiv. 1961-01-01)
  iso.push_back( "0000-01-01T00:00:00" );
  iso.push_back( "0000-01-01T00:00:00" );

  // set dates of time periods
  for( size_t i=0 ; i < sd.size() ; ++i )
  {
    if( sd[i].size() > 3 )
      iso[i].replace(0, 4, sd[i], 0, 4);
    if( sd[i].size() > 5 )
      iso[i].replace(5, 2, sd[i], 4, 2);
    if( sd[i].size() > 7 )
      iso[i].replace(8, 2, sd[i], 6, 2);
    if( sd[i].size() > 9 )
      iso[i].replace(11, 2, sd[i], 8, 2);
    if( sd[i].size() > 11 )
      iso[i].replace(14, 2, sd[i], 10, 2);
    if( sd[i].size() > 13 )
      iso[i].replace(17, 2, sd[i], 12, 2);
  }

  // Apply maximum date ranges when enabled; times remain sharp.
  // Take into account individual settings 'Yx-Mx-...' with
  // x=s for a sharp deadline and x=e for extended maximum period.
  // Defaults for x=s.

  if( sd.size() == 2 && sd[1].size() && isMaxDateRange )
  {
    if( maxDateRange.size() == 0 )
      maxDateRange = "Ye-Me-De-hs-ms-ss" ;

     // year
     if( sd[1].size() == 4 )
     {
        if( maxDateRange.find("Ye") < std::string::npos )
          iso[1].replace(5, 2, "12-31T24");
     }
     else if( sd[1].size() == 6 )
     {
        // months
        if( maxDateRange.find("Me") < std::string::npos )
        {
           // find the end of the month specified
           Date xD(sd[1], refDate.getCalendar());
           double md = xD.getMonthDaysNum(
              hdhC::string2Double( iso[1].substr(5,2) ),    // month
              hdhC::string2Double( iso[1].substr(0,4) ) ) ; //year

          // convert to string with leading zero(s)
          std::string smd( hdhC::double2String(md, 0, "2_0") );

          // the number of days of the end month of the period
          iso[1].replace(8, 2, smd);
          iso[1].replace(11, 2, "24");
        }
     }
     else if( sd[1].size() == 8 )
     {
        // days
        if( maxDateRange.find("De") < std::string::npos )
           iso[1].replace(11, 2, "24");
     }
     else if( sd[1].size() == 10 )
     {
        // hours
        if( maxDateRange.find("he") < std::string::npos )
           iso[1].replace(14, 2, "60");
     }
     else if( sd[1].size() == 12 )
     {
        // minutes
        if( maxDateRange.find("me") < std::string::npos )
           iso[1].replace(17, 2, "60");
     }
     // note: no seconds
  }

  // set dates of time periods
  if( period.size() )
  {
    for( size_t i=0 ; i < sd.size() ; ++i )
    {
      if( sd[i].size() )
      {
         period[i].setUnits( refDate.getUnits() );
         period[i].setDate(iso[i], calendar) ;
      }
    }
  }
  else
  {
    for( size_t i=0 ; i < sd.size() ; ++i )
    {
      if( sd[i].size() )
      {
         period.push_back( Date(iso[i], calendar) );
         period.back().setUnits( refDate.getUnits() );
      }
    }
  }

  return;
}

double
QA_Time::getTimeValue(int rec)
{
  double tv;
  MtrxArr<double> ma_t;

  if( (tv=pIn->nc.getData(ma_t, name, rec)) < MAXDOUBLE)
      tv += refTimeOffset;

  return tv;
}

void
QA_Time::getTimeBoundsValues(double* pair, size_t rec)
{
  MtrxArr<double> ma_tb;
  (void) pIn->nc.getData(ma_tb, boundsName, rec);
  double** m2D = ma_tb.getM();

  pair[0]=m2D[0][0] + refTimeOffset;
  pair[1]=m2D[0][1] + refTimeOffset;

  return ;
}

bool
QA_Time::init(std::vector<std::string>& optStr)
{
   if( !pQA->isCheckTime )
      return (isTime=false) ;

   name = pIn->nc.getUnlimitedDimRepName();

   if( name.size() )
   {
     for( size_t i=0 ; i < pIn->varSz ; ++i )
     {
       if( pIn->variable[i].name == name )
       {
          time_ix = i;
          isTime=true;
          boundsName = pIn->variable[time_ix].getAttValue("bounds") ;
          break;
       }
     }
   }
   else
   {
     size_t i;
     for( i=0 ; i < pIn->varSz ; ++i )
     {
       if( pIn->variable[i].name == name )
       {
         name = pIn->variable[i].name ;
         time_ix = static_cast<int>(i) ;
         isTime=true;

         boundsName = pIn->variable[time_ix].getAttValue("bounds") ;
         break;
       }
     }

     if( time_ix == -1 )
     {
       if( pQA->isCheckTime )
         name="fixed";

       return false;
     }
   }

   // is time unlimited?
   if( isTime && ! pIn->nc.isDimUnlimitedGenuine() )
   {
        std::string key("T_1");
        if( notes->inq( key, name) )
        {
            std::string capt("Warning: Dimension " + hdhC::tf_val(name));
            capt += " is not unlimited";

            if( notes->operate(capt) )
            {
                notes->setCheckStatus("TIME", fail);
                pQA->setExitState( notes->getExitState() ) ;
            }
        }

   }

   // time_bnds available? Yes, then enable a check
   if( boundsName.size() )
   {
      timeBounds_ix = pIn->getVarIndex(boundsName);

      if( timeBounds_ix == -1 )
      {
        std::string key("T_2");
        if( notes->inq( key, name) )
        {
            std::string capt(hdhC::tf_var(boundsName));
            capt += " is missing, but was declared by ";
            capt += hdhC::tf_att(name, "time_bnds") ;

            if( notes->operate(capt) )
            {
                notes->setCheckStatus("TIME", fail);
                pQA->setExitState( notes->getExitState() ) ;
            }
        }
      }
      else if( ! pIn->variable[timeBounds_ix].isExcluded )
        enableTimeBoundsTest();
   }

//   if( pIn->nc.isEmptyData(name) )
//     return false;

   timeOutputBuffer.initBuffer(pQA, pQA->currQARec, pQA->bufferSize);
   timeOutputBuffer.setName(name);

   sharedRecordFlag.initBuffer(pQA, pQA->currQARec, pQA->bufferSize);
   sharedRecordFlag.setName( name + "_flag" );

   // set date to a reference time
   std::string str(pIn->getTimeUnit());

   if( str.size() == 0 ) // fixed field
     return false ;
   else if( str.find("%Y") < std::string::npos )
   {
     if( initAbsoluteTime(str) )
        return false;
   }
   else if( initRelativeTime(str) )
       return false;  // could  not read any time value

   applyOptions(optStr);
   initTimeTable();

   return true;
}


bool
QA_Time::initAbsoluteTime(std::string &units)
{
  size_t i;
  for( i=0 ; i < pIn->varSz ; ++i)
    if( pIn->variable[i].name == name )
       break;

  if( i == pIn->varSz )
     return true;  // no time

  isFormattedDate = true;

  // proleptic Gregorian, no-leap, 360_day
  calendar = pIn->nc.getAttString("calendar", name);
  if( calendar.size() )
  {
     isNoCalendar=false;
     refDate.setCalendar(calendar);
  }

  // time_bounds
  disableTimeBoundsTest();
  boundsName = pIn->variable[i].bounds;

  // time_bnds available? Yes, then enable a check
  if( boundsName.size() )
     if( ! pIn->variable[i].isExcluded )
        enableTimeBoundsTest();

  refTimeOffset=0.;

  Split x_units(units);
  for( size_t i=0 ; i< x_units.size() ; ++i )
  {
     if( x_units[i][0] == '%' )
     {
        dateFormat = x_units[i] ;
        break;
     }
  }

  if( (firstTimeValue = getTimeValue(0)) == MAXDOUBLE )
     return true;

  size_t recSz = pIn->nc.getNumOfRows(name) ;
  lastTimeValue = getTimeValue(-1) ;

  if( prevTimeValue == MAXDOUBLE )
  {
     // 0) no value; caught elsewhere
     if( recSz == 0 )
       return false;

     // 1) out of two time values within a file
     if( recSz > 1 )
     {
       if( pIn->currRec < recSz )
         prevTimeValue = getTimeValue(1) ;

       // an arbitrary setting that would pass the first test;
       // the corresponding results is set to FillValue
       Date prevDate(prevTimeValue, calendar);
       Date currDate(firstTimeValue, calendar);
       refTimeStep=fabs(prevDate.getJulianDay() - currDate.getJulianDay());
       prevDate.addTime( -2.*refTimeStep ) ;

       std::string s(prevDate.str());
       std::string t(s.substr(0,4));
       t += s.substr(5,2);
       t += s.substr(8,2);
       double h = hdhC::string2Double(s.substr(11,2));
       h += hdhC::string2Double(s.substr(14,2)) / 60.;
       h += hdhC::string2Double(s.substr(17,2)) / 60.;
       prevTimeValue = hdhC::string2Double(t);
       prevTimeValue += h/24.;
     }
     else
     {
       prevTimeValue = 0. ;
       refTimeStep=0.;
     }

     if( isTimeBounds )
     {
       double tb[2];
       getTimeBoundsValues(tb, pIn->currRec) ;

       double dtb = tb[1] - tb[0] ;
       prevTimeBoundsValue[0] = tb[0] - dtb ;
       prevTimeBoundsValue[0] = tb[1] ;
     }
  }

  if( isTimeBounds )
  {
    getTimeBoundsValues(firstTimeBoundsValue, 0) ;
    getTimeBoundsValues(lastTimeBoundsValue, recSz-1) ;
  }

  return false;
}


void
QA_Time::initDefaults(void)
{
   notes=0;
   pIn=0;
   pQA=0;

   currTimeStep=0 ;
   prevTimeValue=MAXDOUBLE;
   prevTimeBoundsValue[0]=0.;
   prevTimeBoundsValue[1]=0.;
   refTimeStep=0.;

   firstTimeValue=0.;
   currTimeValue=0.;
   lastTimeValue=0.;

   firstTimeBoundsValue[0]=0.;
   currTimeBoundsValue[0]=0.;
   lastTimeBoundsValue[0]=0.;
   firstTimeBoundsValue[1]=0.;
   currTimeBoundsValue[1]=0.;
   lastTimeBoundsValue[1]=0.;

   // time steps are regular. Unsharp logic (i.e. month
   // Jan=31, Feb=2? days is ok, but also numerical noise).

   isFormattedDate=false;
   isMaxDateRange=false;
   isNoCalendar=true;
   isNoProgress=false;
   isPrintTimeBoundDates=false;
   isReferenceDate=true ;
   isRegularTimeSteps=true;
   isSingleTimeValue=false;
   isTime=false;
   isTimeBounds=false;

   time_ix = -1 ;
   timeBounds_ix = -1 ;

   refTimeOffset=0.; // !=0, if there are different reference dates

   bufferCount=0;
   maxBufferSize=1500;

   timeTableMode=UNDEF;

   fail="FAIL";
   notAvailable="N/A";

   return;
}

bool
QA_Time::initRelativeTime(std::string &units)
{
   if( !isTime )
     return true;  // no time

   // proleptic Gregorian, no-leap, 360_day
   calendar = pIn->nc.getAttString("calendar", name);

   if( calendar.size() )
   {
      isNoCalendar=false;
      refDate.setCalendar(calendar);
   }
   refDate.setDate( units );

   if( (firstTimeValue = getTimeValue(0)) == MAXDOUBLE )
      return true;

   currTimeValue = firstTimeValue;

   size_t recSz = pIn->nc.getNumOfRows(name) ;

   if( prevTimeValue == MAXDOUBLE )
   {
     // 0) no value; caught elsewhere
     if( recSz == 0 )
       return false;

     // 1) out of two time values within a file
     if( recSz > 1 )
     {
       double t1=2.*firstTimeValue;

       if( pIn->currRec < recSz )
          t1 = getTimeValue(1) ;

       // an arbitrary setting that would pass the first test;
       // the corresponding results is set to FillValue
       refTimeStep=fabs(t1 - firstTimeValue);
     }

     // 2) guess
     else
     {
       // a) from time:units and :frequency
       std::string freq( pQA->qaExp.getFrequency() );
       if( freq.size() && refDate.getUnits() == "day" )
       {
         if( freq == "3hr" )
           refTimeStep = 1./8. ;
         else if( freq == "6hr" )
           refTimeStep = 1./4. ;
         else if( freq == "day" )
           refTimeStep = 1. ;
         else if( freq == "mon" )
           refTimeStep = 30. ;
         else if( freq == "yr" )
         {
           if( refDate.getCalendar() == "equal_month" )
             refTimeStep = 360. ;
           else
             refTimeStep = 365. ;
         }
       }
       else
         refTimeStep=currTimeValue;
     }

     if(recSz)
       lastTimeValue = getTimeValue(-1);
     else
       lastTimeValue = firstTimeValue ;

     prevTimeValue = firstTimeValue - refTimeStep ;

     if( isTimeBounds )
     {
       if( initTimeBounds() )
       {
         double dtb = firstTimeBoundsValue[1] - firstTimeBoundsValue[0];

         prevTimeBoundsValue[0]=firstTimeBoundsValue[0] - dtb ;
         prevTimeBoundsValue[1]=firstTimeBoundsValue[0]  ;
       }
       else
       {
          // empty equivalent to only _FillValue
          isTimeBounds=false;
          std::string key("T_3a");

          if( notes->inq( key, boundsName ) )
          {
            std::string capt(hdhC::tf_var("time_bnds", hdhC::colon));
            capt += "no data" ;

            (void) notes->operate(capt) ;
            notes->setCheckStatus("TIME", pQA->n_fail);
          }
       }
     }
   }

   return false;
}

void
QA_Time::initResumeSession(void)
{
    // if files are synchronised, i.e. a file hasn't changed since
    // the last qa, this will exit in member finally()
   isNoProgress = sync();

   std::vector<double> dv;

   // Simple continuation.
   // Note: this fails, if the previous file has a
   //       different reference date AND the QA resumes the
   //       current file after an error.
   pQA->nc->getAttValues( dv, "last_time", name);
   prevTimeValue=dv[0];

   pQA->nc->getAttValues( dv, "last_time_bnd_0", name);
   prevTimeBoundsValue[0]=dv[0];

   pQA->nc->getAttValues( dv, "last_time_bnd_1", name);
   prevTimeBoundsValue[1]=dv[0];

   pQA->nc->getAttValues( dv, "last_time_step", name + "_step");
   refTimeStep=dv[0];

   // case: two different reference dates are effective.
   std::string tu_0(pQA->nc->getAttString("units", name));
   std::string tu_1(pIn->nc.getAttString("units", name));
   if( ! (tu_0 == tu_1 ) )
   {
      if( ! isFormattedDate )
      { // note: nothing for fomatted dates
        Date thisRefDate( tu_1, calendar );

        // Current refDate is set to thisDate.
        // Reset it to the previous date.
        refDate.setDate( tu_0 );
        Date d_x0( refDate );
        Date d_x1( thisRefDate );

        // adjust all time values of the current file to
        // the reference date of the first chunk, which is
        // stored in the qa-nc file
        refTimeOffset= d_x0.getSince( d_x1 );
      }
   }

   // get internal values
   isTimeBounds =
     static_cast<bool>(pQA->nc->getAttValue("isTimeBoundsTest", name));

   return;
}

bool
QA_Time::initTimeBounds(void)
{
  if( timeBounds_ix == -1 )
  {
    firstTimeBoundsValue[0]=0.;
    firstTimeBoundsValue[1]=0.;
    lastTimeBoundsValue[0]=0.;
    lastTimeBoundsValue[1]=0.;
    return false;
  }

  getTimeBoundsValues(firstTimeBoundsValue, 0);

  size_t sz = pIn->nc.getNumOfRows(boundsName);
  if(sz)
      --sz;

  getTimeBoundsValues(lastTimeBoundsValue, sz);

  ANNOT_ACCUM="ACCUM";

  return true;
}

void
QA_Time::initTimeTable(void)
{
   std::string id_1st(QA::tableID);
   std::string id_2nd(QA::tableIDSub);

   if( timeTableMode == UNDEF )
     return;

   if( id_1st.size() == 0 )
   {
     timeTableMode = UNDEF ;
     return;
   }

   tt_block_rec=0;
   tt_count_recs=0;

   tt_id = id_1st; // only purpose: designator i output
   if( id_2nd.size() )
   {
      tt_id += '_' ;
      tt_id += id_2nd ;
   }

   // default for no table found: "id_1st,,any(regular)"
   if( !timeTable.is() )
   {
     timeTableMode = REGULAR ;
     return ;
   }

   // This class provides the feature of putting back an entire line
   ReadLine ifs(timeTable.getFile());

   if( ! ifs.isOpen() )
   {
     timeTableMode = REGULAR ;
     return ;
   }

   ifs.skipLeadingChar();
   ifs.skipComment();

   // find the identifier in the table
   Split splt_line;
   splt_line.setSeparator(',');
   splt_line.enableEmptyItems();

   std::string str;

   do
   {
     // the loop cycles over the sub-tables

     //find a line with the current table name
     if( ifs.findLine(str, id_1st) )
     {
        //default for missing/omitted frequencies in the time table
        timeTableMode = REGULAR ;
//        ifs.close();  // no longer needed
        return ;
     }

     splt_line=str;
     if( splt_line[1].size() )
     {
       // find the sub-table, if there is any (not CORDEX)
       std::string sstr(splt_line[1].substr(0,10)) ;

       // substr is necessary for sub-tables with a counting number added
       if( sstr == id_2nd.substr(0,10) )
          break ;
     }
     else
        break;
   } while( true );

   // ok, the right line was found. Now, extract experiments.
   // Take continuation lines into account.
   std::string str_exp( splt_line[2] ) ;
   size_t pos;

   do
   {
      if( ifs.getLine(str) )
        break;

      if( (pos=str.find(':')) == std::string::npos )
        break; // this is no continuation line

      str_exp += str.substr(++pos) ;
   } while ( true );

   // erase all blanks:
   str = hdhC::clearSpaces(str_exp);
   str_exp.clear();

   // insert a ' ' after each closing ')', but the last
   size_t pos0=0;
   size_t sz = str.size();
   do
   {
      if( (pos = str.find(')',pos0 )) < std::string::npos )
      {
        ++pos;
        str_exp += str.substr(pos0, pos - pos0);

        if( pos == sz )
          break;

        str_exp += ' ';
        pos0 = pos ;
      }
   } while( true );

   // scan the time-table entry for the current experiments
   size_t p0=0;
   Split splt_exp ;
   splt_exp = str_exp ;

   for( size_t i=0 ; i < splt_exp.size() ; ++i )
   {
     str_exp = splt_exp[i] ;

     p0=str_exp.find('(') + 1;

     // find matching experiment
     std::string str2(str_exp.substr(0,p0-1));
   }

   // Get value(s) from the time-table.
   // Pairs of () must match; this is not checked.
   tt_xmode.setSeparator('|');
   tt_xmode = str_exp.substr(p0, str_exp.size() - p0 - 1) ;

   if( tt_xmode[0] == "regular" )
     timeTableMode = REGULAR ;
   else if( tt_xmode[0] == "orphan" )
     timeTableMode = ORPHAN ;
   else if( tt_xmode[0] == "disable" )
     timeTableMode = DISABLE ;
   else
   {
     // other modes are updated in cycles
     timeTableMode = CYCLE ;
     tt_index=0;

     // init a range
     Split splt_range;
     splt_range.setSeparator('-');
     std::vector<std::string> sd;
     splt_range = tt_xmode[0] ;

     sd.push_back( splt_range[0]) ;

     if( splt_range.size() == 2 )
       sd.push_back( splt_range[1] );

     getDRSformattedDateRange(tt_dateRange, sd);

     if( refDate.getDate(currTimeValue) < tt_dateRange[0] )
     { // error: time record out of range (before)
       std::string key("T_4");
       if( notes->inq( key, name) )
       {
         std::string capt("time value before the first time-table range");

         std::string text("frequency=" + id_1st);
         text += "\nrec#=0" ;
         text += "\ndate in record=" ;
         text += refDate.getDate(currTimeValue).str() ;
         text += "\nrange from time-table=" ;
         text += tt_dateRange[0].str() + " - " ;
         text += tt_dateRange[1].str() ;

         if( notes->operate(capt, text) )
         {
           notes->setCheckStatus("TIME", fail);
           pQA->setExitState( notes->getExitState() ) ;
         }
       }
     }
   }

   return;
}

bool
QA_Time::parseTimeTable(size_t rec)
{
   if( timeTableMode == UNDEF )
     return false;

   // return == true: no error messaging, leave the
   // calling method, i.e. testTimeStep, immediately.

   // note: this method could be called severalm times,
   //       even for the mode ORPHAN

   // preserve over the record for some operational modes
   if( tt_block_rec == rec && tt_isBlock )
     return true;
   else
     tt_isBlock=false;

   // regular time test for each record
   if( timeTableMode == REGULAR )
     return false;

   if( timeTableMode == DISABLE )
     return true;

   // No time test for the first record, after this regular.
   if( timeTableMode == ORPHAN )
   {
     tt_isBlock=true;
     tt_block_rec = rec ;
     timeTableMode = REGULAR ;
     return true;
   }

   if( timeTableMode == CYCLE
          && refDate.getJulianDate(currTimeValue)
                  < tt_dateRange[1].getJulianDate() )
     return false; //an error happened within the range

  // other modes are updated in cycles
  if( tt_xmode[0][0] == 'N' )
  {
     ++tt_count_recs ;
     size_t num = static_cast<size_t>(
           hdhC::string2Double( tt_xmode[0] ) ) ;

     if( tt_count_recs > num )
     {  // issue an error flag; number of records too large
        std::string key("T_5");
        if( notes->inq( key, name) )
        {
          std::string capt("too many time values compared to the time-table");

          std::string text("frequency=" + tt_id);
          text += "\nrec#=" + hdhC::double2String(rec);
          text += "\ndate in record=" ;
          text += refDate.getDate(currTimeValue).str() ;
          text += "\nvalue from time-table=" + tt_xmode[tt_index] ;

          if( notes->operate(capt, text) )
          {
            notes->setCheckStatus("TIME", fail);
            pQA->setExitState( notes->getExitState() ) ;
          }
        }
     }

     tt_block_rec = rec ;
     tt_isBlock=true;

     return true;
  }
  else if( tt_xmode[0][0] == '+' )
  {
    // too less information  for an algorithmic approach
    timeTableMode = DISABLE ;
    return true;
  }
  else if( tt_xmode[tt_index].find('-') == std::string::npos )
  {  // singular values must match; but this could happen
     // several times (e.g. 12 months a year)
     // update the static index
     std::string currDateStr(refDate.getDate(currTimeValue).str());
     std::string t0( currDateStr.substr(0,4) );
     while( t0 > tt_xmode[tt_index] )
       ++tt_index;

     if( tt_index == tt_xmode.size() || t0 != tt_xmode[tt_index] )
     {
       std::string key("T_6");
       if( notes->inq( key, name) )
       {
         std::string capt("time value does not match time_table value");

         std::string text("frequency=" + tt_id);
         text += "\nrec#=" + hdhC::double2String(rec);
         text += "\ndate in record=" ;
         text += refDate.getDate(currTimeValue).str() ;
         text += "\nvalue from time-table=" + tt_xmode[tt_index] ;

         if( notes->operate(capt, text) )
         {
            notes->setCheckStatus("TIME", fail);
            pQA->setExitState( notes->getExitState() ) ;
         }
       }
     }

     tt_block_rec = rec ;
     tt_isBlock=true;

     return true;
  }

  // a range is specified
  Split splt_range;
  splt_range.setSeparator('-');
  std::vector<std::string> sd(2);

  do
  {
    if( tt_index == tt_xmode.size() )
    { // error: not enough ranges or misplaced
       std::string key("T_7");
       if( notes->inq( key, name) )
       {
         std::string capt("time record exceeds the time-table range");

         std::ostringstream ostr(std::ios::app);
         ostr << "rec#=" << rec;
         ostr << "\ndate in record=" << refDate.getDate(currTimeValue).str() ;
         ostr << "\nrange from time-table="
              << tt_dateRange[0].str() << " - "
              << tt_dateRange[1].str() ;

         if( notes->operate(capt, ostr.str()) )
         {
            notes->setCheckStatus("TIME", fail);
            pQA->setExitState( notes->getExitState() ) ;
         }
       }

       timeTableMode = DISABLE ;
       return true;
    }

    splt_range = tt_xmode[tt_index++] ;

    sd[0] = splt_range[0];
    sd[1] = splt_range[1];

    getDRSformattedDateRange(tt_dateRange, sd);
  } while( refDate.getJulianDate(currTimeValue)
              > tt_dateRange[1].getJulianDate() ) ;

  tt_block_rec = rec ;
  tt_isBlock=true;

  return true;
}


void
QA_Time::openQA_NcContrib(NcAPI *nc)
{
   // dimensions
   nc->defineDim(name);
   nc->copyVarDef(pIn->nc, name);
   nc->setDeflate(name, 1, 1, 9);

   std::vector<std::string> vs;

   // time increment
   std::string tU;

   // get time attribute units and extract keyword
   pIn->nc.getAttValues(vs, "units", name) ;
   std::string tInc( vs[0].substr(0, vs[0].find(" since")) );

   vs.clear();
   vs.push_back(name);

   nc->setAtt( name, "first_time", firstTimeValue);
   nc->setAtt( name, "first_date", refDate.getDate(firstTimeValue).str() );
   nc->setAtt( name, "isTimeBoundsTest", static_cast<double>(0.));

   std::string str0(name+"_step");
   nc->defineVar( str0, NC_DOUBLE, vs);
   nc->setDeflate(str0, 1, 1, 9);
   nc->setAtt( str0, "long_name", name) ;
   nc->setAtt( str0, "units", tInc);

   vs.clear();
   str0 = name + "_flag" ;
   vs.push_back(name);
   nc->defineVar( str0, NC_INT, vs);
   nc->setDeflate(str0, 1, 1, 9);
   vs[0]="accumulated record-tag number";
   nc->setAtt( str0, "long_name", vs[0]);
   nc->setAtt( str0, "units", "1");
   nc->setAtt( str0, "_FillValue", static_cast<int>(-1));
   vs[0]="tag number composite by Rnums of the check-list table.";
   nc->setAtt( str0, "comment", vs[0]);

   return;
}

void
QA_Time::setNextFlushBeg(size_t r)
{
   nextFlushBeg=r;
   sharedRecordFlag.setNextFlushBeg(r);
   timeOutputBuffer.setNextFlushBeg(r);

   return;
}

void
QA_Time::setParent(QA* p)
{
   pQA = p;
   notes = p->notes;
   pIn = p->pIn;

   return;
}

bool
QA_Time::sync(void)
{
  // Synchronise the in-file and the qa-netCDF file.
  // Failure: call setExitState(error_code).

  // return value: true for isNoProgress

  // num of recs in current data file
  int inRecNum = pIn->nc.getNumOfRecords() ;

  // a new qa-ncfile? In fact, sync is not called for this case.
  size_t qaRecNum;
  if( (qaRecNum=pQA->nc->getNumOfRecords() ) == 0 )
    return false;

  // get last time value from the previous file
  std::vector<double> dv;
  pQA->nc->getAttValues( dv, "last_time", pIn->timeName);
  double t_qa=dv[0];

  double epsilon = refTimeStep/10.;

  // preliminary tests
  // a) a previous QA had checked a complete previous sub-temporal infile
  //    and time continues in the current infile.
  if( firstTimeValue > (t_qa+epsilon) )
    return false;

  if( hdhC::compare(t_qa, "=", lastTimeValue,  epsilon) )
    return true; // up-to-date

  // epsilon around a time value must not be too sharp, because
  // of the spread of months
  double t_qa_eps = t_qa - epsilon;

  // scanning time
  for( int i=0 ; i < inRecNum ; ++i )
  {
    double t_in = getTimeValue(i) ;

    if( t_in < t_qa_eps )
      continue;

    if( (t_qa + epsilon) < t_in )
      return false ;  // the usual case

    if( hdhC::compare(t_qa, "=", t_in, epsilon) )
    {
      // a previous QA had checked an infile that was extended
      // in the meantime.
      pIn->setCurrRec(t_in+1);

      return false ;
    }
  }

  // arriving here is an error, because the infile production
  // was reset or the file was shortened.
  std::string key("9_2");
  if( notes->inq( key, name) )
  {
     std::string capt("renewal of a file?") ;

     double t = getTimeValue(0) ;

     std::ostringstream ostr(std::ios::app);
     ostr << "\nlast time of previous QA=" << t_qa;
     ostr << "\nfirst time in file=" << t ;

     if( notes->operate(capt, ostr.str()) )
     {
       notes->setCheckStatus("TIME", fail);

       pQA->setExitState( notes->getExitState() ) ;
     }
  }

  return true;
}

void
QA_Time::testTimeBounds(NcAPI &nc)
{
  // check time bounds
  if( timeTableMode == DISABLE )
  {
    prevTimeBoundsValue[0]=currTimeBoundsValue[0];
    prevTimeBoundsValue[1]=currTimeBoundsValue[1];
    return ;
  }

  double currTimeBoundsValue[2];
  getTimeBoundsValues(&currTimeBoundsValue[0], pIn->currRec);

  double diff;

  if( isFormattedDate )
  {
    refDate.setDate( currTimeBoundsValue[0] );
    long double j0=refDate.getJulianDate();
    refDate.setDate( currTimeBoundsValue[1] );
    long double j1=refDate.getJulianDate();
    diff=static_cast<double>( j1-j0 ) ;
  }
  else
    diff=(currTimeBoundsValue[1] - currTimeBoundsValue[0]) ;

  if( diff <= 0. )
  {
    std::string key=("R8");  // no multiple
    if( notes->inq( key, name, ANNOT_ACCUM) )
    {
      sharedRecordFlag.currFlag += 8 ;

      // store this text only once in an attribute
      std::string capt("negative/zero time-bounds range") ;

      std::ostringstream ostr(std::ios::app);
      ostr.setf( std::ios::fixed, std::ios::floatfield);

      ostr << "rec#:" << pIn->currRec;
      ostr << ", tb0= " << currTimeBoundsValue[0];
      ostr << ", tb1= " << currTimeBoundsValue[1] ;

      ostr  << " " << refDate.getDate(currTimeBoundsValue[0]).str();
      ostr  << " " << refDate.getDate(currTimeBoundsValue[1]).str();

      (void) notes->operate(capt, ostr.str()) ;
      notes->setCheckStatus("TIME", fail);
    }
  }

  // checks below don't make any sense for climatologies
  if( pIn->variable[timeBounds_ix].isClimatology )
    return;

  if( isFormattedDate )
  {
    refDate.setDate( currTimeBoundsValue[0] );
    long double j0=refDate.getJulianDate();
    refDate.setDate( prevTimeBoundsValue[1] );
    long double j1=refDate.getJulianDate();
    diff=static_cast<double>( j1-j0 );
  }
  else
    diff = currTimeBoundsValue[0] - prevTimeBoundsValue[1] ;

  if( diff < 0. )
  {
    // ignore effect of least digits; rounding to the 5-th decimal
    double tb0 =
      hdhC::string2Double( hdhC::double2String(currTimeBoundsValue[0], -5) );
    double tb1 =
      hdhC::string2Double( hdhC::double2String(prevTimeBoundsValue[1], -5) );

    if( (tb0 - tb1 ) < 0. )
    {
      std::string key;
      bool isAcrossFiles = pIn->currRec == 0 ;

      if( isAcrossFiles || timeTableMode == CYCLE )
      {
        if( parseTimeTable(pIn->currRec) )
        {
          prevTimeBoundsValue[0]=currTimeBoundsValue[0];
          prevTimeBoundsValue[1]=currTimeBoundsValue[1];
          return ;  // no error messaging
        }

        key="6_10";
      }
      else
      {
        key="R16";
        sharedRecordFlag.currFlag += 16;
      }

      if( notes->inq( key, boundsName, ANNOT_ACCUM) )
      {
        std::string capt("overlapping time bounds");
        if( isAcrossFiles )
          capt = " across files" ;

        std::ostringstream ostr(std::ios::app);
        ostr.setf(std::ios::fixed, std::ios::floatfield);
        ostr << "rec#="  << pIn->currRec << std::setprecision(0);
        ostr << "\nprev t-b values=[" << prevTimeBoundsValue[0] << " - " ;
        ostr                           << prevTimeBoundsValue[1] << "]" ;

        ostr << ", dates=[" << refDate.getDate(prevTimeBoundsValue[0]).str()
             << " - ";
        ostr << refDate.getDate(prevTimeBoundsValue[1]).str() << "]" ;

        ostr << "\ncurr t-b values=[" << currTimeBoundsValue[0]
                                       << " - " << currTimeBoundsValue[1] << "]" ;
        std::string cT( hdhC::double2String(currTimeBoundsValue[0]) );
        ostr << ", dates=[" << refDate.getDate(currTimeBoundsValue[0]).str()
            << " - ";
        ostr << refDate.getDate(currTimeBoundsValue[1]).str() << "]";

        (void) notes->operate(capt, ostr.str()) ;
        notes->setCheckStatus("TIME", fail);
      }
    }
  }
  else if( isRegularTimeSteps && diff > 0. )
  {
    // ignore effect of least digits; rounding to the 5-th decimal
    double tb0 =
      hdhC::string2Double( hdhC::double2String(prevTimeBoundsValue[1], -5) );
    double tb1 =
      hdhC::string2Double( hdhC::double2String(currTimeBoundsValue[0], -5) );

    if( isMaxDateRange && (tb1 - tb0 ) == 1. )
       tb0 += 1. ;

    if( (tb1 - tb0 ) > 0. )
    {
      std::string key;
      bool isAcrossFiles = pIn->currRec == 0 ;

      if( isAcrossFiles )
        key="6_11";
      else
      {
        key="R32";
        sharedRecordFlag.currFlag += 32 ;
      }

      if( notes->inq( key, boundsName, ANNOT_ACCUM) )
      {
        std::string capt("gap between time bounds ranges");

        if( timeTableMode == CYCLE || isAcrossFiles )
        {
          if( parseTimeTable(pIn->currRec) )
          {
            prevTimeBoundsValue[0]=currTimeBoundsValue[0];
            prevTimeBoundsValue[1]=currTimeBoundsValue[1];
            return ;  // no error messaging
          }

          capt += " across files";
        }

        std::string text("rec#=");
        text += hdhC::double2String(pIn->currRec) ;
        text += "\nprev t-b values=[" ;
        text += hdhC::double2String(prevTimeBoundsValue[0]) + " - " ;
        text += hdhC::double2String(prevTimeBoundsValue[1]) + "]" ;

        text += ", [" + refDate.getDate(prevTimeBoundsValue[0]).str();
        text += " - ";
        text += refDate.getDate(prevTimeBoundsValue[1]).str() + "]" ;

        text += "\ncurr t-b values=[" ;
        text += hdhC::double2String(currTimeBoundsValue[0]) + " - " ;
        text += hdhC::double2String(currTimeBoundsValue[1]) + "]";
        text += ", [" + refDate.getDate(currTimeBoundsValue[0]).str();
        text += " - ";
        text += refDate.getDate(currTimeBoundsValue[1]).str() + "]";

        (void) notes->operate(capt, text) ;
        notes->setCheckStatus("TIME", fail);
      }
    }
  }

  prevTimeBoundsValue[0]=currTimeBoundsValue[0];
  prevTimeBoundsValue[1]=currTimeBoundsValue[1];

  return ;
}

void
QA_Time::testDate(NcAPI &nc)
{
  currTimeValue = getTimeValue(pIn->currRec) ;

  // is current time reasonable?
  (void) testTimeStep() ;

  if( isTimeBounds  )
    testTimeBounds(nc);

  prevTimeValue=currTimeValue;

  return;
}

bool
QA_Time::testPeriod(Split& x_f)
{
  // return true, if a file is supposed to be not complete.
  // return false, a) if there is no period in the filename
  //               b) if an error was found
  //               c) times of period and file match

  // The time value given in the first/last record is assumed to be
  // in the range of the period of the file, if there is any.

  // If the first/last date in the filename period and the
  // first/last time value match within the uncertainty of the
  // time-step, then the file is complete.
  // If the end of the period exceeds the time data figure,
  // then the nc-file is considered to be not completely processed.

  // Does the filename have a trailing date range?
  std::vector<std::string> sd;
  sd.push_back( "" );
  sd.push_back( "" );

  // is it formatted as expected?
  if( testPeriodFormat(x_f, sd) )
    return true;

  // now that we have found two candidates for a date
  // compose ISO-8601 strings

  Date* pDates[6];
  // index 2: date of first time value
  // index 3: date of last  time value
  // index 4: date of first time-bound value, if available; else 0
  // index 5: date of last  time-bound value, if available; else 0

  pDates[0] = new Date() ;
  pDates[1] = new Date() ;

  pDates[0]->setFormattedDate();
  pDates[0]->setDate(sd[0], refDate.getCalendar());

  pDates[1]->setFormattedDate();
  pDates[1]->setDate(sd[1], refDate.getCalendar());

  // necessary for validity (not sufficient)
  if( *(pDates[0]) != *(pDates[1]) && *(pDates[0]) > *(pDates[1]) )
  {
     std::string key("T_10a");
     if( notes->inq( key, pQA->fileStr) )
     {
       std::string capt("Filename: invalid StartTime-EndTime");
       std::string text("Found");
       text += hdhC::tf_val(sd[0] + "-" + sd[1]);

       (void) notes->operate(capt, text) ;
       notes->setCheckStatus(pQA->drsF, pQA->n_fail );
     }

     return false;
  }

  for( size_t i=2 ; i < 6 ; ++i )
     pDates[i] = 0 ;

  pDates[2] = new Date(refDate);
  if( firstTimeValue != 0. )
    pDates[2]->addTime(firstTimeValue);

  pDates[3] = new Date(refDate);
  if( lastTimeValue != 0. )
    pDates[3]->addTime(lastTimeValue);

  if( isTimeBounds)
  {
    if( pQA->pIn->variable[time_ix].isInstant )
    {
        std::string tb_name(getBoundsName());

        if( tb_name.size() )
        {
            std::string key("T_3c");

            if( notes->inq( key, tb_name ) )
            {
              std::string capt(hdhC::tf_var(tb_name));
              capt += " declaration is inconsistent with cell_methods time: point" ;

              (void) notes->operate(capt) ;
              notes->setCheckStatus(pQA->drsF, pQA->n_fail);
            }
        }
    }

    pDates[4] = new Date(refDate);
    pDates[5] = new Date(refDate);

    if( firstTimeValue != firstTimeBoundsValue[0] )
        if( firstTimeBoundsValue[0] != 0 )
            pDates[4]->addTime(firstTimeBoundsValue[0]);

    // regular: filename Start/End time vs. TB 1st_min/last_max
    if( lastTimeValue != lastTimeBoundsValue[1] )
        if( lastTimeBoundsValue[1] != 0 )
            pDates[5]->addTime(lastTimeBoundsValue[1]);

    if( notes->inq( "T_3b", getBoundsName() ) )
    {
        // disabled for all but CORDEX
        double db_centre=(firstTimeBoundsValue[0]
                             + firstTimeBoundsValue[1])/2. ;
        if( ! hdhC::compare(db_centre, "=", firstTimeValue) )
        {
            std::string key("T_3b");
            if( notes->inq( key, getBoundsName()) )
            {
                std::string capt("Range of ");
                capt += hdhC::tf_var("time_bnds") ;
                capt += "is not centred around <time> value." ;

                (void) notes->operate(capt) ;
                notes->setCheckStatus("TIME", pQA->n_fail );
            }
        }
    }
  }
  else if( ! pQA->pIn->variable[time_ix].isInstant )
  {
    std::string key("T_9");

    if( notes->inq( key, pQA->qaExp.getVarnameFromFilename()) )
    {
      std::string capt("Missing time_bounds");

      (void) notes->operate(capt) ;
      notes->setCheckStatus(pQA->drsF, pQA->n_fail );
    }
  }

  std::vector<std::string> key0;
  std::vector<std::string> capt0;
  std::vector<std::string> text0;

  std::vector<std::string> key1;
  std::vector<std::string> capt1;
  std::vector<std::string> text1;

  if( ! testPeriod_regularBounds(sd, pDates, key0, capt0, text0) )
  {
    if( ! testPeriod_FN_with_centre(sd, pDates, key1, capt1, text1) )
    {
       for( size_t i=0 ; i < key0.size() ; ++i )
       {
          if( notes->inq( key0[i], pQA->qaExp.getVarnameFromFilename()) )
          {
             (void) notes->operate(capt0[i], text0[i]) ;
             notes->setCheckStatus(pQA->drsF, pQA->n_fail );
          }
       }
    }
  }

  // note that indices 0 and 1 belong to a vector
  for(size_t i=0 ; i < 6 ; ++i )
    if( pDates[i] )
      delete pDates[i];

  // complete
  return false;
}

bool
QA_Time::testPeriodAlignment(std::vector<std::string>& sd, Date** pDates,
  std::vector<std::string> &key, std::vector<std::string> &capt,
  std::vector<std::string> &text)
{
  // return true for matching dates

  // regular test: filename vs. time_bounds
  if( pDates[4] && pDates[5] )
    if( *pDates[0] == *pDates[4] && *pDates[1] == *pDates[5] )
       return true;

  if( notes->findAnnotation("T_3c", getBoundsName()) )
      return false ;

  // alignment of time bounds and period in the filename
  bool is[] = { true, true, true, true };
  double dDiff[]={0., 0., 0., 0.};

  double uncertainty= refTimeStep * 0.25;

      // time value: already extended to the left-side
  dDiff[0] = fabs(*pDates[2] - *pDates[0]) ;
  is[0] = dDiff[0] < uncertainty ;

  // time value: already extended to the right-side
  dDiff[1] = fabs(*pDates[3] - *pDates[1]) ;
  is[1] = dDiff[1] < uncertainty ;

  if(isTimeBounds)
  {
    if( firstTimeValue != firstTimeBoundsValue[0] )
    {
      is[0] = is[1] = true;

      // time_bounds: left-side
      if( ! (is[2] = *pDates[0] == *pDates[4]) )
        dDiff[2] = *pDates[4] - *pDates[0] ;

      // time_bounds: right-side
      if( ! (is[3] = *pDates[1] == *pDates[5]) )
        dDiff[3] = *pDates[5] - *pDates[1] ;
    }
  }

  bool bRet=true;

  for(size_t i=0 ; i < 2 ; ++i)
  {
    // i == 0: left; 1: right

    // skip for the mode of checking during production
    if( i && !pQA->isFileComplete )
       continue;

    if( !is[0+i] || !is[2+i] )
    {
      key.push_back("T_10h");
      capt.push_back("Misaligned ");
      text.push_back(hdhC::empty);

      if( i == 0 )
        capt.back() += "start-time: " ;
      else
        capt.back() += "end-time: " ;
      capt.back() += "filename vs. ";

      size_t ix;

      if( isTimeBounds )
      {
        capt.back() += "time bounds";
        text.back() = "Found difference of ";
        ix = 4 + i ;
        text.back() += hdhC::double2String(dDiff[2+i]);
        text.back() += " day(s)";
      }
      else
      {
        capt.back() += "time values ";
        ix = 2 + i ;

        text.back() = "Found " + sd[i] ;
        text.back() += " vs. " ;
        text.back() += pDates[ix]->str();
      }

      bRet=false;
    }
  }

  return bRet;
}

void
QA_Time::testPeriodPrecision(std::vector<std::string>& sd,
  std::vector<std::string> &key, std::vector<std::string> &capt,
  std::vector<std::string> &text)
{
  // Partitioning of files check are equivalent.
  // Note that the format was tested before.
  // Note that desplaced start/end points, e.g. '02' for monthly data, would
  // lead to a wrong cut.

  if( sd[0].size() != sd[1].size() )
  {
    std::string ky("T_10b");
    if( notes->inq(ky, pQA->fileStr) )
    {
       key.push_back(ky);
       capt.push_back("Filename: Start- and EndTime of different precision") ;
       text.push_back(hdhC::empty);
    }
    return;
  }

  if( notes->inq("P_1", pQA->fileStr) )
  {
    // only CORDEX
    testPeriodPrecision_CORDEX(sd, key, capt, text);
    return;
  }

  if( ! notes->inq("P_2", pQA->fileStr) )
      return ;  // CMIP6


  int len ;

  size_t sz = pQA->qaExp.MIP_tableNames.size() ;
  size_t i;
  for( i=0 ; i < sz ; ++i )
  {
     if( QA::tableID == pQA->qaExp.MIP_tableNames[i] )
     {
        len = pQA->qaExp.MIP_FNameTimeSz[i] ;
        break;
     }
  }

  if( len == -1 || i == sz )
     return;

  int len_sd = sd[0].size() ;
  std::string txt;

  // a) yyyy
  if( len_sd == 4 && len_sd != len )
      txt =", expected YYYY, found " + sd[0] + "-" + sd[1] ;

  // b) ...mon, aero, Oclim, and cfOff
  else if( len_sd == 6 && len_sd != len )
      txt = ", expected YYYYMM, found " + sd[0] + "-" + sd[1] ;

  // c) day, cfDay
  else if( len_sd == 8 && len_sd != len )
      txt = ", expected YYYYMMDD, found " + sd[0] + "-" + sd[1] ;

    // d) x-hr
  else if( len_sd == 10 && len_sd != len )
      txt = ", expected YYYYMMDDhh, found " + sd[0] + "-" + sd[1] ;

  // e) cfSites
  else if( len_sd == 12 && len_sd != len )
      txt = ", expected YYYYMMDDhhmm, found " + sd[0] + "-" + sd[1] ;

  else if( len_sd == 14 && len_sd != len )
      txt = ", expected YYYYMMDDhhmmss, found " + sd[0] + "-" + sd[1] ;

  if( txt.size() )
  {
    key.push_back("T_10f");
    capt.push_back("period in the filename with wrong precision") ;
    text.push_back(txt);
  }

  return;
}

void
QA_Time::testPeriodPrecision_CORDEX(std::vector<std::string>& sd,
  std::vector<std::string> &key, std::vector<std::string> &capt,
  std::vector<std::string> &text)
{
  // Partitioning of files check are equivalent.
  // Note that the format was tested before.
  // Note that desplaced start/end points, e.g. '02' for monthly data, would
  // lead to a wrong cut.

  bool isInstant = ! isTimeBounds ;

  bool isBegin = pQA->fileSequenceState == 'l' || pQA->fileSequenceState == 's' ;
  bool isEnd   = pQA->fileSequenceState == 'f' || pQA->fileSequenceState == 's' ;

  std::string frequency(pQA->qaExp.getFrequency());
  std::string my_key("T_10e");

  // period length per file as recommended?
  if( frequency == "3hr" || frequency == "6hr" )
  {
    // same year.
    bool isA = sd[1].substr(4,4) == "1231";
    bool isB = sd[1].substr(4,4) == "0101" && sd[1].substr(8,2) == "00" ;

    // should be the same year
    if( isBegin && isEnd )
    {
      int yBeg=hdhC::string2Double( sd[0].substr(0,4) );
      int yEnd=hdhC::string2Double( sd[1].substr(0,4) );
      if( isB )  // begin of a next year
        --yEnd;

      if( (yEnd-yBeg) )
      {
        key.push_back(my_key);
        capt.push_back("Filename: non-conforming precision of StartTime-EndTime");
        text.push_back("Time span of a full year is exceeded");
      }
    }

    // cut of period
    std::string s_sd0( sd[0].substr(4,4) );
    std::string s_sd1( sd[1].substr(4,4) );

    std::string s_hr0( sd[0].substr(8,2) );
    std::string s_hr1( sd[1].substr(8,2) );

    std::string t_hr0;
    std::string t_hr1;

    if( isBegin )
    {
      if( s_sd0 != "0101" )
      {
        key.push_back(my_key);
        capt.push_back("Filename: non-conforming precision of StartTime");
        text.push_back( "Not the begin of the year, found " + s_sd0);
      }

      if( isInstant )
      {
        double t = firstTimeValue ;
        // recalc remainder to hr
        std::string fTV(hdhC::double2String( (t - static_cast<int>(t))*24.) );

        if( s_hr0 != fTV )
        {
          key.push_back(my_key);
          capt.push_back("Filename: non-conforming StartTime");
          text.push_back("expected hr=" + fTV + ", because of cell_methods");
        }
      }
      else
      {
        bool isAlt = false;
        double t = firstTimeBoundsValue[0];

        std::string fTV;
        std::string fTBV(hdhC::double2String( (t - static_cast<int>(t))*24.) );

        if( frequency == "3hr" || frequency == "6hr" )
        {
          isAlt=true;
          double t = firstTimeValue;
          fTV = hdhC::double2String( (t - static_cast<int>(t))*24.) ;
        }

        if( isTimeBounds && s_hr0 == fTBV )
          isAlt = false;
        else if( isAlt && s_hr0 == fTV )
          isAlt=false ;
        else
        {
          key.push_back(my_key);
          capt.push_back("Filename: non-conforming precision of StartTime");
          text.push_back("expected hr=" + fTBV + ", because of cell_methods");
        }

        if( isAlt )
        {
          key.push_back(my_key);
          capt.push_back("Filename: non-conforming precision of StartTime");
          text.push_back("expected hr=" + fTBV + ", because of cell_methods");
        }
      }
    }

    if( isEnd )
    {
      if( !(isA || isB)  )
      {
        key.push_back(my_key);
        capt.push_back("Filename: non-conforming precision of EndTime");
        text.push_back("Not the end of the year, found " + s_sd1);
      }

      if( isInstant )
      {
        double t = pQA->qaTime.lastTimeValue ;
        // recalc remainder to hr
        std::string lTV(hdhC::double2String( (t - static_cast<int>(t))*24.) );

        if( s_hr1 != lTV )
        {
          key.push_back(my_key);
          capt.push_back("Filename: non-conforming precision of EndTime");
          text.push_back("expected hr=" + lTV + ", because of cell_methods");
        }
      }
      else
      {
        bool isAlt = false;
        double t = pQA->qaTime.lastTimeBoundsValue[1];
        std::string lTV;
        std::string lTBV(hdhC::double2String( (t - static_cast<int>(t))*24.) );

        if( frequency == "3hr" || frequency == "6hr" )
        {
          isAlt=true;
          double t = pQA->qaTime.lastTimeValue;
          lTV = hdhC::double2String( (t - static_cast<int>(t))*24.) ;
        }

        if( pQA->qaTime.isTimeBounds && s_hr1 == lTBV )
          isAlt = false;
        else if( isAlt && s_hr1 == lTV )
          isAlt=false ;
        else
        {
          key.push_back(my_key);
          capt.push_back("Filename: non-conforming precision of EndTime");
          text.push_back("expected hr=" + lTBV + ", because of cell_methods");
        }

        if( isAlt )
        {
          key.push_back(my_key);
          capt.push_back("Filename: non-conforming precision of EndTime");
          text.push_back("expected hr=" + lTV + ", because of cell_methods");
        }
      }
    }
  }

  else if( frequency == "day" )
  {
     // 5 years or less
     if( isBegin && isEnd )
     {
       int yBeg=hdhC::string2Double(sd[0].substr(0,4));
       int yEnd=hdhC::string2Double(sd[1].substr(0,4));
       if( (yEnd-yBeg) > 5 )
       {
          key.push_back(my_key);
          capt.push_back("Filename: non-conforming precision of StartTime-EndTime");
          text.push_back("time span of 5 years is exceeded");
       }
     }

     if( isBegin )
     {
       if( ! (sd[0][3] == '1' || sd[0][3] == '6') )
       {
          key.push_back(my_key);
          capt.push_back("Filename: non-conforming precision of StartTime");
          text.push_back("StartTime should begin with YYY1 or YYY6");
       }

       if( sd[0].substr(4,4) != "0101" )
       {
          key.push_back(my_key);
          capt.push_back("Filename: non-conforming precision of StartTime");
          text.push_back("StartTime should be YYYY0101");
       }
     }

     if( isEnd )
     {
       if( ! (sd[1][3] == '0' || sd[1][3] == '5') )
       {
          key.push_back(my_key);
          capt.push_back("Filename: non-conforming precision of EndTime");
          text.push_back("StartTime should begin with YYY0 or YYY5");
       }

       std::string numDec(hdhC::double2String(
                          pQA->qaTime.refDate.regularMonthDays[11]));
       if( sd[1].substr(4,4) != "12"+numDec )
       {
          key.push_back(my_key);
          capt.push_back("Filename: non-conforming precision of EndTime");
          text.push_back("StartTime should be YYYY12"+numDec);
       }
     }
  }
  else if( frequency == "mon" )
  {
     if( isBegin && isEnd )
     {
       // 10 years or less
       int yBeg=hdhC::string2Double(sd[0].substr(0,4));
       int yEnd=hdhC::string2Double(sd[1].substr(0,4));
       if( (yEnd-yBeg) > 10 )
       {
          key.push_back(my_key);
          capt.push_back("Filename: non-conforming precision of StartTime-EndTime");
          text.push_back("Time span of 10 years is exceeded");
       }
     }

     if( isBegin )
     {
       if( sd[0][3] != '1')
       {
          key.push_back(my_key);
          capt.push_back("Filename: non-conforming precision of StartTime");
          text.push_back("StartTime should begin with YYY1");
       }

       if( sd[0].substr(4,4) != "01" )
       {
          key.push_back(my_key);
          capt.push_back("Filename: non-conforming precision of StartTime");
          text.push_back("StartTime should be YYYY01");
       }
     }

     if( isEnd )
     {
       if( ! (sd[1][3] == '0' || sd[1][3] == '5') )
       {
          key.push_back(my_key);
          capt.push_back("Filename: non-conforming precision of EndTime");
          text.push_back("EndTime should begin with YYY0");
       }

       std::string numDec(hdhC::double2String(
                          pQA->qaTime.refDate.regularMonthDays[11]));
       if( sd[1].substr(4,4) != "12" )
       {
          key.push_back(my_key);
          capt.push_back("Filename: non-conforming precision of EndTime");
          text.push_back(": EndTime should be YYYY12"+numDec);
       }
     }
  }
  else if( frequency == "sem" )
  {
      if( isBegin && isEnd )
      {
        // 10 years or less
        int yBeg=hdhC::string2Double(sd[0].substr(0,4));
        int yEnd=hdhC::string2Double(sd[1].substr(0,4));
        if( (yEnd-yBeg) > 11 )  // because of winter across two year
         {
           key.push_back(my_key);
           capt.push_back("Filename: non-conforming precision of StartTime-EndTime");
           text.push_back("Time span of 10 years is exceeded");
        }
      }

      if( isBegin )
      {
        if( sd[0].substr(4,2) != "12" )
       {
          key.push_back(my_key);
          capt.push_back("Filename: non-conforming precision of StartTime");
          text.push_back("StartTime should be YYYY12");
       }
      }

      if( isEnd )
      {
        if( sd[1].substr(4,2) != "11" )
        {
          key.push_back(my_key);
          capt.push_back("Filename: non-conforming precision of EndTime");
          text.push_back("EndTime should be YYYY11");
        }
      }

      {
        // background: CMOR doesn't know seasonal
        // CMOR cuts/extends the first and last time value, which are centred,
        // to the respective time interval. Saesons hoewever, start/end the
        // month before/after such a time value.

        bool is;
        int shiftedMon[]={ 1, 4, 7, 10};

        if( isBegin )
        {
            int currMon = hdhC::string2Double(sd[0].substr(4,2));
            for( size_t i=0 ; i < 4 ; ++i )
            {
                if( currMon == shiftedMon[i] )
                {
                    is=true;
                    break;
                }
            }
        }

        if( !is && isEnd )
        {
            int currMon = hdhC::string2Double(sd[0].substr(4,2));
            for( size_t i=0 ; i < 4 ; ++i )
            {
                if( currMon == shiftedMon[i] )
                {
                    is=true;
                    break;
                }
            }
        }

        if(is)
        {
           key.push_back("T_10g");
           capt.push_back("Filename: CMOR shifted seasonal StartTime-EndTime by one month");
           text.push_back("");
        }
      }
  }

  return;
}

bool
QA_Time::testPeriodDatesFormat(std::vector<std::string>& sd,
  std::vector<std::string> &key, std::vector<std::string> &capt,
  std::vector<std::string> &text)
{
  // return: true means go on for testing the period cut
  // partitioning of files
  if( sd.size() != 2 )
    return false;  // no period; is variable time invariant?

  std::string frequency(pQA->qaExp.getFrequency());
  std::string str;

  if( frequency == "3hr" )
  {
      if( ! ( (sd[0].size() == 10 && sd[1].size() == 10)
            || (sd[0].size() == 12 && sd[1].size() == 12) ) )
        str += "YYYYMMDDhh[mm] for 3-hourly time step";
  }
  else if( frequency == "6hr" )
  {
      if( sd[0].size() != 10 || sd[1].size() != 10 )
        str += "YYYYMMDDhh for 6-hourly time step";
  }
  else if( frequency == "day" )
  {
      if( sd[0].size() != 8 || sd[1].size() != 8 )
        str += "YYYYMMDD for daily time step";
  }
  else if( frequency == "mon" || frequency == "sem" )
  {
     if( sd[0].size() != 6 || sd[1].size() != 6 )
     {
        str += "YYYYMM for ";
        if( frequency == "mon" )
          str += "monthly";
        else
          str += "seasonal";
        str += " data";
     }
  }

  if( str.size() )
  {
     key.push_back("T_10a");

     capt.push_back("period in filename is incorrectly format");
     text.push_back("Found ");
     text.back() += sd[0] + "-" +  sd[1];
     text.back() += " expected " + str ;
  }

  return true;
}

bool
QA_Time::testPeriodFormat(Split& x_f, std::vector<std::string>& sd)
{
  // return true, if the format is invalid
  bool is=false;

  // CMIP5, CMIP6, or HAPPI
  if( notes->inq("P_0", pQA->fileStr)
        || notes->inq("P_2", pQA->fileStr)
          || notes->inq("P_3", pQA->fileStr) )
     is = testPeriodFormat_C56(x_f, sd) ;

  //CORDEX
  else if( notes->inq("P_1", pQA->fileStr) )
     is = testPeriodFormat_CORDEX(x_f, sd) ;

  return is;
}

bool
QA_Time::testPeriodFormat_C56(Split& x_f, std::vector<std::string>& sd)
{
  // CMIP5, CMIP6, HAPPI
  int x_fSz=x_f.size();

  if( ! x_fSz )
    return true; // caught before

  // skip any geographic subset even with wrong separator '_'?
  if( x_f[x_fSz-1].substr(0,2) == "g-" )
    --x_fSz;
  else if( x_f[x_fSz-1] == "g" )
    --x_fSz;
  else if( x_fSz > 1 && x_f[x_fSz-2].substr(0,2) == "g" )
    x_fSz -= 2 ;

  if( x_fSz < 1 )
    return true;

  if( x_f[x_fSz-1] == "clim" || x_f[x_fSz-1] == "ave" )
  {
    // Wrong separator for appendix clim or ave, found '_'
    std::string key = "T_10c" ;
    if( notes->inq( key, pQA->fileStr) )
    {
      std::string capt("Filename: wrong separator for ");
      capt += hdhC::tf_val(x_f[x_fSz-1]);
      capt += ", found underscore";

      (void) notes->operate(capt) ;
      notes->setCheckStatus(pQA->drsF, pQA->n_fail );
    }

    --x_fSz;
  }

  if( x_fSz < 1 )
    return true;

  Split x_last(x_f[x_fSz-1],'-') ;
  int x_lastSz = x_last.size();

  // minimum size of x_last could never be zero

  // elimination of 'ave' or 'clim' separated by a dash
  if( x_last[x_lastSz-1] == "clim" || x_last[x_lastSz-1] == "ave" )
    --x_lastSz ;

  bool isRegular=true;
  if( x_lastSz == 2 )
  {
    // the regular case
    std::vector<int> ix;
    if( hdhC::isDigit(x_last[0]) )
      sd[0]=x_last[0] ;
    else
    {
      ix.push_back(0);
      isRegular=false;
    }

    if( hdhC::isDigit(x_last[1]) )
      sd[1]=x_last[1] ;
    else
    {
      isRegular=false;
      ix.push_back(1);
    }

    if( ix.size() == 2 )
      return true;  // apparently not a period
    else if( ix.size() == 1 )
    {
      // try for clim or ave without any separator with a wrong one
      size_t pos;
      std::string f(x_last[ix[0]]);
      if( (pos=f.rfind("clim")) < std::string::npos
             || (pos=f.rfind("ave")) < std::string::npos )
      {
        if( pos && hdhC::isDigit( f.substr(0, pos-1) ) )
        {
          if( ix[0] == 1 )
            isRegular = true;

          sd[ix[0]] = f.substr(0,pos-1) ;
        }
        else if( pos && hdhC::isDigit( f.substr(0,pos) ) )
        {
          if( ix[0] == 1 )
            isRegular = true;

          sd[ix[0]] = f.substr(0,pos) ;
        }
        else
          isRegular=false;

        if(ix.size())
        {
          // Wrong separator for appendix clim or ave, found '_'
          std::string key("T_10d") ;
          if( notes->inq( key, pQA->fileStr) )
          {
            std::string capt("Filename: wrong separator ");
            capt += hdhC::tf_val(x_last[ix[0]]);

            (void) notes->operate(capt) ;
            notes->setCheckStatus(pQA->drsF, pQA->n_fail );
          }
        }
      }
    }
  }
  else
    isRegular=false;

  if( ! isRegular )
  {
    std::string f;
    if( sd[0].size() )
      f = x_last[1];
    else
      f = x_last[0];

    // could be a period with a wrong separator
    std::string sep;
    size_t pos=std::string::npos;

    bool isSep=false;
    for( size_t i=0 ; i < f.size() ; ++i )
    {
      if( !hdhC::isDigit(f[i]) )
      {
        if(isSep)
          return true;  // not a wrong separator, just anything

        isSep=true;
        sep=f[i];
        pos=i;
      }
    }

    if( pos < std::string::npos )
    {
      sd[0]=f.substr(0, pos) ;
      sd[1]=f.substr(pos+1) ;

      // Wrong separator for appendix clim or ave, found '_'
      std::string key("T_10c") ;
      if( notes->inq( key, pQA->fileStr) )
      {
        std::string capt("Filename: wrong separator in StartTime-EndTime");
        std::string text("Found "+ hdhC::tf_val(sep) );

        (void) notes->operate(capt, text) ;
        notes->setCheckStatus(pQA->drsF, pQA->n_fail );
      }
    }
  }

  // in case of something really nasty
  if( !( hdhC::isDigit(sd[0]) && hdhC::isDigit(sd[1]) ) )
    return true;

  return false;
}

bool
QA_Time::testPeriodFormat_CORDEX(Split& x_f, std::vector<std::string> &sd)
{
  int x_fSz=x_f.size();

  if( ! x_fSz )
    return true; // trapped before

  // any geographic subset? Even with wrong separator '_'?
  if( x_f[x_fSz-1].substr(0,2) == "g-" )
    --x_fSz;
  else if( x_f[x_fSz-1] == "g" )
    --x_fSz;
  else if( x_fSz > 1 && x_f[x_fSz-2].substr(0,2) == "g" )
    x_fSz -= 2 ;

  if( x_fSz < 1 )
    return true;

  if( x_f[x_fSz-1] == "clim" || x_f[x_fSz-1] == "ave" )
  {
    // Wrong separator for appendix clim or ave, found '_'
    std::string key = "1_6b" ;
    if( notes->inq( key, pQA->fileStr) )
    {
      std::string capt("Wrong separation of filename's period's appendix");
      capt += hdhC::tf_val(x_f[x_fSz-1]);
      capt += ", found underscore";

      (void) notes->operate(capt) ;
      notes->setCheckStatus(pQA->drsF, pQA->n_fail );
    }

    --x_fSz;
  }

  if( x_fSz < 1 )
    return true;

  Split x_last(x_f[x_fSz-1],'-') ;
  int x_lastSz = x_last.size();

  // minimum size of x_last could never be zero

  // elimination of 'ave' or 'clim' separated by a dash
  if( x_last[x_lastSz-1] == "clim" || x_last[x_lastSz-1] == "ave" )
    --x_lastSz ;

  bool isRegular=true;
  if( x_lastSz == 2 )
  {
    // the regular case
    std::vector<int> ix;
    if( hdhC::isDigit(x_last[0]) )
      sd[0]=x_last[0] ;
    else
    {
      ix.push_back(0);
      isRegular=false;
    }

    if( hdhC::isDigit(x_last[1]) )
      sd[1]=x_last[1] ;
    else
    {
      isRegular=false;
      ix.push_back(1);
    }

    if( ix.size() == 2 )
      return true;  // apparently not a period
    else if( ix.size() == 1 )
    {
      // try for clim or ave without any separator with a wrong one
      size_t pos;
      std::string f(x_last[ix[0]]);
      if( (pos=f.rfind("clim")) < std::string::npos
             || (pos=f.rfind("ave")) < std::string::npos )
      {
        if( pos && hdhC::isDigit( f.substr(0, pos-1) ) )
        {
          if( ix[0] == 1 )
            isRegular = true;

          sd[ix[0]] = f.substr(0,pos-1) ;
        }
        else if( pos && hdhC::isDigit( f.substr(0,pos) ) )
        {
          if( ix[0] == 1 )
            isRegular = true;

          sd[ix[0]] = f.substr(0,pos) ;
        }
        else
          isRegular=false;

        if(ix.size())
        {
          // Wrong separator for appendix clim or ave, found '_'
          std::string key = "1_6b" ;
          if( notes->inq( key, pQA->fileStr) )
          {
            std::string capt("Wrong separation of filename's period's appendix");
            capt += hdhC::tf_val(x_last[ix[0]]);

            (void) notes->operate(capt) ;
            notes->setCheckStatus(pQA->drsF, pQA->n_fail );
          }
        }
      }
    }
  }
  else
    isRegular=false;

  if( ! isRegular )
  {
    std::string f;
    if( sd[0].size() )
      f = x_last[1];
    else
      f = x_last[0];

    // could be a period with a wrong separator
    std::string sep;
    size_t pos=std::string::npos;

    bool isSep=false;
    for( size_t i=0 ; i < f.size() ; ++i )
    {
      if( !hdhC::isDigit(f[i]) )
      {
        if(isSep)
          return true;  // not a wrong separator, just anything

        isSep=true;
        sep=f[i];
        pos=i;
      }
    }

    if( pos < std::string::npos )
    {
      sd[0]=f.substr(0, pos) ;
      sd[1]=f.substr(pos+1) ;

      // Wrong separator for appendix clim or ave, found '_'
      std::string key = "1_6b" ;
      if( notes->inq( key, pQA->fileStr) )
      {
        std::string capt("Wrong separation of filename's period's dates");
        capt += ", found";
        capt += hdhC::tf_val(sep);

        (void) notes->operate(capt) ;
        notes->setCheckStatus(pQA->drsF, pQA->n_fail );
      }
    }
  }

  // in case of something really nasty
  if( !( hdhC::isDigit(sd[0]) && hdhC::isDigit(sd[1]) ) )
    return true;

  return false;
}




bool
QA_Time::testPeriod_regularBounds(std::vector<std::string> &sd, Date** pDatesOrig,
  std::vector<std::string> &key, std::vector<std::string> &capt,
  std::vector<std::string> &text)
{
  Date* pDates[6];

  for( size_t i=0 ; i < 6 ; ++i )
  {
     pDates[i] = 0 ;

     if( pDatesOrig[i] )
        pDates[i] = new Date(*pDatesOrig[i]);
  }

  if( ! pQA->pIn->variable[time_ix].isInstant )
  {
     // in case that the mid-frequency time value is provided.
     // sd[0] and sd[1] are of equal size.
     if( sd[0].size() < 5 )
     {
         //pDates[0]->shift("beg,yr");
         pDates[1]->shift("end,yr");

         pDates[2]->shift("beg,yr");
         pDates[3]->shift("end,yr");
     }
     else if( sd[0].size() < 7 )
     {
         //pDates[0]->shift("beg,mo");
         pDates[1]->shift("end,mo");

         pDates[2]->shift("beg,mo");
         pDates[3]->shift("end,mo");
     }
     else if( sd[0].size() < 9 )
     {
         //pDates[0]->shift("beg,d");
         pDates[1]->shift("end,d");

         pDates[2]->shift("beg,d");
         pDates[3]->shift("end,d");
     }
     else if( sd[0].size() < 11 )
     {
         pDates[0]->shift("beg,h");
         pDates[1]->shift("end,h");

         pDates[2]->shift("beg,h");
         pDates[3]->shift("end,h");
     }
     else if( sd[0].size() < 13 )
     {
         pDates[0]->shift("beg,mi");
         pDates[1]->shift("end,mi");

         pDates[2]->shift("beg,mi");
         pDates[3]->shift("end,mi");
     }
  }

  bool retVal=true;

  // the annotations
  if( testPeriodAlignment(sd, pDates, key, capt, text) )
  {
    if( testPeriodDatesFormat(sd, key, capt, text) ) // format of period dates.
    {
      // period requires a cut specific to the various frequencies.
      testPeriodPrecision(sd, key, capt, text) ;
    }
  }
  else
      retVal=false;

  for(size_t i=2 ; i < 6 ; ++i )
    if( pDates[i] )
      delete pDates[i];

  return retVal;
}

bool
QA_Time::testPeriod_FN_with_centre(std::vector<std::string> &sd, Date** pDates,
  std::vector<std::string> &key, std::vector<std::string> &capt,
  std::vector<std::string> &text)
{
  if( ! pQA->pIn->variable[time_ix].isInstant )
  {
     double delta_tb = firstTimeBoundsValue[1];
     delta_tb -= firstTimeBoundsValue[0];

     // shift t-vals of FN by half the time bounds
     pDates[0]->addDays(-delta_tb/2.);
     pDates[1]->addDays(delta_tb/2.);
  }

  bool retVal=true;

  // the annotations
  if( testPeriodAlignment(sd, pDates, key, capt, text) )
  {
    if( testPeriodDatesFormat(sd, key, capt, text) ) // format of period dates.
    {
      // period requires a cut specific to the various frequencies.
      testPeriodPrecision(sd, key, capt, text) ;
    }
  }
  else
      retVal=false;

  return retVal;
}

bool
QA_Time::testTimeStep(void)
{
  // returns true, if an error was detected.

  // timeBound set here is a substitute if there are no
  // time_bnds in the netCDF

  // no time checks
  if( timeTableMode == DISABLE )
    return false;

  // time step(s) into the past
  // method returns true in case of error
  double dif;
  if( isFormattedDate )
  {
    refDate.setDate( prevTimeValue );
    long double j0=refDate.getJulianDate();

    refDate.setDate( currTimeValue );
    long double j1=refDate.getJulianDate();

    dif=static_cast<double>( j1-j0 );
  }
  else
    dif=currTimeValue - prevTimeValue;

  currTimeStep=dif;

  if( dif < 0. )
  {
    bool isAcrossFiles = pIn->currRec == 0 ;

    if( timeTableMode == CYCLE || isAcrossFiles)
    {
      // varMeDa[0] provides the name of the MIP table
      if( parseTimeTable(pIn->currRec) )
        return false ;  // no error messaging
    }

    std::string key;
    if( isAcrossFiles )
      key="6_12";
    else
    {
      key="R1";
      sharedRecordFlag.currFlag += 1;
    }

    if( notes->inq( key, name, ANNOT_ACCUM) )
    {
      std::string capt ;
      std::ostringstream ostr(std::ios::app);

      if( isAcrossFiles )
      {
        capt = "overlapping time values across files" ;

        ostr << "last time of previous file=";
        ostr << prevTimeValue ;
        ostr << ", first time of this file=" ;
        ostr << currTimeValue << ", " ;
      }
      else
      {
        capt = "negative time step" ;

        ostr << "prev rec# ";
        ostr << (pIn->currRec-1) << ", time: " ;
        ostr << prevTimeValue ;
        ostr << ", curr " << "rec# " << pIn->currRec ;
        ostr << ", time:" << currTimeValue  ;
      }

      prevTimeValue=currTimeValue;

      if( notes->operate(capt, ostr.str()) )
      {
        notes->setCheckStatus("TIME", fail);

        pQA->setExitState( notes->getExitState() ) ;
      }
    }

    return true;
  }

  // identical time step
  if( prevTimeValue==currTimeValue  )
  {
    bool isAcrossFiles = pIn->currRec == 0 ;

    if( timeTableMode == CYCLE || isAcrossFiles )
    {
      // varMeDa[0] provides the MIP table
      if( parseTimeTable(pIn->currRec) )
        return false ;  // no error messaging
    }

    std::string key ;
    if( isAcrossFiles )
      key="6_14";
    else
      key="R4";

    if( ! isSingleTimeValue )
    {
      if( isAcrossFiles )
        sharedRecordFlag.currFlag += 4 ;

      if( notes->inq( key, name, ANNOT_ACCUM) )
      {
        std::string capt("identical time values");
        std::string text;

        if( isAcrossFiles )
           capt += " across files";

        if( isNoCalendar )
        {
          if( isAcrossFiles )
          {
            text = "last time of previous file=" ;
            text += hdhC::double2String(prevTimeValue) ;
            text += ", first time of curent file=" ;
            text += hdhC::double2String(currTimeValue)  ;
          }
          else
          {
            text  = "prev rec# " + hdhC::double2String(pIn->currRec-1) ;
            text += ", time=" + hdhC::double2String(prevTimeValue) ;
            text += ", curr rec# " + hdhC::double2String(pIn->currRec) ;
            text += ", time=" +hdhC::double2String( currTimeValue) ;
          }
        }

        if( notes->operate(capt, text) )
        {
          notes->setCheckStatus("TIME", fail);

          pQA->setExitState( notes->getExitState() ) ;
        }
      }
    }

    return true;
  }

  // missing time step(s)
  if( isRegularTimeSteps &&
         dif > (1.25*refTimeStep ))
  {
    bool isAcrossFiles = pIn->currRec == 0 ;

    if( timeTableMode == CYCLE || isAcrossFiles )
    {
      // varMeDa[0] provides the MIP table
      if( parseTimeTable(pIn->currRec) )
        return false ;  // no error messaging
    }

    std::string key ;
    if( isAcrossFiles )
      key="6_13";
    else
      key="R2";

    if( ! notes->findAnnotation("R1", name) &&
           notes->inq( key, name, ANNOT_ACCUM) )
    {
      std::string capt;
      std::string text;

      if( isAcrossFiles )
        capt = "gap between time values across files";
      else
      {
        capt = "missing time step";
        sharedRecordFlag.currFlag += 2 ;
      }

      if( isNoCalendar )
      {
        if( isAcrossFiles )
        {
          text  = "last time of previous file=";
          text += prevTimeValue ;
          text += ", first time of this file=" ;
          text += currTimeValue  ;
        }
        else
        {
          text  = "prev=" ;
          text += hdhC::double2String(prevTimeValue) ;
          text += ", curr=" ;
          text += hdhC::double2String(currTimeValue) ;
        }
      }
      else
      {
        if( isAcrossFiles )
        {
           text  = "t(last) of previous file=" ;
           text += hdhC::double2String(prevTimeValue);
           text += " (" ;
           text += refDate.getDate(prevTimeValue).str() ;
           text += "), t(1st) of current file=" ;
           text += hdhC::double2String(currTimeValue);
           text += " (" ;
           text += refDate.getDate(currTimeValue).str() ;
           text += ")" ;

        }
        else
        {
           text  = "prev=" ;
           text += hdhC::double2String(prevTimeValue);
           text += " (" ;
           text += refDate.getDate(prevTimeValue).str() ;
           text += "), curr=" ;
           text += hdhC::double2String(currTimeValue) ;
           text += " (" ;
           text += refDate.getDate(currTimeValue).str() ;
           text += ")" ;
        }
      }

      if( notes->operate(capt, text) )
      {
        notes->setCheckStatus("TIME", fail);
        pQA->setExitState( notes->getExitState() ) ;
      }
    }

    return true;
  }

  return false;
}

// ===========  class TimeOutputBuffer ===============

TimeOutputBuffer::TimeOutputBuffer()
{
  buffer=0; // 0-pointer
  bufferCount=0;

  // may be changed with setFlushCounter()
  maxBufferSize=1500; //change with setFlushCounter

  pQA=0;
}

TimeOutputBuffer::~TimeOutputBuffer()
{
  if( buffer )
    clear();
}

void
TimeOutputBuffer::clear(void)
{
  if( buffer )
  {
    delete buffer;
    delete buffer_step;

    bufferCount=0;
    buffer=0;
  }
}

void
TimeOutputBuffer::flush(void)
{
  if( bufferCount )
  {
    // Flush temporarily arrayed values
    pQA->nc->putData(nextFlushBeg, bufferCount, name, buffer );

    if( nextFlushBeg )
      pQA->nc->putData(nextFlushBeg, bufferCount, name_step, buffer_step );
    else
      // Leave out the first time for fillingValue
      pQA->nc->putData(1, bufferCount-1, name_step, (buffer_step + 1) );

    nextFlushBeg += bufferCount;
    bufferCount = 0;
  }

  return;
}

void
TimeOutputBuffer::initBuffer(QA* p, size_t nxt, size_t mx)
{
  pQA = p;
  maxBufferSize=mx;
  nextFlushBeg=nxt;

  if( buffer )
  {
     if( bufferCount )
       flush();

     clear();
  }

  buffer =       new double [maxBufferSize] ;
  buffer_step =  new double [maxBufferSize] ;

  bufferCount = 0;

  return;
}

void
TimeOutputBuffer::setName(std::string nm)
{
  name = nm;
  name_step = nm + "_step";
  return;
}

void
TimeOutputBuffer::store(double val, double step)
{
   // flush collected qa results to the qa-results netCDF file
   if( bufferCount == maxBufferSize )
     flush();  // resets countTime, too

   buffer[bufferCount]=val;
   buffer_step[bufferCount++]=step;

   return;
}
