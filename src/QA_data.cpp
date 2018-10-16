DataOutputBuffer::DataOutputBuffer()
{
  // might be changed with initBuffer()
  maxBufferSize=1500;
  bufferCount=0;

  // pointers
  min=max=ave=0;
  fill_count=0;
  checksum=0;

  pQA=0;
}

void
DataOutputBuffer::clear()
{
  bufferCount=0;

  if(min)
    delete [] min;

  if(max)
    delete [] max;

  if(ave)
    delete [] ave;

  if(fill_count)
    delete [] fill_count;

  if(checksum)
    delete [] checksum;

  min=max=ave=0;
  fill_count=0;
  checksum=0;

  return;
}

void
DataOutputBuffer::flush(void)
{
   if( ! bufferCount )
      return;

   std::string t;

   if( min )
   {
     t = name + "_min";
     pQA->nc->putData(nextFlushBeg, bufferCount, t, min );
   }

   if( max )
   {
     t = name + "_max";
     pQA->nc->putData(nextFlushBeg, bufferCount, t, max );
   }

   if( ave )
   {
     t = name + "_ave";
     pQA->nc->putData(nextFlushBeg, bufferCount, t, ave );
   }

   if( fill_count )
   {
     t = name + "_fill_count";
     pQA->nc->putData(nextFlushBeg, bufferCount, t, fill_count );
   }

   if( checksum )
   {
     t = name + "_checksum";
     pQA->nc->putData(nextFlushBeg, bufferCount, t, checksum );
   }

   nextFlushBeg += bufferCount;
   bufferCount=0;

   return;
}

void
DataOutputBuffer::initBuffer(QA* p, size_t nxt, size_t mx)
{
  pQA = p;
  maxBufferSize=mx;
  nextFlushBeg=nxt;

  // for a change on the fly
  if( min )
  {
    if( bufferCount )
      flush();

    clear() ;
  }

  // container for results temporary holding the data
  min=        new double [maxBufferSize] ;
  max=        new double [maxBufferSize] ;
  ave=        new double [maxBufferSize] ;
  fill_count= new int    [maxBufferSize] ;
  checksum=   new int    [maxBufferSize] ;

  bufferCount=0;

  return;
}

void
DataOutputBuffer::store(hdhC::FieldData &fA)
{
   // flush collected qa results to the qa-results netCDF file
   if( bufferCount == maxBufferSize )
     flush();  // resets countTime, too

   if( fA.isValid )
   {
     min[bufferCount]       =fA.min;
     max[bufferCount]       =fA.max;

     ave[bufferCount]       =fA.areaWeightedAve;
     fill_count[bufferCount]=fA.fillingValueNum;

     checksum[bufferCount] = fA.checksum;
   }
   else
   {
    return ; //
    /*
     min[bufferCount]       =1.E+20;
     max[bufferCount]       =1.E+20;

     ave[bufferCount]       =1.E+20;
     fill_count[bufferCount]=-1;
     checksum[bufferCount]  =0;
    */
   }

   fill_count[bufferCount]=fA.fillingValueNum;

   ++bufferCount;

   return;
}

SharedRecordFlag::SharedRecordFlag()
{
  buffer=0;
  currFlag=0;
  bufferCount=0;
  maxBufferSize=1500;
}

SharedRecordFlag::~SharedRecordFlag()
{
  if( buffer )
     delete[] buffer;
}

void
SharedRecordFlag::adjustFlag(int num, size_t rec, int erase)
{
  // adjust coded error flags written already to qa_<filename>.nc
  if( pQA->nc->getNumOfRecords(name, true) == 0 )
    return;

  MtrxArr<int> ma_i;
  pQA->nc->getData(ma_i, name, rec );

  std::vector<int> decomp;

  if( ma_i[0] > 0 )
  {
    // check whether the identical error flag is already available
    int flags[] = { 0, 1, 2, 4, 8, 16, 32, 64,
                  100, 200, 400, 800, 1600, 3200, 6400, 12800, 25600};
    int sz = 15;
    int iv=ma_i[0] ;

    for( int i=sz-1 ; i > 0 ; --i )
    {
       if( iv < flags[i] )
         continue;

       decomp.push_back( flags[i] );
       iv -= flags[i] ;

       if( iv == 0 )
         break;
    }

    for( size_t i=0 ; i < decomp.size() ; ++i )
    {
      if( num == decomp[i] )
      {
        num=0;
        break;
      }
    }
  }

  int iv=num + ma_i[0] - erase;  // note that erase equals zero by default
  pQA->nc->putData(rec, 1, name, &iv );

  return;
}

void
SharedRecordFlag::flush(void)
{
  if(bufferCount)
  {
    pQA->nc->putData(nextFlushBeg, bufferCount, name, buffer );

    nextFlushBeg += bufferCount;
    bufferCount=0;
  }

  return;
}

void
SharedRecordFlag::initBuffer(QA* pq, size_t nxt, size_t mx)
{
  pQA = pq;
  maxBufferSize=mx;
  nextFlushBeg=nxt;

  if( buffer )
  {
    // for a change on the fly
    if( bufferCount )
      flush();

    delete [] buffer ;
  }

  buffer = new int [maxBufferSize] ;
  bufferCount=0;

  return;
}

void
SharedRecordFlag::store(void)
{
   if( bufferCount == maxBufferSize )
      flush();

   buffer[bufferCount++] = currFlag;

   currFlag=0;
   return;
}


Outlier::Outlier( QA *p, VariableMetaData* v)
{
  pQA=p;
  vMD=v;
  name=v->var->name;
}

bool
Outlier::isSelected( Variable& var,
     std::vector<std::string> &options, std::string& unlimName)
{
  if( options.size() == 1 && options[0] == "t" )
    return true; // default for all

  bool isThis  =true;
  bool isZeroDim=false;
  int effDim=-1;

  // dismember options
  for( size_t k=0 ; k < options.size() ; ++k )
  {
    bool isVar  =false;
    isThis      =true;
    isZeroDim   =false;

    Split cvs(options[k],",");
    for( size_t i=0 ; i < cvs.size() ; ++i )
    {
      if( cvs[i] == "no_0-D" )
      {
        if( effDim == -1 )
        {
          effDim = var.dimName.size() ;
          for( size_t j=0 ; j < var.dimName.size() ; ++j )
            if( var.dimName[j] == unlimName )
              --effDim;
        }

        if( effDim < 1 )
          isZeroDim=true;
      }
      else if( cvs[i] == var.name )
        isVar=true;  // valid for this specific variable
    }

    if( isVar )
    {
      // this dedicated variable
      if( isZeroDim )
         return false;

      return isThis ;
    }
  }

  // all variables
  if( isZeroDim )
    return false;

  return isThis ;
}

