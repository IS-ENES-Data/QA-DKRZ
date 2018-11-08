#include "annotation.h"

Annotation::Annotation(Annotation *n)
{
  initDefaults();

  if( n )
     copyInit(n);
}

void
Annotation::applyOptions(void)
{
  Split split;
  split.setSeparator("=");
  split.setIgnore(" ");

     // the first loop for items with higher precedence
  for( size_t i=0 ; i < optStr.size() ; ++i)
  {
     split = optStr[i] ;

     if( split[0] == "cL"
         || split[0] == "checkList"
            || split[0] == "check_list" )
     {
          checkList=split[1] ;
          continue;
     }

     if( split[0] == "d" || split[0] == "display" )
     {
        isDisplay=true;
        continue;
     }

     if( split[0] == "note" )
     {
       if( split.size() == 2 )
       {
          BraceOP groups;
          groups.set( split[1] );

          std::string str0;
          while ( groups.next(str0) )
            // empty string for txt
            setConfVector( "", str0 ) ;

          continue;
       }
     }

     if( split[0] == "nA"
          || split[0] == "noteAlways" || split[0] == "note_always")
     {
       if( split.size() == 2 )
       {
          setUseAlways( split[1], "always" );
          continue;
       }
     }

     if( split[0] == "nLL"
          || split[0] == "noteLevelLimit"
             || split[0] == "note_level_limit")
     {
       if( split.size() == 2 )
       {
         if( split[i].size() == 1 )
         {
           // only a number
           std::string s("L");
           s += split[i] ;
           setUseAlways( s, "limit" );
         }
         else
           setUseAlways( split[1], "limit" );

          continue;
       }
     }

     if( split[0] == "nCR"
          || split[0] == "no_check_results" || split[0] == "noCheckResults")
     {
        isCheckResultsWasPrinted=true;
        continue;
     }

     if( split[0] == "oP"
          || split[0] == "output_PASS" || split[0] == "outputPASS")
     {
        isOutputPASS=true;
        continue;
     }

  }

  return;
}

void
Annotation::config(void)
{
  // note: first items of options may be already defined
  //       in QA.applyOPtions()

  // basic default settings
  if( isUseDefault )
  {
    if( levelLimit )
      options.push_back("L" + hdhC::itoa(levelLimit));
    else
      options.push_back("L1");  // severe, not emergency

    // three place holders corresponding to options
    descript.push_back( "" );
  }

  // get flag configuration from file
  readConf();

  parse();

  return;
}

void
Annotation::copyInit(Annotation *n)
{
  currTable = n->currTable;
  currTableAcronym = n->currTableAcronym;
  isCheckResultsWasPrinted = n->isCheckResultsWasPrinted;

  for( size_t i=0 ; i < n->code.size() ; ++i )
     code.push_back( n->code[i] ) ;
  for( size_t i=0 ; i < n->count.size() ; ++i )
     count.push_back( n->count[i] ) ;
  for( size_t i=0 ; i < n->level.size() ; ++i )
     level.push_back( n->level[i] ) ;
  for( size_t i=0 ; i < n->task.size() ; ++i )
     task.push_back( n->task[i] ) ;
  for( size_t i=0 ; i < n->text.size() ; ++i )
     text.push_back( n->text[i] ) ;
  for( size_t i=0 ; i < n->var.size() ; ++i )
     var.push_back( n->var[i] ) ;

  for( size_t i=0 ; i < n->frq.size() ; ++i )
    frq.push_back( std::vector<std::string>( n->frq[i]) );

  for( size_t i=0 ; i < n->value.size() ; ++i )
    value.push_back( std::vector<std::string>(n->value[i]) );

  for( size_t i=0 ; i < n->permittedFlagBegin.size() ; ++i )
     permittedFlagBegin.push_back( n->permittedFlagBegin[i] ) ;

  for( size_t i=0 ; i < n->xRecord_0.size() ; ++i )
  {
    xRecord_0.push_back( std::vector<size_t>() );

    for( size_t j=0 ; j < n->xRecord_0[i].size() ; ++j )
       xRecord_0[i].push_back( n->xRecord_0[i][j] ) ;
  }

  for( size_t i=0 ; i < n->xRecord_1.size() ; ++i )
  {
    xRecord_1.push_back( std::vector<size_t>() );

    for( size_t j=0 ; j < n->xRecord_1[i].size() ; ++j )
       xRecord_1[i].push_back( n->xRecord_1[i][j] ) ;
  }

  return;
}