void
Outlier::parseOption(std::vector<std::string> &opts)
{
  if( opts.size() == 0 )
     return;

  BraceOP groups ;
  std::string s0;

  Split csv;
  csv.setSeparator(',');

  for( size_t k=0 ; k < opts.size() ; ++k )
  {
    s0=opts[k];

    groups.set( s0 );
    while ( groups.next(s0) )
    {
       csv = s0;

       bool isGeneral=true;
       for( size_t l=0 ; l < csv.size() ; ++l )
       {
         s0 = csv[l] ;

         if( s0[0] == 'L' )
           continue;
         else if( s0[0] == 'M' )
           continue;
         else if( s0[0] == 'P' )
           continue;
         else if( s0[0] == 'N' )
           continue;

         isGeneral = false;
       }

       if( isGeneral )
       {
         for( size_t l=0 ; l < csv.size() ; ++l )
           options.push_back( csv[l] ) ;
         continue;
       }

       bool isVarSpecific=false;
       for( size_t l=0 ; l < csv.size() ; ++l )
       {
         if( name == csv[l] )
         {
            isVarSpecific = true;
            break;
         }
       }

       if( isVarSpecific )
       {
         for( size_t l=0 ; l < csv.size() ; ++l )
           options.push_back( csv[l] ) ;

         return;
       }
    }
  }

  return;
}

bool
Outlier::test(QA_Data *pQAD)
{
  // Acknowledgement: Dr. Andreas Chlond, MPI-M Hamburg, suggested to
  //                  exploit the function N(sigma) as criterion.*/

  bool retCode = false;

  if( ! pQA->isCheckData )
    return retCode;

  if( notes->findAnnotation("6_16", name) )
      return retCode;  // a test is pointless

  std::vector<std::string> names;

  if( name.size() == 0 )
    return retCode;

  if( !( notes->inq( "R400", name, "INQ_ONLY")
          || notes->inq( "R800", name, "INQ_ONLY")) )
    return retCode;

  names.push_back( name + "_min"  );
  names.push_back( name + "_max"  );

  // get mean and standard deviation
  double ave[2];
  double sigma[2];

  size_t sampleSize=pQAD->statMin.getSampleSize();
  if( sampleSize < 2 )
    return retCode;

  if( ! pQAD->statMin.getSampleAverage( &ave[0] ) )
    return retCode;

  if( ! pQAD->statMin.getSampleStdDev( &sigma[0] ) )
    return retCode;

  if( ! pQAD->statMax.getSampleAverage( &ave[1] ) )
    return retCode;

  if( ! pQAD->statMax.getSampleStdDev( &sigma[1] ) )
    return retCode;

  for( size_t i=0 ; i < 2 ; ++i )
    sigma[i] /= 4.;

  // init outlier options
  size_t outOptL=5;
  size_t outOptM=5;
  size_t outOptN=0;
  double outOptPrcnt=0.;

  for( size_t i=0 ; i < options.size() ; ++i )
  {
     std::string &t0 = options[i] ;

     if( t0[0] == 'L' )
       outOptL = static_cast<size_t>(hdhC::string2Double(t0));
     else if( t0[0] == 'M' )
       outOptM = static_cast<size_t>(hdhC::string2Double(t0));
     else if( t0[0] == 'P' )
       outOptPrcnt = hdhC::string2Double(t0) / 100.;
     else if( t0[0] == 'N' )
       outOptN = static_cast<size_t>(hdhC::string2Double(t0)) ;
  }

  // default: both are specified
  if( outOptN == 0 && outOptPrcnt == 0. )
  {
     outOptN=5;
     outOptPrcnt=0.01;
  }

  // read min / max from the qa-nc file

  std::vector<std::string> extStr;
  extStr.push_back( "record-wide minima" );
  extStr.push_back( "record-wide maxima" );

  int errNum[2];
  errNum[0]=400;
  errNum[1]=800;

  std::vector<size_t> outRec;
  std::vector<double> outVal;

  size_t recNum=pQA->nc->getNumOfRecords(true);  // force new inquiry

  // adjustment of outOptPrcnt: the effective percentage must be related
  // to the total number of data points in a way that 10 outlier must
  // be possible in principle.
  if( outOptPrcnt > 0. )
  {
    while ( static_cast<double>(recNum) * outOptPrcnt < .1 )
     outOptPrcnt *= 10. ;
  }

  double extr[recNum];

  MtrxArr<double> ma_d;
  size_t N[100]; // counter for exceeding extreme values

  for( size_t i=0 ; i < 2 ; ++i )
  {
    outRec.clear();
    outVal.clear();
    bool isOut = false;

    pQA->nc->getData(ma_d, names[i], 0, -1);
    if( ! ma_d.validRangeBegin.size() )
        return retCode;

    size_t recNum=0;
    for( size_t j=0 ; j < ma_d.validRangeBegin.size() ; ++j )
        for(size_t k=ma_d.validRangeBegin[j] ; k < ma_d.validRangeEnd[j] ; ++k )
          extr[recNum++] = ma_d[k] ;

    double extrMin = extr[0];
    double extrMax = extr[0];
    for( size_t j=1 ; j < recNum ; ++j )
    {
       if( extr[j] > extrMax )
          extrMax = extr[j];
       if( extr[j] < extrMin )
          extrMin = extr[j];
    }

    // find number of extreme values exceeding N*sigma
    size_t n=0;
    size_t countConst=1;
    double n_sigma;
    size_t nLimit = 100;

    do
    {
      sigma[i] *= 2.;

      for(  n=0 ; n < nLimit ; ++n )
      {
        n_sigma = sigma[i] + static_cast<double>(n) * sigma[i];
        N[n]=0;

        outRec.clear();
        outVal.clear();

        for( size_t j=0 ; j < recNum ; ++j )
        {
          if( fabs(extr[j] - ave[i]) > n_sigma )
          {
            ++N[n] ;
            outRec.push_back( j );
            outVal.push_back( extr[j] );
          }
        }

        // no detection
        if( N[n] == 0 )
          break;

        if( N[n] == N[n-1] )
          ++countConst ;
        else
          countConst=1 ;

        if( countConst == outOptM )
        {
          isOut = true;
          break;
        }
      }
    } while( n == nLimit );

    // if there are too many peaks/breaks, then consider this as regular.
    // default: > 1%
    if( isOut )
    {
      // if only one of the two is specified or both
      bool isTooManyA ;
      bool isTooManyB ;

      if( outOptPrcnt > 0. )
      {
        double a =
           static_cast<double>(N[n]) / static_cast<double>(recNum) ;
        isTooManyA =  a > outOptPrcnt ;
      }
      else
        isTooManyA = false;

      if( outOptN )
        isTooManyB = outRec.size() > outOptN ;
      else
        isTooManyB = false;

      if( isTooManyA && isTooManyB )
      {
        isOut = false;

        // too many, thus check the magnitude
        double outOptMagOrder=1.;
        for( size_t j=0 ; j < outOptL ; ++j )
          outOptMagOrder *= 10.;

        double scale=0.1;
        double a;
        double aveSave = a = fabs( ave[i] );
        do
        {
          scale *= 10.;
          a /= 10.;
        } while( a > 1. );
        scale *= outOptMagOrder ;

        std::vector<size_t> outRec_n;
        std::vector<double> outVal_n;
        for( size_t j=0 ; j < outRec.size() ; ++j )
        {
          if( fabs( aveSave - outVal[j] ) > scale )
          {
            outRec_n.push_back( outRec[j] );
            outVal_n.push_back( outVal[j] );
          }
        }

        if( outRec_n.size() )
        {
           outRec = outRec_n ;
           outVal = outVal_n ;
           outRec_n.clear();
           outVal_n.clear();
           isOut = true;
        }
      }
    }

    if( isOut )
    {
        // for preventing spurious minima slightly above zero.
        if( extrMin > 0. )
        {
          double th=(ave[i]-extrMin)/extrMin;
          if( th > 1.e+03 )
            isOut=false;
        }
        else if( extrMax < 0. )
        {
          double th=(ave[i]-extrMax)/extrMax;
          if( th > 1.e+03 )
            isOut=false;
        }
    }

    if( isOut )
    {
      retCode=true;

      std::string key("R");
      key += hdhC::itoa(errNum[i]);
      if( notes->inq( key, name) )
      {
        pQAD->sharedRecordFlag.currFlag += errNum[i];

        // sort outlier values
        size_t swap_t;
        double swap_d;
        size_t sz=outRec.size()-1;

        for( size_t k0=0 ; k0 < sz ; ++k0 )
        {
          size_t k_max=k0;

          for( size_t k1=k0+1 ; k1 < outRec.size() ; ++k1 )
            if ( outVal[k0] < outVal[k1] )
                k_max = k1 ;

          if ( k_max > k0 )
          {
            swap_d = outVal[k0];
            swap_t = outRec[k0];
            outVal[k0] = outVal[k_max];
            outRec[k0] = outRec[k_max];
            outVal[k_max] = swap_d;
            outRec[k_max] = swap_t;
          }
        }

        std::string capt(hdhC::tf_var(vMD->var->name, hdhC::colon)) ;
        capt += " Suspected outlier for " + extStr[i] ;

        std::ostringstream ostr(std::ios::app);
        ostr.setf(std::ios::scientific, std::ios::floatfield);

        double cT;
        std::string text;
        for( size_t k=0 ; k < outRec.size() ; ++k )
        {
          // adjust coded flags
          pQAD->sharedRecordFlag.adjustFlag(errNum[i], outRec[k] ) ;

          if( (cT=pQA->qaTime.getTimeValue(outRec[k])) < MAXDOUBLE)
          {
            text += "\nrec#=" + hdhC::double2String(outRec[k]);
            text += ", time=" + hdhC::double2String(cT);
            text += ", date=" + pQA->qaTime.refDate.getDate(cT).str();
            ostr.str(", value=");
            ostr << std::setw(12) << std::setprecision(5) << outVal[k];
            text += ostr.str() ;
          }
        }

        if( (retCode=notes->operate(capt, text)) )
        {
          notes->setCheckStatus(pQA->n_data, pQA->n_fail);
        }
      }
    }
  }

  return retCode;
}

ReplicatedRecord::ReplicatedRecord( QA *p, VariableMetaData* v)
{
  pQA=p;
  vMD=v;
  name=v->var->name;

  groupSize=0;
  enableReplicationOnlyGroups = false ;
}

void
ReplicatedRecord::getRange(size_t i, size_t bufferCount, size_t recNum,
  size_t *arr_rep_ix,  size_t *arr_1st_ix,
  bool *arr_rep_bool, bool *arr_1st_bool,
  std::vector<std::string> &range)
{
  double cT_1st, cT_rep ;

  size_t ix_1st = arr_1st_ix[i];
  size_t ix_rep = arr_rep_ix[i];

  if( arr_1st_bool[i] )
  {
    range.push_back( hdhC::double2String(ix_1st) );
    cT_1st = pQA->qaTime.getTimeValue(ix_1st) ;
  }
  else
  {
    cT_1st = pQA->qaTime.timeOutputBuffer.buffer[ix_1st];
    range.push_back( hdhC::double2String(ix_1st+recNum) );
  }

  if( arr_rep_bool[i] )
  {
    cT_rep = pQA->qaTime.getTimeValue(ix_rep) ;
    range.push_back( hdhC::double2String(ix_rep) );
  }
  else
  {
    cT_rep = pQA->qaTime.timeOutputBuffer.buffer[ix_rep];
    range.push_back( hdhC::double2String(ix_rep+recNum) );
  }

  range.push_back(pQA->qaTime.refDate.getDate(cT_1st).str() );
  range.push_back(pQA->qaTime.refDate.getDate(cT_rep).str() );

  return;
}

bool
ReplicatedRecord::isSelected(Variable& var,
    std::vector<std::string>& options, std::string &unlimName )
{
  if( options.size() == 1 && options[0] == "t" )
    return true; // default for all

  bool isThis  =true;
  bool isZeroDim=false;
  int effDim=-1;

  for( size_t k=0 ; k < options.size() ; ++k )
  {
    bool isVar  =false;
    isThis      =true;
    isZeroDim   =false;

    Split cvs(options[k],",");
    for( size_t i=0 ; i < cvs.size() ; ++i )
    {
      if( cvs[i].substr(0,10) == "clear_bits" )
        ;
      else if( cvs[i] == "no_0-D" )
      {
        if( effDim == -1 )
        {
          effDim = var.dimName.size() ;
          for( size_t j=0 ; j < var.dimName.size() ; ++j )
            if( var.dimName[j] == unlimName )
              --effDim;
        }

        if( effDim < 1 )
          isZeroDim=true;
      }
      else if( cvs[i].substr(0,11) == "only_groups" )
        ;
      else if( cvs[i] == var.name )
        isVar=true;  // valid for this specific variable
    }

    if( isVar )
    {
      // this dedicated variable
      if( isZeroDim )
         return false;

      return isThis ;
    }
  }

  // all variables
  if( isZeroDim )
    return false;

  return isThis ;
}

void
ReplicatedRecord::parseOption(std::vector<std::string> &options)
{
  // dismember options
  for( size_t k=0 ; k < options.size() ; ++k )
  {
    Split cvs(options[k],",");
    for( size_t i=0 ; i < cvs.size() ; ++i )
    {
      if( cvs[i].substr(0,10) == "clear_bits" )
      {
        size_t numOfClearBits=2; //default

        if( cvs[i].size() > 10 )
        {
          // note: from index 10
          if( hdhC::isDigit( cvs[i].substr(10) ) )
            numOfClearBits=static_cast<size_t>(
              hdhC::string2Double( cvs[i].substr(10) ) );
        }

        // enable clearing of the least significant bits for R32 flags
        if( numOfClearBits )
        {
           vMD->var->pDS->enableChecksumWithClearedBits( numOfClearBits ) ;
           vMD->qaData.numOfClearedBitsInChecksum = numOfClearBits;
        }
      }
      else if( cvs[i].substr(0,11) == "only_groups" )
      {
        // return zero in case of no digit
        setGroupSize( static_cast<size_t>(
           hdhC::string2Double( cvs[i], 1, true ) ) ) ;

        enableReplicationOnlyGroups = true;
      }
    }
  }

  return;
}