void
Annotation::eraseAnnotation(std::string str, std::string name)
{
  if( !findAnnotation(str, name) )
    return;

  // Note: composition: level-flag.
  std::map<std::string, std::string>::iterator it;

  if( name.size() )
    str += "_" + name ;

  for( it=mp.begin() ; it != mp.end() ; ++it )
  {
    // flag found
    if( str == it->first.substr(0,str.size()) )
      mp_count[it->first] = 0;
  }

  return;
}

bool
Annotation::findAnnotation(std::string str, std::string name)
{
  if ( mp.begin() == mp.end() )
    return false;

  if( name.size() )
     str += "_" + name ;

  std::map<std::string, std::string>::iterator it;

  for( it=mp.begin() ; it != mp.end() ; ++it )
    if( str == it->first.substr(0,str.size()) )
      return true;

  return false;
}

bool
Annotation::findIndex(std::string &key, bool isOnly)
{
  // scan over explicit flags and explicit name

  // isOnly==true: don't mark for effVectors;
  //               will not be applied by operate()

  // scan already known indexes
  for( size_t i=0 ; i < effIndex.size() ; ++i)
  {
    if( code[effIndex[i]] == key )
    {
      if( effVar[i] == currName )
      {
         currIndex = effIndex[i] ;

         return (isMultipleTags||isAccumText) ? false : true ;
      }
    }
  }

  // scan over explicit flags and explicit name
  for( size_t i=0 ; i < code.size() ; ++i)
  {
     if( code[i] == key )
     {
        if( currName == var[i] )
        {
          currIndex = i;

          if( ! isOnly )
          {
            effIndex.push_back( i );
            effVar.push_back( currName );
          }

          return false;
        }
     }
  }

  // scan over explicit flags, but wild-card name
  for( size_t i=0 ; i < code.size() ; ++i)
  {
     if( code[i] == key )
     {
        if( var[i] == "*" )
        {
          currIndex = i;

          if( ! isOnly )
          {
            effIndex.push_back( i );
            effVar.push_back( currName );
          }

          return false;
        }
     }
  }

  // for more general settings, apply to all flags and all variables
  for( size_t i=0 ; i < code.size() ; ++i)
  {
     if( code[i] == "*" )
     {
        if( currName == var[i] || var[i] == "*" )
        {
          push_back( key, currName, frq[i], level[i],
              task[i], "", value[i], xRecord_0[i], xRecord_1[i]);

          currIndex = code.size() -1 ;

          if( ! isOnly )
          {
            effIndex.push_back( currIndex );
            effVar.push_back( currName );
          }

          return false;
        }
     }
  }

  return false;
}

std::vector<std::string>
Annotation::getAnnotation(std::string tag)
{
  std::vector<std::string> s;
    return getAnnotation(tag, s);
}

std::vector<std::string>
Annotation::getAnnotation(std::string tag, std::vector<std::string>& txt)
{
  std::vector<std::string> vs;

  if ( mp.begin() == mp.end() )
    return vs;

  std::string str;

  std::map<std::string, std::string>::iterator it;

  for( it=mp.begin() ; it != mp.end() ; ++it )
  {
    // remove internal versioning
    size_t sz;
    if( (sz=it->first.find('|')) == std::string::npos )
      sz = it->first.size();

    str = it->first.substr(0,sz) ;

    if( str.find(tag) < std::string::npos )
    {
       vs.push_back(str);
       txt.push_back(it->second);
    }
  }

  return vs;
}

std::string
Annotation::getCheckStatus(bool withRank)
{
  std::map<std::string, std::string>::iterator it;
  std::string out;

  std::string drsF;
  std::string drsP;

  for( it=checkID.begin() ; it != checkID.end() ; ++it )
  {
    if( it->first == "DRS(F)" )
        drsF = it->second;
    if( it->first == "DRS(P)" )
        drsP = it->second;
  }

  bool isDRS = drsF == drsP;
  bool isDRS_First=true;

  for( it=checkID.begin() ; it != checkID.end() ; ++it )
  {
    std::string f(it->first);
    const std::string& s=it->second;

    if( isDRS && it->first.size() > 2 && it->first.substr(0,3) == "DRS" )
    {
      if( isDRS_First )
      {
        f="DRS";
        isDRS_First=false;
      }
      else
        continue;
    }

    if( out.size() )
        out += '|';

    out += f + "%";
    out += s ;
    if( withRank )
      out += "=" + hdhC::itoa(checkRank[f]) ;
  }

  return out;
}