void
ReplicatedRecord::test(int nRecs, size_t bufferCount, size_t nextFlushBeg,
                       bool isMultiple)
{
  // Test for replicated records.
  // Before an array of values (e.g. ave, max ,min)
  // is flushed to the qa_<filename>.nc file, the values
  // in the array are tested for replicated records in
  // the priviously written qa_<filename>.nc as well as
  // in the array itself.*/
  std::string name_chks;

  // temporary arrays
  size_t arr_rep_ix[bufferCount] ;
  size_t arr_1st_ix[bufferCount] ;
  bool   arr_rep_bool[bufferCount] ; // if read from file, then true
  bool   arr_1st_bool[bufferCount] ; // if read from file, then true

  size_t status [bufferCount] ;
  bool isGroup=false;

  // 0: no, 1: singleton, 2: group (beg), 3: group(end)
  // 4: filling and constant values

  for( size_t j=0 ; j < bufferCount ; ++j )
  {
    int n ;
    n = vMD->qaData.sharedRecordFlag.buffer[j];

    // exclude code numbers 100 and 200
    if( n > 99 && n < 400 )
      status[j]=4 ;
    else
      status[j]=0 ;
  }

  name_chks = name + "_checksum";

  int recNum = pQA->nc->getNumOfRecords(true);  // force nc-inquiry

  std::pair<int,int> readRange(0,0);
  MtrxArr<double> ma_chks;

  // reading by chunks
  while( readRange.second < recNum )
  {
    // time and checksum array are synchronised
    (void) pQA->qaTime.getTimeValue(readRange.second) ;
    (void) pQA->nc->getData(ma_chks, name_chks, readRange.second, -1) ;

    readRange = pQA->nc->getDataIndexRange(name_chks) ;
    size_t sz = ma_chks.size();

    // loop over the temporarily stored values
    for( size_t j0=0 ; j0 < bufferCount ; ++j0 )
    {
      if( status[j0] )
        continue;

      // loop over the qa_<filename>.nc file
      size_t j=j0;
      int low0=-1;

      for( size_t i=0 ; i < sz ; ++i, j=j0 )
      {
        size_t countGroupMembers=0;
        int high ;

        while( ma_chks[i]
          == (high=vMD->qaData.dataOutputBuffer.checksum[j]) )
        {
          // we suspect identity.
          // is it a group?
          ++countGroupMembers;

          if( high == low0 && countGroupMembers > 1)
          {
            status[j-1] = 3;
            countGroupMembers=1;
          }

          status[j] = 2;

          arr_rep_ix[j] = j;
          arr_1st_ix[j] = readRange.first + i++ ;
          arr_rep_bool[j] = false;
          arr_1st_bool[j] = true;

          if( ++j == bufferCount )
            break;

          if( low0 == -1 )
            low0 = ma_chks[j0] ;
        }

        if( countGroupMembers && j < bufferCount )
        {
          --j;
          if( countGroupMembers == 1 )
            status[j] = 1;
          else
            status[j] = 3;
        }
      }
    }
  }

  // replications within the current buffer
  for( size_t j0=0 ; j0 < bufferCount ; ++j0 )
  {
    if( status[j0] )
      continue;

    size_t j=j0;
    int low0 = -1 ;

    for( size_t i=j+1 ; i < bufferCount ; ++i, j=j0 )
    {
      if( status[i] )
        continue;

      size_t countGroupMembers=0;
      int high;

      while( vMD->qaData.dataOutputBuffer.checksum[j]
        == (high=vMD->qaData.dataOutputBuffer.checksum[i]) )
      {
        // we suspect identity.
        ++countGroupMembers;

        if( high == low0 && countGroupMembers > 1 )
          status[i-1] = 3;

        status[i] = 2;

        arr_rep_ix[i]   = i ;
        arr_1st_ix[i]   = j++ ;
        arr_rep_bool[i] = false;
        arr_1st_bool[i++] = false;

        if( low0 == -1 )
          low0 = vMD->qaData.dataOutputBuffer.checksum[j0];

        if( i == bufferCount )
          break;
      }

      if( countGroupMembers )
      {
        --i;
        if( countGroupMembers == 1 )
          status[i] = 1;
        else
          status[i] = 3;
      }
    }
  }

  // close an open end
  if( bufferCount > 1 && status[bufferCount-1] == 2 )
  {
    if( status[bufferCount-2] == 2 )
      status[bufferCount-1] = 3;
    else
      status[bufferCount-1] = 1;
  }

  // And now the error flag
  isGroup=true;
  for( size_t i=0 ; i < bufferCount ; ++i )
  {
    if( status[i] && status[i] < 4 )
    {
      vMD ->qaData.sharedRecordFlag.buffer[i] += 3200;

      if( isGroup )
        isGroup=false;
    }
  }

  if( isGroup )
    return;  // no replication found

  std::vector<std::string> range;

  for( size_t i=0 ; i < bufferCount ; ++i )
  {
    if( status[i] == 0   ||  status[i] == 4 || status[i] == 3 )
    {
       if( isGroup )
       {
         getRange(i, bufferCount, recNum, arr_rep_ix, arr_1st_ix,
            arr_rep_bool, arr_1st_bool, range);

         isGroup = false;
       }
       else
         continue;
    }
    else if( status[i] == 1 )
    {
       if( isGroup )
          isGroup=false;  // should not happen
       else
         getRange(i, bufferCount, recNum, arr_rep_ix, arr_1st_ix,
            arr_rep_bool, arr_1st_bool, range);

    }
    else if( status[i] == 2 )
    {
       if( ! isGroup )
       {
         getRange(i, bufferCount, recNum, arr_rep_ix, arr_1st_ix,
            arr_rep_bool, arr_1st_bool, range);

         isGroup=true;
       }

       continue;
    }

    // issue message
    report(range, bufferCount, isMultiple );
    range.clear();
  }

  return;
}

void
ReplicatedRecord::report(std::vector<std::string> &range,
                         size_t bufferCount, bool isMultiple)
{
   // Is it a group?
   bool isIsolated=false;  // !! true is not enabled at present

   std::string capt(hdhC::tf_var(name, hdhC::colon) ) ;
   capt += " suspicion of replicated ";

   if( enableReplicationOnlyGroups )
     capt += "group(s) of records" ;
   else
   {
     if( isIsolated )
       capt += "isolated record(s)" ;
     else
       capt += "group(s) of records" ;
   }


   if( range.size() > 4 )
   {
     if( groupSize )
     {
       size_t diff=static_cast<size_t>(hdhC::string2Double(range[4]) );
       diff -= static_cast<size_t>(hdhC::string2Double(range[0]) );
       if( diff < groupSize )
         return;
     }
   }
   else if( enableReplicationOnlyGroups )
       return;

   std::string key("R3200");
   if( notes->inq( key, name, "ACCUM") )
   {
     std::string text;

     // Is it a group?
     if( isIsolated )
     {
       text += "rec# " + range[0] + " --> " + range[1];
       text += ", date=" + range[2] + " --> " + range[3] + "\n";
     }
     else
     {
       text += "rec# " + range[0] + " - " + range[4] + " --> ";
       text +=           range[1] + " - " + range[5] ;
       text += ", 1st date: " + range[2] + " --> " + range[3] + "\n";
     }

     // note: no update of record_flag variable; already done
     (void) notes->operate(capt, text) ;
     notes->setCheckStatus(pQA->n_data, pQA->n_fail);
   }

   return;
}

QA_Data::QA_Data()
{
   validMax=1.e+17;
   validMin=-1.e+15;

   // only used in context of enabled replication test
   allRecordsAreIdentical=true;

   enableConstValueTest = true ;
   enableFillValueTest = true ;
   enableOutlierTest = true;
   enableReplicationTest = true;

   constValueRecordState=false;
   fillValueRecordState=false;

   isEntirelyConst=true;
   isEntirelyFillValue=true;
   isForkedAnnotation=false;

   numOfClearedBitsInChecksum = 0;
   bufferCount=0;
   maxBufferSize=1500;

   outlier = 0;
   replicated = 0;
}

QA_Data::~QA_Data()
{
   if( replicated )
     delete replicated;
   if( outlier )
     delete outlier;

}

void
QA_Data::applyOptions(std::vector<std::string>& optStr)
{
  for( size_t i=0 ; i < optStr.size() ; ++i)
  {
     Split split(optStr[i], "=");

     if( split[0] == "dVMX"
          || split[0] == "defaultValidMax" || split[0] == "default_valid_max" )
     {
        if( split.size() == 2 )
          // path to the directory where the execution takes place
          validMax = hdhC::string2Double(split[1]);

        continue;
     }

     if( split[0] == "dVMN"
          || split[0] == "defaultValidMin" || split[0] == "default_valid_min" )
     {
        if( split.size() == 2 )
          // path to the directory where the execution takes place
          validMin = hdhC::string2Double(split[1]);

        continue;
     }
  }

  return;
}

void
QA_Data::checkFinally(Variable *var)
{
   if( var->isNoData() )
   {
      std::string key("6_15");
      if( notes->inq( key, var->name) )
      {
        std::string capt(hdhC::tf_var(var->name));
        capt += "empty data body" ;

        if( notes->operate(capt) )
          notes->setCheckStatus(pQA->n_data, pQA->n_fail );
      }
   }
   else if( isEntirelyConst && ! var->isScalar )
   {
      std::string key("6_1");
      if( notes->inq( key, name, ANNOT_NO_MT) )
      {
        std::string capt(hdhC::tf_var(var->name, hdhC::colon));
        capt += "Entire file of const value=";
        capt += hdhC::double2String( currMin );

        if( notes->operate(capt) )
          notes->setCheckStatus(pQA->n_data, pQA->n_fail);

        // erase redundant map entries
        notes->eraseAnnotation("R200");
        notes->eraseAnnotation("R3200");

        return;
     }
   }
   else if( constValueRecord.size() )
   {
     if( constValueRecordStartTime.size() == (constValueRecordEndTime.size() + 1) )
     {
       // the range with constant records reached the file end
       constValueRecordEndTime.push_back(pQA->qaTime.currTimeValue) ;
       constValueRecordEndRec.push_back(pIn->currRec-1) ;
     }

     std::string key=("R200");
     if( notes->inq( key, name) )
     {
       std::string capt("Warning: Data record totally with constant value ");
       capt += hdhC::tf_val(constValueRecord[0]);

       std::string text;
       for( size_t i=0 ; i < constValueRecordStartRec.size() ; ++i )
       {
         if(i)
           text += "\n" ;

         text="rec#:";

         if( constValueRecordStartRec[i] == constValueRecordEndRec[i] )
         {
           text += hdhC::tf_val( hdhC::double2String(constValueRecordStartRec[i]) );

           double cT0;
           cT0 = pQA->qaTime.getTimeValue(constValueRecordStartRec[0]) ;

           std::string date0(pQA->qaTime.refDate.getDate(cT0).str());

           text += ", date: ";
           text += hdhC::tf_val(date0);

         }
         else
         {
           text += hdhC::tf_range(
                         hdhC::double2String(constValueRecordStartRec[0]),
                         hdhC::double2String(constValueRecordEndRec[0]) );

           double cT0, cT1;
           cT0 = pQA->qaTime.getTimeValue(constValueRecordStartRec[0]);
           cT1 = pQA->qaTime.getTimeValue(constValueRecordEndRec[0]) ;

           std::string date0(pQA->qaTime.refDate.getDate(cT0).str());
           std::string date1(pQA->qaTime.refDate.getDate(cT1).str());

           text += ", dates:";
           text += hdhC::tf_range(date0, date1);
         }
       }

       (void) notes->operate(capt, text) ;
       notes->setCheckStatus(pQA->n_data);
     }
   }

  if( isEntirelyFillValue )
  {
     std::string key("6_2");
     if( notes->inq( key, name, ANNOT_NO_MT) )
     {
        std::string capt("data set entirely of _FillValue");

        if( notes->operate(capt) )
          notes->setCheckStatus(pQA->n_data, pQA->n_fail);

        // erase redundant map entries
        notes->eraseAnnotation("R100");

        return;
     }
   }
   else if( fillValueRecordStartTime.size() )
   {
     if( fillValueRecordStartTime.size() == (fillValueRecordEndTime.size() + 1) )
     {
       fillValueRecordEndTime.push_back(pQA->qaTime.currTimeValue) ;
       fillValueRecordEndRec.push_back(pIn->currRec) ;
     }

     std::string key=("R100");
     if( notes->inq( key, name) )
     {
       std::string capt("data record totally with _FillValue");
       std::string text("Found ");
       if( fillValueRecordStartTime.size() > 1 )
         text += "first ";

       text += "for rec# ";
       std::string r( hdhC::double2String(fillValueRecordStartRec[0]) );
       text += hdhC::tf_val(r);

       text += ", time value ";
       r =  hdhC::double2String(fillValueRecordStartTime[0]) ;

       text += hdhC::tf_val(r);

       (void) notes->operate(capt, text) ;
       notes->setCheckStatus(pQA->n_data, pQA->n_fail);
     }
   }

   if( replicated && allRecordsAreIdentical && ! pIn->isRecSingle )
   {
      std::string key("6_3");
      if( notes->inq( key, name, ANNOT_NO_MT) )
      {
        std::string capt("all data records are identical") ;

        notes->operate(capt) ;
        notes->setCheckStatus(pQA->n_data, pQA->n_fail);

        // erase redundant map entries
        notes->eraseAnnotation("R3200");
      }
   }

  return;
}