int
Annotation::getExitState(int e_in)
{
  int retVal=0;

  if( isExit[4] )  // stop session
    retVal = 4;
  else if( isExit[3] )  // stop immediately and lock
    retVal = 3;
  else if( isExit[2] )  // stop after meta-data check and lock
    retVal = 2;
  else if( isExit[1] )  // issue warning
    retVal = 1;

  if( e_in > retVal )
      retVal = e_in;

  return retVal;
}

bool
Annotation::init(void)
{
  if( isInit )
    return false;

  isInit=true;

  applyOptions();

  config();

  return true;
}

void
Annotation::initDefaults(void)
{
  setObjName("X");
  ofsNotes=0;

  recErrCountLimit=5;

  isCheckResultsWasPrinted = false;
  isDisplay=false;
  isOutputPASS=false;
  isUseDefault=true;

  countMultipleTags=0;
  levelLimit=0;

  mail_level = 0;
  numOfExits = 5;

  for( size_t i=0 ; i < numOfExits ; ++i )
    isExit[i]=false;

  notes=0;
  cF=0;
  pIn=0;
  fDI=0;
  pOper=0;
  pOut=0;
  qA=0;
  tC=0;

  isInit=false;

  return;
}

bool
Annotation::inq( std::string key, std::string name, std::string mode)
{
  // SYNTAX mode: [INQ::ONLY[|...]]
  // with     ACCUM  NO_MT with accumulated text messages
  //       INQ_ONLY  simulation without triggering an action
  //          NO_MT  no multiple tags
  //       LIST_TXT  disable the default text from the check-list table
  currName=name;

  bool isOnly=false;
  isMultipleTags=true;
  isDescriptionFromTable=false;
  isAccumText=false;
  isApply_MORE=false;
  if( key[0] == 'R' && hdhC::isDigit(key.substr(1)) )
      isApply_MORE=true;

  if( mode.size() )
  {
    Split splt(mode," |", true);  // string is a container of separators
    for( size_t i=0 ; i < splt.size() ; ++i )
    {
       if( splt[i] == "INQ_ONLY" )
         isOnly=true;
       else if( splt[i] == "NO_MT" )
         isMultipleTags=false;
       else if( splt[i] == "LIST_TXT" )
         isDescriptionFromTable=true;
       else if( splt[i] == "ACCUM" )
       {
         isAccumText=true;
         isMultipleTags=false;
       }
    }
  }

  // find handling setting for key; sets variable currIndex
  // return value true, for a redo of non-multiple tags
  if( findIndex(key, isOnly) )
    return false;

  // configured to discard?
  if( task[currIndex].find("D") < std::string::npos )
  {
    // check any specified constraint
    if( value[currIndex].size() || frq[currIndex].size() )
    {
      bool isV=false;
      bool isF=false;

      for(size_t i=0 ; i < value[currIndex].size() ; ++i )
        if( value[currIndex][i] == constraintValue )
          isV=true;

      for(size_t i=0 ; i < frq[currIndex].size() ; ++i )
        if( frq[currIndex][i] == constraintFrq )
          isF=true;

      if( value[currIndex].size() && frq[currIndex].size() )
      {
         if( isV && isF )
            return false;
      }
      else if( value[currIndex].size() && isV )
          return false;
      else if( frq[currIndex].size() && isF )
          return false;
      else
          return true;
    }

    // check excluded record
    for(size_t i=0 ; i < xRecord_0[currIndex].size() ; ++i )
    {
      for(size_t j=xRecord_0[currIndex][i] ; j < xRecord_1[currIndex][i] ; ++j )
      {
        if( j == currentRecord )
          return false;  // discard any kind of notification
      }

      return true;
    }

    return false;
  }

  return true;   // default exception handling
}

/*
void
Annotation::linkObject(IObj *p)
{
  std::string className = p->getObjName();

  if( className == "X" )
    notes = dynamic_cast<Annotation*>(p) ;
  else if( className ==  "CF" )
    cF = dynamic_cast<CF*>(p) ;
  else if( className == "FD_interface" )
    fDI = dynamic_cast<FD_interface*>(p) ;
  else if( className ==  "IN" )
    pIn = dynamic_cast<InFile*>(p) ;
  else if( className == "Oper" )
    pOper = dynamic_cast<Oper*>(p) ;
  else if( className == "Out" )
    pOut = dynamic_cast<OutFile*>(p) ;
  else if( className == "QA" )
    qA = dynamic_cast<QA*>(p) ;
  else if( className == "TC" )
    tC = dynamic_cast<TimeControl*>(p) ;

  return;
}
*/