void
QA_Data::disableTests(std::string name)
{
   if( notes == 0 )
      return;

   // only for tests on data
   std::string key;

   // note: exceptipon() returns false, when no
   //       directive was specified in PROJECT_flags.conf

   key="R100";
   if( ! notes->inq( key, name, "INQ_ONLY") )
   {
     isEntirelyFillValue=false;
     enableFillValueTest=false;
   }

   key="R200";
   if( ! notes->inq( key, name, "INQ_ONLY") )
   {
     isEntirelyConst=false;
     enableConstValueTest=false;
   }

   key="R400";
   bool is = notes->inq( key, name, "INQ_ONLY") ;

   key="R800";
   if( ! (is || notes->inq( key, name, "INQ_ONLY")) )
     enableOutlierTest=false;

   key="R3200";
   if( ! notes->inq( key, name, "INQ_ONLY") )
     enableReplicationTest=false;

   // note: R6400 will be disabled in the InFile object. This is
   // invoked by the qaExecutor script.

   key="6_1";
   if( ! notes->inq( key, name, "INQ_ONLY") )
     isEntirelyConst=false;

   key="6_2";
   if( ! notes->inq( key, name, "INQ_ONLY") )
     isEntirelyFillValue=false;

   key="6_3";
   if( ! notes->inq( key, name, "INQ_ONLY") )
     allRecordsAreIdentical=false;

  return;
}

int
QA_Data::finally(int eCode)
{
  if( pQA->isCheckData )
    // write pending results to qa-file.nc. Modes are considered there
    flush();

  // annotation obj forked by the parent VMD
  notes->printFlags();

  int rV = notes->getExitState();
  eCode = ( eCode > rV ) ? eCode : rV ;

  return eCode ;
}

void
QA_Data::forkAnnotation(Annotation *n)
{
//   if( isForkedAnnotation )
//      delete notes;

   notes = new Annotation(n);

   isForkedAnnotation=true;
   return;
}

void
QA_Data::flush(void)
{
  if( bufferCount )
  {

     // test for entirely identical records
     if( allRecordsAreIdentical )
     {
       for( size_t i=1 ; i < bufferCount ; ++i )
       {
         if( dataOutputBuffer.checksum[0]
                   != dataOutputBuffer.checksum[i] )
         {
           allRecordsAreIdentical=false;
           break;
         }
       }
     }

     // Test for replicated records.
     //else

       if( replicated )
     {
         int nRecs=static_cast<int>( pQA->nc->getNumOfRecords(true) );

         replicated->test(nRecs, bufferCount, nextFlushBeg,
              pQA->qaExp.varMeDa.size() > 1 ? true : false );
     }

     for( size_t i=0 ; i < bufferCount ; ++i)
     {
       statMin.add( dataOutputBuffer.min[i] );
       statMax.add( dataOutputBuffer.max[i] );
       statAve.add( dataOutputBuffer.ave[i] );
     }

     // any extraordinary extreme value?
     double samMin = statMin.getSampleMin();
     double samMax = statMax.getSampleMax();

     if( samMin < validMin || samMax > validMax )
     {
       bool isMax=true;
       if(samMin < validMin)
         isMax=false;

       std::string key=("6_16");
       if( notes->inq( key, name, ANNOT_NO_MT) )
       {
         std::string capt(hdhC::tf_var(name, hdhC::colon)) ;
         capt += "extraordinary extreme value, found";

         if( isMax )
           capt += hdhC::tf_val( hdhC::double2String(samMax));
         else
           capt += hdhC::tf_val( hdhC::double2String(samMin));

         std::string text;
         double samMinMax = statMin.getSampleMax();
         double samMaxMin = statMax.getSampleMin();
         if( (isMax && samMax == samMaxMin) || (samMin == samMinMax) )
            text = "suspection of an erroneous set missing_value";

         (void) notes->operate(capt, text) ;
         notes->setCheckStatus(pQA->n_data, pQA->n_fail);
       }
     }

     dataOutputBuffer.flush();
     sharedRecordFlag.flush();

//     pQA->nc->clear();
//     pQA->nc->close();
//     pQA->nc->open(pQA->qaFile.getFile(), "", false);

     bufferCount = 0;
  }

  return ;
}

void
QA_Data::init(QA *q, std::string nm)
{
   pQA =q;
   pIn = pQA->pIn;
   name = nm;

   // apply parsed command-line args
   applyOptions(q->optStr);


/*
   doesn't work, because according to CF Conventions, each value outside
   of valid_range is a valid missing_value. Relying on this, such values are
   not taken into account at all and therefore not detectable.

   int j;
   std::string str("valid_range");

   if( (j=var->getAttIndex(str)) > -1)
   {
       Split x_vr(var->getAttValue(str));
       validMin = hdhC::string2Double( x_vr[0] );
       validMax = hdhC::string2Double( x_vr[1] );
   }
   else
   {
       str = "valid_min";
       if( (j=var->getAttIndex(str)) > -1 )
         validMin = hdhC::string2Double(var->getAttValue(str));

       str = "valid_max";
       if( (j=var->getAttIndex(str)) > -1)
         validMax = hdhC::string2Double(var->getAttValue(str));
   }
*/

   dataOutputBuffer.setName(name) ;
   sharedRecordFlag.setName(nm + "_flag") ;

   ANNOT_ACCUM="ACCUM";
   ANNOT_NO_MT="NO_MT";
   isForkedAnnotation=false;

   return ;
}

void
QA_Data::initBuffer(QA* p, size_t nxt, size_t mx)
{
  maxBufferSize=mx;
  nextFlushBeg=nxt;

  dataOutputBuffer.initBuffer(p, nxt, mx);
  sharedRecordFlag.initBuffer(p, nxt, mx);

  bufferCount=0;

  return;
}

void
QA_Data::initResumeSession(std::string& nomen)
{
  if( ! pQA->isCheckData )
    return;

  // read data values from the previous QA run
  std::vector<double> dv;
  std::string statStr;
  std::string s0;

  s0 = nomen + "_max";
  pQA->nc->getAttValues( dv, "valid_range", s0);
  statStr  ="sampleMin=" ;
  statStr += hdhC::double2String( dv[0] );
  statStr +=", sampleMax=" ;
  statStr += hdhC::double2String( dv[1] );
  statStr += ", ";
  statStr += pQA->nc->getAttString("statistics", s0) ;
  statMax.setSampleProperties( statStr );

  s0 = nomen + "_min";
  pQA->nc->getAttValues( dv, "valid_range", s0);
  statStr  ="sampleMin=" ;
  statStr += hdhC::double2String( dv[0] );
  statStr +=", sampleMax=" ;
  statStr += hdhC::double2String( dv[1] );
  statStr += ", ";
  statStr += pQA->nc->getAttString("statistics", s0) ;
  statMin.setSampleProperties( statStr );

  s0 = nomen + "_ave";
  pQA->nc->getAttValues( dv, "valid_range", s0);
  statStr  ="sampleMin=" ;
  statStr += hdhC::double2String( dv[0] );
  statStr +=", sampleMax=" ;
  statStr += hdhC::double2String( dv[1] );
  statStr += ", ";
  statStr += pQA->nc->getAttString("statistics", s0) ;
  statAve.setSampleProperties( statStr );

  return;
}