bool
Annotation::operate(std::string headline,
      std::string passedText,
      std::string mail_subject, std::string mail_body )
{
    std::vector<std::string> vs;
    if( passedText.size() )
        vs.push_back(passedText);

    return operate(headline, &vs, mail_subject, mail_body);
}

bool
Annotation::operate(std::string headline,
      std::vector<std::string>* passedText,
      std::string mail_subject, std::string mail_body )
{
   bool isReturn=false;

   std::string tag(code[currIndex]) ;
   if( currName.size() )
      tag  += "_" + currName ;

   if( isMultipleTags )
   {
      // the annex will be erased before printing
      tag += "|";
      tag += hdhC::itoa( countMultipleTags++ );
   }

   if( mp.count(tag) == 0 )
   {
     mp[tag] = code[currIndex] ;
     mp_lev[tag] = level[currIndex] ;

     if( headline.size() )
       mp_capt[tag] = headline;
     else
       mp_capt[tag] = text[currIndex];
   }

   if( mp_count.count(tag) == 0 )
     mp_count[tag]=1 ;
   else
     mp_count[tag] = mp_count[tag]+1 ;

   // levels
   if( level[currIndex] == "L1" )
   {
     isExit[1]=true;
     isReturn=true;
   }
   else if( level[currIndex] == "L2" )
   {
     // Stop after meta-data check
     // or if in time or data mode immediately
     isExit[2]=true;
     isReturn=true;
   }
   else if( level[currIndex] == "L3" )
   {
     // Stop immediately
     isExit[3]=true;
     isReturn=true;
   }
   else if( level[currIndex] == "L4" )
   {
     //Emergency.
     isExit[4]=true;
     isReturn=true;
   }

   // print notes
   if( isDescriptionFromTable )
   {
     passedText->clear();
     passedText->push_back(text[currIndex]) ;
   }

   // a caption only for the 'classical' output into the QA_RESULTS DRS
   std::string capt(code[currIndex]);
   capt += ": ";
   capt += mp_capt[tag] ;

   //if( !isDisplay )
     printNotes( tag, capt, passedText );

   // prepare email
   if( task[currIndex].find("EM") < std::string::npos )
   {
      if( mail_body.size() )
      {
         mail_out_body += "\n" ;
         mail_out_body += mail_body ;
      }

      if( mail_subject.size() && getExitState() > mail_level )
      {
         mail_out_subject = "SUBJECT-BEG" ;
         mail_out_subject += mail_subject ;
         mail_out_subject += "SUBJECT-END" ;

         mail_level = getExitState() ;
      }
   }

   return isReturn ;
}

void
Annotation::parse(void)
{
  Split csl;
  csl.setSeparator(',');
  csl.setIgnore(" ");

  std::vector<std::string> tmpCodes;
  std::vector<std::string> vars;
  std::string              checkListText;
  std::string              str0;

  std::vector<std::string> currFrq;
  std::vector<std::string> currValue;
  std::vector<size_t>      xRec_0;
  std::vector<size_t>      xRec_1;

  for( size_t i=0 ; i < options.size() ; ++i )
  {
     csl=options[i];

     if( csl.size() == 0 )
       continue;  // default behaviour

     tmpCodes.clear();  // reset
     vars.clear();

     if( currFrq.size() )
       currFrq.clear();
     if( currValue.size() )
       currValue.clear();

     if( xRec_0.size() )
     {
       xRec_0.clear();
       xRec_1.clear();
     }

     bool isEmail=false;
     bool isL1=false;
     bool isL2=false;
     bool isL3=false;
     bool isEmergency=false;
     bool isD=false;

     checkListText = descript[i];

     // extract flags and tasks;
     // both will be applied to each variable below or if the variable
     // is omitted, then to all variables.
     for( size_t j=0 ; j < csl.size() ; ++j )
     {
        str0 = csl[j] ;

        if( str0 == "*" )
          continue;

        if( str0 == "L1" )
        {
          isL1=true;
          continue;
        }

        if( str0 == "L2" )
        {
          isL2=true;
          continue;
        }

        if( str0 == "L3" )
        {
          isL3=true;
          continue;
        }

        if( str0 == "L4" )
        {
          isEmergency=true;
          continue;
        }

        if( str0 == "D" )
        {
          isD=true;
          continue;
        }

        if( str0 == "EM" )
        {
          isEmail=true;
          continue;
        }

        // any begin with a digit indicates a flag as well as beginning with
        // a vector member of permitted FlagBegin whose size is smaller than that of str0.
        if( hdhC::isDigit( str0[0]) )
        {
          tmpCodes.push_back( str0 ) ;
          continue;
        }
        else
        {
          if( str0.find('=') == std::string::npos)  // not a record
          {
            bool isCont=false;
            for(size_t ifb=0 ; ifb < permittedFlagBegin.size() ; ++ifb)
            {
              size_t sz = permittedFlagBegin[ifb].size();
              if( str0.size() > sz && str0.substr(0,sz) == permittedFlagBegin[ifb] )
              {
                tmpCodes.push_back( str0 ) ;
                isCont=true;
                break;
              }
            }

            if( isCont )
               continue;
          }

        }

        if( str0.size() > 2 && str0[0] == 'F' )
        {
          size_t pos;

          if( (pos=str0.find('=')) < std::string::npos )
            currFrq.push_back( str0.substr(pos+1) );
          continue;
        }

        if( str0.size() > 2 && str0[0] == 'V' )
        {
          size_t pos;

          if( (pos=str0.find('=')) < std::string::npos )
            currValue.push_back( str0.substr(pos+1) );
          continue;
        }

        if( str0.size() > 2 && str0[0] == 'R' )
        {
          size_t pos;

          if( (pos=str0.find('=')) < std::string::npos )
          {
            Split recs( str0.substr(pos+1), "-");
            if( recs.size() == 0 )
              continue;  // silently dropped
            else
            {
              xRec_0.push_back( static_cast<size_t>(
              hdhC::string2Double(str0.substr(pos+1))) ) ;

              std::string t(str0.substr(pos+1));
              if( (pos=t.find('-')) < std::string::npos )
              {
                 xRec_1.push_back( static_cast<size_t>(
                 hdhC::string2Double(t.substr(pos+1))) +1 ) ;
              }
              else
                xRec_1.push_back(xRec_0.back() + 1) ;
            }
          }
          continue;
        }

        // only variables
        if( str0.substr(0,4) == "var=" )
          vars.push_back( str0.substr(4) );
        else
          vars.push_back( str0 );
     }

     // any variable specified? No? Then, set '*' for all variables
     if( vars.size() == 0 )
       vars.push_back("*");

     if( tmpCodes.size() == 0 )
       tmpCodes.push_back("*");  // apply to all kinds of error flags

     // now scan for all the variables and update vectors
     for( size_t j=0 ; j < vars.size() ; ++j )
     {
       // here, a variable is specified
       for( size_t cc=0 ; cc < tmpCodes.size() ; ++cc )
       {
          // push the corresponding vectors
          var.push_back(vars[j]);
          code.push_back(tmpCodes[cc]);

          task.push_back("");
          if( isD )
            task.back() = "D";

          if( isEmail )
            task.back() += ",EM" ;

          if( isL1 )
            level.push_back("L1");
          else if( isL2 )
            level.push_back("L2");
          else if( isL3 )
            level.push_back("L3");
          else if( isEmergency )
            level.push_back("L4");
          else
            level.push_back("L1");

          count.push_back( 0 );
          text.push_back( checkListText );

          value.push_back( std::vector<std::string>(currValue) );
          frq.push_back( std::vector<std::string>(currFrq) );

          xRecord_0.push_back( std::vector<size_t>() );
          xRecord_1.push_back( std::vector<size_t>() );
          for( size_t i=0 ; i < xRec_0.size() ; ++i )
          {
            xRecord_0.back().push_back( xRec_0[i]) ;
            xRecord_1.back().push_back( xRec_1[i]) ;
          }

/*
std::cout << "v: " << vars[j]
          << "\tl: " << level.back()
          << "\tt: " << task.back() << std::endl ;
*/

       }  // loop over flags
     }  // loop over variables
  }  // loop over all assignments in the configuration

  options.clear();  // no longer needed
  return;
}

void
Annotation::print(void)
{
  printEMail();
  printFlags();
  printCheckResult();

  return;
}