void
QA_Data::openQA_NcContrib(NcAPI *nc, Variable *var)
{
  // a multi-purpose vector
  std::vector<std::string> vs;

  // variable name in inFile
  std::string &vName = name;

  // Define a dummy variable for no data, but information.
  // Define variable explicitly, because of no dimensions
  // and an additional att.
  nc->defineVar(vName, pIn->nc.getVarType(vName), vs);

  nc->setAtt(vName, "about",
                    "original attributes of the checked variable");

  // get original dimensions and convert names into a string
  vs = pIn->nc.getDimName(vName);
  std::string str;

  Split splt( var->units );

  if( splt.size() > 0 )
  {
    str = "(" + vs[0] ;
    for( size_t k=1 ; k < vs.size() ; ++k)
      str += ", " + vs[k];

    str += ")";
  }
  else
    str = "None";

  nc->setAtt(vName, "original_dimensions", str );
  nc->copyAtts(pIn->nc, vName, vName);

  if( !pQA->isCheckData )
    return;

  // define qa-variables
  // either the real time-name of "fixed"
  std::string dimStr( pQA->qaTime.name ) ;

  int shuffle_switch=1;
  int deflate_switch=1;
  int deflate_level=9;

  // different, but derived, varnames
  vs.clear();
  std::string str0( name + "_min" );
  vs.push_back(dimStr);
  nc->defineVar( str0, NC_DOUBLE, vs);
  nc->setDeflate(str0, shuffle_switch, deflate_switch, deflate_level);
  vs[0] = "global_minimum";
  nc->setAtt( str0, "long_name", vs[0]);
  nc->setAtt( str0, "units", var->units);
  nc->setAtt( str0, "_FillValue", static_cast<double>(1.E+20));

  vs.clear();
  str0 = name + "_max" ;
  vs.push_back(dimStr);
  nc->defineVar( str0, NC_DOUBLE, vs);
  nc->setDeflate(str0, shuffle_switch, deflate_switch, deflate_level);
  vs[0] = "global_maximum";
  nc->setAtt( str0, "long_name", vs[0]);
  nc->setAtt( str0, "units", var->units);
  nc->setAtt( str0, "_FillValue", static_cast<double>(1.E+20));

  vs.clear();
  str0 = name + "_ave" ;
  vs.push_back(dimStr);
  nc->defineVar( str0, NC_DOUBLE, vs);
  nc->setDeflate(str0, shuffle_switch, deflate_switch, deflate_level);
  vs[0] = "global_average";
  nc->setAtt( str0, "long_name", vs[0]);
  nc->setAtt( str0, "units", var->units);
  nc->setAtt( str0, "_FillValue", static_cast<double>(1.E+20));

/*
  if( in.variable[m].pGM->isRegularGrid() )
    nc->setAtt( str0, "comment",
      "Note: weighted by latitude-parallel bounded areas");
  else
    nc->setAtt( str0, "comment",
      "Note: weighted by areas of spherical triangles");
*/

  vs.clear();
  vs.push_back(dimStr);
  str0 = name + "_fill_count" ;
  nc->defineVar( str0, NC_INT, vs);
  nc->setDeflate(str0, shuffle_switch, deflate_switch, deflate_level);
  vs[0] = "number of cells with _FillValue";
  nc->setAtt( str0, "long_name", vs[0]);
  nc->setAtt( str0, "units", "1");
  nc->setAtt( str0, "_FillValue", static_cast<int>(-1));

  vs.clear();
  vs.push_back(dimStr);
  str0 = name + "_checksum" ;
  nc->defineVar( str0, NC_INT, vs);
  nc->setDeflate(str0, shuffle_switch, deflate_switch, deflate_level);
  vs[0] = "Fletcher32 checksum of data";
  nc->setAtt( str0, "long_name", vs[0]);
  if( numOfClearedBitsInChecksum )
  {
    std::string st("last ");
    st += hdhC::itoa(numOfClearedBitsInChecksum) ;
    st += " insignificant bits cleared";
    nc->setAtt( str0, "comment", st);
  }

  nc->setAtt( str0, "units", "1");
  nc->setAtt( str0, "_FillValue", static_cast<int>(0));

  vs.clear();
  str0 = name + "_flag" ;
  vs.push_back(dimStr);
  nc->defineVar( str0, NC_INT, vs);
  nc->setDeflate(str0, shuffle_switch, deflate_switch, deflate_level);
  vs[0]="accumulated record-tag number";
  nc->setAtt( str0, "long_name", vs[0]);
  nc->setAtt( str0, "units", "1");
  nc->setAtt( str0, "_FillValue", static_cast<int>(-1));
  vs[0]="tag number composite by Rnums of the check-list table.";
  nc->setAtt( str0, "comment", vs[0]);

  return;
}

void
QA_Data::setAnnotation(Annotation *n)
{
//   if( isForkedAnnotation )
//      delete notes;

   notes = n;

   isForkedAnnotation=false;
   return;
}

void
QA_Data::setNextFlushBeg(size_t n)
{
   nextFlushBeg=n;
   sharedRecordFlag.setNextFlushBeg(n);

   return;
}

void
QA_Data::setStatisticsAttribute(NcAPI *nc)
{
  // store the statistics' properties in qa_<variable>.nc
  std::vector<std::string> vs;
  std::string t;
  size_t sz=2;
  double d_val[2];

  // range of the variable
  d_val[0]=statMin.getSampleMin();
  d_val[1]=statMax.getSampleMax();
  nc_type type = nc->getVarType(name);

  setValidRangeAtt(nc, name, d_val, sz, type);

  // attributes of special interest
  t = name + "_max";
  d_val[0] = statMax.getSampleMin();
  d_val[1] = statMax.getSampleMax();
  nc->setAtt( t, "valid_range", d_val, sz);
  vs.clear();
  vs.push_back( statMax.getSampleProperties() );
  // strip leading part indicating valid_range
  size_t pos = vs[0].find("sampleSize");
  nc->setAtt( t, "statistics", vs[0].substr(pos));

  t = name + "_min";
  d_val[0] = statMin.getSampleMin();
  d_val[1] = statMin.getSampleMax();
  nc->setAtt( t, "valid_range", d_val, sz);
  vs.clear();
  vs.push_back( statMin.getSampleProperties() );
  // strip leading part indicating valid_range
  pos = vs[0].find("sampleSize");
  nc->setAtt( t, "statistics", vs[0].substr(pos));

  t = name + "_ave";
  d_val[0] = statAve.getSampleMin();
  d_val[1] = statAve.getSampleMax();
  nc->setAtt( t, "valid_range", d_val, sz);
  vs.clear();
  vs.push_back( statAve.getSampleProperties() );
  // strip leading part indicating valid_range
  pos = vs[0].find("sampleSize");
  nc->setAtt( t, "statistics", vs[0].substr(pos));

  return;
}