void
Annotation::printCheckResult(void)
{
  if( isCheckResultsWasPrinted )
    return;

  isCheckResultsWasPrinted = true;

  // brief summery of the QA findings, which are currently
  // collected in vectors, but are eventually merged.

  std::string out( "CHECK-BEG" );

  out += getCheckStatus() ;

  out += "CHECK-END";  // mark of the end of an output line
  std::cout << out << std::endl;

  return;
}

void
Annotation::printEMail(void)
{
   if( mail_out_body.size() )
   {
     std::cout << "EMAIL-BEG" ;

     if( mail_out_subject.size() )
        std::cout << mail_out_subject ;

     std::cout << mail_out_body ;

     std::cout << "EMAIL-END" << std::endl ;
   }
   return;
}

void
Annotation::printFlags(void)
{
  std::string out ;

  if( isDisplay )
  {
    out ="----------------------------\n" ;

    if( isOutputPASS )
    {
      out += "path: " + file.path;
      out += "\nfile: " + file.filename;
      if( mp.begin() == mp.end() )
      {
        out += ":\tPASS\n" ;
        std::cout << out << std::endl;
        return;
      }
      else
      {
        out += ":\tFAIL\n" ;
        std::cout << out << std::endl;
      }
    }
  }

  std::map<std::string, std::string>::iterator it;

  for( it=mp.begin() ; it != mp.end() ; ++it )
  {
    const std::string& f=it->first;
    const std::string& s=it->second;

    if( mp_count[f] == 0 )
       continue;  // overruled by another one, so skip

    out.clear();

    if( ! isDisplay )
      out = "FLAG-BEG" ;

    if( mp_lev[f].size() )
      out += mp_lev[f] + '-' ;

    out += s;
    out += ':' ;

    if( mp_capt.count(f) )
    {
      if( ! isDisplay )
         out += "CAPT-BEG" ;
      else
         out += ' ';

      // remove surrounding blanks
      std::string str(hdhC::stripSides(mp_capt[f]));

      // convert the first char of the caption to upper case.
      if( str[0] == '!' ) // suppression
        out += str.substr(1);
      else
      {
        str[0] = toupper(str[0]);
        out += str;
      }
    }

    // any account for accumulated 'R' flags?
    // Still the caption.
/*
    if( s[0] == 'R' )
    {
       int num;
       if( (num=mp_count.count(f)) )
       {
         out += " (total: " ;
         out += hdhC::itoa(num) ;
         out += ")" ;
       }
    }
*/

    int last=out.size() -1 ;
    if( last > -1 &&
        !(out[last] == '.' || out[last] == '!' ||out[last] == '?' ) )
      out += "." ;

    if( ! isDisplay )
      out += "CAPT-END" ;
    else
      out +="\n";

    if( mp_txt.count(f) && mp_txt[f].size() )
    {
      // rm trailing newline, i.e. ';'
      std::string& str = mp_txt[f] ;

      if( str[str.size()-1] == ';' )
        str = str.substr(0,str.size()-1);

      if( ! isDisplay )
      {
        out += "TXT-BEG" ;
        out += str;
        out += "TXT-END" ;
      }
      else
        out += str + "\n";
    }

    if( ! isDisplay )
      out += "FLAG-END";  // mark of the end of an output line

    std::cout << out << std::endl;
  }

  return;
}

void
Annotation::printHeader(std::ofstream *ofs)
{
//   std::string str( where ) ;
   std::string str;
   str +="\nPath: " ;
   str += file.path ;
   str += "\nFile: " ;
   str += file.filename;

   *ofs << str << std::endl ;

   return ;
}