void
QA_Data::setValidRangeAtt(NcAPI* nc, std::string& name, double* d_val, size_t sz,
    nc_type type)
{
  std::string vr("valid_range");

  switch (type)
  {
    case NC_FLOAT:
    {
      float x=0.;
      nc->setAtt( name, vr, d_val, sz, x );
    }
    break;
    case NC_DOUBLE:
    {
      nc->setAtt( name, vr, d_val, sz);
    }
    break;
    case NC_CHAR:
    {
      char x='a';
      nc->setAtt( name, vr, d_val, sz, x );
    }
    break;
    case NC_BYTE:
    {
      signed char x=0;
      nc->setAtt( name, vr, d_val, sz, x );
    }
    break;
    case NC_SHORT:
    {
      short x=0;
      nc->setAtt( name, vr, d_val, sz, x );
    }
    break;
    case NC_INT:
    {
      int x=0;
      nc->setAtt( name, vr, d_val, sz, x );
    }
    break;
    case NC_UBYTE:
    {
      unsigned char x=0;
      nc->setAtt( name, vr, d_val, sz, x );
    }
    break;
    case NC_USHORT:
    {
      unsigned short x=0;
      nc->setAtt( name, vr, d_val, sz, x );
    }
    break;
    case NC_UINT:
    {
      unsigned int x=0;
      nc->setAtt( name, vr, d_val, sz, x );
    }
    break;
    case NC_UINT64:
    {
      unsigned long long x=0;
      nc->setAtt( name, vr, d_val, sz, x );
    }
    break;
    case NC_INT64:
    {
      long long x=0;
      nc->setAtt( name, vr, d_val, sz, x );
    }
  }

  return;
}

void
QA_Data::store(hdhC::FieldData &fA)
{
   if( bufferCount == maxBufferSize )
      flush();

   dataOutputBuffer.store(fA) ;
   sharedRecordFlag.store();

   ++bufferCount;

   return;
}

void
QA_Data::test(int i, hdhC::FieldData &fA)
{
  // all tests return true for finding something no
  if( testValidity(fA) )
  {
    currMin=fA.min;
    currMax=fA.max;

    testInfNaN(fA);

    testConst(fA) ;

    testNegativeVal(fA) ;

    testAnyFillValue(fA) ;

    if( enableOutlierTest )
    {
      if( ! fA.isValid )
      {  // update the statistics
         statMax.add( currMax );
         statMin.add( currMin );
      }
    }
  }

  return ;
}

void
QA_Data::testAnyFillValue(hdhC::FieldData &fA)
{
  if( fA.fillingValueNum > 0 )
  {
    std::string key=("R25600");
    if( notes->inq( key, name, ANNOT_ACCUM) )
    {
      sharedRecordFlag.currFlag += 25600;

      std::string capt(hdhC::tf_var(name, ":") );
      capt += "_FillValue " ;
      capt += " at rec# " ;
      capt += hdhC::tf_val( hdhC::itoa(pIn->currRec));

      (void) notes->operate(capt) ;
      notes->setCheckStatus(pQA->n_data, pQA->n_fail);
    }
  }

  return ;
}

void
QA_Data::testConst(hdhC::FieldData &fA)
{
  if( ! fA.isValid )
    return ;

  if( fA.size == 1 || ! enableConstValueTest )
    return ;

  if( currMin != currMax )
  {
    isEntirelyConst=false;

    if( constValueRecordState )
    {
      constValueRecordEndTime.push_back(pQA->qaTime.prevTimeValue) ;
      constValueRecordEndRec.push_back(pIn->currRec-1) ;
      constValueRecordState=false;
    }

    return ;
  }

  std::string val=hdhC::double2String(currMin);

  // add a constraint to the inquiry, if specified
  notes->setConstraintValue(val);

  std::string key=("R200");
  if( notes->inq( key, name) )
  {
    sharedRecordFlag.currFlag += 200;

    // start of a new record range
    constValueRecordState = true;
    constValueRecord.push_back(val);
    constValueRecordStartTime.push_back(pQA->qaTime.currTimeValue) ;
    constValueRecordStartRec.push_back(pIn->currRec) ;
  }

  return ;
}

void
QA_Data::testInfNaN(hdhC::FieldData &fA)
{
  if( fA.infCount || fA.nanCount )
  {
    std::string key=("R6400");
    if( notes->inq( key, name, ANNOT_ACCUM) )
    {
      sharedRecordFlag.currFlag += 6400;

      std::string capt("Inf or NaN value(s) found in rec#");
      capt += hdhC::tf_val( hdhC::itoa(pIn->currRec));

      (void) notes->operate(capt) ;
      notes->setCheckStatus(pQA->n_data, pQA->n_fail);
    }
  }

  return ;
}

void
QA_Data::testNegativeVal(hdhC::FieldData &fA)
{
  if( fA.min < 0.0 )
  {
    std::string key=("R12800");
    if( notes->inq( key, name, ANNOT_ACCUM) )
    {
      sharedRecordFlag.currFlag += 12800;

      std::string capt(hdhC::tf_var(name, ":") );
      capt += "Negative data value " ;
      capt += hdhC::tf_val( hdhC::double2String(fA.min) ) ;
      capt += " at rec# " ;
      capt += hdhC::tf_val( hdhC::itoa(pIn->currRec));

      (void) notes->operate(capt) ;
      notes->setCheckStatus(pQA->n_data, pQA->n_fail);
    }
  }

  return ;
}

bool
QA_Data::testValidity(hdhC::FieldData &fA)
{
  if( fA.isValid )
  {
    isEntirelyFillValue=false;

    if( fillValueRecordState )
    {
      fillValueRecordEndTime.push_back(pQA->qaTime.prevTimeValue) ;
      fillValueRecordEndRec.push_back(pIn->currRec-1) ;
      fillValueRecordState=false;
    }

    if( fA.size < 2 )
      isEntirelyConst=false;

    return true;
  }

  if( ! enableFillValueTest )
    return false;

  // the flag is always stored in the file and it is used
  // to prevent false replicated record detection
  sharedRecordFlag.currFlag += 100 ;

  if( ! fillValueRecordState )
  {
    // start of a new record range
    fillValueRecordState = true;
    fillValueRecordStartTime.push_back(pQA->qaTime.currTimeValue) ;
    fillValueRecordStartRec.push_back(pIn->currRec) ;
  }

  return false ;
}