void
Annotation::printNotes(std::string &tag, std::string &caption,
   std::vector<std::string>* vs_text, bool enableStopAtErr)
{
  // str == message

  // Occurrence of an error usually stops the run immediately.
  // But, it lies with the calling program unit to exit.
  if( ofsNotes == 0 && ! isDisplay )
  {
    if( file.path.size() == 0 )
      file.path = "undefined_path" ;
    if( file.filename.size() == 0 )
      file.filename = "undefined_filename" ;

    // compose the header of issued messages
    std::string strNotes = "qa_note_" ;
    strNotes += file.basename + ".txt";

    // open file for writing
    if( ! ofsNotes )
      ofsNotes = new std::ofstream(strNotes.c_str());

    printHeader(ofsNotes);
  }

//  if( isMultipleTags || !str.size() )
//     return;

  // prevent error message flooding
  if( isApply_MORE )
  {
    if( count[currIndex]++ > recErrCountLimit )
      return;

    if( count[currIndex] > recErrCountLimit )
    {
      std::string more("more ...");

      //*ofsNotes << more << std::endl;
      ofs_text += more + "\n";
      mp_txt[tag] += more ;

      return;
    }
  }

  // write caption+message, but the caption only once
  if( mp_count[tag] == 1 )
      ofs_text += "\n" + caption + "\n";
      // *ofsNotes << "\n" << caption << std::endl;

  if( mp_txt[tag].size() )
     mp_txt[tag] += ';' ;

  if( ! vs_text )
    return;

  std::string str;
  for( size_t j=0 ; j < vs_text->size() ; ++j )
  {
     std::vector<std::string>& vs = *vs_text;
     std::string& text = vs[j];

     size_t sz=text.size();

     if( text[sz-1] != '\n' )
         text += '\n';

     ofs_text += text;
     //*ofsNotes <<  text ;

     // output the pure text with '\n' replaced by ';'
     // could be internally or eventually
     for( size_t i=0 ; i < sz ; ++i )
     {
       if( text[i] == '\n' )
          str += ';' ;
       else
          str += text[i];
     }
  }

  mp_txt[tag] += str;

  if( ! isDisplay )
      *ofsNotes << ofs_text;

  return ;
}

void
Annotation::push_back(std::string pf_code, std::string pf_var,
      std::vector<std::string> &pf_frq, std::string pf_level,
      std::string pf_task, std::string pf_text,
      std::vector<std::string> &pf_value,
      std::vector<size_t> &pf_xRec_0, std::vector<size_t> &pf_xRec_1)
{
   code.push_back(pf_code) ;
   count.push_back(0) ;
   frq.push_back(pf_frq) ;
   level.push_back(pf_level) ;
   task.push_back(pf_task) ;
   text.push_back(pf_text) ;
   value.push_back(pf_value) ;
   var.push_back(pf_var) ;
   xRecord_0.push_back(pf_xRec_0);
   xRecord_1.push_back(pf_xRec_1);

   return;
}

void
Annotation::push_front(std::string pf_code, std::string pf_var,
      std::vector<std::string> &pf_frq, std::string pf_level,
      std::string pf_task, std::string pf_text,
      std::vector<std::string> &pf_value,
      std::vector<size_t> &pf_xRec_0, std::vector<size_t> &pf_xRec_1)
{

   if( code.size() == 0 )
   {
     code.push_back(pf_code) ;
     count.push_back(0) ;
     frq.push_back(pf_frq) ;
     level.push_back(pf_level) ;
     task.push_back(pf_task) ;
     text.push_back(pf_text) ;
     value.push_back(pf_value) ;
     var.push_back(pf_var) ;
     xRecord_0.push_back(pf_xRec_0);
     xRecord_1.push_back(pf_xRec_1);

     return;
   }

   // testing idendity
   for( size_t i=0 ; i < code.size() ; ++i )
     if( code[i] == pf_code )
       return;

   std::vector<std::string>::iterator it_code  = code.begin();
   std::vector<size_t>::iterator      it_count = count.begin();
   std::vector<std::string>::iterator it_level = level.begin();
   std::vector<std::string>::iterator it_task  = task.begin();
   std::vector<std::string>::iterator it_text  = text.begin();
   std::vector<std::string>::iterator it_var   = var.begin();

   std::vector<std::vector<std::string> >::iterator it_frq = frq.begin();
   std::vector<std::vector<std::string> >::iterator it_value = value.begin();
   std::vector<std::vector<size_t> >::iterator it_xRecord_0 = xRecord_0.begin();
   std::vector<std::vector<size_t> >::iterator it_xRecord_1 = xRecord_1.begin();

   code.insert(it_code, pf_code) ;
   count.insert(it_count, 0) ;
   frq.insert(it_frq, pf_frq) ;
   level.insert(it_level, pf_level) ;
   task.insert(it_task, pf_task) ;
   text.insert(it_text, pf_text) ;
   var.insert(it_var, pf_var) ;
   value.insert(it_value, pf_value) ;
   xRecord_0.insert(it_xRecord_0, pf_xRec_0) ;
   xRecord_1.insert(it_xRecord_1, pf_xRec_1) ;

   return;
}

void
Annotation::readConf(void)
{
  std::string notesConf;

  if( checkList[0] != '/' )
  {
    notesConf = tablePath ;
    if( tablePath[tablePath.size()-1] != '/' )
      notesConf += '/' ;
  }
  notesConf += checkList ;

  ReadLine ifs( notesConf ) ;
  ifs.skipComment();
  ifs.clearSurroundingSpaces();

  // at first, read settings from file
//  std::ifstream ifs(notesConf.c_str(), std::ios::in);

  std::string str0;

  if( !ifs.isOpen() )  // file does not exist
     return ;

  std::string txt;
  BraceOP groups;
  Split x_txt;
  x_txt.setSeparator("&");


  while( !ifs.getLine(txt) )
  {
    if( txt.size() == 0 )
      continue;

    size_t pos;
    if(txt.find("PERMITTED_FLAG_BEGIN") < std::string::npos)
    {
       if( (pos=txt.find("=")) < std::string::npos)
       {
         Split splt(txt.substr(++pos), ",") ;
         for(size_t i=0 ; i< splt.size() ; ++i)
            permittedFlagBegin.push_back(splt[i]);
       }

       continue;
    }

    // one for all
    size_t pos_x=0;
    if( useAlways.size()
          || (pos_x=txt.find("NOTE_ALWAYS=")) < std::string::npos
            || (pos=txt.find("NOTE_ALWAYS")) < std::string::npos )
    {
      pos = 12;
      if( pos_x < std::string::npos )
         ++pos ;  // for the assignment '=' char

      if( useAlways.size() )
        groups.set(useAlways) ;
      else
        groups.set(txt.substr(pos+10)) ;  // size of "NOTE_ALWAYS"

      // clear previous settings
      options.clear();
      descript.clear();

      while ( groups.next(txt) )
        setConfVector( "", txt ) ;

      isUseDefault =false;
      break; // leave the while loop
    }

    // separation of text and task at &
    x_txt = txt ;
    txt=x_txt[0];
    str0=x_txt[1];

    if( levelLimit ||
          txt.find("NOTE_LEVEL_LIMIT") < std::string::npos )
    {
      if( levelLimit == 0 )
      {
         // coding for case-insensitive leading 'L'
        levelLimit = static_cast<size_t>( hdhC::string2Double(txt) );
        levelLimitStr = "L";
        levelLimitStr += hdhC::double2String(levelLimit);

        continue;
      }

      // replace level value by the limitation value
      size_t pos=0 ;
      while( (pos=str0.find("L", pos)) < std::string::npos )
      {
         size_t lval=static_cast<size_t>(
                        hdhC::string2Double(str0.substr(pos,2) ) );

         if( lval > levelLimit )
           str0.replace(pos,2,levelLimitStr);

         ++pos; // avoids an infinite loop
      }
    }

    if( txt.size() && txt[txt.size()-1] != '.' )
      txt += '.' ;

    groups.set(str0);
    while ( groups.next(str0) )
      setConfVector( txt, str0 ) ;
  }

  ifs.close();
  return;
}

void
Annotation::setCheckStatus(std::string id, std::string s, bool isAdd)
{
   // precedence: FAIL:4, PASS: 3, FIXED: 2, disabled: 1, N/A: 0
   // if rnk > -1, then this is applied. Else, extracted from string s
   int rnk = 0;

   if( s.find("FAIL") < std::string::npos )
     rnk=4;
   else if( s.find("PASS") < std::string::npos )
     rnk=3;
   else if( s.find("FIXED") < std::string::npos )
     rnk=2;
   else if( s.find("disabled") < std::string::npos )
     rnk=1;
   else if( s.find("N/A") < std::string::npos )
     rnk=0;

   if( rnk > checkRank[id] )
   {
       checkRank[id] = rnk;
       checkID[id] = s;
   }
   else if(isAdd)
   {
     if(checkID.count(id))
        checkID[id] += ",";

     checkID[id] += s;
   }
   else
     checkID[id] = s;

   return;
}

void
Annotation::setConfVector(std::string txt, std::string str0)
{
  //parse directives supplied by BraceOP

  if( str0.size() == 0 )
    return; // no brace contents

  options.push_back( str0 );
  descript.push_back( txt );

  return ;
}

void
Annotation::setUseAlways(std::string s, std::string mode)
{
  if( mode == "always" )
    useAlways=s;
  else if( mode == "limit" )
  {
    levelLimit=static_cast<size_t>(hdhC::string2Double(s));

    // insensitive to a leading 'L'
    levelLimitStr = "L";
    levelLimitStr += hdhC::double2String(levelLimit);
  }

  return;
}
