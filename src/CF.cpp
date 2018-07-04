CF::CF()
{
  initDefaults();
}

void
CF::applyOptions(void)
{
  for( size_t i=0 ; i < optStr.size() ; ++i)
  {
     Split split(optStr[i], "=");

     if( split.size() == 0 )
       continue;  // this is a fault

     if( split[0] == "fR"
          || split[0] == "follow_recommendations" )
     {
        followRecommendations=true;
        continue;
     }

     if( split[0] == "cF" )
     {
       if( split.size() == 2 )
       {
          cFVersion=split[1] ;
          continue;
       }
     }

     if( split[0] == "cFAT"
          || split[0] == "area_table" )
     {
       if( split.size() == 2 )
       {
          area_table.setFile(split[1]);
          continue;
       }
     }

     if( split[0] == "cFSN"
          || split[0] == "standard_name_table" )
     {
       if( split.size() == 2 )
       {
          std_name_table.setFile(split[1]);
          continue;
       }
     }

     if( split[0] == "cFSRN"
          || split[0] == "region_table" )
     {
       if( split.size() == 2 )
       {
          region_table.setFile(split[1]);
          continue;
       }
     }

     if( split[0] == "tP"
          || split[0] == "tablePath" || split[0] == "table_path")
     {
       if( split.size() == 2 )
       {
          tablePath=split[1];
          continue;
       }
     }
   }

   // apply a general path which could have also been provided by setTablePath()
   if( std_name_table.path.size() == 0 )
      std_name_table.setPath(tablePath);
   if( area_table.path.size() == 0 )
      area_table.setPath(tablePath);
   if( region_table.path.size() == 0 )
      region_table.setPath(tablePath);

   return;
}

void
CF::analyseCoordWeights(void)
{
   // Are auxiliary coord vars consistent with coordinate variabels?
   // If not, then the coordinate variables direction is reset, because the
   // auxiliary coord is more reliable in most cases.
  std::vector<int> vs_aux_ix;

  int gcv_ix[]={0,0,0,0};

  // check that there is only a single coord-var of a given type
  for( size_t i=0 ; i < pIn->varSz ; ++i )
  {
    Variable& var = pIn->variable[i] ;

    if( var.isDataVar() )
      continue;

    int j;
    bool isIx= (j=var.getCoordinateType()) > -1 && j < 5 ;

    if(var.coord.isCoordVar)
    {
      // only genuine coordinate vars
      if(isIx)
        ++gcv_ix[j] ;
    }
    else
    {
      // the auxiliary coord vars; also ambiguous
      if( j > -1)
        vs_aux_ix.push_back( static_cast<int>(i) );
    }
  }

  // compare and adjust weights when undecided
  for( size_t i=0 ; i < pIn->varSz ; ++i )
  {
    if( pIn->variable[i].isDataVar() )
      continue;

    std::vector<bool>& isC = pIn->variable[i].coord.isC ;
    std::vector<int>& weight = pIn->variable[i].coord.weight ;

    for( size_t w=0 ; w < 4 ; ++w )
    {
      if( weight[w] > 0 )
      {
        // is it overruled by weights of other directions?
        for( size_t c=0 ; c < 4 ; ++c )
        {
          if( c != w && weight[c] > weight[w] )
          {
            if( isC[c] || weight[c] > (weight[w]+1) )
            {
              weight[w]=0;
              isC[w]=false;
            }
          }
        }
      }
    }
  }

/*
  // weights for the same direction cross-checked over variables
  for( size_t i=0 ; i < pIn->varSz ; ++i )
  {
    std::vector<bool>& isC_i = pIn->variable[i].coord.isC ;
    std::vector<int>& weight_i = pIn->variable[i].coord.weight ;

    for( size_t c=0 ; c < 4 ; ++c )
    {
      if( weight_i[c] > 0 )
      {
        for( size_t j=0 ; j < pIn->varSz ; ++j )
        {
          if( i==j )
            continue;

          std::vector<int>& weight_j = pIn->variable[j].coord.weight ;
          std::vector<bool>& isC_j = pIn->variable[j].coord.isC ;

          // is it overruled by weights of same directions but other vars?
          if( weight_i[c] > weight_j[c] )
          {
            if( isC[c] || weight[c] > (weight[w]+1) )
            {
              weight[w]=0;
              isC[w]=false;
            }
          }
        }
      }
    }
  }
*/

  // decide undecided
  for( size_t i=0 ; i < pIn->varSz ; ++i )
  {
    if( pIn->variable[i].isDataVar() )
      continue;

    std::vector<bool>& isC = pIn->variable[i].coord.isC ;
    std::vector<int>& weight = pIn->variable[i].coord.weight ;

    int limit;
    for( size_t c=0 ; c < 4 ; ++c )
    {
      if( !isC[c] )
      {
        if( c==2 &&
              !(pIn->variable[i].coord.isZ_p || pIn->variable[i].coord.isZ_DL) )
          limit=1;
        else
          limit=0;

        if( weight[c] > limit )
          isC[c]=true;
      }
    }
  }

  // if stil undecided, but there is a coordinates attribute which has
  // (without time) auxs or dims for the three coordinates including
  // this one
  for( size_t i=0 ; i < pIn->varSz ; ++i )
  {
    Variable& var = pIn->variable[i] ;

    if( var.isDataVar() )
      continue;

    std::vector<bool>& isC = pIn->variable[i].coord.isC ;
    std::vector<int>& weight = pIn->variable[i].coord.weight ;

    for( size_t c=0 ; c < 4 ; ++c )
    {
       if( !isC[c] && weight[c] > 0 )
       {
         for( size_t j=0 ; j < pIn->varSz ; ++j )
         {
// note: verification that the others are also
            if( hdhC::isAmong(var.name, ca_vvs) )
            {
              isC[c]=true;
              break;
            }
         }
       }
    }
  }

  // a scalar aux is considered a coordinate variable if no
  // other coordinate variable exists for the same direction.
  for( size_t i=0 ; i < vs_aux_ix.size() ; ++i )
  {
    Variable& aux = pIn->variable[vs_aux_ix[i]] ;

    if( aux.isScalar )
    {
      for( size_t c=0 ; c < 4 ; ++c )
      {
        if( aux.coord.isC[c] && gcv_ix[c] == 0 )
        {
          aux.coord.isCoordVar = true;
          break;
        }
      }
    }
  }

  return;
}

void
CF::attributeSpellCheck(void)
{
  double editDistance, eDMin;
  size_t eDMin_ix;

  // test for some typos; also for global attributes
  for( size_t i=0 ; i < pIn->variable.size() ; ++i )
  {
    Variable& var = pIn->variable[i];

    for( size_t j=0 ; j < var.attName.size() ; ++j)
    {
      // would accept same names when case is ignored; this is trapped elsewhere.
      std::string t(hdhC::Lower()(var.attName[j])) ;
      bool is=false;
      for( size_t jj=0 ; jj < var.attName.size() ; ++jj )
      {
        if( j != jj )
        {
          if( t == hdhC::Lower()(var.attName[jj]) )
          {
            is=true;
            break;
          }
        }
      }
      if( is )
        continue;

      size_t k;
      eDMin=100.;
      for( k=0 ; k < CF::attName.size() ; ++k)
      {
        std::string s=CF::attName[k];
        editDistance = wagnerFischerAlgo(var.attName[j], CF::attName[k]);

        if( editDistance < eDMin )
        {
           eDMin = editDistance ;
           eDMin_ix = k;
        }

        if( editDistance == 0 )
          break;
      }

      if( eDMin < 0.25 && ! var.isValidAtt(CF::attName[eDMin_ix]) )
      {
        if( notes->inq(bKey + "0h", var.name) )
        {
          std::string capt("is " ) ;
          if( var.name == n_NC_GLOBAL )
          {
            capt += "global " + n_attribute ;
            capt += hdhC::tf_val(var.attName[j]) ;
          }
          else
            capt += hdhC::tf_att(var.name,var.attName[j]) ;

          capt += " misspelled? Did you mean " ;
          capt += CF::attName[eDMin_ix] + "?";

          (void) notes->operate(capt) ;
          notes->setCheckStatus( n_CF, fail );
        }

        // correction for internal use
        std::map<std::string, int>:: iterator it
            = var.attNameMap.find(var.attName[j]);

        var.attName[j] = CF::attName[eDMin_ix] ;

        if (it != var.attNameMap.end())
        {
          int value = it->second;
          var.attNameMap.erase(it);
          var.attNameMap[var.attName[j]] = value;
        }

      }

    }
  }

  return;
}

void
CF::checkCoordinateFillValueAtt(Variable& var)
{
   bool isF = var.isValidAtt(n_FillValue) ? true : false ;
   bool isM = var.isValidAtt(n_missing_value) ? true : false ;

   if( isF || isM )
   {
      if( notes->inq(bKey + "12c", var.name) )
      {
        std::string capt;
        if( var.isAUX )
          capt = "auxiliary ";

        capt += "coordinate " + hdhC::tf_var(var.name);
        capt += "should not have " ;
        if( isF && isM)
        {
          capt += n_attribute + "s" ;
          capt += hdhC::tf_val(n_FillValue, hdhC::blank)  + "and" ;
          capt += hdhC::tf_val(n_missing_value) ;
        }
        else if( isF )
          capt += hdhC::tf_att(n_FillValue) ;
        else
          capt += hdhC::tf_att(n_missing_value) ;

        (void) notes->operate(capt) ;
        notes->setCheckStatus( n_CF, fail );
      }
   }

   return;
}

void
CF::checkCoordinateValues(Variable& var, bool isFormTermAux)
{
  // checks only meta-data declarations; _FV values in the data section
  // are tested below. Chap9 features are tested elsewhere
  if( cFVal < 16 && var.coord.isCoordVar )
    checkCoordinateFillValueAtt(var) ;

  // no check of an unlimited coord var for efficiency
//  if( var.isUnlimited() )
//     return;

  if( var.isNoData() )
  {
    if( notes->inq(bKey + "0d", var.name) )
    {
      std::string capt ;
      if( !var.coord.isCoordVar )
        capt = "auxiliary " ;

      capt += "coordinate " + hdhC::tf_var(var.name, hdhC::colon);
      capt += "No data" ;

      (void) notes->operate(capt) ;
      notes->setCheckStatus( n_CF, fail );
    }

    return;
  }

  signed char x_sc=0;
  char x_c=0;
  short x_s=0;
  int x_i=0;
  float x_f=0.;
  double x_d=0.;
  unsigned char x_uc=0;
  unsigned short x_us=0;
  unsigned int x_ui=0;
  long long x_ll=0;
  unsigned long long x_ull=0;

  if( var.type == NC_BYTE )
    checkCoordinateValues(var, isFormTermAux, x_sc) ;
  else if( var.type == NC_CHAR )
    checkCoordinateValues(var, isFormTermAux, x_c) ;
  else if( var.type == NC_SHORT )
    checkCoordinateValues(var, isFormTermAux, x_s) ;
  else if( var.type == NC_INT )
    checkCoordinateValues(var, isFormTermAux, x_i) ;
  else if( var.type == NC_FLOAT )
    checkCoordinateValues(var, isFormTermAux, x_f) ;
  else if( var.type == NC_DOUBLE )
    checkCoordinateValues(var, isFormTermAux, x_d) ;
  else if( var.type == NC_UBYTE )
    checkCoordinateValues(var, isFormTermAux, x_uc) ;
  else if( var.type == NC_USHORT )
    checkCoordinateValues(var, isFormTermAux, x_us) ;
  else if( var.type == NC_UINT )
    checkCoordinateValues(var, isFormTermAux, x_ui) ;
  else if( var.type == NC_INT64 )
    checkCoordinateValues(var, isFormTermAux, x_ll) ;
  else if( var.type == NC_UINT64 )
    checkCoordinateValues(var, isFormTermAux, x_ull) ;

  return;
}

template <typename T>
void
CF::checkCoordinateValues(Variable& var, bool isFormTermAux, T x)
{
  // all coordinate variables must be strictly monotonic

  // Exceptions:
  // variables named by a formula_terms attribute
  // and auxiliary coordinate variables
  bool testMonotony= ( !var.coord.isCoordVar || isFormTermAux ) \
    ? false : true ;

  MtrxArr<T> mv;

  pIn->nc.getData(mv, var.name );

  if( mv.validRangeBegin.size() )
  {
    // note: [mv.vRB, mv.vRE[:  first contiguous stride without _FV
    // take into account that data may begin/end with _FV
    int i=-1;
    // a single stride
    if(mv.validRangeBegin[0] > 0)
      i=0;
    else if(mv.validRangeEnd[0] < mv.size())
      i=mv.validRangeEnd[0];

    if( i > -1 )
    {
      std::string capt0;
      std::string text;
      if( var.isCoordinate() )
      {
        if( !var.coord.isCoordVar )
          capt0 = "auxiliary " ;

        capt0 = "coordinate " ;
      }
      else if( isFormTermAux )
        capt0 += "Formula terms " ;

      capt0 += hdhC::tf_var(var.name, hdhC::colon) + "Data should not " ;

      std::vector<size_t> vs_ix ;

      // Inf or NaN
      if( mv.valExcp->countInf || mv.valExcp->countNaN )
      {
        std::string capt;

        if( notes->inq(bKey + "12a", var.name) )
        {
          if( mv.valExcp->countInf )
          {
            capt += "be <Inf>";
            text="Found first occurrence at index " ;
            text += mv.indicesStr(mv.valExcp->firstInf_ix) ;
          }

          if( mv.valExcp->countNaN )
          {
            if( mv.valExcp->countInf )
              capt += ", ";
            capt += "<NaN>";
            text="Found first occurrence at index " ;
            text += mv.indicesStr(mv.valExcp->firstNaN_ix) ;
          }

          (void) notes->operate(capt0 + capt, text) ;
          notes->setCheckStatus( n_CF, fail );
        }
      }

      // any kind of missing value
      if( var.coord.isCoordVar && mv.valExcp->exceptionCount.size() )
      {
        bool isFirst[]={ true, true, true, true, true, true};

        for(size_t e=0 ; e < mv.valExcp->exceptionCount.size() ; ++e)
        {
          std::string capt ;

          if( mv.valExcp->exceptionCount[e] )
          {
            if( isFirst[0] && mv.valExcp->exceptionMode[e] == '=')
            {
              if( var.isValidAtt(n_FillValue) )
              {
                capt += "have ";
                capt += hdhC::tf_assign(n_FillValue,
                                      var.getAttValue(n_FillValue)) ;
                 text.clear() ;
                isFirst[0]=false;
              }
              else if( isFirst[1] && var.isValidAtt(n_missing_value) )
              {
                capt += "have " ;
                capt += hdhC::tf_assign(n_missing_value,
                                      var.getAttValue(n_missing_value)) ;
                text.clear() ;
                isFirst[1]=false;
              }
              else if( isFirst[2] )
              {
                capt += "be default " ;
                capt += hdhC::tf_assign(n_FillValue,
                                      hdhC::double2String(mv[i])) ;
                text.clear();
                isFirst[2]=false;
              }
            }

            else if( isFirst[3] && mv.valExcp->exceptionMode[e] == '<')
            {
              capt += "fall below " + hdhC::tf_att(n_valid_min) ;

              text ="Found ";
              text += hdhC::tf_val(hdhC::double2String(mv[i])) ;
              text += " for " ;
              text += hdhC::tf_assign(n_valid_min, var.getAttValue(n_valid_min)) ;

              isFirst[3]=false;
            }
            else if( isFirst[4] && mv.valExcp->exceptionMode[e] == '>')
            {
              capt += "exceed " + hdhC::tf_att(n_valid_max);
              text= "Found ";
              text += hdhC::tf_val(hdhC::double2String(mv[i])) ;
              text += " for " ;
              text += hdhC::tf_assign(n_valid_max, var.getAttValue(n_valid_max)) ;
              isFirst[4]=false;
            }
            else if( isFirst[5] && mv.valExcp->exceptionMode[e] == 'R')
            {
              capt += "be out of " + hdhC::tf_att(n_valid_range) ;
              int j = var.getAttIndex(n_valid_range);
              text= "Found ";
              capt += hdhC::tf_val(hdhC::double2String(mv[i])) ;
              text +="for the range <" + var.attValue[j][0] ;
              text += ", " + var.attValue[j][1] + ">" ;
              isFirst[5]=false;
            }

            if( notes->inq(bKey + "12d", var.name) )
            {
              capt += " at index " ;
              capt += mv.indicesStr(i) ;

              (void) notes->operate(capt0+capt, text) ;
              notes->setCheckStatus( n_CF, fail );
            }
          }
        }
      }

      return;
    }
  }

  if( !testMonotony || mv.size() < 2 )
    return;

  if( notes->findAnnotation(bKey + "12d", var.name) )
    return; // test below would fail anyway

  bool isMonotone=true;
  bool isDifferent=true;

  size_t i=0;
  size_t mon_ix=0;
  size_t dif_ix=0;

  // more than a single value
  if( var.dimName.size() > 1 )
    return;

  double sig=1.;
  for( size_t j=0 ; j < mv.validRangeBegin.size() ; ++j )
  {
    // start at [1], because of the difference below
    i=mv.validRangeBegin[j] + 1 ;

    if( !j )
      sig=(static_cast<double>(mv[i]-mv[i-1])) / fabs(mv[i]-mv[i-1]);

    for( ; i < mv.validRangeEnd[j] ; ++i)
    {
      double v0=sig*mv[i-1];
      double v1=sig*mv[i];

      if( v1 > v0 )
        continue;

      if( v1 < v0 )
      {
        if(!mon_ix)
          mon_ix = i;

        isMonotone=false;
      }
      else if( v1 == v0 )
      {
        if(!dif_ix)
          dif_ix = i;

        isDifferent=false;
      }
    }
  }

  if( ! isMonotone )
  {
    i = mon_ix;

    if( notes->inq(bKey + "12b", var.name) )
    {
      // no considering about matrix indexing
      std::string capt("coordinate " + hdhC::tf_var(var.name));
      capt += "has to be strictly monotonic";

      std::string text("Found around index ") ;
      text += mv.indicesStr(i) + " values" ;

      if(i>1)
        text += hdhC::tf_val(hdhC::double2String(mv[i-2]));

      text += "," + hdhC::tf_val(hdhC::double2String(mv[i-1]));

      if( i < mv.size() )
        text += "," + hdhC::tf_val(hdhC::double2String(mv[i]));

      (void) notes->operate(capt, text) ;
      notes->setCheckStatus( n_CF, fail );
    }
  }

  else if( ! isDifferent && notes->inq(bKey + "12b", var.name) )
  {
    std::vector<size_t> vs_ix ;
    std::string capt("coordinate " + hdhC::tf_var(var.name));
    capt += "has to be strictly monotonic";

    std::string text("Found at indices ") ;
    text += mv.indicesStr(dif_ix-1) ;
    text += " and " ;
    text += mv.indicesStr(dif_ix) ;
    text += ", found equal values" ;

    text += hdhC::tf_val(hdhC::double2String(mv[dif_ix-1]));
    text += "," + hdhC::tf_val(hdhC::double2String(mv[dif_ix]));

    (void) notes->operate(capt, text) ;
    notes->setCheckStatus( n_CF, fail );
  }

  return;
}

void
CF::checkGroupRelation(void)
{
  if( pIn->varSz < 2 )  // at least two files are required for a relation
     return;

  std::vector<std::string>& aG = associatedGroups;

  for( size_t i=0 ; i < pIn->varSz ; ++i )
  {
    Variable& var = pIn->variable[i];

    if( aG.size() )
    {
      if( var.dimName.size() == 1 && hdhC::isAmong(var.dimName[0], aG) )
      {
//      var.addAuxCount();
        continue;
      }
    }

    // find unrelated scalar variables
    if( var.isCoordinate() )
    {
      if( aG.size() && hdhC::isAmong(var.name, aG) )
        continue;

      if( var.boundsOf.size() )
        continue;

      bool isCont2=false;

      // is it specified in a coordinates attribute?
      if( hdhC::isAmong( var.name, ca_vvs) )
        continue;

      for( size_t p=0 ; p < pIn->varSz ; ++p )
      {
        if(i != p)
        {
          Variable& var_p = pIn->variable[p] ;

          if( hdhC::isAmong( var.name, var_p.dimName) )
          {
            isCont2=true;
            break;
          }
        }
      }

      if( isCont2 )
        continue;

      // isXYZinCoordAtt checks a rule not described by CF, but is common.
      // If all spatial coordinates are specified
      // by the coordinate attribute, then the label may be
      // omitted in a coordinates attribute.
      if( isXYZinCoordAtt(i) )
        continue; // only once for all scalars
      else if( ! (isCompressAux(var) || var.isFormulaTermsVar || var.boundsOf.size() ) )
      {
        if( notes->inq(bKey + "0c", var.name) )
        {
          std::string capt("warning: " );
          if( var.isScalar )
            capt += "Scalar c";
          else
            capt += "C";

          capt += "oordinate " ;
          capt += hdhC::tf_var(var.name, hdhC::colon) ;
          capt += "Is it missing in any ";
          capt += hdhC::tf_att("", n_coordinates, hdhC::no_blank) ;
          capt += "?";

          (void) notes->operate(capt) ;
          notes->setCheckStatus( n_CF, fail );
        }
      }
    }
  }

  // dimension not in use
  for( size_t i=0 ; i < dimensions.size() ; ++i )
  {
    bool isCont2=false;

    for( size_t j=0 ; j < pIn->varSz ; ++j )
    {
      Variable& var = pIn->variable[j];

      if( hdhC::isAmong(dimensions[i], var.dimName ) )
      {
        isCont2=true;
        break;
      }
    }

    if(isCont2)
      continue;

    for( size_t j=0 ; j < pIn->varSz ; ++j )
    {
      if( pIn->variable[j].isValidAtt(n_compress, dimensions[i]))
      {
        isCont2=true;
        break;
      }
    }

    if(isCont2)
      continue;

    // is a dimension specified by any coordinate attribute
    for( size_t j=0 ; j < pIn->varSz ; ++j )
    {
      if( pIn->variable[j].isValidAtt(n_coordinates, dimensions[i]))
      {
        isCont2=true;
        break;
      }
    }

    if(isCont2)
      continue;

    if( notes->inq(bKey + "0b") )
    {
      std::string capt("warning: Unused " + n_dimension) ;
      capt += hdhC::tf_val(dimensions[i]) ;

      (void) notes->operate(capt) ;
      notes->setCheckStatus( n_CF, fail );
    }
  }

  return;
}

void
CF::checkSN_Modifier(Variable& var)
{
  // Is it a valid modifier?

  std::vector<std::string> valid_mod;
  valid_mod.push_back("detection_minimum");
  valid_mod.push_back(n_number_of_observations);
  valid_mod.push_back("standard_error");
  valid_mod.push_back("status_flag");

  if( var.snTableEntry.remainder.size() == 0
        && !var.snTableEntry.hasBlanks )
     return;

  Split x_remainder( var.snTableEntry.remainder );

  int countValidMod=0;

  size_t sz(x_remainder.size()) ;

  for( size_t i=0 ; i < sz ; ++i )
    if( hdhC::isAmong(x_remainder[i], valid_mod) )
      ++countValidMod ;

  if( countValidMod )
  {
    if( x_remainder.size() == 1 && ! var.snTableEntry.hasBlanks )
      return; // fine

    if( countValidMod > 1 && sz > 1 )
    {
      if( notes->inq(bKey + "33c", var.name) )
      {
        std::string capt(
          hdhC::tf_att(var.name,n_standard_name,var.std_name)) ;
        capt += "with too many modifiers" ;

        (void) notes->operate(capt) ;
        notes->setCheckStatus( n_CF, fail );
      }
    }
  }

  if( (countValidMod == 0 || var.snTableEntry.hasBlanks)
          && notes->inq(bKey + "33b", var.name) )
  {
    std::string capt(
      hdhC::tf_att(var.name, n_standard_name, var.std_name));
    capt += "contains " ;
    if( countValidMod == 0 && var.snTableEntry.remainder.size()
            && var.snTableEntry.hasBlanks )
      capt += "blanks or undefined modifier";
    else if ( var.snTableEntry.hasBlanks )
      capt += "blanks";
    else
      capt += "undefined modifier";

    (void) notes->operate(capt) ;
    notes->setCheckStatus( n_CF, fail );
  }

  return ;
}

bool
CF::cmpUnits( std::string s, std::string ref)
{
//  if( s == ref )
//    return true;

  ut_unit* from = ut_parse(unitSystem, s.c_str(), UT_ASCII);
  bool is=false;

  if( from )
  {
    ut_unit* to = ut_parse(unitSystem, ref.c_str(), UT_ASCII);

    if( to )
    {
      cv_converter* converter = ut_get_converter(from, to);

      if( converter )
      {
        cv_free(converter);
        is=true;
      }
    }

    if(to == from)
      from=0;

    if(to)
      ut_free(to);
  }

  if(from)
    ut_free(from);

  return is;
}

bool
CF::entry(void)
{
  // get properties from the file according to CF-1.x chapters
  chap();

/*
  // no data?
  for( size_t i=0 ; i < pIn->varSz ; ++i )
  {
    Variable& var = pIn->variable[i] ;

    if( var.isNoData() && !var.isVoid && var.isDataVar() )
    {
      if( pIn->nc.isIndexType(var.name)
            && notes->inq(bKey + "12e", var.name) )
      {
        std::string capt("index ");
        capt += hdhC::tf_var(var.name);
        capt += "must have data" ;

        (void) notes->operate(capt) ;
        notes->setCheckStatus( n_CF, fail );
      }

      else if( notes->inq(bKey + "0e", var.name) )
      {
        std::string capt(hdhC::tf_var(var.name, hdhC::colon));
        capt += "No data" ;

        (void) notes->operate(capt) ;
        notes->setCheckStatus( n_CF, fail );
      }
    }
  }
*/

  // find groups of related variables
  checkGroupRelation();

  // coordinates, axis, positive
  finalAtt() ;

  findAmbiguousCoords();

  if( isCheck )
  {
    if(followRecommendations )
      chap_reco();
  }

  // a few post-check considerations about combinations of annotations
  postAnnotations();

  for( size_t i=0 ; i < pIn->varSz ; ++i )
  {
    Variable& var = pIn->variable[i] ;

    if( var.isDataVar() )
      var.isDATA = true;
    else
      var.isAUX = true;

    if( var.isAUX && var.isDATA )
    {
      // time dependent var in context of dimless Z
      var.isDATA = false;
      var.countData = 0;
    }
  }

  return true;
}

void
CF::finalAtt(void)
{
   final_dataVar();

   // check requirements of axes
   finalAtt_axis();
   finalAtt_coordinates();
   finalAtt_positive();
   finalAtt_units();

   return;
}

void
CF::finalAtt_axis(void)
{
  // CF-1.4: only coordinate vars may have attached an axis attribute.
  // CF-1.6: aux coord vars AND coordinate vars may have attached
  //         an axis  attribute.
  // CF-1.6: A data variable may have both coordinate and auxiliary
  //         coordinate variables, but only one of them may have an
  //         axis attribute.
  //         It is recommended that one of them has an axis attribute.

  // the checks start from a data variables point of view
  for( size_t j=0 ; j < pIn->varSz ; ++j )
  {
    Variable& var = pIn->variable[j] ;

    if( var.isValidAtt(n_axis) )
    {
      // legal values of axis are T,Z,Y,X (case insensitive)
      std::string axis( var.getAttValue(n_axis, lowerCase) );

      if( ! (axis == "x" || axis == "y" || axis == "z" || axis == "t")  )
      {
        if( notes->inq(bKey + "4a", var.name) )
        {
          std::string capt(hdhC::tf_att(var.name, n_axis));
          capt += "is not X,Y,Z,T";

          std::string text("Found ") ;
          text += hdhC::tf_val(var.getAttValue(n_axis));

          (void) notes->operate(capt, text) ;
          notes->setCheckStatus( n_CF, fail );
        }
      }

      if( var.isDataVar() )
      {
        if( notes->inq(bKey + "5f", var.name) )
        {
          std::string capt("data " + hdhC::tf_var(var.name));
          capt += "should not have " + hdhC::tf_att(n_axis);

          (void) notes->operate(capt) ;
          notes->setCheckStatus( n_CF, fail );
        }
      }
      else if( var.coord.isCoordVar || cFVal > 15 )
      {
        // axis must be consistent with the coordinate variable
        std::string text;

        if( axis != "x" && var.coord.isC[0] )
          text =  "X";
        else if( axis != "y" && var.coord.isC[1] )
          text = "Y";
        else if( axis != "z" && var.coord.isC[2] )
          text = "Z";
        else if( axis != "t" && var.coord.isC[3] )
          text = "T";

        if( text.size() && !notes->findAnnotation(bKey+"4a",var.name) )
        {
          if( notes->inq(bKey + "4b", var.name) )
          {
            std::string capt( hdhC::tf_att(var.name, n_axis, hdhC::colon) );
            capt += "Found";
            capt += hdhC::tf_val(axis);
            capt += ", exptected ";
            capt += hdhC::tf_val(text);

            (void) notes->operate(capt) ;
            notes->setCheckStatus( n_CF, fail );
          }
        }
      }
    }
  }

  char xs[]={'X', 'Y', 'Z', 'T'};

  // relations between data variables and associated coordinate variables
  // as well as auxiliary coordinate variables.
  for( size_t i=0 ; i < pIn->varSz ; ++i )
  {
    Variable& var = pIn->variable[i] ;

    if( !var.isDataVar() || var.coord.isCoordVar || var.isCoordinate() )
       continue;

    // collection of indices of all var-reps of the current variable's dims
    // and of existing variables contained in the coordinates attribute.
    std::vector<int> coll_ix;
    std::vector<int> coll_dim_ix;

    // dim-list
    int ix;
    for( size_t l=0 ; l < var.dimName.size() ; ++l )
    {
       if( (ix=pIn->getVarIndex(var.dimName[l])) > -1 )
       {
          coll_dim_ix.push_back(ix);

          if( !hdhC::isAmong(ix, coll_ix) )
            coll_ix.push_back(ix);
       }
    }

    // coordinates attribute list
    for(size_t l=0 ; l < ca_vvs[i].size() ; ++l )
      if( (ix=pIn->getVarIndex(ca_vvs[i][l])) > -1 )
        if( !hdhC::isAmong(ix, coll_ix) )
          coll_ix.push_back(ix);

    // take into account the four axis
    std::vector<size_t> ix_AuxCoordAxis[4];
    std::vector<size_t> ix_CoordAxis[4];
    std::vector<size_t> ix_Axis[4];
    std::vector<size_t> ix_CoordVar[4];

    for( size_t c=0 ; c < coll_ix.size() ; ++c )
    {
      Variable& var_ix = pIn->variable[coll_ix[c]] ;

      std::string axis( var_ix.getAttValue(n_axis) );

      for( size_t l=0 ; l < 4 ; ++l )
        if( var_ix.coord.isC[l] && var_ix.coord.isCoordVar )
          ix_CoordVar[l].push_back(ix);

      for( size_t l=0 ; l < 4 ; ++l )
      {
        if( toupper(axis[0]) == xs[l] )
        {
          ix_Axis[l].push_back(ix);

          if( var_ix.coord.isCoordVar )
            ix_CoordAxis[l].push_back(ix);
          else
            ix_AuxCoordAxis[l].push_back(ix);

          break;
        }
      }
    }

    for( size_t l=0 ; l < 4 ; ++l )
    {
      if( ix_CoordAxis[l].size() > 1 )
      {
        if( notes->inq(bKey + "5g", var.name) )
        {
          std::string capt( hdhC::tf_var(var.name) );
          capt += "must not depend on more than one coordinate " + n_variable ;
          capt += " with the same axis";

          std::string text("Found ");

          for( size_t k=0 ; k < ix_CoordAxis[l].size() ; ++k )
          {
            if( k )
              text += "," ;
            text += hdhC::tf_val(pIn->variable[ ix_CoordAxis[l][k] ].name) ;
          }

          (void) notes->operate(capt, text) ;
          notes->setCheckStatus( n_CF, fail );
        }
      }

      if( cFVal > 15 &&
              ix_AuxCoordAxis[l].size() && ix_CoordAxis[l].size() )
      {
        if( notes->inq(bKey + "5e", var.name) )
        {
          std::string capt(hdhC::tf_var(var.name) );
          capt += "should not have a coordinate " + n_variable ;
          capt += " and an auxiliary coordinate " + n_variable ;
          capt += " both with " + hdhC::tf_att(n_axis) + "= <";
          capt += xs[l] ;
          capt += ">";

          (void) notes->operate(capt) ;
          notes->setCheckStatus( n_CF, fail );
        }
      }
    }
  }

  // works even for orphaned auxiliary coordinate variables
  for( size_t i=0 ; i < pIn->varSz ; ++i )
  {
    Variable& var = pIn->variable[i] ;

    if( var.isDataVar() )
      continue;

    if( cFVal < 16 && var.isValidAtt(n_axis)
           && ! var.coord.isCoordVar && var.isCoordinate() )
    {
      if( notes->inq(bKey + "5h", var.name) )
      {
        std::string capt(cFVersion + ": Auxiliary coordinate ");
        capt += hdhC::tf_var(var.name);
        capt += "must not have " + hdhC::tf_att(n_axis);

        (void) notes->operate(capt) ;
        notes->setCheckStatus( n_CF, fail );
      }
    }

    if( followRecommendations )
    {
      // recommendation: horizontal coordinate variable other than
      // longitude, latitude should have an axis attribute
      if( var.coord.isCoordVar && (var.coord.isC[0] || var.coord.isC[1]) )
      {
        // cases with a false axis value would match this
        if( !var.isValidAtt(n_axis) )
        {

          if( ! (var.coord.isBasicType[0] || var.coord.isBasicType[1]) )
          {
             std::string tag(bKey + "5d");

             if( notes->inq(tag, var.name) )
             {
                std::string capt("reco: Horizontal coordinate ");
                capt += hdhC::tf_var(var.name) ;
                capt += "should have " + hdhC::tf_att(n_axis, hdhC::no_blank);
                capt += "=";
                if( var.coord.isC[0] )
                   capt += "X";
                else
                   capt += "Y";

                (void) notes->operate(capt) ;
                notes->setCheckStatus( n_CF, fail );
             }
          }
        }
      }
    }
  }

  return;
}

void
CF::finalAtt_coordinates(void)
{
  // cf_conventions-1.4.pdf:
  // All of a variable's spatio temporal dimensions that are not latitude, longitude,
  // vertical, or time dimensions are required to be associated with the relevant
  // latitude, longitude, vertical, or time coordinates via the coordinates
  // attribute of the variable.

  // find non-existing var contained in a coords att
  finalAtt_coordinates_A();

  // auxiliary is missing in a coordinates attribute; pass when in doubt.
  finalAtt_coordinates_B();

  // coordinate var must not have coordinates attribute
  finalAtt_coordinates_C();

  // aux coordinate identified but nnot declared in any coordinates attribute
  finalAtt_coordinates_D();

  return;
}

void
CF::finalAtt_coordinates_A(void)
{
  // find non-existing var contained in a coords att

  // collection of all vars in any coords att, which have been checked
  std::vector<std::string> vs_cav;

  for(size_t i=0 ; i < ca_vvs.size() ; ++i )
  {
    for(size_t j=0 ; j < ca_vvs[i].size() ; ++j )
    {
      if( !hdhC::isAmong(ca_vvs[i][j], vs_cav) )
      {
        vs_cav.push_back(ca_vvs[i][j]);

        if( pIn->getVarIndex(ca_vvs[i][j]) == -1 )
        {
          if( notes->inq(bKey + "5a") )
          {
            Variable& var = pIn->variable[i];

            std::string capt(hdhC::tf_att(var.name, n_coordinates));
            capt += "references non-existing " + hdhC::tf_var(ca_vvs[i][j]) ;

            (void) notes->operate(capt) ;
            notes->setCheckStatus( n_CF, fail );
          }
        }
      }
    }
  }

  return;
}

void
CF::finalAtt_coordinates_B(void)
{
  // Auxiliary is missing in a coordinates attribute.
  // Pass when in doubt.

  size_t dimSzMax=0;  // variable with highest dimensionality
  size_t ii;
  for( size_t i=0 ; i < pIn->varSz ; ++i )
    if( dimSzMax < (ii=pIn->variable[i].dimName.size()) )
      dimSzMax=ii;

  for( size_t i=0 ; i < pIn->varSz ; ++i )
  {
    Variable& var = pIn->variable[i] ;

    if( var.isCoordinate() )
      continue;
    if( var.boundsOf.size() )
      continue;
    if( var.isLabel )
      continue;

    // a variable depends soley on coordinate dimensions
    bool isSole=true;
    for( size_t j=0 ; j < var.dimName.size() ; ++j )
    {
      size_t pos;
      if( hdhC::isAmong(var.dimName[j], pIn->variableNames, pos) )
      {
         if( ! pIn->variable[pos].coord.isCoordVar )
         {
           isSole=false;
           break;
         }
      }
      else
      {
         isSole=false;
         break;
      }
    }

    if(isSole)
      continue;

    if( pIn->nc.isIndexType(var.name) )
      continue;

    // a different type of variable or with a subset of dimensions
    if( var.dimName.size() < dimSzMax )
      continue;

    // if var appears in any coordinates att, then it cannot have a missing ca
    if( hdhC::isAmong( var.name, ca_vvs) )
      continue;

    // is var named in cell_methods or cell_measures?
    int jx;
    bool is=false;
    for(size_t j=0 ; j < pIn->varSz ; ++j )
    {
      if( (jx=pIn->variable[j].getAttIndex(n_cell_methods)) > -1 )
      {
        if( pIn->variable[j].attValue[jx][0].find(var.name) < std::string::npos )
        {
          is=true;
          break;
        }
      }

      if( (jx=pIn->variable[j].getAttIndex(n_cell_measures)) > -1 )
      {
        if( pIn->variable[j].attValue[jx][0].find(var.name) < std::string::npos )
        {
          is=true;
          break;
        }
      }
    }

    if(is)
      continue;

    // is any dim associated with an aux coord var?
    for( size_t j=0 ; j < var.dimName.size() ; ++j )
    {
      // dim is in the coordinates att
//      if( hdhC::isAmong(var.dimName[j], ca_vvs[i]) )
//        continue;

      // scan for missing aux-var
      for( size_t k=0 ; k < pIn->varSz ; ++k )
      {
        Variable& aux = pIn->variable[k] ;

        if( (aux.isCoordinate() && ! aux.coord.isCoordVar) || aux.isLabel )
        {
          int jx;

          if( aux.boundsOf.size() )
            continue;
          if( aux.isFormulaTermsVar )
            continue;
          else if( hdhC::isAmong(aux.name, ca_vvs[i]) )
            continue;
          else if( ! hdhC::isAmong(var.dimName[j], aux.dimName) )
            continue;
          else if( isXYZinCoordAtt(i) )
            // check also for an undocumented but commonly used rule
            continue;
          else if( (jx=var.getAttIndex(n_grid_mapping)) > -1 )
          {
            if( var.attValue[jx][0] == aux.name )
              continue;
          }

          if( ca_vvs[i].size() )
          {
            if( notes->inq(bKey + "5j", aux.name, NO_MT ) )
            {
              std::string capt(hdhC::tf_att(var.name,n_coordinates, hdhC::colon));
              capt += "Missing auxiliary name" ;
              capt += hdhC::tf_val(aux.name);

              (void) notes->operate(capt) ;
              notes->setCheckStatus( n_CF, fail );
            }
          }

          else
          {
            if( notes->inq(bKey + "5i", var.name, NO_MT ) )
            {
              std::string capt(hdhC::tf_var(var.name, hdhC::colon));
              capt += "Missing " ;
              capt += hdhC::tf_att(n_coordinates) ;

              (void) notes->operate(capt) ;
              notes->setCheckStatus( n_CF, fail );
            }
          }
        }
      }
    }
  }

  return;
}

void
CF::finalAtt_coordinates_C(void)
{
  // coordinate var must not have coordinates attribute

  for( size_t ix=0 ; ix < pIn->varSz ; ++ix )
  {
    Variable& var = pIn->variable[ix] ;

    if( ca_vvs[ix].size() && var.isCoordinate() )
    {
      if( notes->inq(bKey + "5b") )
      {
        std::string capt;
        if( !var.coord.isCoordVar )
            capt = "auxiliary ";

        capt += "coordinate " + hdhC::tf_var(var.name);
        capt += "should not have " + hdhC::tf_att(n_coordinates);

        (void) notes->operate(capt) ;
        notes->setCheckStatus( n_CF, fail );
      }
    }
  }

  return;
}

void
CF::finalAtt_coordinates_D(void)
{
  // find aux coords not declared in attribute 'coordinates'

  for( size_t ix=0 ; ix < pIn->varSz ; ++ix )
  {
     Variable& aux = pIn->variable[ix] ;

     if( aux.coord.isCoordVar || aux.isLabel )
         continue;

     if( ! aux.isCoordinate() )
         continue;

     if( aux.coord.isZ_DL || aux.isFormulaTermsVar)
         continue ;

     if( aux.boundsOf.size() )
         continue ;

     bool is=true;

     for( size_t jx=0 ; jx < pIn->varSz ; ++jx )
     {
        if( jx == ix )
            continue;

        Variable& var = pIn->variable[jx] ;
        if( ! var.isDataVar() )
            continue;

        std::vector<std::string>& ca_vs = ca_vvs[jx] ;

        if( ca_vs.size() && hdhC::isAmong(aux.name, ca_vs) )
        {
            is=false;
            break;
        }
     }

     if(is)
     {
        if( notes->inq(bKey + "5k", aux.name, NO_MT ) )
        {
            std::string capt("Warning: auxiliary " + hdhC::tf_var(aux.name));
            capt += " is not used in any coordinates attribute" ;

            (void) notes->operate(capt) ;
            notes->setCheckStatus( n_CF, fail );
        }
     }
  }

  return;
}

void
CF::finalAtt_positive(void)
{
   for( size_t ix=0 ; ix < pIn->varSz ; ++ix )
   {
     Variable& var = pIn->variable[ix] ;
     std::string positive;

     int j;
     if( (j=var.getAttIndex(n_positive)) > -1 )
        positive = hdhC::Upper()(var.attValue[j][0]);

     // check properties and validity
     if( var.coord.isC[2] && !var.coord.isZ_p && !var.coord.isZ_DL )
     {
       if( positive.size() )
       {
         bool is = positive == "UP" || positive == "DOWN" ;

         if( !is && notes->inq(bKey + "43a", var.name) )
         {
           std::string capt(hdhC::tf_att(var.name, n_positive));
           capt += "should be Up|Down";
           std::string text("Found ") ;
           text += hdhC::tf_val(var.attValue[j][0]) ;

           (void) notes->operate(capt, text) ;
           notes->setCheckStatus( n_CF, fail );
         }
       }
       else if( notes->inq(bKey + "43c", var.name) )
       {
         std::string capt(hdhC::tf_var(var.name, hdhC::colon));
         capt += hdhC::tf_att(hdhC::empty,n_positive, hdhC::upper);
         capt += "is required for a " + n_dimension ;
         capt += "al Z-coordinate with non-pressure " + n_units ;

         (void) notes->operate(capt) ;
         notes->setCheckStatus( n_CF, fail );
       }
     }

     // check the correct usage of the positive attribute
     if( var.isCoordinate() && !var.coord.isC[2] && var.isValidAtt(n_positive) )
     {
       if( notes->inq(bKey + "43b", var.name) )
       {
          std::string capt(hdhC::tf_att(var.name, n_positive));
          capt += "is only allowed for vertical " + n_coordinates ;

          (void) notes->operate(capt) ;
          notes->setCheckStatus( n_CF, fail );
       }
     }

   }

   return;
}

void
CF::finalAtt_units(void)
{
  for( size_t ix=0 ; ix < pIn->varSz ; ++ix )
  {
    Variable& var = pIn->variable[ix] ;
    std::string positive;

    if( var.coord.isC[2] && !var.coord.isZ_DL && !var.isValidAtt(n_units) )
    {
      // was is rated a dimless Z without formula_terms attribute?
      if( notes->inq(bKey + "43d", var.name) )
      {
        std::string capt(hdhC::tf_var(var.name));
        capt += "was rated a Z-coordinate, but ";
        capt += n_units + " are missing";

        (void) notes->operate(capt) ;
        notes->setCheckStatus( n_CF, fail );
      }
    }
  }

  return;
}

void
CF::final_dataVar(void)
{
  // ambiguous (weired) setting of attributes may result in a state
  // pretending that there is no data variable

  bool is=true;

  for( size_t i=0 ; i < pIn->varSz ; ++i )
  {
    if( pIn->variable[i].isDataVar() )
    {
      is=false;
      break;
    }
  }

  if( is && notes->inq(bKey + "0f") )
  {
    std::string capt("Warning: No data variable in the file" ) ;

    (void) notes->operate(capt) ;
    notes->setCheckStatus( n_CF, fail );
  }

  return;
}

int
CF::finally(int eCode)
{
//  notes->print();
  int ec= notes->getExitState();
  eCode = (eCode > ec ) ? eCode : ec ;
  return  eCode;
}

void
CF::findAmbiguousCoords(void)
{
   // test for variables with ambiguous coordinate info .
   int count[4];

   for( size_t i=0 ; i < pIn->varSz ; ++i )
   {
      Variable& var = pIn->variable[i] ;

      if( var.isDataVar() || var.isMapVar )
         continue;

      for(size_t c=0 ; c < 4 ; ++c )
        count[c] = var.coord.weight[c];

      int type_ix[] = {0,1,2,3} ;

      // sort
      for( size_t s0=0 ; s0 < 3 ; ++s0 )
      {
        for( size_t s1=s0+1 ; s1 < 4 ; ++s1 )
        {
           if( count[s0] < count[s1] )
           {
              int swap_count=count[s0];
              int swap_type=type_ix[s0];
              count[s0] = count[s1];
              type_ix[s0] = type_ix[s1];
              count[s1] = swap_count;
              type_ix[s1] = swap_type;
           }
        }
      }

      // analysis
      if( count[0] > 1 )
      {
         if( count[0] > count[1] )
         {
            for( int c=0 ; c < 4 ; ++c )
              if( type_ix[0] == c )
               var.coord.isC[c] = true;
         }
         else
         {
            if( notes->inq(bKey + "0i", var.name) )
            {
              std::string capt;
              if( !var.coord.isCoordVar )
                capt = "auxiliary ";
              capt += "coordinate ";
              capt += hdhC::tf_var(var.name, hdhC::colon) ;
              capt += "Ambiguous axis, could be <";
              capt += var.coord.cType[type_ix[0]] ;
              capt += "> or <";
              capt += var.coord.cType[type_ix[1]] ;
              capt += ">";

              (void) notes->operate(capt) ;
              notes->setCheckStatus( n_CF, fail );
            }
         }
      }
   }

   return;
}

void
CF::findCellMeasures(Variable& var)
{
   // find valid variable declared by a cell_measures attribute
   int j ;

   if( (j=var.getAttIndex(n_cell_measures)) > -1 )
   {
     // get a list of strings clear of :-tailed key-words
     std::vector<std::string> vs( getKeyWordArgs(var.attValue[j][0], "arg", ':') ) ;

     for( size_t v=0 ; v < vs.size() ; ++v )
     {
       for( size_t i = 0 ; i < pIn->varSz ; ++i )
       {
         Variable& vari = pIn->variable[i] ;

         if( vari.name == vs[v] )
         {
            var.push_aux(vari.name);

            var.addDataCount();
            vari.addAuxCount();
            return ;
         }
       }
     }
   }

   return ;
}

bool
CF::findLabel(Variable& var)
{
   if( var.isLabel )
     return var.isLabel;

   // evidences for a label:
   // a) variable is char type
   // b) the last dimension is used only once for all variables

   if( var.type != NC_CHAR || var.dimName.size() == 0 )
     return (var.isLabel=false);

   std::string& lastDimName = var.dimName[var.dimName.size()-1];

   for( size_t i=0 ; i < pIn->varSz ; ++i )
   {
     if( pIn->variable[i].name == var.name )
       continue;

     if( hdhC::isAmong(lastDimName, pIn->variable[i].dimName) )
       return (var.isLabel=false);
   }

   return (var.isLabel=true) ;
}

void
CF::getAssociatedGroups(void)
{
  bool isSD=true;

  for( size_t i=0 ; i < pIn->varSz ; ++i )
  {
    Variable& var = pIn->variable[i];

    int j;
    if( (j=var.getAttIndex(n_sample_dimension)) > -1 )
    {
      isSD=false;

      if( var.dimName.size() == 1 )
        associatedGroups.push_back(var.dimName[0]);
    }

    if( (j=var.getAttIndex(n_instance_dimension)) > -1 )
    {
      associatedGroups.push_back(var.attValue[j][0]);
    }
  }

  // unnamed, but effective sample_dimension, e.g. in featureType=point
  if(isSD)
  {
    // data vars must only depend on a single dim
    std::vector<std::string> vs_single;
    std::vector<size_t> vs_count;

    for( size_t i=0 ; i < pIn->varSz ; ++i )
    {
      Variable& var = pIn->variable[i];

      if( var.isDataVar() )
      {
        if( var.dimName.size() == 1 )
        {
          size_t vs_ix;
          if( hdhC::isAmong(var.dimName[0], vs_single, vs_ix) )
            ++vs_count[vs_ix];
          else
          {
            vs_single.push_back(var.dimName[0]) ;
            vs_count.push_back(1);
          }
        }
      }
    }

    for( size_t i=0 ; i < vs_single.size() ; ++i )
      if( vs_count[i] > 1 )
        associatedGroups.push_back(vs_single[i]);
  }

  return;
}

void
CF::getDims(void)
{
  if( dimensions.size() )
     return  ;

  dimensions = pIn->nc.getDimName() ; // all dimension names

  for( size_t k=0 ; k < dimensions.size() ; ++k )
  {
    size_t ix;
    for( ix=0 ; ix < pIn->varSz ; ++ix )
    {
      Variable& var = pIn->variable[ix] ;
      size_t sz = var.dimName.size();

      // exclude char-lengths
      if( findLabel(var) && sz )
        --sz;

      // exclude vertices of bounds vars
      if( var.boundsOf.size() && sz )
        --sz;

      if( sz )
      {
        size_t j;
        for(j=0 ; j < sz ; ++j)
        {
          if( var.dimName[j] == dimensions[k] )
          {
            effDims_ix.push_back(k);
            break;
          }
        }

        if( j < sz )
          break;
      }
    }
  }

  return  ;
}

std::vector<std::string>
CF::getKeyWordArgs(std::string& str, std::string mode, char tail)
{
   // mode:s
   //      "char"           all key-words tailed by "char"
   //      "arg" [default]  all words not a key:
   //      "1st"            first of each pair
   //      "2nd"            second of each pair

   // return corresponding list

   std::vector<std::string> vs;

   if( str.size() == 0 )
      return vs;

   bool isArg= ( mode == "arg" ) ? true : false;
   bool is1st= ( mode == "1st" ) ? true : false;
   bool is2nd= ( mode == "2nd" ) ? true : false;

   // syntax: 'key: arg'
   Split x_str(str);

   // check key-word
   for( size_t i=0 ; i < x_str.size() ; ++i )
   {
      if( isArg )
      {
         // e.g., "myKey:" where tail == ":"
         size_t last=x_str[i].size() - 1 ;

         if( x_str[i][last] != tail )
           vs.push_back( x_str[i] ) ;
      }
      else if( is1st && ! i%2 )
        vs.push_back( x_str[i] ) ;
      else if( is2nd && i%2 )
        vs.push_back( x_str[i] ) ;
      else
      {
        size_t last=x_str[i].size() - 1 ;

        if(x_str[i][last] == tail)
           vs.push_back( x_str[i] ) ;
      }
    }

    return vs;
}

void
CF::getSN_TableEntry(void)
{
   // http://cf-pcmdi.llnl.gov/documents/cf-standard-names/cf-standard-name-table.xml

   // index to standard_names
   std::vector<int> zx;

   for( size_t i=0 ; i < pIn->varSz ; ++i )
   {
      Variable& var = pIn->variable[i] ;
      var.snTableEntry.found=false;
      var.snTableEntry.hasBlanks=false;

      int j;
      if( (j=var.getAttIndex(n_standard_name)) > -1 )
      {
        var.std_name = var.attValue[j][0] ;
        var.snTableEntry.std_name=var.std_name;  // initially, just for a test
        zx.push_back(i) ;
      }
      else if( (j=var.getAttIndex(n_long_name)) > -1 )   // try long_name
      {
        // however, long-name should have no blanks
        var.snTableEntry.std_name = var.attValue[j][0];
        if( var.snTableEntry.std_name.find(" ") == std::string::npos )
          zx.push_back(i) ;
      }

      if( (j=var.getAttIndex(n_units)) > -1 )
        var.units = var.attValue[j][0] ;
   }

   // search the cf-standard-name-table.xml;
   // for efficiency: sort standard_names: if these are all CF conform, then
   // the table will be read only once.
   size_t v_swap;
   int sz=static_cast<int>(zx.size());

   for( int i=0 ; i < sz ; ++i )
   {
     for( int j=i+1 ; j < sz-1 ; ++j )
     {
        std::string &var_i = pIn->variable[ zx[i] ].snTableEntry.std_name;
        std::string &var_j = pIn->variable[ zx[j] ].snTableEntry.std_name;

        if( var_j <  var_i )
        {
          v_swap = zx[i] ;
          zx[i] = zx[j];
          zx[j] = v_swap;
        }
     }
   }

   // note in the structure whether a name was found and the standard name,
   // moving any modifier into the struct member remainder.
   // Also works for blank-separated (instead of '_') standard names.
   scanStdNameTable(zx) ;

   return;
}

void
CF::getVarStateEvidences(Variable& var)
{
  // bounds and boundsOf are always auxiliaries; addAuxCount is done inside
  (void) isBounds(var) ;

  // look for attributes inidicating whether data or aux var
  int j;

  if( (j=var.getAttIndex(n_ancillary_variables)) > -1 )
  {
    var.addDataCount() ;

    // the named variable is also type data var
    int j2;
    if( (j2=pIn->getVarIndex(var.attValue[j][0])) > -1 )
      pIn->variable[j2].addDataCount() ;
  }

  if( var.isValidAtt(n_cell_measures) )
    var.addDataCount() ;
  if( var.isValidAtt(n_cell_methods) )
    var.addDataCount() ;
  if( var.isValidAtt(n_coordinates) )
    var.addDataCount() ;
  if( var.isValidAtt(n_FillValue) || var.isValidAtt(n_missing_value))
    var.addDataCount() ;
  if( var.isValidAtt(n_flag_values) )
    var.addDataCount() ;
  if( var.isValidAtt(n_flag_masks) )
    var.addDataCount() ;
  if( var.isValidAtt(n_grid_mapping) )
    var.addDataCount() ;

  if( var.dimName.size() > 2 )
    var.addDataCount();

  if( var.isValidAtt(n_axis) )
    var.addAuxCount() ;
  if( var.isValidAtt(n_cf_role) )
    var.addAuxCount() ;
  if( var.isValidAtt(n_climatology) )
    var.addAuxCount() ;
  if( var.isValidAtt(n_instance_dimension) )
    var.addAuxCount() ;
  if( var.isValidAtt(n_sample_dimension) )
    var.addAuxCount() ;
  if( var.isValidAtt(n_positive) )
    var.addAuxCount() ;

  // dims of a varRep form a sub-set of the corresponding data variable
  size_t sz;
  if( (sz=var.dimName.size()) )
  {
    for( size_t i=0 ; i < pIn->varSz ; ++i )
    {
      Variable& vari = pIn->variable[i] ;

      // a sub-set of dims?
      if( sz < vari.dimName.size() )
      {
        if( hdhC::isAmong(var.dimName, vari.dimName, true) )
        {
          var.addAuxCount();
          break;
        }
      }
    }
  }

  // support point for aux: var is named by coordinates attributes
  if( hdhC::isAmong(var.name, ca_vvs) )
    var.addAuxCount();

  return;
}

void
CF::hasBounds(Variable& var)
{
  // this checks whether a variable declared by a bounds or climatology attribute
  // is supplied.

  int j;
  std::string* type[2];
  type[0] = &n_bounds;
  type[1] = &n_climatology;

  for( size_t i=0 ; i < 2 ; ++i )
  {
    if( (j=var.getAttIndex( *type[i]) ) > -1 )
    {
       int i2;
       if( (i2=pIn->getVarIndex( var.attValue[j][0] )) == -1 )
       {
         if( notes->inq(bKey + "71g") )
         {
           std::string capt(hdhC::tf_att(var.name, *type[i]) ) ;
           capt += "declares non-existing " ;
           if(i)
              capt += n_climatology + hdhC::blank;
           else
              capt += "boundary " ;
           capt += hdhC::tf_var(var.attValue[j][0]);

           (void) notes->operate(capt) ;
           notes->setCheckStatus( n_CF, fail );
         }
       }
       else
       {
         pIn->variable[i2].addAuxCount();
         var.push_aux(var.attValue[j][0]);
       }
    }
  }

  return  ;
}

bool
CF::isBounds(Variable& var)
{
  // return true when var is bounds or climatology variable
  // declared by another variable.

  // known by a previous call
  if( var.boundsOf.size() )
    return true;

  std::string* str[2];
  str[0] = &n_bounds;
  str[1] = &n_climatology;
  std::string t;

  bool is=false;

  for( size_t ix=0 ; ix < pIn->varSz ; ++ix )
  {
    Variable& var2 = pIn->variable[ix] ;

    if( var2.name == var.name )
       continue;

    for( size_t n=0 ; n < 2 ; ++n )
    {
      // the value of the attribute
      t = var2.getAttValue(*(str[n])) ;

      if( t == var.name )
      {
        // test whether var2 is bounds and has bounds
        if( var.isValidAtt(*(str[n])) )
        {
          if( notes->inq(bKey + "71i", var2.name) )
          {
            std::string capt(hdhC::tf_var(var.name)) ;
            capt += "should not be and have " ;
            capt += *(str[n]) + " at the same time" ;

            (void) notes->operate(capt) ;
            notes->setCheckStatus( n_CF, fail );
          }
        }

        if( n == 1 )
           var.isClimatology=true;

        var.boundsOf = var2.name ;
        var2.bounds = var.name ;

        var.addAuxCount() ;
        var2.addAuxCount() ;

        is=true;
      }
    }
  }

  return is ;
}

bool
CF::init()
{
  notes->setFilename(fSplit);

  notes->init();  // safe

  applyOptions();

  return true;
}

void
CF::initDefaults()
{
  setObjName("CF");

  notes=0;
  cF=0;
  pIn=0;
  fDI=0;
  pOper=0;
  pOut=0;
  qA=0;
  tC=0;

  cFVal=14;  // the default for checking
  followRecommendations=false;
  isFeatureType = false;
  isCF14_timeSeries=false;
  isCheck=false;

  time_ix=-1;
  compress_ix=-1;

  bKey="CF_";
  fail="FAIL";

  n_ancillary_variables="ancillary_variables";
  n_area="area";
  n_attribute="attribute";
  n_axis="axis";
  n_bounds="bounds";
  n_calendar="calendar";
  n_cell_measures="cell_measures";
  n_cell_methods="cell_methods";
  n_CF="CF";
  n_cf_role="cf_role";
  n_climatology="climatology";
  n_compress="compress";
  n_Conventions="Conventions";
  n_coordinates="coordinates";
  n_dimension="dimension";
  n_featureType="featureType";
  n_FillValue="_FillValue";
  n_flag_masks="flag_masks";
  n_flag_values="flag_values";
  n_formula_terms="formula_terms";
  n_global="global";
  n_grid_mapping="grid_mapping";
  n_instance_dimension="instance_dimension";
  n_long_name="long_name";
  n_latitude="latitude";
  n_leap_month="leap_month";
  n_leap_year="leap_year";
  n_longitude="longitude";
  n_missing_value="missing_value";
  n_month_lengths="month_lengths";
  n_NC_GLOBAL="NC_GLOBAL";
  n_number_of_observations="number_of_observations";
  n_positive="positive";
  n_reco="CF Recommendation";
  n_sample_dimension="sample_dimension";
  n_standard_name="standard_name";
  n_time="time";
  n_units="units";
  n_valid_min="valid_min";
  n_valid_max="valid_max";
  n_valid_range="valid_range";
  n_variable="variable";

  n_grid_latitude="grid_" + n_latitude;
  n_grid_longitude="grid_" + n_longitude;

  NO_MT="NO_MT";

  ut_set_error_message_handler( ut_ignore );
  unitSystem = ut_read_xml(NULL);
}

bool
CF::isChap9_specialLabel(Variable& label, Variable& dv)
{
   if( cFVal < 16 || label.isScalar )
     return false;

   if( hdhC::isAmong(label.dimName[0], associatedGroups) )
     return true;

/*
   // a label may delegate its "dimensionality" to an index variable,
   // indicated by attribute sample_dimension=label_name
   for( size_t i=0 ; i < pIn->varSz ; ++i )
   {
      Variable& var = pIn->variable[i] ;

      if( var.name == label.name )
        continue;

      // condition 1
      int j;
      if( (j=var.getAttIndex(n_sample_dimension)) > -1 )
      {
         // char  label(label_dim)
         // int   var(label_dim)
         //         var:sample_dimension="dim"
         // float data_var(...,dim, ...)
         if( var.dimName.size() == 1 && var.dimName[0] == label.dimName[0] )
         {
            // the sample_dimension attribute names the dimension,
            // which must be shared by dv
            if( hdhC::isAmong(var.attValue[j][0], dv.dimName) )
               return true;
         }
      }

      // condition 2
      else if( (j=var.getAttIndex(n_instance_dimension)) > -1 )
      {
         // char  label(label_dim,n)
         // int   var(dim)
         //         var:instance_dimension="label_dim"
         // float data_var(...,dim, ...)
         if( var.attValue[j][0] == label.dimName[0] )
         {
            // the dim of the variable with sample_dimension must be shared by dv
            if( var.dimName.size() == 1
                  && hdhC::isAmong(var.dimName[0], dv.dimName) )
                return true;
         }
      }
   }
*/
   return false;
}

bool
CF::isCompressAux(Variable& var)
{
  std::string vs_cmpr;
  int j;
  for(size_t i=0 ; i < pIn->varSz ; ++i )
  {
    if( var.name == pIn->variable[i].name )
      continue;

    if( (j=pIn->variable[i].getAttIndex(n_compress)) > -1 )
      vs_cmpr += pIn->variable[i].attValue[j][0] + hdhC::blank;
  }

  if( vs_cmpr.size() )
  {
    Split x_vs_cmpr(vs_cmpr);
    for( size_t i=0 ; i < x_vs_cmpr.size() ; ++i )
      if( x_vs_cmpr[i] == var.name )
        return true;
  }

  return false;
}

bool
CF::isCompressEvidence(Variable& var, bool* isIntType)
{
   // evidence of a compress index variable is given when
   // 1) the type of the variable is int-alike
   // 2) coordinate vars lon and lat are not referenced any further
   // 3) no attributes: axis, positive, units, standard_name,
   //    flag_values, flag_masks, _FillValue
   // 4) lon and lat must be coordinate variables

   // 1)
   if( (*isIntType=pIn->nc.isIndexType(var.name)) )
   {
     std::vector<std::string> vs_notAnAtt;
     vs_notAnAtt.push_back(n_units);
     vs_notAnAtt.push_back(n_standard_name);
     vs_notAnAtt.push_back(n_FillValue);
     vs_notAnAtt.push_back(n_axis);
     vs_notAnAtt.push_back(n_positive);
     vs_notAnAtt.push_back(n_flag_values);
     vs_notAnAtt.push_back(n_flag_masks);

     // 3)
     for( size_t i=0 ; i < vs_notAnAtt.size() ; ++i )
        if( var.isValidAtt( vs_notAnAtt[i] ) )
           return false;
   }
   else
     return false;

   size_t count=0;
   for( size_t i=0 ; i < pIn->varSz ; ++i )
   {
     Variable& coordVar = pIn->variable[i];

     if( coordVar.coord.isCoordVar
           && (coordVar.coord.isC[0] || coordVar.coord.isC[1]) )
     {
       ++count;

       // 2)
       for( size_t j=0 ; j < pIn->varSz ; ++j )
       {
          if( i == j )
             continue;

          Variable& rVar = pIn->variable[j] ;

          if( hdhC::isAmong(coordVar.name, rVar.dimName) )
            return false;
       }
     }
   }

   // 4
   if( count != 2 )
      return false;

   return true; // looks like a compress index variable
}

bool
CF::isLatitude(void)
{
   for( size_t i=0 ; i < pIn->varSz ; ++i )
     if( pIn->variable[i].coord.isC[1] )
       return true;

   return false;
}

bool
CF::isLongitude(void)
{
   for( size_t i=0 ; i < pIn->varSz ; ++i )
     if( pIn->variable[i].coord.isC[0] )
       return true;

   return false;
}

bool
CF::isXYZinCoordAtt(size_t ix)
{
  // not CF, but common practice.

  // If all spatial coordinates are specified
  // by the coordinates attribute of var, then a label may be
  // omitted in the coordinates attribute. Then, return true:

  if( ca_vvs[ix].size() )
  {
    Variable& var = pIn->variable[ix];

    // Note: time may be omitted, when there is no time variable
    bool isTime=false;
    for( size_t i=0 ; i < pIn->varSz ; ++i )
    {
      if( pIn->variable[i].coord.isC[3] )
      {
        isTime=true;
        if( hdhC::isAmong(pIn->variable[i].name, var.dimName) )
          isTime=false;

        break;
      }
    }

    std::vector<std::string>& ca_vs = ca_vvs[ix];

    bool is[] = {false, false, false, false};

    for(size_t k=0 ; k < ca_vs.size() ; ++k )
    {
      int j;
      if( (j=pIn->getVarIndex(ca_vs[k])) > -1 )
      {
        for( size_t c=0 ; c < 4 ; ++c )
          if( pIn->variable[j].coord.isC[c] )
            is[c] = true;
      }
    }

    if( is[0] && is[1] && is[2] )
    {
      // is var depending on time?
      if( isTime && !is[3] )
        return false;

      return true;
    }
  }

  return false;
}

void
CF::linkObject(IObj *p)
{
  std::string className = p->getObjName();

  if( className == "X" )
    notes = dynamic_cast<Annotation*>(p) ;
  else if( className ==  "CF" )
    cF = dynamic_cast<CF*>(p) ;

#ifndef CF_MACRO
  else if( className == "QA" )
    qA = dynamic_cast<QA*>(p) ;
#endif

  else if( className ==  "IN" )
    pIn = dynamic_cast<InFile*>(p) ;

  return;
}

bool
CF::parseUnits( std::string s)
{
   ut_unit* u = ut_parse(unitSystem, s.c_str(), UT_ASCII);

   return u ? true : false ;
}

void
CF::postAnnotations(void)
{
  std::vector<std::vector<std::string> > tag;
  std::vector<std::string> newTag;
  std::vector<std::string> capt;

  // definition of tasks; there are different modes:
  // a) replace all tags by a new one;
  //    a newtag is defined by newTag.size() > 0
  // b) erase all tags, but the first one when newTag is empty.

  tag.push_back(std::vector<std::string>()) ;
  tag.back().push_back(bKey + "0f");
  tag.back().push_back(bKey + "0c");
  newTag.push_back(hdhC::empty);
  capt.push_back(hdhC::empty);

  tag.push_back(std::vector<std::string>()) ;
  tag.back().push_back(bKey + "432b");
  tag.back().push_back(bKey + "432d");
  newTag.push_back(bKey + "432j");
  capt.push_back("Suspecting a misplaced formula_terms attribute");

  tag.push_back(std::vector<std::string>()) ;
  tag.back().push_back(bKey + "26a");
  tag.back().push_back(bKey + "81e");
  newTag.push_back("NC_3_12");
  capt.push_back("NC_3_12");

  tag.push_back(std::vector<std::string>()) ;
  tag.back().push_back(bKey + "432b");
  tag.back().push_back(bKey + "33e");
  newTag.push_back(hdhC::empty);
  capt.push_back(hdhC::empty);

  tag.push_back(std::vector<std::string>()) ;
  tag.back().push_back(bKey + "5i");
  tag.back().push_back(bKey + "0c");
  newTag.push_back(hdhC::empty);
  capt.push_back(hdhC::empty);

  tag.push_back(std::vector<std::string>()) ;
  tag.back().push_back(bKey + "5j");
  tag.back().push_back(bKey + "0c");
  newTag.push_back(hdhC::empty);
  capt.push_back(hdhC::empty);

  tag.push_back(std::vector<std::string>()) ;
  tag.back().push_back(bKey + "56a");
  tag.back().push_back(bKey + "5c");
  newTag.push_back(hdhC::empty);
  capt.push_back(hdhC::empty);

  tag.push_back(std::vector<std::string>()) ;
  tag.back().push_back(bKey + "74e");
  tag.back().push_back(bKey + "73e");
  newTag.push_back(hdhC::empty);
  capt.push_back(hdhC::empty);

  for( size_t t=0 ; t < tag.size() ; ++t )
  {
    // include | exclude the first tag
    int ntIx=newTag[t].size() ? 1 : 0 ;

    // collect items
    std::vector<std::string> vs_annot;
    std::vector<std::string> vs_item;

    for( size_t i=0 ; i < tag[t].size() ; ++i )
      // same tag for various variables possible
      vs_item = notes->getAnnotation(tag[t][i]);

    if( !vs_item.size() )
      continue;

    if( !ntIx && !hdhC::isAmong(tag[t][0], vs_item, "beg") )
      continue;

    // extract variable names; multiples are possible
    size_t pos;
    std::string spec0;
    std::string spec;

    for( size_t i=0 ; i < vs_annot.size() ; ++i )
    {
      if( (pos=vs_item[i].rfind('_')) < std::string::npos )
      {
        if( vs_annot[i].size() > pos+1 )
          spec = vs_annot[i].substr(pos+1) ;

        if(i)
        {
          if( spec0 == spec )
            vs_annot.push_back(vs_annot[i].substr(0,pos));
        }
        else
          spec0 = spec;
      }
    }

    for( size_t i=0 ; i < vs_annot.size() ; ++i )
    {
      for( size_t j=ntIx ; j < tag[t].size() ; ++j )
      {
        if( tag[t][j] == vs_annot[i] )
        {
          notes->eraseAnnotation(vs_annot[i], spec);
          vs_annot[i].clear();
          break;
        }
      }

      if( !i && newTag[t].size() && notes->inq(newTag[t], spec)  )
      {
        (void) notes->operate(capt[t]) ;
        notes->setCheckStatus( n_CF, fail );
      }
    }
  }

  return;
}

bool
CF::run()
{
   init();

   getDims() ;


   for( size_t i=0 ; i < pIn->varSz ; ++i )
   {
      Variable& var = pIn->variable[i] ;

      if( var.name == dataVarName )
         var.addDataCount(10);

      getVarStateEvidences(var);

      if( var.isValidAtt(n_compress) )
      {
        compress_ix=var.getVarIndex();
        var.isCompress=true;
      }

      // find coordinate variables
      chap51(var);

      if( findLabel(var) )
         var.addAuxCount();

      findCellMeasures(var) ;
   }

   // get index and contents of coordinates attributes
   int sz=pIn->varSz;
   for( int i=0 ; i < sz ; ++i )
   {
      Variable& var = pIn->variable[i];
      vIx[ var.name ] = i ;
      ca_vvs.push_back( std::vector<std::string>() );

      int j;
      if( (j=var.getAttIndex(n_coordinates)) > -1 )
      {
        ca_pij.push_back( std::pair<int, int> (i,j) );
        Split x_ca(var.attValue[j][0]);
        ca_vvs.back() = x_ca.getItems() ;
      }
   }

   getSN_TableEntry();  //read table and set also variables std_name and units.

   getAssociatedGroups();

   // spell-checking of attribute names provided in the file vs.
   // the CF specific ones.
   attributeSpellCheck();

   // which CF Convention?
   if( ! chap261() )
     return false;  // undefined Convention is specified

   // dimensionless vertical coordinate? Could set var.isChecked=true
   chap432() ;  // preponed to make processing easier

   // a) Indexes to variables and attributes.
   // b) It is difficult to find faults in CF-1.4 time-series files
   //    for a z-coordinate. So, detection of a time-series file helps.
   std::string firstDim;

   for( size_t i=0 ; i < pIn->varSz ; ++i )
   {
     Variable& var = pIn->variable[i];

     // some checks are brought forward for convenience
     if( var.dimName.size() == 0
           || (var.type == NC_CHAR && var.dimName.size() == 1 ) )
       var.isScalar = true;

     // find auxiliaries of any kind. Only a start, further
     // pushes may come later.
     for( size_t j=0 ; j < var.dimName.size() ; ++j )
     {
       int k;
       if( (k=pIn->getVarIndex(var.dimName[j])) > -1 )
       {
         pIn->variable[k].addAuxCount();
         var.push_aux(var.dimName[j]);
       }
     }

     // find old-fashion type of time-series, i.e. all variables
     // depend on the same single dimension
     if( i == 0 && var.dimName.size() == 1 )
       firstDim=var.dimName[0] ;

     if( firstDim.size() )
     {
       if( var.dimName.size() == 0 )
         firstDim.clear();
       else if( var.dimName.size() > 1 )
         firstDim.clear();
       else if( var.dimName[0] != firstDim )
         firstDim.clear();
     }
   }

   if( firstDim.size() )
     isCF14_timeSeries = true;

   return entry();
}

bool
CF::scanStdNameTable(std::vector<int> &zx)
{
  // zx: vector of indices to variables with standard_name and
  // sorted according to the standard_names

  if( zx.size() == 0 )  // no standard name
     return false;

  if( !std_name_table.is() )  // name of the table
     return false;

  ReadLine ifs( std_name_table.getFile() ) ;

  if( !ifs.isOpen() )
  {
    if( notes->inq(bKey + "0t") )
    {
      std::string capt("fatal: the CF-standard-name table could not be opened");

      (void) notes->operate(capt) ;
      notes->setCheckStatus( n_CF, fail );
    }

    return false;
  }

  ifs.clearSurroundingSpaces();

  std::string testName[zx.size()];
  for( size_t i=0 ; i < zx.size() ; ++i )
  {
     Variable& var = pIn->variable[zx[i]];

     // remember the original test name
     testName[i] = var.snTableEntry.std_name;

     bool isCont=false;

     // any identical standard_names in the race?
     for( size_t k=0 ; k < i ; ++k )
     {
       Variable& var_k = pIn->variable[zx[k]];

       if( var.snTableEntry.std_name == testName[k] )
       {
         var.snTableEntry.std_name  = var_k.snTableEntry.std_name ;
         var.snTableEntry.found     = var_k.snTableEntry.found ;
         var.snTableEntry.alias     = var_k.snTableEntry.alias;
         var.snTableEntry.amip      = var_k.snTableEntry.amip;
         var.snTableEntry.grib      = var_k.snTableEntry.grib;
         var.snTableEntry.remainder = var_k.snTableEntry.remainder ;
         var.snTableEntry.canonical_units = var_k.snTableEntry.canonical_units;
         isCont=true;
         break;
       }
     }

     if( isCont )
       continue;

     Split x_sn_name(testName[i]) ;

     if( x_sn_name.size() == 1 )
       scanStdNameTable(ifs, var, testName[i]) ;
     else
     {
       // try successively the blank separated sub-strings;
       // start with the longest string possible
       int j;
       std::string s;
       for( j=static_cast<int>(x_sn_name.size()-1) ; j > -1 ; --j )
       {
         s = x_sn_name[0];
         for( int l=1 ; l <= j ; ++l )
           s += "_" + x_sn_name[l] ;

         scanStdNameTable(ifs, var, s) ;

         if( var.snTableEntry.found )
         {
            if( testName[i].size() > s.size() )
              s = testName[i].substr(s.size()) ;
            var.snTableEntry.remainder = hdhC::stripSides(s) ;

            // if sn is right, but there is something with the modifier
            if( x_sn_name[0] != var.snTableEntry.std_name )
              var.snTableEntry.hasBlanks = true ;

            break;
         }
       }
     }
  }

  ifs.close();

  return true;
}

bool
CF::scanStdNameTable(ReadLine& ifs, Variable& var, std::string testName)
{
  // Find entries, aliases and units.
  // Units are only avaialble for entries.

  // Table: a block of entries comes first followed by a block of alias entries.
  //        alphabetically ordered.
  // Provided standard_name structs are ordered to ensure to minimise rewindings,
  // therefore the intricate algorithm structure.

  std::string beg_entry("entry id=");
  std::string end_entry("/entry");

  std::string beg_alias("alias id=");
  std::string end_alias("/alias");

  std::string n_grib("grib");
  std::string n_amip("amip");
  std::string canonical_units("canonical_units");

  Split x_line;
  x_line.setSeparator("<>\"", true);

  std::string line;
  int countRewinds=0;

  while ( true )
  {
    while( ! ifs.getLine(line) )
    {
      // look for standard name entries
      x_line = line ;

      if( x_line[1] == testName )
      {
        if( x_line[0] == beg_entry)
        {
           var.snTableEntry.std_name = testName ;
           var.snTableEntry.found=true;

           // exploit entry block
           while( ! ifs.getLine(line) )
           {
              if( line.find(end_entry) < std::string::npos )
                break;

              x_line = line ;
              if( x_line.size() > 1 )
              {
                if( x_line[0] == canonical_units )
                  var.snTableEntry.canonical_units = x_line[1] ;
                else if( x_line[0] == n_grib )
                  var.snTableEntry.grib = x_line[1] ;
                else if( x_line[0] == n_amip )
                  var.snTableEntry.amip = x_line[1] ;
              }
           }

           return true;
        }
        else if( line.find(beg_alias) < std::string::npos )
        {
           // try again later with the real entry
           var.snTableEntry.alias=testName;
           --countRewinds;

           if( ifs.getLine(line) )  // only the next line
              break; // should not happen

           x_line = line ;
           if( x_line.size() > 1 )
             testName=x_line[1] ;
        }
      }
    }

    // standard_name not in the table
    ifs.rewind();

    if( ++countRewinds == 2 )
       break;
  }

  return false;
}

bool
CF::setCheck(std::string &s)
{
   isCheck =true;

   // note: cFVal=14 by default

   if( s.find("1.4") < std::string::npos )
     ; // cFVal=14 by default
   else if( s.find("1.5") < std::string::npos )
     cFVal=15;
   else if( s.find("1.6") < std::string::npos )
     cFVal=16;
   else if( s.find("1.7") < std::string::npos )
	   cFVal=17;
   else if( s.find("1.0") < std::string::npos
         || s.find("1.1") < std::string::npos
           || s.find("1.2") < std::string::npos
             || s.find("1.3") < std::string::npos )
   {
     if( notes->inq(bKey + "261c") )
     {
        std::string capt(s);
        capt += " is not implemented, using CF-1.4 for the check";

        (void) notes->operate(capt) ;
        notes->setCheckStatus( n_CF, fail );
     }
   }
   else
   {
     std::string tag(bKey+"261b");
     if( notes->inq(tag) )
     {
       std::string capt("unknown Conventions=" + s);
       capt += ", restart with providing a valid convention, e.g. '-C CF-1.4'";

       (void) notes->operate(capt) ;
       notes->setCheckStatus( n_CF, fail );
     }

     isCheck=false;
     return isCheck;  // leads to exit
   }

   return true;
}

void
CF::setCheckStatus(std::string s)
{
  if(notes)
    notes->setCheckStatus( n_CF,s);

  return;
}

bool
CF::timeUnitsFormat(Variable& var, bool isAnnot)
{
  // true: is a time units format; perhaps wrong

  if( !isCheck )
    return false;

  if( var.isValidAtt(n_cf_role) )
    return false;

  std::string units( var.getAttValue(n_units));

  if( isAnnot && var.coord.isCoordVar && units.size() == 0 )
    return false;

/*
  //  "days since 1990-1-1 0-0-0" passes ut_parse()

  ut_unit* p = ut_parse(unitSystem, units.c_str(), UT_ASCII);

  if( p )
  {
    ut_free(p);  // valid format
    return true;
  }
*/

  // replace the T-separator of dateTtime by a blank; doesn't matter if
  // other Ts are replaced, too, because then units wouldn't be of time
  size_t pos=0;
  while( (pos=units.find('T',pos)) < std::string::npos )
  {
    if( hdhC::isDigit(units[pos-1]) )
    {
      if( pos < units.size() && hdhC::isDigit(units[pos+1]))
        units[pos]=' ';
    }

    ++pos;
  }

  Split x_units(units);

  size_t sz ;
  if( (sz=x_units.size()) < 2 )
    return false;  // to short; even for invalid time units

  //is any time-zone directly appended to time?
  size_t last = x_units.size() - 1;
  for( size_t i=0 ; i < x_units[last].size() ; ++i )
  {
    if( hdhC::isAlpha(x_units[last][i]) )
    {
      if( i && hdhC::isDigit(x_units[last][i-1]) )
      {
        // could be an appended TZ, if the string were not totally wrong
        std::string t0(x_units[last].substr(i));
        x_units[last] = x_units[last].substr(0,i);
        x_units.append(t0);  // a trailung tz was found and is now a separate item
      }

      break;
    }
  }

  // check the value of units
  bool isInvalidFreq=true;
  bool isInvalidKey=true;
  bool isInvalidDate=true;
  bool isInvalidTime=false;  // could be omitted
  bool isInvalidTZ=false;    // could be omitted

  bool tryFreq=true;
  bool tryKey=true;
  bool tryDate=true;
  bool tryTime=true;
  bool tryTZ=true;

  int seq[] = { 5, 5, 5, 5, 5};  // sequence of recognised units items

  std::vector<std::string> capt;
  std::vector<std::string> text;
  size_t prevSz;

  for( size_t itm=0 ; itm < x_units.size() ; ++itm )
  {
    prevSz=text.size();

    if( tryFreq )
    {
      if( timeUnitsFormat_frq(x_units[itm], capt, text) )
      {
        tryFreq=false;
        seq[itm]=itm;

        if( prevSz == text.size() )
          isInvalidFreq=false;
      }
    }
    else if( tryKey )
    {
      if( timeUnitsFormat_key(x_units[itm], capt, text) )
      {
        seq[itm]=itm;

        if( prevSz == text.size() )
          isInvalidKey=false;
      }
      tryKey=false;
    }
    else if( tryDate )
    {
      if( timeUnitsFormat_date(var, x_units[itm], capt, text, isAnnot) )
      {
        seq[itm]=itm;

        if( prevSz == text.size() )
          isInvalidDate=false;
      }
      tryDate=false;
    }
    else if( tryTime )
    {
      if( timeUnitsFormat_time(x_units[itm], capt, text) )
      {
        seq[itm]=itm;

        if( prevSz < text.size() )
          isInvalidTime=true;
      }
      tryTime=false;
    }
    else if( tryTZ )
    {
      if( timeUnitsFormat_TZ(x_units[itm], capt, text) )
      {
        seq[itm]=itm;

        if( prevSz < text.size() )
          isInvalidTZ=true;
      }
      tryTZ=false;
    }
  }

  bool isInvalid = isInvalidFreq || isInvalidKey || isInvalidDate
                   || isInvalidTime || isInvalidTZ ;

  // annotation section
  bool is=true;
  if( ! isInvalid )
  {
    for( size_t i=1 ; i < 5 ; ++i )
    {
      if( is && seq[i-1] > seq[i] )
      {
        is=false;
        text.push_back("Fault in the sequence of units items");
        break;
      }
    }

    if(is)
      return true;
  }
  // too many failures: probably not a time unit
  size_t countInvalid=0;
  size_t countValid=0;

  if( isInvalidFreq )
    ++countInvalid;
  else
    ++countValid;

  if( isInvalidKey )
    ++countInvalid;
  else
    ++countValid;

  if( isInvalidDate )
    ++countInvalid;
  else
    ++countValid;

  if( isInvalidTime )
    ++countInvalid;

  if( isInvalidTZ )
    ++countInvalid;

  if( countInvalid && countValid > countInvalid && isAnnot )
  {
      std::string capt0(hdhC::tf_att(var.name, n_units, hdhC::colon)) ;

      for( size_t i=0 ; i < text.size() ; ++i )
      {
        if( notes->inq(bKey + "44b", var.name) )
        {
          (void) notes->operate(capt0+capt[i], text[i]) ;
          notes->setCheckStatus( n_CF, fail );
        }
      }

    return true;
  }

  return false; //units? probably not
}

bool
CF::timeUnitsFormat_frq(std::string item,
                        std::vector<std::string>& capt,
                        std::vector<std::string>& text )
{
  std::vector<std::string> period;
  period.push_back("year");
  period.push_back("month");
  period.push_back("day");
  period.push_back("hour");
  period.push_back("minute");
  period.push_back("second");
  period.push_back("d");
  period.push_back("hr");
  period.push_back("h");
  period.push_back("min");
  period.push_back("sec");

  int last=item.size()-1;

  if(last > -1)
  {
    // both singular and plural are accepted
    if( item[last] == 's' )
      item = item.substr(0,last) ;
  }

  if( hdhC::isAmong(item, period) )
    return true;  // ok

  capt.push_back("Missing or wrong frequency") ;
  text.push_back("Found in ") ;

  return false ;
}

bool
CF::timeUnitsFormat_key(std::string item,
                        std::vector<std::string>& capt,
                        std::vector<std::string>& text )
{
  if( item == "since" )
    return true;  // ok

  if( item == "before" )
  {
    capt.push_back("Wrong key-word" + hdhC::tf_val(item)) ;
    text.push_back("Found in ");
    return true;
  }

  return false;
}

bool
CF::timeUnitsFormat_date(Variable& var, std::string item,
                        std::vector<std::string>& capt,
                        std::vector<std::string>& text, bool isAnnot)
{
  // true: is a date, perhaps faulty

  size_t countYes=0;
  size_t countNo=0;
  bool isBC=false;
  bool isColon=false;

  if( item[0] == '-' )
  {
    item=item.substr(1);
    ++countYes;
    isBC=true;
  }

  if( item.find(':') < std::string::npos )
  {
    isColon=true;
    ++countNo;
  }

  Split x_item(item, "-:", true);
  size_t sz = x_item.size();

  if( sz == 3 )
    ++countYes;
  else
    ++countNo;

  size_t countItems=0;
  bool isInvalidValue=false;

  for( size_t j=0 ; j < x_item.size() ; ++j )
  {
    if( hdhC::isDigit(x_item[j]) )
    {
        int num = hdhC::string2Double(x_item[j]);

        if( j == 0 ) // could be any year
          ++countItems;
        else if( j == 1  && num < 13 )
          ++countItems;
        else if( j == 2 && num < 32 )
          ++countItems;
        else
          ++countNo;

        if( j && num == 0 )
            isInvalidValue=true;
    }
    else
      ++countNo;
  }

  if( isInvalidValue )
  {
    capt.push_back("Invalid date, expected yyyy-mm-dd, found ") ;
    capt.back() += hdhC::tf_val(item);
    text.push_back("");
    return false;
  }

  // decision whether a date or not
  if( countNo >= (countYes+countItems) )
    return false;

  // a colon is never valid
  if(isColon)
  {
    capt.push_back("The date item should be separated by");
    capt.back() += hdhC::tf_val("-");
    capt.back() += "Found " ;
    capt.back() += hdhC::tf_val(":");
    text.push_back("");
  }

  if( countItems == 3 && !isBC )
  {
    if( isAnnot && followRecommendations
        && (x_item[0] == "0" || x_item[0] == "00")
            && (x_item[1] == "1" || x_item[1] == "01")
              && (x_item[2] == "1" || x_item[2] == "01") )
      chap44a_reco(var);

    return true;  // ok
  }

  if( countItems < 3 )
  {
    capt.push_back("Invalid date, expected yyyy-mm-dd, found ") ;
    capt.back() += hdhC::tf_val(item);
    text.push_back("");
  }

  return true;
}

bool
CF::timeUnitsFormat_time(std::string item,
                        std::vector<std::string>& capt,
                        std::vector<std::string>& text )
{
  // true: is a time item, perhaps faulty

  size_t countYes=0;
  size_t countNo=0;

  if( item[0] == '-' )
  {
    item=item.substr(1);
    ++countNo;
  }

  Split x_item(item, "-:", true);
  size_t sz = x_item.size();

  if( sz == 3 )
    ++countYes;
  else
    ++countNo;

  size_t countItems=0;

  for( size_t j=0 ; j < x_item.size() ; ++j )
  {
    if( hdhC::isDigit(x_item[j]) )
    {
        int num = hdhC::string2Double(x_item[j]);

        if( j == 0 && num < 24 )
          ++countItems;
        else if( num < 60 )
          ++countItems;
        else
          ++countNo;
    }
    else
      ++countNo;
  }

  // decision whether a date or not
  if( countNo >= (countYes+countItems) )
    return false;

  // a colon is never valid
  if( item.find('-') < std::string::npos )
  {
    capt.push_back("Time item should be separated by");
    capt.back() += hdhC::tf_val(":") ;
    text.push_back("Found " + hdhC::tf_val("-") + " in ");
  }

  if( countItems < x_item.size() )
  {
    capt.push_back("Invalid time, expected hh:mm:ss[.DEC]") ;
    text.push_back(hdhC::tf_val(item) + " in");
  }

  return true;
}

bool
CF::timeUnitsFormat_TZ(std::string item,
                        std::vector<std::string>& capt,
                        std::vector<std::string>& text )

{
  // true: is TZ item

  if( item == "Z" || item == "z" )
    return true;

  size_t countYes=0;
  size_t countNo=0;

  if( item[0] == '-' )
  {
    item=item.substr(1);
    ++countNo;
    item=item.substr(1);
  }

  if( item[0] == ':' )  // invalid begin
  {
    capt.push_back("time-zone item should not begin with a colon");
    text.push_back("Found in ") ;
    ++countNo;
  }

  std::string t0( hdhC::Lower()(item) );

  if( hdhC::isAlpha(t0) && t0.size() > 1 )
  {
    if( ! (t0 == "utc" || t0 == "gmt") )
    {
      capt.push_back("invalid time-zone indicator") ;
      text.push_back(hdhC::tf_val(item));
      text.back() += " instead of Z, UTC, GMT, or omission";

      return true;
    }
  }

  Split x_item(item, hdhC::colon);

  size_t sz = x_item.size();

  for( size_t j=0 ; j < sz ; ++j )
  {
    if( hdhC::isDigit(x_item[j]) )
      ++countYes;
  }

  if( sz == 1  )
  { // n, nn, nnnn  are valid
    if( x_item[0].size() > 4 )
      ++countNo;
    else
      ++countYes;
  }

  else if( sz == 2 )
  { // n:n, nn:n, n:nn, nn:nn are valid
    if( x_item[0].size() > 2 || x_item[1].size() > 2 )
      ++countNo;
    else
      ++countYes;
  }

  // decision whether a date or not
  if( countNo >= countYes )
    return false;

  capt.push_back("invalid time-zone item") ;
  text.push_back("Found in " + hdhC::tf_val(item) );

  return true;
}


std::string
CF::units_lon_lat(Variable& var, std::string units)
{
  // Latitude / Longitude Coordinate; desig==lat||lon
  if( units.size() == 0 )
    units = var.getAttValue(n_units) ;

  if( units.size() == 0 )
     return units;  // empty string

  std::string coord;

  std::string unitsBase="degree";

  std::vector <std::string> def_units;
  def_units.push_back( hdhC::empty );  // rotated (single)
  def_units.push_back( "s" );  // rotated (plural)

  def_units.push_back( "_north" );  // lat
  def_units.push_back( "s_north" );
  def_units.push_back( "_N" );
  def_units.push_back( "s_N" );
  def_units.push_back( "N" );
  def_units.push_back( "sN" );

  def_units.push_back( "_east" );  //lon
  def_units.push_back( "s_east" );
  def_units.push_back( "_E" );
  def_units.push_back( "s_E" );
  def_units.push_back( "E" );
  def_units.push_back( "sE" );

  std::string s;

  for( size_t j=0 ; j < def_units.size() ; ++j)
  {
    s = unitsBase + def_units[j] ;
    if( units == s )
    {
      // just find the corresponding
      if( j < 2 )
      {
        coord = "rot" ;
      }
      else if( j < 7 )
      {
        coord = n_latitude ;
        if( var.is_1st_Y )
        {
          ++var.coord.weight[1];
          var.is_1st_Y=false;
        }
      }
      else
      {
        coord = n_longitude ;
        if( var.is_1st_X )
        {
          ++var.coord.weight[0];
          var.is_1st_X=false;
        }
      }

      break;
    }
  }

  if( coord == "rot" )
  {
    // identification requires also a standard_name
    std::string sn( var.getAttValue(n_standard_name, lowerCase)) ;
    if( sn.size() )
    {
      if( sn == n_grid_longitude )
      {
        coord = sn ;
        if( var.is_1st_rotX )
        {
          ++var.coord.weight[0];
          var.coord.isC[0]=true;
          var.is_1st_rotX=false;
        }
      }
      else if( sn == n_grid_latitude )
      {
        coord = sn ;
        if( var.is_1st_rotY )
        {
          ++var.coord.weight[1];
          var.coord.isC[1]=true;
          var.is_1st_rotY=false;
        }
      }
    }
  }

  return coord;
}

double
CF::wagnerFischerAlgo(std::string& s, std::string& t)
{
  // http://en.wikipedia.org/wiki/Wagner-Fischer_algorithm

  // For all i and j, d[i,j] will hold the Levenshtein distance between
  // the first i characters of s and the first j characters of t.
  // Note that d has (m+1)  x(n+1) values.
  size_t i_sz= s.size()+1 ;
  size_t j_sz= t.size()+1 ;

  int d[i_sz][j_sz];

  // the distance of any first string to an empty second string
  for( size_t i=0 ; i < i_sz ; ++i )
    d[i][0] = i ;

  // the distance of any second string to an empty first string
  for( size_t j=0 ; j < j_sz ; ++j )
    d[0][j] = j ;

  int cost[3];
  int swp;

  for( size_t i=1 ; i < i_sz ; ++i )
  {
    for( size_t j=1 ; j < j_sz ; ++j )
    {
      if( s[i-1] == t[j-1] )
        d[i][j] = d[i-1][j-1] ;
      else
      {
        cost[0]=d[i-1][j] ;
        cost[1]=d[i][j-1] ;
        cost[2]=d[i-1][j-1] ;

        for( size_t l0=0 ; l0 < 2 ; ++l0 )
        {
          for( size_t l1=l0+1 ; l1 < 3 ; ++l1 )
          {
             if( cost[l1] < cost[l0] )
             {
                swp = cost[l0] ;
                cost[l0] = cost[l1] ;
                cost[l1] = swp ;
             }
          }
        }

        d[i][j] = cost[0]+1 ;
      }
    }
  }

  return static_cast<double>(d[i_sz-1][j_sz-1]) / static_cast<double>(t.size()) ;
}

void
CF::chap(void)
{
   // dimensionless vertical coordinate? Could set var.isChecked=true
   // chap432() ;  // was checked before

   chap2();  // NetCDF Files and Components
   chap3();  // Description of the Data
   chap4();  // Coordinate Types
   chap5();  // Coordinate Systems
   chap6();  // Labels
   chap7();  // Data Representative of Cells
   chap8();  // Reduction of Dataset Size
   chap9();  // Discrete Sampling Geometries

   analyseCoordWeights();

   return;
}

void
CF::chap_reco(void)
{
   chap2_reco();
   chap3_reco();
   chap5_reco();

   return ;
}

void
CF::chap2(void)
{
  if( ! notes->inq(bKey + "2", hdhC::empty, "INQ_ONLY" ) )
     return ;  // checks are selected to be discarded by 'D'

  chap21();   // filename extension
  chap22() ;  // data types
  chap23() ;  // naming
  chap24() ;  // dimensions
  chap251(); // missing data
  chap26() ;  // conventions, file contents

  return;
}

void
CF::chap2_reco(void)
{
   chap23_reco();    // naming
   chap24_reco();    // dimensions' order
   chap262_reco();  // title and history

   return;
}

void
CF::chap21(void)
{
  if( !isCheck )
     return;

  // filename extension
  if( pIn->file.extension != "nc" )
  {
    if( notes->inq(bKey + "21a") )
    {
      std::string capt("the extension of the filename should be '.nc'") ;

      (void) notes->operate(capt) ;
      notes->setCheckStatus( n_CF, fail );
    }
  }

  return;
}

void
CF::chap22(void)
{
  // no string type
  for( size_t i=0 ; i < pIn->varSz ; ++i )
  {
    Variable& var = pIn->variable[i];

    if( var.type == NC_STRING )
    {
      if( notes->inq(bKey + "22a", var.name) )
      {
        std::string capt(hdhC::tf_var(var.name)) ;
        capt += "should not be type NC_STRING";

        (void) notes->operate(capt) ;
        notes->setCheckStatus( n_CF, fail );
      }
    }

    for( size_t j=0 ; j < var.attName.size() ; ++j )
    {
      if( var.attType.size() == j )
        break;

      if( var.attType[j] == NC_STRING )
      {
        if( notes->inq(bKey + "26b", var.name) )
        {
          std::string capt(hdhC::tf_att(var.name, var.attName[j])) ;
          capt += "should not be type NC_STRING";

          (void) notes->operate(capt) ;
          notes->setCheckStatus( n_CF, fail );
        }
      }
    }
  }

  return;
}

void
CF::chap23(void)
{
   if( !isCheck )
     return;

   // naming convention
   std::vector<std::string> fault;

   // dimensions
   for( size_t i=0 ; i < dimensions.size() ; ++i )
   {
     if( ! hdhC::isAlpha(dimensions[i][0]) )
     {
       fault.push_back(n_dimension) ;
       fault.back() += hdhC::tf_val(dimensions[i], hdhC::blank) ;
       fault.back() += "should begin with a letter" ;
     }

     for( size_t j=1 ; j < dimensions[i].size() ; ++j )
     {
       if( ! ( hdhC::isAlpha(dimensions[i][j])
                 || hdhC::isDigit(dimensions[i][j])
                     || dimensions[i][j] == '_') )
       {
         fault.push_back(n_dimension) ;
         fault.back() += hdhC::tf_val(dimensions[i], hdhC::blank) ;
         fault.back() += "should be composed of letters, digits, and underscores" ;
         break;
       }
     }
   }

   // variables
   for( size_t i=0 ; i < pIn->varSz ; ++i )
   {
     Variable& var = pIn->variable[i];

     if( ! hdhC::isAlpha(var.name[0]) )
     {
       fault.push_back(hdhC::tf_var(var.name)) ;
       fault.back() += "should begin with a letter" ;
     }

     for( size_t j=1 ; j < var.name.size() ; ++j )
     {
       if( ! ( hdhC::isAlpha(var.name[j])
                   || hdhC::isDigit(var.name[j])
                        || var.name[j] == '_') )
       {
         fault.push_back(hdhC::tf_var(var.name)) ;
         fault.back() += "should be composed of letters, digits, and underscores" ;
         break;
       }
     }

     // attributes
     for( size_t j=0 ; j < var.attName.size() ; ++j )
     {
       // _FillValue contradicts the CF naming
       if( ! hdhC::isAlpha(var.attName[j][0])
          && var.attName[j] != n_FillValue )
       {
         fault.push_back(hdhC::tf_att(var.name, var.attName[j], hdhC::upper)) ;
         fault.back() += "should begin with a letter" ;
       }

       for( size_t k=1 ; k < var.attName[j].size() ; ++k )
       {
         if( ! ( hdhC::isAlphaNum(var.attName[j][k])
                   || var.attName[j][k] == '_') )
         {
           fault.push_back(hdhC::tf_var(var.name, hdhC::colon)) ;
           fault.back() += "Attribute name=" + var.attName[j] ;
           fault.back() += " should not contain character " ;
           fault.back() += var.attName[j][k] ;
           break;
         }
       }
     }
   }

   for( size_t i=0 ; i < fault.size() ; ++i )
   {
     if( notes->inq(bKey + "23a") )
     {
       std::string capt(fault[i]) ;

       (void) notes->operate(capt) ;
       notes->setCheckStatus( n_CF, fail );
     }
   }

   return;
}

void
CF::chap23_reco(void)
{
   // no two dimensions, variables, nor attributes
   // should be identical when case is ignored

   std::string case_ins;
   std::string case_sens;
   std::vector<std::string*> type;
   std::vector<std::string*> myAtt;
   std::vector<std::string> what;

   // dimensions
   std::vector<std::string> dims( dimensions );
   size_t sz=dims.size();

   for( size_t i=0 ; i < sz ; ++i )
   {
     case_ins = hdhC::Lower()(dims[i]);

     for( size_t j=i+1 ; j < sz ; ++j )
     {
       case_sens = hdhC::Lower()(dims[j]);

       if( case_sens == case_ins )
       {
          type.push_back(&n_dimension) ;
          what.push_back(hdhC::tf_val(dims[i]) + "and" + hdhC::tf_val(dims[j]));
       }
     }
   }

   sz=pIn->varSz;
   for( size_t i=0 ; i < sz ; ++i )
   {
     Variable& var = pIn->variable[i];

     case_ins = hdhC::Lower()(var.name);

     // variables
     for( size_t j=i+1 ; j < sz ; ++j )
     {
       case_sens = hdhC::Lower()(pIn->variable[j].name);

       if( case_sens == case_ins )
       {
          type.push_back(&n_variable) ;
          what.push_back(hdhC::tf_val(var.name, hdhC::blank)
               + "and" + hdhC::tf_val(pIn->variable[j].name));
       }
     }

     // attributes
     size_t asz=var.attName.size();
     for( size_t ai=0 ; ai < asz ; ++ai )
     {
       case_ins = hdhC::Lower()(var.attName[ai]);

       // variables
       for( size_t aj=ai+1 ; aj < asz ; ++aj )
       {
         case_sens = hdhC::Lower()(var.attName[aj]);

         if( case_sens == case_ins )
         {
            type.push_back(&var.name) ;
            what.push_back(hdhC::tf_val(var.attName[ai], hdhC::blank)
                 + "and" + hdhC::tf_val(var.attName[aj]));
         }
       }
     }
   }

   std::string tag(bKey + "23b");

   for( size_t i=0 ; i < what.size() ; ++i )
   {
      if( notes->inq(tag) )
      {
        std::string capt(n_reco);

        if( *(type[i]) == n_dimension )
          capt += " for dimensions" ;
        else if( *(type[i]) == n_variable )
          capt += "for variables" ;
        else
          capt += "for attributes of " + hdhC::tf_var(*(type[i])) ;

        capt += ": Avoid same names when case is ignored";
        std::string text("Found " + what[i]);

        (void) notes->operate(capt, text) ;
        notes->setCheckStatus( n_CF );
      }
   }

   return;
}

void
CF::chap24(void)
{
   if( !isCheck )
     return;

   // the dimensions of a variable must have different names

   for( size_t i=0 ; i < pIn->varSz ; ++i )
   {
     bool is=false;

     Variable& var = pIn->variable[i];

     int sz = var.dimName.size();

     for( int j=0 ; j < sz - 1 ; ++j )
     {
       for( int k=j+1 ; k < sz ; ++k )
       {
         if( var.dimName[j] == var.dimName[k] )
         {
            is=true ;
            break;
         }
       }

       if( is )
         break;
     }

     if( is )
     {
       if( notes->inq(bKey + "24a", var.name) )
       {
         std::string capt(hdhC::tf_var(var.getDimNameStr(true), hdhC::colon));
         capt += "Dimensions should have different names" ;

         (void) notes->operate(capt) ;
         notes->setCheckStatus( n_CF, fail );
       }
     }
   }

   return;
}

void
CF::chap24_reco(void)
{
   // order of dimensions: T, Z, Y, X
   std::vector<int> dIx;

   // 2nd check: anything else should be on the left side of the dim string
//   size_t any_ixMax; // the corresponding recommended rule is not feasable
                       // when T is unlimited
   size_t tzyx_ixMin;
   size_t sz;

   for( size_t ix=0 ; ix < pIn->varSz ; ++ix )
   {
      Variable& var = pIn->variable[ix];

      if( (sz=var.dimName.size()) < 2 )
        continue;

      tzyx_ixMin=sz;

      for( size_t i=0 ; i < sz ; ++i )
      {
         if( pIn->varNameMap.count(var.dimName[i]) )
         {
           Variable& dvar
              = pIn->variable[ pIn->varNameMap[pIn->variable[ix].dimName[i]] ];

           if( dvar.coord.isC[3] )
           {
             dIx.push_back(static_cast<int>(0));
             if( tzyx_ixMin > i )
               tzyx_ixMin = i ;
           }
           else if( dvar.coord.isC[2] || dvar.coord.isZ_DL )
           {
             dIx.push_back(static_cast<int>(1));
             if( tzyx_ixMin > i )
               tzyx_ixMin = i ;
           }
           else if( dvar.coord.isC[1] )
           {
             dIx.push_back(static_cast<int>(2));
             if( tzyx_ixMin > i )
               tzyx_ixMin = i ;
           }
           else if( dvar.coord.isC[0] )
           {
             dIx.push_back(static_cast<int>(3));
             if( tzyx_ixMin > i )
               tzyx_ixMin = i ;
           }
         }
      }

      // the sequence of found indices must ascend
      for(size_t j=1 ; j < dIx.size() ; ++j)
      {
        if( dIx[j-1] > dIx[j] )
        {
          std::string tag(bKey + "24b");
          if( notes->inq(tag, var.name) )
          {
            std::string capt(n_reco + ": The order of " + n_dimension + "s of ");
            capt += hdhC::tf_var(var.getDimNameStr(true));
            capt += "should be T,Z,Y,X" ;

            (void) notes->operate(capt) ;
            notes->setCheckStatus( n_CF );
          }

          break;
        }
      }

      dIx.clear();

      // recommendation conforming to the COARDS subset of CF:
      // any variable other than TZYX should be added "to the left"

/*
      if( any_ixMax > tzyx_ixMin )
      {
        if( notes->inq(bKey + "24c", var.name) )
        {
          std::string capt(n_reco + ": ");
          capt += var.name + ": other " + n_dimension + "s than T,Z,Y,X should be on the left" ;
          std::string text("found ");
          text += var.getDimNameStr(true);

          (void) notes->operate(capt, text) ;
          notes->setCheckStatus( n_CF );
        }
      }
*/
   }

   return;
}

void
CF::chap251(void)
{
   // for preventing compiler warnings for -O2
   double minVal=0.;
   double maxVal=0.;
   double range[] = { 0., 0.};

   int countMV=0;
   int countFV=0;

   // missing data
   for( size_t ix=0 ; ix < pIn->varSz ; ++ix )
   {
      Variable& var = pIn->variable[ix];

      int jxMV     = var.getAttIndex(n_missing_value) ;

      if( jxMV > -1 && cFVal == 14)
      {
        if( notes->inq(bKey + "251c", NO_MT) )
        {
          std::string capt(n_reco + " for CF-1.4: " + hdhC::tf_att(n_missing_value));
          capt += " is deprecated";

          std::string text("Note that this recommendation is only given for CF-1.4");

          (void) notes->operate(capt) ;
          notes->setCheckStatus( n_CF );
        }
      }

      int jxFV     = var.getAttIndex(n_FillValue) ;
      int jxVRange = var.getAttIndex(n_valid_range) ;
      int jxVMin   = var.getAttIndex(n_valid_min);
      int jxVMax   = var.getAttIndex(n_valid_max);

      if( jxVMin > -1 )
        minVal=hdhC::string2Double(var.attValue[jxVMin][0]) ;
      if( jxVMax > -1 )
        maxVal=hdhC::string2Double(var.attValue[jxVMax][0]) ;
      if( jxVRange > -1 )
      {
        range[0] = hdhC::string2Double(var.attValue[jxVRange][0]) ;
        range[1] = hdhC::string2Double(var.attValue[jxVRange][1]) ;
      }

      if(jxFV > -1)
        ++countFV;

      if(jxMV > -1)
        ++countMV;

      std::vector<std::string> mfName;
      std::vector<std::string> mfvStr;
      std::vector<double> mfv;

      if( jxFV > -1 && jxMV > -1 )
      {
        mfName.push_back(n_missing_value);
        mfName.push_back(n_FillValue);

        mfvStr.push_back(var.attValue[jxMV][0]);
        mfv.push_back(hdhC::string2Double(mfvStr.back()) );

        mfvStr.push_back(var.attValue[jxFV][0]);
        mfv.push_back(hdhC::string2Double(mfvStr.back()) );

        if( mfvStr[0] == mfvStr[1] )
           mfName[0] = hdhC::empty ;
      }
      else if( jxMV > -1 )
      {
         mfName.push_back(n_missing_value);
         mfName.push_back(hdhC::empty);
         mfvStr.push_back(var.attValue[jxMV][0]);
         mfvStr.push_back(hdhC::empty);
         mfv.push_back(hdhC::string2Double(mfvStr[0]) );
      }
      else if( jxFV > -1 )
      {
         mfName.push_back(hdhC::empty);
         mfName.push_back(n_FillValue);
         mfvStr.push_back(hdhC::empty);
         mfvStr.push_back(var.attValue[jxFV][0]);
         mfv.push_back( 0. );
         mfv.push_back(hdhC::string2Double(mfvStr[1]) );
      }

      for( size_t k=0 ; k < mfvStr.size() ; ++k )
      {
         if( followRecommendations && mfName[k].size() == 0 )
           continue;  // not provided

         //_FillValue/missing_value should not be within a specified valid_range
         if( jxVRange > -1 )
         {
           if( mfv[k] >= range[0] && mfv[k] <= range[1] )
           {
             if( notes->inq(bKey + "251a", var.name) )
             {
               std::string capt(n_reco + ": ");
               capt += hdhC::tf_att(var.name, mfName[k], mfvStr[k], hdhC::upper);
               capt += "should not be within ";
               capt += hdhC::tf_assign(n_valid_range,
                      var.attValue[jxVRange][0] + ", "
                          + var.attValue[jxVRange][1]);

               (void) notes->operate(capt) ;
               notes->setCheckStatus( n_CF );
             }
           }

           else if( jxVMin > -1 && jxVMax > -1 )
           {
             if( minVal < mfv[k] && maxVal > mfv[k] )
             {
               if( notes->inq(bKey + "251a", var.name) )
               {
                 std::string capt(n_reco + ": ");
                 capt += hdhC::tf_att(var.name, mfName[k], hdhC::upper) ;
                 capt += "should not be within the range specified by ";
                 capt += n_valid_min + " and " + n_valid_max  ;

                 std::string text("Found " + hdhC::tf_assign(mfName[k], mfvStr[k]));
                 text += ", [" + hdhC::tf_assign(n_valid_min, var.attValue[jxVMin][0]) ;
                 text += ", " + hdhC::tf_assign(n_valid_max, var.attValue[jxVMax][0]) + "]" ;

                 (void) notes->operate(capt, text) ;
                 notes->setCheckStatus( n_CF );
               }
             }
           }

           else if( jxVMax > -1 )
           {
             if( maxVal > mfv[k] )
             {
               if( notes->inq(bKey + "251a", var.name) )
               {
                 std::string capt(n_reco + ": ");
                 capt += hdhC::tf_att(var.name, mfName[k], hdhC::upper) ;
                 capt += "should not be smaller than the " ;
                 capt += n_valid_max +" value" ;

                 std::string text("Found " + hdhC::tf_assign(mfName[k], mfvStr[k]));
                 text += ", " + hdhC::tf_assign(n_valid_max, var.attValue[jxVMax][0]) ;

                 (void) notes->operate(capt, text) ;
                 notes->setCheckStatus( n_CF );
               }
             }
           }
           else if( jxVMin > -1 )
           {
             if( minVal < mfv[k] )
             {
               if( notes->inq(bKey + "251a", var.name) )
               {
                 std::string capt(n_reco + " for ") ;
                 capt += hdhC::tf_att(var.name, mfName[k]) ;
                 capt += "should not be larger than the ";
                 capt += n_valid_min + " value" ;

                 std::string text("Found " + hdhC::tf_assign(mfName[k], mfvStr[k]));
                 text += ", " + hdhC::tf_assign(n_valid_min, var.attValue[jxVMin][0]) ;

                 (void) notes->operate(capt, text) ;
                 notes->setCheckStatus( n_CF );
               }
             }
           }
         }
      }

      if( jxVRange > -1 && ( jxVMin > -1 || jxVMax > -1) )
      {
         std::string  text[2];
         std::string* ptext[2];
         std::string* p_vm[2];
         std::string* p_vr[2];
         bool is[2];

         text[0] = "Maximum" ;
         text[1] = "Minimum" ;
         ptext[0] = &n_valid_max;
         ptext[1] = &n_valid_min;
         p_vm[0] = &var.attValue[jxVMax][0];
         p_vm[1] = &var.attValue[jxVMin][0];
         p_vr[0] = &var.attValue[jxVRange][1];
         p_vr[1] = &var.attValue[jxVRange][0];
         is[0] = jxVMax > -1 && range[1] != maxVal;
         is[1] = jxVMin > -1 && range[0] != minVal;

         for( size_t i=0 ; i < 2 ; ++i)
         {
           if( is[i] && notes->inq(bKey + "251b", var.name) )
           {
             std::string capt("warning: " + text[i] + " of ");
             capt += hdhC::tf_att(var.name, n_valid_range, *(p_vr[i]));
             capt += "and " ;
             capt += hdhC::tf_assign(*(ptext[i]), *(p_vm[i]) ) ;
             capt += " are different";

             (void) notes->operate(capt) ;
             notes->setCheckStatus( n_CF, fail );
           }
         }
      }
  }

  if( countMV == 1 && countFV == 0)
  {
    if( notes->inq(bKey + "251d", NO_MT) )
    {
      std::string capt(n_reco + ": " + hdhC::tf_att(n_missing_value));
      capt += " should be replaced by _FillValue";

      (void) notes->operate(capt) ;
      notes->setCheckStatus( n_CF );
    }
  }

  return;
}

void
CF::chap26(void)
{
   if( !isCheck )
     return;

   // standardised attributes and types

   // from CF-1.4 Appendix A
   // attribute names are stored in CF::attName
   // attribute types are stored in CF::attType
   //      with S: string, N: numeric, D: data type

   // note that there is no explicit rq_chap26_2; it is already
   // checked here.

   // indices of variables and attributes
   for( size_t i=0 ; i < pIn->varSz ; ++i )
   {
     Variable& var = pIn->variable[i];

     for( size_t j=0 ; j < pIn->variable[i].attName.size() ; ++j)
     {
       std::string &aName = pIn->variable[i].attName[j];

       if( hdhC::stripSides(pIn->variable[i].attValue[j][0]).size()
              == 0  && notes->inq(bKey + "0g") )
       {
         std::string capt("warning: ");
         capt += hdhC::tf_att(var.name, aName) + "is empty" ;

         (void) notes->operate(capt) ;
         notes->setCheckStatus( n_CF, fail );
       }

       size_t k;
       if( hdhC::isAmong(aName, CF::attName, k) )
       {
          if( CF::attType[k] == 'D' )
          {
            if( var.attType[j] != var.type )
            {
              if( notes->inq(bKey + "26a", var.name) )
              {
                std::string capt(hdhC::tf_var(var.name));
                capt += "and " + hdhC::tf_att(aName);
                capt += "have to be the same type";
                std::string text("Found ");
                text += hdhC::tf_val(pIn->nc.getTypeStr(var.attType[j])) ;
                text += ", expected";
                text += hdhC::tf_val(pIn->nc.getTypeStr(var.type)) ;

                (void) notes->operate(capt, text) ;
                notes->setCheckStatus( n_CF, fail );
              }
            }
          }

          else if( CF::attType[k] == 'S' )
          {
            if( var.attType[j] != NC_CHAR )
            {
              if( notes->inq(bKey + "26b", var.name) )
              {
                std::string capt(hdhC::tf_att(var.name, aName));
                capt += "should be character type";
                std::string text("Found ");
                text += pIn->nc.getTypeStr(var.attType[j]) ;

                (void) notes->operate(capt, text) ;
                notes->setCheckStatus( n_CF, fail );
              }
            }
          }

          else if( CF::attType[k] == 'N' )
          {
            if( ! pIn->nc.isNumericType(var.name, CF::attName[k] ) )
            {
              if( notes->inq(bKey + "26c", var.name) )
              {
                std::string capt(hdhC::tf_att(var.name, aName));
                capt += "should be of numeric type, found ";
                capt += hdhC::tf_val(pIn->nc.getTypeStr(var.attType[j])) ;

                (void) notes->operate(capt) ;
                notes->setCheckStatus( n_CF, fail );
              }
            }
          }
       }
     }
   }

   return;
}

bool
CF::chap261(void)
{
  // find the CF Conventions ID
  std::string glob_cf;
  std::string aName;

  int j;
  if( (j=pIn->getVarIndex(n_NC_GLOBAL)) > -1 )
  {
    std::string n_att(n_Conventions);
    glob_cf = pIn->variable[j].getAttValue(n_att) ;
  }

  if( glob_cf.size() == 0 )
  {
    if( notes->inq(bKey + "261a") )
    {
      std::string capt("missing global ");
      capt += hdhC::tf_att(hdhC::empty, n_Conventions) ;

      (void) notes->operate(capt) ;
      notes->setCheckStatus( n_CF, fail );
    }

    return false;  // no more checks
  }

  Split x_glob_cf(glob_cf, ",; ", true);

  std::string low_val ;
  bool isCorrected=false;
  bool isMissingPrefix=true;

  for( size_t i=0 ; i < x_glob_cf.size() ; ++i )
  {
     low_val = hdhC::Lower()(x_glob_cf[i]) ;

     if( low_val.substr(0,3) == "cf-" )
         isMissingPrefix = false;

     size_t off = isMissingPrefix ? 0 : 3 ;

     if( low_val.size() > (2+off) )
     {
        if( hdhC::isDigit(low_val[0+off]) && hdhC::isDigit(low_val[2+off]) )
        {
            if( low_val[1+off] == '-' )
            {
              low_val[1+off] = '.' ;  // correction for internal use
              isCorrected=true;
            }

            if( isMissingPrefix )
              low_val = "cf-" + low_val;
        }
     }

     if( ! isMissingPrefix || isCorrected )
         break;
  }

  if( isMissingPrefix )
  {
      std::string tag(bKey+"261b");
      if( notes->inq(tag) )
      {
        std::string capt("unknown global ") ;
        capt += hdhC::tf_att(hdhC::empty, n_Conventions, glob_cf);

        (void) notes->operate(capt) ;
        notes->setCheckStatus( n_CF, fail );
      }
  }
  else if( isCorrected )
  {
    // test whether "CF-" is missing
    std::string tag(bKey+"261b");
    if( notes->inq(tag) )
    {
        std::string capt("global ") ;
        capt += hdhC::tf_att(hdhC::empty, n_Conventions, glob_cf);
        capt += ", expected " ;
        glob_cf = "CF-" + low_val.substr(3) ;

        (void) notes->operate(capt) ;
        notes->setCheckStatus( n_CF, fail );
    }
  }

  if( cFVersion.size() )
  {
    if( !setCheck(cFVersion) ) // option-provided convention supersedes
       return false;
  }
  else if( low_val.size() )
  {
    if( isMissingPrefix && ! isCorrected )
       return false;

    if( !setCheck(low_val) )
       return false;

    if( cFVal == 14 )
      cFVersion="CF-1.4" ;
    else if( cFVal == 16 )
      cFVersion="CF-1.6" ;
	else if( cFVal == 17 )
		cFVersion="CF-1.7draft" ;
  }
  else
  {
    cFVersion="CF-1.4";
    if( !setCheck(cFVersion) ) // by default
       return false;
  }

  return true;
}

void
CF::chap262_reco(void)
{
  // title and history should be global atts. In case there are none,
  // check whether they exist per variable.
  bool isNotTitle=true;
  bool isNotHistory=true;

  std::string t("title");
  std::string h("history");
  std::string *p[] = { 0, 0};

  for( int ix=pIn->variable.size()-1 ; ix > -1  ; --ix )
  {
     Variable& var = pIn->variable[ix];

     if( var.name == n_NC_GLOBAL)
     {
       isNotTitle   = ! var.isValidAtt(t);
       isNotHistory = ! var.isValidAtt(h);

       if( !isNotTitle && !isNotHistory )
          return; // both are global atts

       continue;
     }

     if( var.isValidAtt(t) )
       p[0] = &t;
     if( var.isValidAtt(h) )
       p[1] = &h;

     for( size_t k=0 ; k < 2 ; ++k )
       if( p[k] )
          p[k]=0;
  }

  return;
}

void
CF::chap3(void)
{
  if( ! notes->inq(bKey + "3", hdhC::empty, "INQ_ONLY" ) )
     return ;  // checks are selected to be discarded by 'D'

  chap33() ; // standard_name; include look-up to xml table
  chap34();  // ancillary_variables
  chap35();  // flags

  return;
}

void
CF::chap3_reco(void)
{
  // all variables should use either the long_name or the standard_name
  for( size_t ix=0 ; ix < pIn->varSz ; ++ix )
  {
     Variable& var = pIn->variable[ix];

     // these are exceptions
     if( var.boundsOf.size() || var.isMapVar || var.isCompress || var.isLabel )
       continue;

     if( ! (var.isValidAtt(n_standard_name) || var.isValidAtt(n_long_name) ) )
     {
        // there is an exception for grid_mapping variables
        if( ! var.isValidAtt(n_grid_mapping +"_name") )
        {
          if( notes->inq(bKey + "32a", var.name) )
          {
            std::string capt(n_reco + " for " + hdhC::tf_var(var.name, hdhC::colon));
            capt += "Use "  + n_standard_name + " and/or " + n_long_name;

            (void) notes->operate(capt) ;
            notes->setCheckStatus( n_CF );
          }
        }
     }
  }

  chap35_reco() ;  // flags
  return;
}

void
CF::chap33(void)
{
   // standard_name and some more, comparable to
   // http://cf-pcmdi.llnl.gov/documents/cf-standard-names/cf-standard-name-table.xml,
   // which was read in getSN_TableEntry().

   // note that units, amip and/or grib are only checkable for a valid standard_name.

   // any annotation for a standard_name
   for( size_t i=0 ; i < pIn->varSz ; ++i )
   {
      Variable& var = pIn->variable[i] ;

      if( var.std_name.size() == 0 )
        continue;

      // take into account that a standard name may contain blanks (error)
      // or a modifier may be appended
      if( ! var.snTableEntry.found )
      {
        if( notes->inq(  bKey + "33a", var.name) )
        {
          std::string capt(hdhC::tf_att(var.name, n_standard_name, var.std_name));
          capt += "is not CF conform" ;

          (void) notes->operate(capt) ;
          notes->setCheckStatus( n_CF, fail );
        }

        continue; // disable the following units check
      }

      if( var.snTableEntry.remainder.size() || var.snTableEntry.hasBlanks )
        checkSN_Modifier(var) ;

      // units convertable to the canonical one. Note that udunits can not
      // discern degrees_north and degrees_east. This will be done in chap41.

      // special: standard_name with modifier=number_of_observations requires units=1
      if( var.units.size() && var.snTableEntry.canonical_units.size() )
      {
        bool ordCase = cmpUnits( var.units, var.snTableEntry.canonical_units) ;
        bool spcCase = var.snTableEntry.remainder == n_number_of_observations ;
        bool dimlessZ=false;

        if( var.units == "level" && var.snTableEntry.canonical_units == "1" )
           dimlessZ = true;


        if( !ordCase && !spcCase && !dimlessZ )
        {
           if( var.snTableEntry.canonical_units == "s" )
           {
               Split x_units(var.units) ;
               if( x_units.size() > 2
                      && ! hdhC::isDigit(x_units[0])
                         && ! hdhC::isDigit(x_units[1]) )
               {
                  ;  // the date is tested elsewhere
               }
           }

           else if( notes->inq(bKey + "33e", var.name) )
           {
             std::string capt(hdhC::tf_att(var.name, n_units, var.units));
             capt += "is not CF compatible with " ;
             capt += hdhC::tf_assign(n_standard_name, var.snTableEntry.std_name) ;

             (void) notes->operate(capt) ;
             notes->setCheckStatus( n_CF, fail );
           }
        }

        if(spcCase && var.units.size() && var.units != "1" )
        {
          if( notes->inq(bKey + "33d", var.name) )
          {
            std::string capt(hdhC::tf_att(var.name, n_standard_name, hdhC::colon) );
            capt += "The ";
            capt += hdhC::tf_assign("modifier", n_number_of_observations);
            capt += " requires units=1";

            std::string text("Found ");
            text += hdhC::tf_val(var.units) ;

            (void) notes->operate(capt, text) ;
            notes->setCheckStatus( n_CF, fail );
          }
        }
      }

      // amip
      int j;
      if( (j=var.getAttIndex("amip", lowerCase)) > -1 )
      {
        Split x_amip(var.snTableEntry.amip );
        std::vector<std::string> vs(x_amip.getItems());

        if( !hdhC::isAmong(var.attValue[j][0], vs) )
        {
          if( notes->inq(bKey + "33f", var.name) )
          {
            std::string capt(hdhC::tf_var(var.name, hdhC::colon));
            capt += hdhC::tf_assign("AMIP code", var.attValue[j][0]) ;
            capt += " is not CF compatible, expected " ;
            capt += var.snTableEntry.amip ;

            (void) notes->operate(capt) ;
            notes->setCheckStatus( n_CF, fail );
          }
        }
      }

      // grib
      if( (j=var.getAttIndex("grib", lowerCase)) > -1 )
      {
        Split x_grib(var.snTableEntry.grib );
        std::vector<std::string> vs(x_grib.getItems());

        if( !hdhC::isAmong(var.attValue[j][0], vs) )
        {
          if( notes->inq(bKey + "33g", var.name) )
          {
            std::string capt(hdhC::tf_var(var.name, hdhC::colon));
            capt += hdhC::tf_assign("GRIP code", var.attValue[j][0]);
            capt += " is not CF compatible, expected " ;
            capt += var.snTableEntry.grib;

            (void) notes->operate(capt) ;
            notes->setCheckStatus( n_CF, fail );
          }
        }
      }
   }

   return;
}

void
CF::chap34(void)
{
   if( !isCheck )
     return;

   // ancillary_variables
   // there is nothing to check; the attribute just expresses relationships
   // between non-coordinate variables.

   // non-var terms in the attribute
   std::vector<std::string> non_var;

   for( size_t i=0 ; i < pIn->varSz ; ++i )
   {
     Variable& var = pIn->variable[i];

     int j;
     if( (j=var.getAttIndex(n_ancillary_variables)) > -1 )
     {
        non_var.clear();
        size_t countSuccess=0;

        std::string &str = var.attValue[j][0];

        bool isComma=false;
        if( str.find(',') < std::string::npos )
          isComma=true;

        Split x_av(str, "() ,", true);

        // scan over declared variable names
        for( size_t jj=0 ; jj < x_av.size() ; ++jj )
        {
          int ii = pIn->getVarIndex(x_av[jj]);

          if( ii == -1 )
            non_var.push_back(x_av[jj]);
          else
            ++countSuccess;
        }

        std::string text;
        std::string capt;

        if( non_var.size() > 2 && countSuccess )
        {
          capt  = "CF requires a blank separated list of variable names";
          text = "Found names embedded in text";
        }
        else if( non_var.size() )
        {
          capt  = "declares non-existing variables" ;
          text = "Found " + hdhC::getVector2Str(non_var) ;
        }
        else if(isComma)
        {
          capt = "CF requires a blank separated list of variable names";
          text.clear();
        }

        if( text.size() && notes->inq(bKey + "34a", var.name) )
        {
          std::string capt0(hdhC::tf_att(var.name, n_ancillary_variables));

          (void) notes->operate(capt0+capt, text) ;
          notes->setCheckStatus( n_CF, fail );
        }
     }
   }

   return;
}

void
CF::chap35(void)
{
   if( !isCheck )
     return;

   // flags
   std::vector<std::string> f_name;
   f_name.push_back(n_flag_values);
   f_name.push_back(n_flag_masks);
   f_name.push_back("flag_meanings");

   std::vector<std::string> *pf[3] ;
   pf[0]= 0 ;  //flag_values
   pf[1]= 0 ;  //flag_masks
   pf[2]= 0 ;  //flag_meanings

   Split x_f_meanings;

   // variables
   for( size_t i=0 ; i < pIn->varSz ; ++i )
   {
     Variable& var = pIn->variable[i];

     bool flagFound=false;

     for( size_t j=0 ; j < var.attName.size() ; ++j )
     {
       std::string &aName = var.attName[j];

       if( aName == f_name[0] )
       {
         pf[0] = &var.attValue[j] ;
         flagFound=true;
       }
       else if( aName == f_name[1] )
       {
         pf[1] = &var.attValue[j] ;
         flagFound=true;
       }
       else if( aName == f_name[2] )
       {
         pf[2] = &var.attValue[j] ;
         x_f_meanings = var.attValue[j][0] ;
         flagFound=true;
       }
     }

     // valid for both flag_values and flag_masks
     if( flagFound )
     {
        // comparison of types:
        // flag_values and flag_masks, resp., must have the same type
        // as the corresponding variable. This was checked in chap26

        if(pf[0])
        {
          // the values of flag_values must be mutually exclusive
          // note that f_type still holds the type of flag_masks
          bool is=false;
          int sz=static_cast<int>(pf[0]->size());
          for( int l0=0 ; l0 < sz-1 ; ++l0 )
          {
            for( int l1=l0+1 ; l1 < sz ; ++l1 )
            {
              // note that pf[0] is just a pointer to a vector of a vector
              if( (*pf[0])[l0] == (*pf[0])[l1] )
              {
                 is=true;
                 break;
              }
            }

            if( is )
               break;
          }

          if( is )
          {
            if( notes->inq(bKey + "35e", var.name) )
            {
              std::string capt(hdhC::tf_var(var.name, hdhC::colon));
              capt += "Values of flag_values have to be mutually exclusive";

              std::string text("Found ");
              text += var.name + ":flag_values=" ;
              for( int l=0 ; l < sz ; ++l )
              {
                 if( l )
                   text += ", ";
                 text += (*pf[0])[l] ;
              }

              (void) notes->operate(capt, text) ;
              notes->setCheckStatus( n_CF, fail );
            }
          }
        }

        if(pf[1])
        {
          // a variable with flag_mask att must be compatible with bit field
          // expression (char, byte, short, int)

          // note that f_type still holds the type of flag_masks

          if( ! (var.type == pIn->nc.isIndexType(var.name)
                     || var.type == NC_CHAR ) )
          {
            if( notes->inq(bKey + "35h", var.name) )
            {
              std::string capt("type of" + hdhC::tf_val(var.name, hdhC::blank));
              capt += "has to be bit-field-expression compatible";

              std::string text("Type of ");
              text += var.name + "is" ;
              text += pIn->nc.getVarTypeStr(var.name) ;

              (void) notes->operate(capt, text) ;
              notes->setCheckStatus( n_CF, fail );
            }
          }

          // the flag_masks att values must be non-zero; we don't care the correct type,
          // but we can't use the values in pf[1]
          std::vector<int> v_i;
          pIn->nc.getAttValues(v_i, n_flag_masks, var.name);

          for( size_t l=0 ; l < v_i.size() ; ++l )
          {
             if( v_i[l] == 0 )
             {
               if( notes->inq(bKey + "35f", var.name) )
               {
                 std::string capt(hdhC::tf_att(var.name, n_flag_masks));
                 capt += "should be non-zero" ;

                 (void) notes->operate(capt) ;
                 notes->setCheckStatus( n_CF, fail );
               }

               break;
             }
          }
        }

        if( x_f_meanings.size() == 0 && pf[0]->size() )
        {
          // if flag_values is present, then the flag_meanings att must be specified
          if( notes->inq(bKey + "35c", var.name) )
          {
            std::string capt(hdhC::tf_att(var.name, "flag_values", hdhC::colon));
            capt += "Missing " + hdhC::tf_att("flag_meanings");

            (void) notes->operate(capt) ;
            notes->setCheckStatus( n_CF, fail );
          }
        }

        // number of items for flag_values and flag_mask, resp., must equal
        // the number of flag_meanings
        for( size_t k0=0 ; k0 < 2 ; ++k0 )
        {
          if(pf[k0] == 0 )
            continue;

          for( size_t k1=k0+1 ; k1 < 3 ; ++k1 )
          {
            if(pf[k1] == 0 )
              continue;

            size_t sz = k1 == 2 ? x_f_meanings.size() : pf[k1]->size() ;

            if( pf[k0]->size() != sz )
            {
              if( notes->inq(bKey + "35a", var.name) )
              {
                std::string capt(hdhC::tf_var(var.name, hdhC::colon));
                capt += "Attributes " ;
                capt += f_name[k0] ;
                capt += " and ";
                capt += f_name[k1] ;
                capt += " require same number of items";
                std::string text("Found ");
                text += hdhC::tf_assign("num", hdhC::itoa(pf[k0]->size())) ;
                text += " vs.";
                text += hdhC::tf_val(hdhC::itoa(pf[k1]->size())) ;

                (void) notes->operate(capt, text) ;
                notes->setCheckStatus( n_CF, fail );
              }
            }

            k0=3;
            break;
          }
        }

        if( cFVal > 14 )
        {
           // flag_meanings must consist of alphanumerics and _,-,.,+,@
           bool is=false;
           std::string &str = (*pf[2])[0] ;
           char c;
           for(size_t j=0 ; j < str.size() ; ++j )
           {
              c= str[j];
              if( hdhC::isAlphaNum(c) )
                 continue;
              else if( c == '_' )
                 continue;
              else if( c == '-' )
                 continue;
              else if( c == '.' )
                 continue;
              else if( c == '+' )
                 continue;
              else if( c == '@' )
                 continue;
              else if( c == ' ' )
                 continue;

              is=true;
              break;
           }

           if( is && notes->inq(bKey + "35g", var.name) )
           {
              std::string t;
              t = c;
              std::string capt(cFVersion + " for " + hdhC::tf_var(var.name, hdhC::colon) );
              capt += hdhC::tf_att(hdhC::empty, "flag_meanings", hdhC::upper) ;
              capt += "has to consist of alpha-numerics," ;
              capt += " '_', '-', '.', '+', and '@'. Found " + t ;

              (void) notes->operate(capt) ;
              notes->setCheckStatus( n_CF, fail );
          }
        }

        x_f_meanings.clear();
        pf[0]=0;
        pf[1]=0;
        pf[2]=0;
     }
   }

   return;
}

void
CF::chap35_reco(void)
{
   // flags
   // when flag_masks and flag_values are both defined, the boolean AND
   // of each entry in flag_values with its corresponding entry in flag_masks
   // should equal the flag_entry vallue

   for( size_t i=0 ; i < pIn->varSz ; ++i )
   {
     Variable& var = pIn->variable[i];

     size_t m_ix, v_ix;

     if( (v_ix=var.isValidAtt(n_flag_values))
           && (m_ix=var.isValidAtt(n_flag_masks)) )
     {

        std::vector<unsigned int> v_uc;
        pIn->nc.getAttValues(v_uc, n_flag_values, var.name );

        std::vector<unsigned int> m_uc;
        pIn->nc.getAttValues(m_uc, n_flag_masks, var.name );

        if( m_uc.size() == v_uc.size() )
        {
           unsigned int res;
           for( size_t j=0 ; j < m_uc.size() ; ++j )
           {
              res = v_uc[j] & m_uc[j] ;
              if( res != v_uc[j] )
              {
                 if( notes->inq(bKey + "35i", var.name) )
                 {
                    std::string capt(n_reco + " for " + hdhC::tf_var(var.name, hdhC::colon));
                    capt += "The boolean AND of each flag_values and flag_masks ";
                    capt += "should equal the flag_values entry";

                    (void) notes->operate(capt) ;
                    notes->setCheckStatus( n_CF );
                 }
                 break;
              }
           }
        }
     }
   }

   return;
}

void
CF::chap4(void)
{
  // Coordinate Types
  if( ! notes->inq(bKey + "4", hdhC::empty, "INQ_ONLY" ) )
     return ;  // checks are selected to be discarded by 'D'

  for( size_t ix=0 ; ix < pIn->varSz ; ++ix )
  {
     Variable& var = pIn->variable[ix];

     // check (aux) coordinate vars, eventually the data
     if( !var.isChecked && chap4(var) )
     {
        // special for scalars: if a coordinate and there is no other coordinate of
        // the same type, then is is a real coordinate var
        if(var.isScalar)
        {
          bool is=true;

          for( size_t i=0 ; i < pIn->varSz ; ++i )
          {
             Variable& varj = pIn->variable[i] ;
             if(var.name != varj.name)
             {
                if(varj.coord.isCoordVar)
                {
                  for(size_t l=0 ; l < 4 ; ++l )
                  {
                    if(var.coord.isC[l] && varj.coord.isC[l])
                    {
                      is=false;
                      break;
                    }

                    if(!is)
                      break;
                  }
                }
             }
          }

          if(is)
            var.coord.isCoordVar = true;
        }

        bool is = var.coord.isCoordVar ? true : false ;

        // aux coord vars for chap9 features are tested else where
        if( cFVal > 15 && !is )
          continue ;

        // false: disabled monotony test
        checkCoordinateValues(var);
     }
  }

  return;
}

bool
CF::chap4(Variable& var)
{
  if( var.isCompress )
     return false;

  hasBounds(var);

  // identifying bounds before checking coordinate types
  // simplifies things.
  if( isBounds(var) )
    return false;

// Coordinate Types: functions return true if identified

  if( chap44(var) )  // time
    return true;

  if( chap41(var) )  // lat/lon or rotated coord
    return true;

  // note that Z with dimensions for height in units=m is the most difficult
  if( chap43(var) )  // vertical coord, also dimensionless
    return true;

  return false;
}

bool
CF::chap41(Variable& var)
{
  std::string units(var.getAttValue(n_units)) ;
  std::string sn( var.getAttValue(n_standard_name, lowerCase) );
  std::string axis( var.getAttValue(n_axis) );

  // possible return values: lon, lat, rlon, rlat, size==0
  std::string coordType( units_lon_lat(var, units) );

  if( coordType == n_longitude || coordType == "rlon" )
    var.coord.isBasicType[0]=true;
  else if( coordType == n_latitude || coordType == "rlat" )
    var.coord.isBasicType[1]=true;

  bool isAx[2]={false, false};  // optional, but if, then axis=X or Y
  bool isSN[2]={false, false};
  int count[2]= {0,0};

  if( var.coord.isBasicType[0] )
    ++count[0];
  else if( var.coord.isBasicType[1] )
    ++count[1];

  // no standard_name? try the long-name
  bool isLongName=false;
  if( sn.size() == 0 )
  {
    sn = var.getAttValue(n_long_name) ;
    isLongName=true;
  }

  if( sn.size() )
  {
    if( sn == n_longitude
          || (isLongName && sn.find(n_longitude) < std::string::npos) )
    {
      isSN[0]=true;
      ++count[0];
    }
    else if( sn == n_latitude
          || (isLongName && sn.find(n_latitude) < std::string::npos) )
    {
      isSN[1]=true;
      ++count[1];
    }
  }

  // axis is optional, but if provided, then X/Y
  if( axis.size() )
  {
    if( axis == "Y" || axis == "y" )
    {
      isAx[1] = true;
      ++count[1];
    }
    else if( axis == "X" || axis == "x" )
    {
      isAx[0] = true;
      ++count[0];
    }
  }

  for( size_t i=0 ; i < 2 ; ++i )
  {
    if( var.coord.isBasicType[i] && isSN[i] )
      ++count[i];
    if( var.coord.isBasicType[i] && isAx[i] )
      ++count[i];
    if( isSN[i] && isAx[i] )
      ++count[i];
  }

  // decision whether X, Y or neither
  int ix = (count[0] > count[1]) ? 0 : 1 ;

  if( count[ix] < 1 )
    return false;

  var.addAuxCount(count[ix]);
  var.isChecked=true;

  var.coord.weight[ix] += count[ix];
  var.coord.isC[ix]=true;

  if( !isCheck )
    return true;

  if( var.units.size() == 0 )
  {
    if( isSN[ix] )
    {
      if( notes->inq(bKey + "41b", var.name) )
      {
        std::string capt(hdhC::tf_var(var.name) + "was rated a " );
        if( sn == n_grid_latitude || sn == n_grid_longitude )
          capt += "rotational " ;

        if( sn == n_longitude || sn == n_grid_longitude )
          capt += n_longitude ;
        else if( sn == n_latitude || sn == n_grid_latitude)
          capt += n_latitude ;

        capt += " coordinate, but " + hdhC::tf_att(n_units) + "is missing";

        (void) notes->operate(capt) ;
        notes->setCheckStatus( n_CF, fail );
      }
    }
  }

  if( !isSN[ix] && var.coord.isBasicType[ix] )
  {
    if( coordType == "rlon" || coordType == "rlat" )
    {
      if( notes->inq(bKey + "41c", var.name) )
      {
        std::string capt(hdhC::tf_var(var.name) + "was rated a rotational " );
        if( var.coord.isBasicType[ix] )
          capt += n_longitude ;
        else
          capt += n_latitude ;

        capt += " coordinate, but the " + n_standard_name + " is missing";

        (void) notes->operate(capt) ;
        notes->setCheckStatus( n_CF, fail );
      }
    }
  }

  if( units.size() && isSN[ix] )
  {
    bool is=false;
    if( var.coord.isBasicType[ix] )
    {
      std::string sn_coordType( units_lon_lat(var, var.snTableEntry.canonical_units) );
      if( sn_coordType != coordType )
        is=true;
    }
    else
      is=true;

    if( is && notes->inq(bKey + "41a", var.name) )
    {
      std::string capt ;
      if( isSN[ix] )
      {
        capt = hdhC::tf_att(var.name, n_standard_name, var.std_name) ;
      }
      else
        capt = hdhC::tf_att(var.name, n_axis, axis) ;

      capt +=  "is not compatible with ";
      capt += hdhC::tf_assign(n_units, var.units) ;

      (void) notes->operate(capt) ;
      notes->setCheckStatus( n_CF, fail );
    }
  }

  return true;
}

bool
CF::chap43(Variable& var)
{
  // vertical coordinate

  // the case of a dimensionless vertical coordinate was checked elsewhere
  if( chap431(var) )  // n-dimensional (n>1)  or scalar?
    return true;

  return false;
}

bool
CF::chap431(Variable& var)
{
  // dimensional vertical coordinate.

  // 1) units must be available
  // 2) units==pressure or attribute positive is present
  // Optionally:
  //   3) appropriate standard_name
  //   4) axis == Z

  std::string units( var.getAttValue(n_units) ) ;
  std::string axis( var.getAttValue(n_axis, lowerCase) ) ;
  std::string positive( var.getAttValue(n_positive, lowerCase) ) ;
  std::string& canon_u = var.snTableEntry.canonical_units;

  bool isP=false;
  bool isM=false;
  bool isAx=false;

  int countLimit=1;

  if( units.size() ) // required
  {
    if( (isP=cmpUnits(units, "Pa")) )
      var.addWeight(2);
    else if( (isM=cmpUnits( units, "m")) )
      var.addWeight(2);
  }
  else if( canon_u.size() )
  {
    // std_name with appropriate units; optional
    if( (isP=cmpUnits(canon_u, "Pa")) )
      var.addWeight(2);
    else if( (isM=cmpUnits(canon_u, "m")) )
      var.addWeight(2);
  }

  bool isPos ;
  if( (isPos=(positive == "up" || positive == "down") ) )
    var.addWeight(2);

  if( positive.size() )
    var.addWeight(2);

  if( (isAx= axis == "z") )
    var.addWeight(2);
  else if( axis.size() )
    --var.coord.weight[2];

  // some combinations indicating a vertical dimension
  if( isPos && (isP || isM) ) // positive and a kind of units
    var.addWeight(2);
  if( isPos && isAx )  // positive and axis
      var.addWeight(2);
  if( isAx && (isP || isM) )  // axis and units
      var.addWeight(2);

  // finally
  bool isA = hdhC::isAmong(var.name, ca_vvs);

  if(isA)
    var.addAuxCount(1);
  if( var.coord.isCoordVar )
    var.addAuxCount(1);

  if( var.coord.weight[2] == 1 )
  {
    bool isACV = isA || var.coord.isCoordVar ;

    if(isP)
    {
      if( ! isACV )
        return false;

      size_t i;
      for( i=0 ; i < pIn->varSz ; ++i )
      {
        if( hdhC::isAmong(var.name, pIn->variable[i].dimName) )
        {
          var.addWeight(2);
          break;
        }
      }
    }
  }

  if( var.coord.weight[2] > countLimit )
  {
    var.coord.isC[2]=true;
    var.coord.isZ_p=isP;
    var.isChecked=true;
    var.addAuxCount();
  }
  else
    return false;

  if( var.type == NC_CHAR )
    return false; // this may not be true in every case

  return var.coord.isC[2];
}

void
CF::chap432(void)
{
  // dimensionless vertical coordinate?

  // valid standard names
  std::vector<std::string> valid_sn;
  valid_sn.push_back("atmosphere_ln_pressure_coordinate");
  valid_sn.push_back("atmosphere_sigma_coordinate");
  valid_sn.push_back("atmosphere_hybrid_sigma_pressure_coordinate");
  valid_sn.push_back("atmosphere_hybrid_height_coordinate");
  valid_sn.push_back("atmosphere_sleve_coordinate");
  valid_sn.push_back("ocean_sigma_coordinate");
  valid_sn.push_back("ocean_sigma_coordinate");
  valid_sn.push_back("ocean_sigma_z_coordinate");
  valid_sn.push_back("ocean_double_sigma_coordinate");

  //variables in formula_terms; order doesn't matter
  std::vector<std::string> valid_ft;
  valid_ft.push_back("p0: lev:");
  valid_ft.push_back("sigma: ps: ptop:");
  valid_ft.push_back("a: p0: b: ps:"); // alternative is substituted later
  valid_ft.push_back("a: b: orog:");
  valid_ft.push_back("a: b1: b2: ztop: zsurf1: zsurf2:");
  valid_ft.push_back("sigma: eta: depth:");
  valid_ft.push_back("s: eta: depth: a: b: depth_c:");
  valid_ft.push_back("sigma: eta: depth: depth_c: nsigma: zlev:");
  valid_ft.push_back("sigma: depth: z1: z2: a: href: k_c:");

  std::vector<int> ix ;  // collects all the candidates
  std::vector<int> ij_fT;
  std::vector<int> ij_sN;
  std::vector<int> ij_aV;

  std::vector<int> ix_sN ;
  std::vector<int> ix_aV ; // value of an attribute is appropriate
  std::vector<int> jx_sN ;
  std::vector<int> jx_aV ; // value of an attribute is appropriate

  // two ways to identify a dimless vertical coord.
  // a) a formula_terms attribute,
  // b) a standard name indicating a valid dimless method

  for( size_t i=0 ; i < pIn->varSz ; ++i )
  {
     Variable& var = pIn->variable[i] ;
     int j;

     if( (j=var.getAttIndex(n_formula_terms)) > -1 )
     {
        ix.push_back(i);

        ij_fT.push_back(j);
        ij_sN.push_back(-1);
        ij_aV.push_back(-1);
     }

     if( (j=var.getAttIndex(n_standard_name)) > -1 )
     {
        if( hdhC::isAmong(var.attValue[j][0], valid_sn) )
        {
          ix_sN.push_back(i);
          jx_sN.push_back(j);
        }
     }

     // look for a totally misnamed ft att
     Split x_av;
     for( size_t v=0 ; v < var.attName.size() ; ++v )
     {
        if( var.attName[v] == n_cell_measures
              || var.attName[v] == n_cell_methods )
          continue;

        x_av = var.attValue[v][0] ;
        size_t x_av_sz = x_av.size() ;

        // must appear in pairs
        if( x_av_sz % 2 )
          continue;

        if( x_av_sz )
        {
          size_t x;
          for( x=0 ; x < x_av_sz ; ++x )
          {
            size_t tail = x_av[x].size()-1;
            if( x_av[x++][tail] != ':' )
              break;

            else if( !pIn->nc.isVariableValid( x_av[x] ) )
                break;
          }

          if( x == x_av_sz )
          {
            ix_aV.push_back(i);
            jx_aV.push_back(v);
            break;
          }
        }
     }
  }

  // the sequence of tests must be: i), ii)

  // i) try attribute with appropriate format
  for( size_t i=0 ; i < ix_aV.size() ; ++i )
  {
    size_t j;
    for( j=0 ; j < ix.size() ; ++j )
    {
      if( ix_aV[i] == ix[j] )
      {
        if( ij_aV[j] == -1 )
          ij_aV[j] = jx_aV[i];

        break;
      }
    }

    if( j == ix.size() )
    {
      ix.push_back(ix_aV[i]) ;
      ij_fT.push_back(jx_aV[i]) ;
      ij_sN.push_back(-1) ;
    }
  }

  // ii) found appropriate standard name
  for( size_t i=0 ; i < ix_sN.size() ; ++i )
  {
    size_t j;
    for( j=0 ; j < ix.size() ; ++j )
    {
      if( ix_sN[i] == ix[j] )
      {
        if( ij_sN[j] == -1 )
          ij_sN[j] = jx_sN[i];

        break;
      }
    }

    if( j == ix.size() )
    {
      ix.push_back(ix_sN[i]) ;
      ij_fT.push_back(-1) ;
      ij_sN.push_back(jx_sN[i]) ;
    }
  }

  for( size_t i=0 ; i < ix.size() ; ++i)
  {
    Variable& var = pIn->variable[ix[i]];

    if( chap432(var, valid_sn, valid_ft, ij_fT[i], ij_sN[i]) )
    {
      var.coord.isC[2]    = true;
      var.coord.isZ_DL = true;
      var.addWeight(2);
      var.addAuxCount();
      var.isChecked=true;
    }
  }

  return;
}

bool
CF::chap432(Variable& var,
   std::vector<std::string>& valid_sn,
   std::vector<std::string>& valid_ft,
   int att_ft_ix, int att_sn_ix )
{
  // dimensionless vertical coordinate
  int valid_ft_ix=-1;
  int valid_sn_ix=-1;

  int att_units_jx ;
  std::string units;
  if( (att_units_jx=var.getAttIndex(n_units)) > -1 )
  {
    // tolerated units for COARDS compatibility: level, layer, sigma_level;
    // would change deprecated units forms to "1"
    units = var.attValue[att_units_jx][0];
    chap432_deprecatedUnits(var, units) ;
  }

  // cross-check of standard_name vs. formula_terms
  if( chap432_checkSNvsFT(var, valid_sn, valid_ft, valid_sn_ix,
                   att_ft_ix, att_sn_ix, units) )
    return false; // no formula_terms case

  // formula_terms by vector of pairs: 1st=key: 2nd=param_varName
  std::vector<std::pair<std::string, std::string> > att_ft_pv ;

  chap432_getParamVars(var, valid_sn, valid_ft, valid_ft_ix, valid_sn_ix,
                   att_ft_ix, att_ft_pv) ;


  // special for the alternative hybrid sigma pressure coord.
  if( valid_ft_ix == 2 && att_ft_pv.size() == 3 )
    valid_ft[valid_ft_ix]="ap: b: ps:";

  // sn and ft from the same valid algorithm?
  if( att_sn_ix > -1 && valid_ft_ix > -1 && valid_ft_ix != valid_sn_ix)
  {
    if( notes->inq(bKey + "432e", var.name) )
    {
      std::string capt(hdhC::tf_var(var.name, hdhC::colon));
      capt += "The attributes " + n_formula_terms + " and " ;
      capt += n_standard_name + " are not compatible";

      std::string text("Found ") ;
      text += hdhC::tf_att(var.name, n_standard_name, var.attValue[att_sn_ix][0]) + " vs. " ;
      text += hdhC::tf_att(var.name,n_formula_terms, var.attValue[att_ft_ix][0]) ;

      (void) notes->operate(capt, text) ;
      notes->setCheckStatus( n_CF, fail );
    }
  }

  if( isCheck && att_units_jx > -1 )
  {
    if( units.size() && units != "1" )
    {
      if( notes->inq( bKey + "432b", var.name) )
      {
        std::string capt(hdhC::tf_var(var.name, hdhC::colon));
        capt += "Dimensionless vertical coordinate must not have " ;
        capt += hdhC::tf_assign(n_units, units) ;

        (void) notes->operate(capt) ;
        notes->setCheckStatus( n_CF, fail );
      }
    }
  }

  std::vector<std::string> fTerms;
  Split x_fTerms( valid_ft[valid_ft_ix] );
  for( size_t i=0 ; i < x_fTerms.size() ; ++i )
     fTerms.push_back( x_fTerms[i] );

  // after return, fTerms contains associated variables.
  chap432_verify_FT(var,valid_ft_ix,
          valid_ft[ valid_ft_ix ], att_ft_ix, fTerms, att_ft_pv);

  size_t ft_sz = fTerms.size();

  if( ft_sz )
  {
     size_t j;
     for( j=0 ; j < ft_sz ; ++j )
       if( fTerms[j] == var.name )
         break;
     if( j == ft_sz )
       var.isChecked = true;  // data is not necessary

     for( j=0 ; j < fTerms.size() ; ++j )
     {
       int i ;

       if( (i=pIn->getVarIndex(fTerms[j])) > -1 )
       {
         pIn->variable[i].addAuxCount();

         // parameter variables should not have any _FillValue within the data body;
         // this is not mentioned in the CF doc.
         if( !pIn->variable[i].isChecked )
         {
           pIn->variable[i].coord.isCoordVar = true;  // temporarily
           pIn->variable[i].coord.isAny = true;
           pIn->variable[i].isAUX = true;
           pIn->variable[i].isFormulaTermsVar = true;

           checkCoordinateValues(pIn->variable[i], true); // no monotony test

           pIn->variable[i].coord.isCoordVar = false;
           pIn->variable[i].isChecked=true;
         }
       }
     }
  }

  return true;
}

bool
CF::chap432_checkSNvsFT( Variable& var,
   std::vector<std::string>& valid_sn,
   std::vector<std::string>& valid_ft,
   int& valid_sn_ix,
   int& att_ft_ix, int& att_sn_ix, std::string& units)
{
  // return true for no formula_terms.

  // note that in the case of a non_existing ft att, all the atts are searched for
  // a valid ft signature. So, att_ft_ix may return an index.

  // cross-check of standard_name vs. formula_terms (case: dimless vertical coord)
  // return true if no formula_term proceeding is possible
  if( att_sn_ix > -1 )
  {
    std::string str( hdhC::Lower()(var.attValue[att_sn_ix][0]) );

    for( size_t j=0 ; j < valid_sn.size() ; ++j )
    {
      if( str == valid_sn[j] )
      {
        valid_sn_ix = j ;
        break;
      }
    }
  }

  // if there is any appropriate value for a formula_terms attribute, but
  // the name of the attribute is different, then the att name was different
  if( att_ft_ix > -1 && var.attName[att_ft_ix] != n_formula_terms )
  {
    if( notes->inq(bKey + "432f", var.name) )
    {
      std::string capt(hdhC::tf_var(var.name, hdhC::colon));
      capt += "The signature of a " + n_formula_terms + hdhC::blank + n_attribute;
      capt += " was found, but the name of the attribute is" ;
      capt += hdhC::tf_val(var.attName[att_ft_ix]) ;

      (void) notes->operate(capt) ;
      notes->setCheckStatus( n_CF, fail );
    }
  }

  if( att_sn_ix == -1 )
  {
    if( att_ft_ix > -1 && notes->inq(bKey + "432d", var.name) )
    {
      std::string capt(hdhC::tf_att(var.name, n_formula_terms));
      capt += "is available, but ";
      capt += n_standard_name + " is missing";

      (void) notes->operate(capt) ;
      notes->setCheckStatus( n_CF, fail );
    }
  }
  else
  {
    if( att_ft_ix == -1 )
    {
      if( notes->inq(bKey + "432a", var.name) )
      {
        std::string capt(hdhC::tf_var(var.name, hdhC::colon));
        capt += hdhC::tf_att(n_standard_name, var.std_name, hdhC::upper) ;
        capt += "suggests a missing " + hdhC::tf_att(n_formula_terms) ;

        (void) notes->operate(capt) ;
        notes->setCheckStatus( n_CF, fail );
      }

      if( units.size() == 0 || units == "1" ) // another evidence of dim-less Z
      {
        var.coord.isC[2] = true;
        var.coord.isZ_DL = true;
        var.addWeight(2);
        var.addAuxCount();
      }
    }
  }

  // no formula_terms attribute is given
  if( att_ft_ix == -1)
     return true;  // apparently no hybrid case

  return false;
}

void
CF::chap432_deprecatedUnits(Variable& var, std::string &units)
{
   // units level, layer and sigma_level are deprecated

  if( units == "level" || units == "layer" || units == "sigma_level" )
  {
    if( followRecommendations )
    {
      std::string tag(bKey+"31a");
      if( notes->inq(tag, var.name) )
      {
        std::string capt(n_reco + ": ");
        capt += hdhC::tf_att(var.name, n_units, units, hdhC::upper);
        capt += "is deprecated";

        (void) notes->operate(capt) ;
        notes->setCheckStatus( n_CF );
      }
    }

    units = "1" ;
  }

  return ;
}

void
CF::chap432_getParamVars( Variable& var,
   std::vector<std::string>& valid_sn,
   std::vector<std::string>& valid_ft,
   int& valid_ft_ix, int& valid_sn_ix,
   int att_ft_ix,
   std::vector<std::pair<std::string, std::string> >& att_ft_pv)
{
  // test for a valid ft att
  std::string& att_ft = var.attValue[att_ft_ix][0];
  Split x_att_ft(att_ft);

  // the check of the attribute appears cumbersome, because a faulty composition of
  // formula_terms should be recognised in detail (and annotated).
  std::vector<int> key_ix;
  std::vector<int> pv_ix;

  int sz=x_att_ft.size() ;
  for(int i=0 ; i < sz ; ++i )
  {
    std::string &s0 = x_att_ft[i] ;
    if( s0[s0.size()-1] == ':' )
      key_ix.push_back(i);
    else
      pv_ix.push_back(i);
  }

  // build a pair: 1st=key, 2nd=paramVar
  size_t kp_sz = key_ix.size() ;
  if( key_ix.size() < pv_ix.size() )
     kp_sz = pv_ix.size() ;

  for( size_t i=0 ; i < kp_sz ; ++i )
  {
    att_ft_pv.push_back( std::pair<std::string, std::string> (hdhC::empty,hdhC::empty) );

    if( i < key_ix.size() )
      att_ft_pv.back().first = x_att_ft[key_ix[i]] ;

    if( i < pv_ix.size() )
      att_ft_pv.back().second = x_att_ft[pv_ix[i]] ;
  }

  // just for passing the result
  bool isFault=false;
  if( key_ix.size() != pv_ix.size() )
    isFault=true;
  else
  {
    // test the sequence
    for( size_t i=0 ; i < key_ix.size() ; ++i )
    {
        if( !i &&  key_ix[i] != 0 )
          isFault=true;

        if( key_ix[i]+1 != pv_ix[i] )
          isFault=true;
    }
  }

  if( isFault )
    att_ft_pv.push_back( std::pair<std::string, std::string> (hdhC::empty,hdhC::empty) );

  // formula_terms attribute was found, now get the indices in the valid vectors.
  // This is more tricky for found_ft_ix than for found_sn_ix above.
  // Take into account the term: strings and
  // find the number of existing variables for each valid_ft element.
  std::vector<int> num;
  std::vector<Split> x_valid_ft;

  for( size_t i=0 ; i < valid_ft.size() ; ++i )
  {
     num.push_back( 0 );
     x_valid_ft.push_back( Split(valid_ft[i]) ) ;
  }

  // compare to the parameters provided by CF (appendix D)
  for( size_t i=0 ; i < att_ft_pv.size() ; ++i )
  {
     for( size_t j=0 ; j < valid_ft.size() ; ++j )
     {
       for( size_t k=0 ; k < x_valid_ft[j].size() ; ++k )
       {
         if( x_valid_ft[j][k] == att_ft_pv[i].first )
         {
            for( size_t iv=0 ; iv < pIn->varSz ; ++iv )
               if( pIn->variable[iv].name == att_ft_pv[i].second )
                 ++num[j];
         }
       }
     }
  }

  // sort indices according to the highest number
  size_t v_swap;
  std::vector<int> zx;
  sz=static_cast<int>(num.size());
  for( int i=0 ; i < sz ; ++i )
    zx.push_back(i);

  sz=static_cast<int>(zx.size());
  for( int i=0 ; i < sz-1 ; ++i )
  {
    for( int j=i+1 ; j < sz ; ++j )
    {
       if( num[zx[j]] >  num[zx[i]] )
       {
         v_swap = zx[i] ;
         zx[i] = zx[j];
         zx[j] = v_swap;
       }
    }
  }

  if( num[zx[0]] && num[zx[0]] == num[zx[1]] )
  {
     // could not identify unambiguously the valid_ft index. Thus, if one
     // of them matches valid_sn_ix, then take it.
     if( zx[0] == valid_sn_ix )
        valid_ft_ix = zx[0] ;
     else if( zx[1] == valid_sn_ix )
        valid_ft_ix = zx[1] ;
  }
  else
    valid_ft_ix = zx[0] ;

  return ;
}

void
CF::chap432_verify_FT(
  Variable& var,
  int valid_ft_ix,
  std::string &valid_ft,
  int att_ft_ix,
  std::vector<std::string> &fTerms,
  std::vector<std::pair<std::string, std::string> >& att_ft_pv)
{
  // formula_term contains 'terms:' and names of variables, perhaps omitting
  // particular terms.

  // Vector fTerms contains a list of term: strings.
  // Note that it will contain existing var-names on return.

  std::vector<std::string> valid_units;
  valid_units.push_back("Pa 1");
  valid_units.push_back("1 Pa Pa");
  valid_units.push_back("1 Pa 1 Pa");
  valid_units.push_back("m 1 m");
  valid_units.push_back("1 1 1 m m m");
  valid_units.push_back("1 m m");
  valid_units.push_back("1 m m 1 1 m");
  valid_units.push_back("1 m m m 1 m");
  valid_units.push_back("1 m m m 1 m 1");

  if( valid_ft_ix == 2 && att_ft_pv.size() == 3 )
    valid_units[valid_ft_ix] = "Pa 1 Pa" ;

  Split x_fUnits(valid_units[valid_ft_ix]);

  std::vector<std::string> assoc;
  std::vector<std::pair<std::string, std::string> > paramVarUnits;

  // additional empty pair indicating a faulty sequence in ft
  size_t pvSz = att_ft_pv.size() ;
  if( pvSz )
    if( att_ft_pv[pvSz-1].first.size() == 0 && att_ft_pv[pvSz-1].second.size() == 0 )
      --pvSz;

  if( isCheck )
  {
    for( size_t j=0 ; j < pvSz ; ++j )
    {
      int i;

      if( (i=pIn->getVarIndex(att_ft_pv[j].second)) == -1 )
      {
        if( notes->inq(bKey + "432g", var.name) )
        {
          std::string capt(hdhC::tf_att(var.name, n_formula_terms));
          capt += "declares undefined " + n_variable + " by" ;
          capt += hdhC::tf_val(att_ft_pv[j].first + hdhC::blank + att_ft_pv[j].second) ;

          (void) notes->operate(capt) ;
          notes->setCheckStatus( n_CF, fail );
        }

        continue;
      }

      // no variable declared by formula_terms is allowed
      // to have _FillValue occurrences.
      if( att_ft_pv[j].second == pIn->variable[i].name )
      {
        for( size_t k=0 ; k < fTerms.size() ; ++k )
        {
          if( fTerms[k] == att_ft_pv[j].first )
          {
            // check the units of the parameters
            int iu = pIn->variable[i].getAttIndex(n_units);
            if( iu == -1 || pIn->variable[i].attValue[iu][0] == "1" )
            {
              if( x_fUnits[k] != "1" && notes->inq(bKey + "432h", var.name) )
              {
                  std::string capt(hdhC::tf_att(var.name,n_formula_terms, hdhC::colon));
                  capt += "Auxiliary " ;
                  capt += hdhC::tf_var(pIn->variable[i].name, hdhC::colon);
                  capt += hdhC::tf_att(n_units) ;
                  capt += "require" + hdhC::tf_val(x_fUnits[k]) ;
                  capt += ", but found " + n_dimension + "less" ;

                  (void) notes->operate(capt) ;
                  notes->setCheckStatus( n_CF, fail );
              }
            }
            else
            {
              if( cmpUnits(pIn->variable[i].attValue[iu][0], x_fUnits[k]) )
                paramVarUnits.push_back(
                  std::pair<std::string, std::string>
                  (pIn->variable[i].name, pIn->variable[i].attValue[iu][0]) );
              else
              {
                if( notes->inq(bKey + "432h", var.name) )
                {
                  std::string capt(hdhC::tf_att(var.name, n_formula_terms, hdhC::colon));
                  capt += "Auxiliary ";
                  capt += n_variable ;
                  capt += hdhC::tf_val(pIn->variable[i].name, hdhC::blank) ;
                  capt += "has " ;
                  capt += hdhC::tf_assign(n_units, pIn->variable[i].attValue[iu][0]) ;
                  if( x_fUnits[k] == "1" )
                    capt += ", but " + n_dimension + "less is required";
                  else
                  {
                    capt += ", but" ;
                    capt += hdhC::tf_val(x_fUnits[k], hdhC::blank) ;
                    capt += "is required";
                  }

                  (void) notes->operate(capt) ;
                  notes->setCheckStatus( n_CF, fail );
                }
              }
            }

            break;
          }
        }
      }
    }

    // here after having looped through the fT att parameters
    if( paramVarUnits.size() > 1 )
    {
       for( size_t k=1 ; k < paramVarUnits.size() ; ++k )
       {
         if( paramVarUnits[k].second != paramVarUnits[0].second )
         {
           if( notes->inq(bKey + "432i", var.name) )
           {
             std::string capt("warning for ") ;
             capt += hdhC::tf_att(var.name, n_formula_terms, hdhC::colon);
             capt += "Units of auxiliary " + n_variable + "s";
             capt += hdhC::tf_val(paramVarUnits[0].first, hdhC::blank) ;
             capt += "and";
             capt += hdhC::tf_val(paramVarUnits[k].first, hdhC::blank);
             capt += "should be identical";
             std::string text("Found ");
             text += hdhC::tf_val(paramVarUnits[0].second, hdhC::blank) ;
             text += "vs." ;
             text += hdhC::tf_val(paramVarUnits[k].second) ;

             (void) notes->operate(capt, text) ;
             notes->setCheckStatus( n_CF, fail );
           }

           break;
         }
       }
    }

    for(size_t i=0 ; i < att_ft_pv.size() ; ++i )
    {
      if( att_ft_pv[i].first.size() == 0 || att_ft_pv[i].second.size() == 0 )
      {
        // found a syntax error in formula_terms
        if( notes->inq(bKey + "432c", var.name) )
        {
          std::string t;
          Split x_valid_ft(valid_ft);
          for( size_t k=0 ; k < x_valid_ft.size() ; ++k )
          {
            if(k)
              t += hdhC::blank;
            t += x_valid_ft[k] + " v" + hdhC::itoa(k);
          }

          std::string capt(hdhC::tf_att(var.name, n_formula_terms)) ;
          capt += "with syntax error";
          std::string text("Expected " + t) ;
          text += ", found" + hdhC::tf_val(var.attValue[att_ft_ix][0]) ;

          (void) notes->operate(capt, text) ;
          notes->setCheckStatus( n_CF, fail );
        }

        break;
      }
    }
  }

  fTerms.clear();
  for(size_t i=0 ; i < att_ft_pv.size() ; ++i )
  {
    if( att_ft_pv[i].second.size() )
    {
       // any non-existing variable in the formula_terms string?
       int j;
       if( (j=pIn->getVarIndex(att_ft_pv[i].second)) > -1 )
         fTerms.push_back(att_ft_pv[i].second);
    }
  }

  // fTerms contains now only associated variables.

  return ;
}

bool
CF::chap44(Variable& var)
{
  bool isUnits;
  bool isName=false;

  // test for time units and verify a correct format
  if( (isUnits=timeUnitsFormat(var)) )
  {
    isName = true; // works for any name

    // calendar and related stuff
    if( chap441(var) )
    {
      var.addAuxCount();
      var.addWeight(3);
    }
  }
  else
  {
    if( var.name == n_time
         || var.getAttValue(n_standard_name, lowerCase) == n_time )
      isName = true ;
    else
    {
      std::vector<std::string> timeAtt;
      timeAtt.push_back(n_calendar);
      timeAtt.push_back(n_month_lengths);
      timeAtt.push_back(n_leap_year);
      timeAtt.push_back(n_leap_month);

      // particular for some attributes
      if( isCheck )
      {
        for( size_t j=0 ; j < timeAtt.size() ; ++j )
        {
          if( var.isValidAtt( timeAtt[j]) )
          {
            if( notes->inq(bKey + "44d", var.name) )
            {
              std::string capt("warning for "+ hdhC::tf_var(var.name, hdhC::colon));
              capt += hdhC::tf_att(timeAtt[j], hdhC::upper);
              capt += "may only be attached to the time coordinate";

              (void) notes->operate(capt) ;
              notes->setCheckStatus( n_CF, fail );
            }
          }
        }
      }
    }
  }

  if( isName )
  {
    timeName = var.name;
    time_ix = var.getVarIndex();

    var.coord.isC[3]=true;
    var.isChecked=true;
    var.addAuxCount();
    var.addWeight(3);

    if( !isUnits )
    {
      std::string tag(bKey + "44a");
      if( notes->inq(tag, var.name) )
      {
        std::string capt(hdhC::tf_var(var.name));
        capt += "was rated a T-coordinate, but ";
        capt += hdhC::tf_att(n_units) + "is missing";

        (void) notes->operate(capt) ;
        notes->setCheckStatus( n_CF, fail );
      }
    }

    return true ;
  }

  return false;
}

void
CF::chap44a_reco(Variable& var)
{
   // having called this method means that a ref-date 0-1-1 was found
   // note that omission of calendar results in proleptic gregorian
   for( size_t i=0 ; i < var.attName.size() ; ++i )
   {
      if( var.attName[i] == n_calendar
         && hdhC::Upper()(var.attValue[i][0]) == "NONE" )
        return;
   }

   if( notes->inq(bKey + "44c", var.name) )
   {
      std::string capt(hdhC::tf_var(var.name, hdhC::colon));
      capt += "Indication of climatological time by " ;
      capt += "reference time in the year 0-1-1 is deprecated" ;

      (void) notes->operate(capt) ;
      notes->setCheckStatus( n_CF, fail );
   }

   return;
}

bool
CF::chap441(Variable& var)
{
  // calendar and related stuff
  std::vector<std::string> def_cal;

  def_cal.push_back("gregorian");
  def_cal.push_back("standard");
  def_cal.push_back("proleptic_gregorian");
  def_cal.push_back("noleap");
  def_cal.push_back("365_day");
  def_cal.push_back("all_leap");
  def_cal.push_back("366_day");
  def_cal.push_back("360_day");
  def_cal.push_back("julian");
  def_cal.push_back("none");

  std::string v_cal( var.getAttValue(n_calendar, lowerCase) );

  std::string str;

  if( v_cal.size() == 0 )
  {
     if( ! var.isValidAtt(n_month_lengths) )
       v_cal = def_cal[0] ;  // by default
  }
  else if( v_cal == "360" || v_cal == "365" || v_cal == "366" )
  {
     str=v_cal;
     v_cal += "_day" ;
  }
  else if( v_cal.find("days") < std::string::npos )
  {
     str=v_cal;
     v_cal = v_cal.substr(0, v_cal.size()-1) ;
  }

  if( str.size() )
  {
    if( notes->inq(bKey + "441g", var.name) )
    {
      std::string capt(hdhC::tf_att(var.name, n_calendar, str));
      capt += "is misspelled, did you mean" + hdhC::tf_val(v_cal) + "?" ;

      (void) notes->operate(capt) ;
      notes->setCheckStatus( n_CF, fail );
    }
  }

  bool isTime = v_cal.size();

  if( !isCheck )
    return isTime;

  if( v_cal.size() )
  {
     if( hdhC::isAmong(v_cal, def_cal) )
        return isTime;
  }

  // calendar is non-standard or not defined.

  // --> month_lengths is required with 12 values
  int ml;
  if( (ml=var.getAttIndex(n_month_lengths)) > -1 )
  {
    if( var.attValue[ml].size() != 12 )
    {
      if( notes->inq(bKey + "441a", var.name) )
      {
        std::string capt(hdhC::tf_att(var.name, n_month_lengths)) ;
        capt += "requires 12 values";
        std::string text("Found ") ;
        text += hdhC::tf_val(hdhC::getVector2Str(var.attValue[ml])) ;

        (void) notes->operate(capt, text) ;
        notes->setCheckStatus( n_CF, fail );
      }
    }
  }
  else if( v_cal.size() && notes->inq(bKey + "441d", var.name) )
  {
    std::string capt(hdhC::tf_var(var.name, hdhC::colon)) ;
    capt += "Non-standard calendar" + hdhC::tf_val(v_cal, hdhC::blank);
    capt += "requires " + hdhC::tf_att(n_month_lengths) ;

    (void) notes->operate(capt) ;
    notes->setCheckStatus( n_CF, fail );
  }

  // if att=leap_month is defined, then leap_year is required
  bool isLMV=false;

  int i;
  if( (i=var.getAttIndex(n_leap_month)) > -1 )
  {
    isLMV=true;

    std::string capt;
    std::string text;

    // value must be in the range 1-12
    if( var.attValue[i][0].size() )
    {
      double dv=hdhC::string2Double( var.attValue[i][0] ) ;
      int iv = static_cast<int>(dv);

      if(! (iv > 0 && iv < 13) )
      {
        capt = hdhC::tf_att(var.name, n_leap_month) ;
        capt += "requires a value in the range 1-12";

        text="Found ";
        text += hdhC::tf_val(var.attValue[i][0]);
      }
    }
    else  // empty attribute
    {
      capt = hdhC::tf_att(var.name, n_leap_month);
      capt += "requires a value in the range 1-12";
    }

    if( capt.size() && notes->inq(bKey + "441c", var.name) )
    {
      (void) notes->operate(capt, text) ;
      notes->setCheckStatus( n_CF, fail );
    }
  }

  // chap441_reco()
  if( followRecommendations && isLMV && !var.isValidAtt(n_leap_year) )
  {
    if( notes->inq(bKey + "441b", var.name) )
    {
      std::string capt("warning: " + hdhC::tf_att(var.name, n_leap_month, hdhC::upper));
      capt += "is ignored because " + n_leap_year + " is not specified" ;

      (void) notes->operate(capt) ;
      notes->setCheckStatus( n_CF, fail );
    }
  }

  return isTime;
}

void
CF::chap5(void)
{
  // coordinate systems
  if( ! notes->inq(bKey + "5", hdhC::empty, "INQ_ONLY" ) )
    return ;  // checks are selected to be discarded by 'D'

  // find coordinate variables
  // chap51(Variable&) ; // applied in run() for convenience

  // find 'coordinates' attribute(s)
  chap52() ;

  // reduced horizontal grid
  // chap53() ;  see chap82()

  // time series of station data
//  if( cFVal < 16 )
//    chap54() ;

  // chap55 (trajectories): checked implicitely

  // grid mapping
  chap56();

  // scalar coordinate was checked in run()

  return ;
}

void
CF::chap5_reco(void)
{
  size_t ix;

  for( ix=0 ; ix < pIn->varSz ; ++ix )
  {
    Variable& var = pIn->variable[ix] ;

    // the name of multi-coord-var should not match the name of any of
    // its dimensions.

    if( var.isScalar )
    {
      for( size_t i=0 ; i < dimensions.size() ; ++i )
      {
         if( var.name == dimensions[i] )
         {
           if( notes->inq(bKey + "57a", var.name) )
           {
             std::string capt(n_reco + ":: Scalar ");
             capt += hdhC::tf_var(var.name) ;
             capt += "should not match the name of " + n_dimension ;
             capt += hdhC::tf_val(dimensions[i]) ;

             (void) notes->operate(capt) ;
             notes->setCheckStatus( n_CF );
           }
        }
      }
    }
    else
    {
      size_t sz=var.dimName.size();
      if( var.isLabel && sz > 1 )
        --sz;

      if( sz > 1 )
      {
        for( size_t i=0 ; i < sz ; ++i )
        {
          if( var.name == var.dimName[i] )
          {
            if( notes->inq(bKey + "5c", var.name) )
            {
              std::string capt(n_reco + ":: Multi-" + n_dimension + "al ");
              capt += hdhC::tf_var(var.name) ;
              capt += "should not match the name of any of its " + n_dimension + "s" ;

              (void) notes->operate(capt) ;
              notes->setCheckStatus( n_CF );
            }
          }
        }
      }
    }


  }

  return;
}

void
CF::chap51(Variable& var)
{
  // Coordinate variables: a single dimension and identical name for
  // dimension and variable, e.g. time(time).

  if( var.dimName.size() == 1 && var.dimName[0] == var.name )
  {
     var.coord.isCoordVar=true;
     var.addAuxCount(10);

     // test for axis, because this woould not be set for
     // e.g. cartesian coord would not

     int j;
     if( (j=var.getAttIndex(n_axis)) > -1 )
     {
       char ax = var.attValue[0][0][0] ;

       if( toupper(ax) == 'X' )
         var.coord.isC[0]=true;
       else if( toupper(ax) == 'Y' )
         var.coord.isC[1]=true;
       else if( toupper(ax) == 'Z' )
         var.coord.isC[2]=true;
       else if( toupper(ax) == 'T' )
         var.coord.isC[3]=true;
     }
  }

  return ;
}

void
CF::chap52(void)
{
  // two-dimensional coordinates lon/lat
  for( size_t ix=0 ; ix < pIn->varSz ; ++ix )
  {
    Variable& var = pIn->variable[ix];

    if( var.isChecked )
      continue ;

    if( ! (ca_vvs.size() && ca_vvs[ix].size()) )
      continue;

    std::vector<std::string>& ca_vs = ca_vvs[ix];
    int ii;
    for(size_t k=0 ; k < ca_vs.size() ; ++k )
    {
      // It is permissible to have coordinate variables included.
      if( (ii=pIn->getVarIndex(ca_vs[k])) > -1 )
      {
        pIn->variable[ii].addAuxCount();
        var.push_aux(pIn->variable[ii].name);

        if( ! pIn->variable[ii].coord.isCoordVar )
        {
          std::string su( units_lon_lat(var) );

          if( su == n_latitude )
            ++pIn->variable[ii].coord.weight[1];
          else if( su == n_longitude )
            ++pIn->variable[ii].coord.weight[0];
          else if( su == n_grid_latitude )
            ++pIn->variable[ii].coord.weight[1];
          else if( su == n_grid_longitude )
            ++pIn->variable[ii].coord.weight[0];
          else
          {
             // a standard name could help
             if( pIn->variable[ii].getAttValue(n_standard_name, lowerCase)
                     == n_latitude )
               ++pIn->variable[ii].coord.weight[1];
             else if( pIn->variable[ii].getAttValue(n_standard_name, lowerCase)
                     == n_longitude )
               ++pIn->variable[ii].coord.weight[0];
          }
        }
      }
    }
  }

  // Any aux coord without coordinates attribute?
  // Look for 2-D vars with lon/lat units.
  for( size_t ix=0 ; ix < pIn->varSz ; ++ix )
  {
    Variable& var2 = pIn->variable[ix];

    if( var2.coord.isCoordVar )
       continue;

    size_t sz;
    if( (sz=var2.dimName.size()) == 2 && !var2.isUnlimited() )
    {
      int cnt=0;
      int ii;
      for( size_t i=0 ; i < sz ; ++i )
        if( (ii=pIn->getVarIndex(var2.dimName[i])) > -1
            && ( pIn->variable[ii].coord.isC[1]
                  || pIn->variable[ii].coord.isC[1] ) )
              ++cnt;

      if( cnt == 2 )
      {
         var2.addAuxCount() ;

         std::string su( units_lon_lat(var2) );
         if( su == n_latitude || su == n_grid_latitude )
           ++var2.coord.weight[1];
         else if( su == n_longitude || su == n_grid_longitude )
           ++var2.coord.weight[0];

         continue ;
      }
    }
  }

  return;
}

void
CF::chap56(void)
{
  // data var, grid_mapping attribute value == grid_mapping variable
  std::vector<std::pair<std::string, std::string> > dv_gmv;

  std::string mapCoord[2];

  // scan for attribute 'grid_mapping'
  for( size_t ix=0 ; ix < pIn->varSz ; ++ix )
  {
    Variable& var = pIn->variable[ix];

    // look for a data variable with a grid_mapping attribute
    int j;

    if( (j=var.getAttIndex(n_grid_mapping)) > -1 )
    {
       std::string& str = var.attValue[j][0] ;

       if( str.size() == 0 )
         continue;

       size_t j;
       for( j=0 ; j < dv_gmv.size() ; ++j )
          if( dv_gmv[j].second == str )
             break;

       if( j == dv_gmv.size() )
         dv_gmv.push_back(
            std::pair<std::string, std::string> (var.name, str)) ;

       int retVal;
       bool isMapping=true;

       if( ! (retVal=chap56_gridMappingVar(var, str, "latitude_longitude")) )
       {
         isMapping=false;
       }
       else if( ! (retVal=chap56_gridMappingVar(var, str, "rotated_latitude_longitude")) )
       {
         mapCoord[0] = n_grid_longitude ;
         mapCoord[1] = n_grid_latitude ;
       }
       else if( ! (retVal=chap56_gridMappingVar(var, str, hdhC::empty)) )
       {
         mapCoord[0] = "projection_x_coordinate" ;
         mapCoord[1] = "projection_y_coordinate" ; // value for most methods
       }
       else
       {
         if( retVal == 1 )
         {
           if( str.size() && notes->inq(bKey + "56c") )
           {
             std::string capt(hdhC::tf_att(var.name, n_grid_mapping));
             capt += "names non-existing ";
             capt += hdhC::tf_var(var.getAttValue(n_grid_mapping));

             (void) notes->operate(capt) ;
             notes->setCheckStatus( n_CF, fail );
           }
         }

         else if( retVal == 2 && notes->inq(bKey + "56f") )
         {
           std::string capt(hdhC::tf_var(str, hdhC::colon));
           capt += "Missing " ;
           capt += hdhC::tf_att(n_grid_mapping +"_name");

           (void) notes->operate(capt) ;
           notes->setCheckStatus( n_CF, fail );
         }

         else if( retVal > 2 && notes->inq(bKey + "56g") )
         {
           // index==retVal-3 takes back an action done in chap56_gridMappingVar()
           std::string gmn(n_grid_mapping +"_name");

           std::string capt("grid mapping " + hdhC::tf_var(str) );
           capt += "with undefined " + gmn ;
           capt += hdhC::tf_val(pIn->variable[retVal-3].getAttValue(gmn));

           (void) notes->operate(capt) ;
           notes->setCheckStatus( n_CF, fail );
         }

         continue;
       }

       // coordinates attribute issues
       if( isMapping )
         chap56_attProps(var, mapCoord) ;
    }
  }

  // look for an existing grid-mapping variables which has not been declared
  // by a grip_mapping attribute of a data variable.
  for( size_t i=0 ; i < pIn->varSz ; ++i )
  {
    Variable& var = pIn->variable[i];

    if( var.isChecked )
       continue;

    // exclude previously found grid mapping variables
    size_t j;
    for( j=0 ; j < dv_gmv.size() ; ++j )
       if( dv_gmv[j].second == var.name )
         break;

    if( j == dv_gmv.size() )
    {
      if( var.isValidAtt(n_grid_mapping + "_name") )
      {
        if( notes->inq(bKey + "56e", var.name) )
        {
          std::string capt("found a detached grid-mapping ") ;
          capt += hdhC::tf_var(var.name, hdhC::no_blank);
          capt += ", but a variable with " + hdhC::tf_att(n_grid_mapping) + "is missing" ;

          (void) notes->operate(capt) ;
          notes->setCheckStatus( n_CF, fail );
        }

        if( var.isNoData() )
        {
          var.countData=0;
          var.addAuxCount(10);
        }
      }
    }
  }

  return;
}

int
CF::chap56_gridMappingVar(Variable& var, std::string &s, std::string gmn)
{
   // Param s: the grid_mapping attribute of the data variable.
   //     gmn: name of a particular grid mapping method

   // return: 0: found grid_mapping variable with valid grid_mapping_name
   //         1: found no grid_mapping variable
   //         2: found grid_mapping variable with missing grid_mapping_name
   //    3 + ix: found grid_mapping variable with invalid grid_mapping_name


   // find grid_mapping variable gmn
   // and check existence of grid_mapping_name att
   int ix = pIn->getVarIndex(s);  // index of the grid mapping variable

   int retVal=1;

   if( ix > -1 )
   {
     Variable& var_gmv = pIn->variable[ix] ;
     var_gmv.isMapVar=true;
     var_gmv.addAuxCount();
     var.push_aux(var_gmv.name);

     retVal=2;

     if( var_gmv.isValidAtt(n_grid_mapping + "_name") )
     {
        // the grid mapping variabel must have attribute grid_mapping_name
        std::string str( var_gmv.getAttValue(n_grid_mapping +"_name") );

        if( str.size() )
        {
          var_gmv.isVoid=true;  // no data

          if( gmn.size() == 0 )
          {
            // all methods defined in CF
            std::vector<std::string> vs_gmn;
            vs_gmn.push_back("albers_conical_equal_area");
            vs_gmn.push_back("azimuthal_equidistant");
            vs_gmn.push_back("lambert_azimuthal_equal_area");
            vs_gmn.push_back("lambert_conformal_conic");
            vs_gmn.push_back("lambert_cylindrical_equal_area");
            vs_gmn.push_back("latitude_longitude");
            vs_gmn.push_back("mercator");
            vs_gmn.push_back("orthographic");
            vs_gmn.push_back("polar_stereographic");
            vs_gmn.push_back("rotated_latitude_longitude");
            vs_gmn.push_back("stereographic");
            vs_gmn.push_back("transverse_mercator");
            vs_gmn.push_back("vertical_perspective");

            if( hdhC::isAmong(str, vs_gmn) )
              gmn = str;
          }

          retVal = (str == gmn) ? 0 : (3+ix);
        }
        else
          retVal = 0 ;  // caught elsewhere
     }

     // recommendation: grid mapping variable should not have dimensions
     if( followRecommendations && var_gmv.dimName.size() )
     {
        if( notes->inq(bKey + "56d", var_gmv.name, NO_MT) )
        {
          std::string capt(n_reco + ":: Grid_mapping-") ;
          capt += hdhC::tf_assign(n_variable, var_gmv.name) ;
          capt += " should not have " + n_dimension + "s" ;

          (void) notes->operate(capt) ;
          notes->setCheckStatus( n_CF );
        }
     }
   }

   return retVal;
}

void
CF::chap56_attProps(
  Variable& dataVar,       // variable with a grid_mapping attribute
  std::string mCV[] )      // required std_names of coord vars
{
   // only called when not latitude_longitude, i.e. there is a mapped grid-lon/grid-lat
   // and a true lon/lat.

   // the coordinates att of the data var must exist
   if( ! dataVar.isValidAtt(n_coordinates) )
   {
     if( notes->inq(bKey + "5i", dataVar.name, NO_MT) )
     {
        std::string capt(hdhC::tf_var(dataVar.name, hdhC::colon));
        capt += "Missing coordinates attribute" ;
        std::string text("When the coordinate variables for a horizontal grid are not longitude and latitude, it is required that the true latitude and longitude coordinates be supplied via the coordinates attribute.");

        (void) notes->operate(capt, text) ;
        notes->setCheckStatus( n_CF, fail );
     }
   }

   // each name in mCV should have a corresponding standard_name
   // in a coordinate variable.
   bool is_sn[] = {true, true};

   for( size_t i=0 ; i < pIn->varSz ; ++i )
   {
     // a matching standard_name between a variable and the one of
     // the grid mapping procedure is used to identify the associated
     // coordinate variable.
     Variable& var = pIn->variable[i] ;

     std::string curr_sn( var.getAttValue(n_standard_name, lowerCase) );
     if( curr_sn.size() )
     {
       for( size_t l=0 ; l < 2 ; ++l )
       {
         if( curr_sn == mCV[l] )
         {
           is_sn[l]=false;
           var.coord.isC[l]=true;
           var.addWeight(l);
         }
       }

       if( !is_sn[0] && !is_sn[1] )
         break;
     }
   }

   if( !isCheck )
     return;

   // required standard_names not supported by any variable's standard-name
   for( size_t l=0 ; l < 2 ; ++l )
   {
     if( is_sn[l] )
     {
       // standard_name requirements
       if( notes->inq(bKey + "56b") )
       {
         std::string capt("grid mapping: missing ");
         capt += hdhC::tf_assign(n_standard_name, mCV[l]) ;
         std::string text("Perhaps a missing map coordinate.") ;

         (void) notes->operate(capt, text) ;
         notes->setCheckStatus( n_CF, fail );
       }
     }
   }

   return;
}

void
CF::chap6(void)
{
  // labels
  // a) char-type
  // b) and dim-size > 0 && with a dim which is unique across all variables
  if( ! notes->inq(bKey + "6", hdhC::empty, "INQ_ONLY" ) )
    return ;  // checks are selected to be discarded by 'D'

  std::vector<size_t> label_ix;

  for( size_t i=0 ; i < pIn->varSz ; ++i )
  {
     Variable& var = pIn->variable[i];

     if( var.type == NC_CHAR )
     {
        var.isLabel=true;
        label_ix.push_back(i);
     }
  }

  if( label_ix.size() == 0 )
     return;

  // auxiliary coordinates declared by labels
  std::vector<std::string> vs_lbl_aux;
  std::vector<bool> isNoException;

  for( size_t i=0 ; i < label_ix.size() ; ++i )
  {
    Variable& label = pIn->variable[label_ix[i]];
    vs_lbl_aux.push_back(hdhC::empty);
    isNoException.push_back(true);

    for( size_t j=0 ; j < pIn->varSz ; ++j )
    {
      Variable& var = pIn->variable[j];

      if( label.name == var.name )
        continue;

      if( var.coord.isCoordVar || ! var.isCoordinate() )
        continue;

      if( var.isScalar && label.isScalar )
        vs_lbl_aux[i] += hdhC::blank + var.name ;
      else if( var.dimName.size() == 1 && label.dimName.size() > 1 )
      {
        if( var.dimName[0] == label.dimName[0] )
          vs_lbl_aux[i] += hdhC::blank + var.name ;
      }
    }
  }

  // declared here because of the scope
  std::vector<std::string> vs_region_table;

  // any label not named in a coordinates attribute?
  for( size_t l=0 ; l < label_ix.size() ; ++l )
  {
    if( !isNoException[l] )
      continue;

    Variable& label = pIn->variable[label_ix[l]];

    for( size_t i=0 ; i < pIn->varSz ; ++i )
    {
      if( i == label_ix[l] )
         continue;

      Variable& var = pIn->variable[i];

      int j;

      if( (j=var.getAttIndex(n_grid_mapping)) > -1 )
        if( var.attValue[j][0] == label.name )
           isNoException[l] = false;
    }

    for( size_t c=0 ; c < ca_pij.size() ; ++c)
    {
      // A label must have one or two dimensions.
      // The maximum string-length cannot be checked

      // If the label is not a scalar auxiliary type one , then the first dimension
      // must match one of the dims of the data variable
      if( label.dimName.size() == 2 )
      {
        Variable& dv = pIn->variable[ca_pij[c].first] ;

        if( ! hdhC::isAmong(label.dimName[0], dv.dimName)
              && ! isChap9_specialLabel(label, dv) )
        {
          if( notes->inq(bKey + "6a", label.name, NO_MT) )
          {
             std::string capt("non-scalar label " + hdhC::tf_var(label.name));
             capt += "does not share a " + n_dimension ;
             capt += " with associated " + hdhC::tf_var(dv.name);

             std::string text("Label: ");
             text += label.getDimNameStr(true);
             text += "\n " + n_variable + ": ";
             text += dv.getDimNameStr(true);

             (void) notes->operate(capt, text) ;
             notes->setCheckStatus( n_CF, fail );
          }
        }
      }
    }

    // check standardized-region-names. Note that it is possible that
    // neither a standard_name nor any data is provided
    bool isRegion=false;
    int j = label.getAttIndex(n_standard_name) ;
    if( j > -1 && label.attValue[j][0] == "region" )
      isRegion=true;

    std::vector<std::string> vs_region_file;

    if( label.type == NC_CHAR )
    {
      pIn->nc.getData(vs_region_file, label.name);

      for( size_t f=0 ; f < vs_region_file.size() ; ++f)
      {
        if( vs_region_file[f].size() )
          vs_region_file[f] = hdhC::Lower()(vs_region_file[f]);
        else
          vs_region_file.clear();
      }
    }

    // only read once
    if( ! vs_region_table.size() && region_table.is() )
    {
      ReadLine ifs( region_table.getFile() ) ;

      if( ifs.isOpen() )
      {
        std::string line;
        while( ! ifs.getLine(line) )
          vs_region_table.push_back(line);

        ifs.close();
      }
      else
      {
        if( notes->inq(bKey + "0t") )
        {
          std::string capt("fatal: the CF-standardized-region-names table could not be opened");

          (void) notes->operate(capt) ;
          notes->setCheckStatus( n_CF, fail );
        }
      }
    }

    // any standardized-region-name available?
    if( vs_region_file.size() )
    {
      if( vs_region_table.size() )
      {
        bool isMatch;

        // a string from the table matches one in the file
        if( (isMatch=hdhC::isAmong(vs_region_file, vs_region_table))
                && ! isRegion )
        {
          if( notes->inq(bKey + "6b", label.name) )
          {
              std::string capt(hdhC::tf_var(label.name, hdhC::colon));
              capt += "A label taken from table ";
              capt += std_name_table.filename ;
              capt += " requires " + n_standard_name + "=region";

              (void) notes->operate(capt) ;
              notes->setCheckStatus( n_CF, fail );
          }
        }

        else if( !isMatch && isRegion )
        {
          if( notes->inq(bKey + "6c", label.name) )
          {
            std::string capt("label " + hdhC::tf_var(label.name, hdhC::colon));
            capt += hdhC::tf_att(hdhC::empty, n_standard_name, hdhC::upper) ;
            capt += hdhC::tf_val("region", hdhC::blank) ;
            capt += "requires labels taken from";
            capt += hdhC::tf_val(std_name_table.filename) ;
            std::string text("Found ") ;
            for(size_t f=0 ; f < vs_region_file.size() ; ++f )
            {
              if( !hdhC::isAmong(vs_region_file[f], vs_region_table) )
              {
                if(f)
                  text += ",";
                text += hdhC::tf_val(vs_region_file[f]) ;
              }
            }

            (void) notes->operate(capt, text) ;
            notes->setCheckStatus( n_CF, fail );
          }
        }
      }
    }

    // vs_region_file is empty
    else if(isRegion)
    {
      if( notes->inq(bKey + "6d", label.name) )
      {
          std::string capt("label " + hdhC::tf_var(label.name, hdhC::colon));
          capt += hdhC::tf_att(hdhC::empty, n_standard_name, "region", hdhC::upper) ;
          capt += "is specified, but data, i.e. labels, are missing";

          (void) notes->operate(capt) ;
          notes->setCheckStatus( n_CF, fail );
      }
    }
  }

  return ;
}

void
CF::chap7(void)
{
  // cells
  if( ! notes->inq(bKey + "7", hdhC::empty, "INQ_ONLY" ) )
    return ;  // checks are selected to be discarded by 'D'

  // cell boundaries
  chap71() ;

  // cell measures
  chap72() ;

  // cell methods
  chap73() ;

  // climatological statistics
  chap74a() ;

  return ;
}

void
CF::chap71(void)
{
  if( !isCheck )
    return ;

  // this pertains to both cell boundaries and climatologies
  std::string units;

  // if method isBounds() detects bounds, then it sets:
  // var.boundsOf = pIn->variable[...].name ;
  // pIn->variable[...].bounds = var.name ;

  // look for some indices
  // first: variable.boundsOf, i.e. is bound
  // second: variable.bounds, i.e. has bounds
  std::vector<std::pair<int, int> > ix;

  // Is a variable declared as bounds|climatology of another variable? Yes, then
  // corresponding variable members are 'boundsOf' and 'bounds', repectively.
  int sz = static_cast<int>(pIn->varSz);

  for( int i=0 ; i < sz ; ++i )
  {
    // find coordinate variables with a bound or climatology declaration.
    if( isBounds(pIn->variable[i]) )
       ix.push_back( std::pair<int, int>
                     (i, pIn->getVarIndex(pIn->variable[i].boundsOf) ) );
  }

  // analyse matching pairs
  for( size_t i=0 ; i < ix.size() ; ++i )
  {
    Variable& var_is   = pIn->variable[ix[i].first];
    Variable& var_has  = pIn->variable[ix[i].second];

    // the bounds-var must have the same dimensions as its associated
    // variable plus a trailing one indicating the maximum number of vertices.

    // potential faults:
    // the additional dim for vertices is missing,
    // the sub-set of dims is different from the associated variable,
    // too many dims, if auxiliary coordinate and coordinate variable are mistaken.

    // note: a misplaced climatology attribute will be detected later, thus
    // skip such a case.
    if( var_is.isClimatology )
    {
      if( var_has.name != timeName )
        continue;
    }

    size_t sz_is = var_is.dimName.size() ;
    size_t sz_has = var_has.dimName.size() ;
    size_t sz_scalar=0;

    if( sz_has == 0 )
    {
       if( pIn->nc.isDimValid(var_has.name)
              && pIn->nc.getDimSize(var_has.name) == 1 )
          sz_scalar=1;
    }

    bool isMissingVerticesDim=false;
    bool hasTooManyVerticesDim=false;
    bool isNotIdenticalSubSet=false;

    size_t sz = ( sz_is < sz_has ) ? sz_is : sz_has ;

    size_t v;
    for( v=0 ; v < sz ; ++v )
        if( var_is.dimName[v] != var_has.dimName[v] )
          break;

    if( !( v == sz && (v+1+sz_scalar) == sz_is ) )
    {
      if( v < sz )
        isNotIdenticalSubSet=true;
      else if( sz_is <= sz_has )
        isMissingVerticesDim=true;
      else if( sz_is > sz_has )
        hasTooManyVerticesDim=true;
    }

    if( isNotIdenticalSubSet )
    {
      if( notes->inq(bKey + "71a", var_is.name) )
      {
        std::string capt;
        if( var_is.isClimatology )
          capt = n_climatology;
        else
          capt = n_bounds ;
        capt += "-" + hdhC::tf_var(var_is.getDimNameStr(true));
        capt += "should share the set of " + n_dimension ;
        capt += "s of the associated " +  n_variable;
        capt += hdhC::tf_val(var_has.getDimNameStr(true)) ;

        std::string text("Found ");
        text += var_is.getDimNameStr(true);
        text += " vs. " + var_has.getDimNameStr(true);

        (void) notes->operate(capt, text) ;
        notes->setCheckStatus( n_CF, fail );
      }
    }

    else if( isMissingVerticesDim )
    {
      if( notes->inq(bKey + "71b", var_is.name) )
      {
        std::string capt;
        if( var_is.isScalar )
          capt = "scalar ";

        if( var_is.isClimatology )
          capt += n_climatology;
        else
          capt += n_bounds ;
        capt += "-" + hdhC::tf_var(var_is.getDimNameStr(true));
        capt += "is missing the ";

        if( !var_is.isScalar )
          capt += "additional ";
        capt += n_dimension + " for the vertices" ;

        std::string text("Found ");
        text += var_is.getDimNameStr(true);
        text += " vs. " + var_has.getDimNameStr(true);

        (void) notes->operate(capt, text) ;
        notes->setCheckStatus( n_CF, fail );
      }
    }

    else if( hasTooManyVerticesDim )
    {
      if( notes->inq(bKey + "71d", var_is.name) )
      {
        std::string capt;
        if( var_is.isClimatology )
          capt = n_climatology;
        else
          capt = n_bounds ;
        capt += "-" + hdhC::tf_var(var_is.getDimNameStr(true), hdhC::colon);
        capt += "Dimensions are incompatible with the associated " ;
        capt += hdhC::tf_var(var_has.getDimNameStr(true)) ;

        (void) notes->operate(capt) ;
        notes->setCheckStatus( n_CF, fail );
      }
    }

    // a boundary variable must have a numeric type
    if( var_is.type == NC_CHAR )
    {
      if( notes->inq(bKey + "71c", var_is.name) )
      {
        std::string capt;
        if( var_is.isClimatology )
          capt = n_climatology;
        else
          capt = n_bounds ;
        capt += hdhC::blank + hdhC::tf_var(var_is.name);
        capt += "should be numeric type" ;

        std::string text("Found type=" );
        text += pIn->nc.getVarTypeStr(var_is.name);

        (void) notes->operate(capt, text) ;
        notes->setCheckStatus( n_CF, fail );
      }
    }

    bool isFault[] = { false, false} ;

    if( var_is.isValidAtt(n_units) )
    {
       std::string units_is( var_is.getAttValue(n_units) );
       std::string units_has( var_has.getAttValue(n_units) );

       if( units_is != units_has )
         isFault[0]=true;
    }

    if( var_is.isValidAtt(n_standard_name) )
    {
       std::string sn_is( var_is.getAttValue(n_standard_name) );
       std::string sn_has( var_has.getAttValue(n_standard_name) );

       if( sn_is != sn_has )
         isFault[1]=true;
    }

    for( size_t f=0 ; f < 2 ; ++f )
    {
       if( isFault[f] )
       {
         std::string tag;
         std::string* att;
         if( f )
         {
           tag = "71e";
           att = &n_standard_name ;
         }
         else
         {
           tag = "71f";
           att = &n_units ;
         }

         if( notes->inq(bKey + tag , var_is.name) )
         {
           std::string capt(
                hdhC::tf_att(var_is.name, *att, var_is.getAttValue(*att))) ;
           capt +=  + "and " ;
           capt += hdhC::tf_att(var_has.name, *att, var_has.getAttValue(*att)) ;
           capt += "are different" ;

           (void) notes->operate(capt) ;
           notes->setCheckStatus( n_CF, fail );
         }
       }
    }
  }

    std::string* s[2];
    s[0] = &n_FillValue;
    s[1] = &n_missing_value;

    for( size_t i=0 ; i < ix.size() ; ++i )
    {
      Variable& var_is  = pIn->variable[ix[i].first];

      for( size_t i=0 ; i < 2 ; ++i )
      {
        if( var_is.isValidAtt( *s[i]) )
        {
          if( notes->inq(bKey + "251e", var_is.name) )
          {
            std::string capt("Coordinate variable");
            capt += hdhC::tf_val(var_is.name, hdhC::blank) ;
            capt += "should not have " ;
            capt += hdhC::tf_att( *s[i]) ;

            std::string text("Appendix A: Not allowed for coordinate data " );
            text += "except in the case of auxiliary coordinate variables in ";
            text += "discrete sampling geometries.";

            (void) notes->operate(capt, text) ;
            notes->setCheckStatus( n_CF, fail );
          }
        }
      }
    }

  return;
}

void
CF::chap72(void)
{
  if( !isCheck )
    return ;

  // cell_measures
  std::string cm;
  int meas_ix; // index of the measure variable

  for( size_t ix=0 ; ix < pIn->varSz ; ++ix )
  {
    Variable& var = pIn->variable[ix];

    int jx = var.getAttIndex(n_cell_measures) ;

    if( jx == -1 )  // cell_measures not available
      continue;

    // syntax: 'key: variable'
    // only two key-words (simultaneously the name of variable)
    // are defined: area and volume.
    std::string cm(var.getAttValue(n_cell_measures));

    // check key-words
    std::vector<std::string> cm_key( getKeyWordArgs(cm, "key", ':') );

    if( cm_key.size() == 0 )
    {
       // missing key-word
      if( notes->inq(bKey + "72b", var.name) )
      {
        std::string capt(hdhC::tf_att(var.name, n_cell_measures)) ;
        capt += "requires missing name area: or volume:" ;

        (void) notes->operate(capt) ;
        notes->setCheckStatus( n_CF, fail );
      }

      var.attValue[jx][0]="CF failed" ;

      continue ;
    }
    else
    {
      if( ! (cm_key[0] == "area:" || cm_key[0] == "volume:") )
      {
        // wrong key-word
        if( notes->inq(bKey + "72b", var.name) )
        {
          std::string capt(hdhC::tf_att(var.name, n_cell_measures)) ;
          capt += "with invalid " + hdhC::tf_assign("name", cm_key[0]);
          capt += ", required is area: or volume:";

          (void) notes->operate(capt) ;
          notes->setCheckStatus( n_CF, fail );
        }
      }
    }

    // check cell_measures variable
    // get variable name given in the cell_measures attribute
    std::vector<std::string> cm_arg( getKeyWordArgs(cm, "arg", ':') );

    if( cm_arg.size() )
    {
      for( size_t k=0 ; k < cm_arg.size() ; ++k )
      {
        if( (meas_ix = pIn->getVarIndex(cm_arg[k])) == -1 )
        {
          if( notes->inq(bKey + "72e", "", "INQ_ONLY") )
          {
            bool isCont=false;

            // accepted to be provided globally or even in another var
            for( size_t m=0 ; m < pIn->variable.size() ; ++m )
            {
              Variable& var_af = pIn->variable[m];

              if( (jx = var_af.getAttIndex("associated_files")) > -1 )
              {
                // note that at present the existence of an associated file is not tested
                for(size_t i=0 ; i < var_af.attValue[jx].size() ; ++i )
                {
                  Split x_av(var_af.attValue[jx][i]);

                  for(size_t j=0 ; j < x_av.size() ; ++j )
                  {
                    if( x_av[j] == (cm_arg[k]+":") )
                    {
                      isCont=true;
                      break;
                    }
                  }

                  if(isCont)
                    break;
                }

                if(isCont)
                  break;
              }
            }

            if(isCont)
              continue;

            if( notes->inq(bKey + "72d", var.name) )
            {
              std::string capt(hdhC::tf_att(var.name, n_cell_measures));
              capt += "names a non-existing " + n_variable ;
              capt += " by" + hdhC::tf_val(cm) ;

              (void) notes->operate(capt) ;
              notes->setCheckStatus( n_CF, fail );
            }
          }

          continue;
        }

        Variable& mvar = pIn->variable[meas_ix];

        mvar.addAuxCount();
        var.push_aux(mvar.name);

        if(mvar.name == var.name && notes->inq(bKey + "72f", mvar.name) )
        {
          std::string capt(hdhC::tf_att(var.name,mvar.name)) ;
          capt += "must not declare the data variable itself" ;

          (void) notes->operate(capt) ;
          notes->setCheckStatus( n_CF, fail );

          return;
        }

        // must have units
        bool is=false;
        if( mvar.units.size() == 0 )
          is=true;
        else
        {
          // a measure variable must have units that are consistent with
          // the measure type, i.e., m2 for area and m3 for volume.
          std::string xpctdU;

          if( cm_key[0].find(n_area) < std::string::npos )
            xpctdU = "m2";
          else if( cm_key[0].find("volume") < std::string::npos )
            xpctdU = "m3";

          if( xpctdU.size() && ! cmpUnits( mvar.units, xpctdU) )
            is=true;
        }

        if(is && notes->inq(bKey + "72c", mvar.name) )
        {
          std::string capt(hdhC::tf_var(mvar.name) + "requires missing ") ;
          std::string text;
          if( cm_key[0].find(n_area) < std::string::npos )
            capt += hdhC::tf_assign(n_units, "m2");
          else if( cm_key[0].find("volume") < std::string::npos )
            capt += hdhC::tf_assign(n_units, "m3");

          if( mvar.units.size() )
            text = "Found" + hdhC::tf_val(mvar.units);

          (void) notes->operate(capt, text) ;
          notes->setCheckStatus( n_CF, fail );
        }

        // The dimensions of the measure variable should be the same as or a subset of
        // the dimensions of the variable to which they are related, but their order
        // is not restricted.
        for( size_t im=0 ; im < mvar.dimName.size() ; ++im )
        {
          if( ! hdhC::isAmong(mvar.dimName[im], var.dimName) )
          {
            if( notes->inq(bKey + "72a", mvar.name) )
            {
              std::string capt("dimensions of measure-");
              capt += hdhC::tf_var(mvar.getDimNameStr(true));
              capt += "are not a (sub-)set of ";
              capt += hdhC::tf_var(var.getDimNameStr(true));

              (void) notes->operate(capt) ;
              notes->setCheckStatus( n_CF, fail );
            }

            break;
          }
        }
      }
    }
  }

  return;
}

void
CF::chap73(void)
{
  if( !isCheck )
    return ;

  // cell_methods
  for( size_t ix=0 ; ix < pIn->varSz ; ++ix )
  {
    Variable& var = pIn->variable[ix];

    // Note that key-words used for climatological statistics
    // are checked in chap74b()

    int jx ;
    if( (jx=var.getAttIndex(n_cell_methods)) == -1 )
      continue;

    // only data variables may have this attribute
    if( !var.isDataVar() || var.coord.isAny || var.isAUX || var.isFormulaTermsVar )
    {
      if( notes->inq(bKey + "73f", var.name) )
      {
        std::string capt(hdhC::tf_var(var.name, hdhC::colon) );
        capt += " Only data variables may have ";
        capt += hdhC::tf_att(hdhC::empty, n_cell_methods);

        (void) notes->operate(capt) ;
        notes->setCheckStatus( n_CF, fail );
      }

      continue;
    }

    std::string cm ;
    for( size_t i=0 ; i < var.attValue[jx].size() ; ++i )
    {
       if( i )
          cm += hdhC::blank ;
       cm += var.attValue[jx][i] ;
    }

    cm = hdhC::stripSides(cm);

    std::string s0;
    size_t last = cm.size()-1;
    bool isColonFail=true;

    if( cm.find(':') < std::string::npos )
       isColonFail = false;

    // ':' must not be the last character
    if( cm[last] == ':' )
      isColonFail = true;

    bool isSepFail=false;
    int bracketOpen=0;
    int bracketClose=0;

    // look for typos and repair
    size_t pos=0 ;
    while( cm[pos] == ':' )
    {
       isColonFail=true;
       ++pos;
    }

    for( ; pos < cm.size() ; ++pos )
    {
      // a) is each 'key:' separated by blank(s) from the next item?
      // b) is each '(' separated by blank(s) from the previous item?
      // c) is each ')' separated by blank(s) from the next item?
      //    note that ')' may be the last character.

      if( cm[pos] == ':' )
      {
        if( pos < last &&  ! (cm[pos+1] == ' ' || cm[pos+1] == ')' ) )
        {
          s0 += ": ";
          isSepFail=true;
          continue;
        }
        else if( cm[pos-1] == ' ' )
        {
          isColonFail=true;

          // try to repair the trivial case "key :word", but annotate
          if( pos > 1 && cm[pos-2] != ':' )
          {
             s0[s0.size()-1] = ':' ;  // swap : and ' '
             s0 += ' ' ;
             continue;  // skip current colon at pos
          }
        }
      }
      else if( cm[pos] == '(' )
      {
        ++bracketOpen;

        if( cm[pos-1] != ' ' )
        {
          s0 += ' ';
          isSepFail=true;
        }
      }
      else if( cm[pos] == ')' )
      {
        ++bracketClose;

        if( pos != last && cm[pos+1] != ' ' )
        {
          isSepFail=true;

          s0 += cm[pos] ;
          s0 += ' ';
          continue;
        }
      }

      s0 += cm[pos] ;
    }

    if( bracketClose != bracketOpen )
    {
      if( notes->inq(bKey + "732e", var.name) )
      {
        std::string capt(hdhC::tf_att(var.name, n_cell_methods, hdhC::colon));
        capt += "Comment without a closing paranthesis" ;

        (void) notes->operate(capt) ;
        notes->setCheckStatus( n_CF, fail );
      }
    }

    // parse 'wordLists', i.e., sequences of
    // dim1: [name:2: [dim3 ...] method [....].
    // Any dimN: after a non-colon item indicates the start
    // of another wordList, except the phrase within parenthesis.

    Split x_cm(s0);

    if( ! isColonFail )
    {
      // no begin with name:
      if( x_cm[0].size() && x_cm[0][x_cm[0].size()-1] != ':' )
        isColonFail = true;

      // an end with name:
      if( x_cm.size() )
      {
         size_t pos = x_cm.size()-1;
         size_t sz = x_cm[pos].size();

         if( sz && x_cm[pos][sz-1] == ':' )
           isColonFail = true;
      }
    }

    if( isSepFail )  // only once
    {
      if( notes->inq(bKey + "73a", var.name) )
      {
         std::string capt(hdhC::tf_att(var.name,n_cell_methods));
         capt += "requires blank separated word-list";
         std::string text("Found ") ;
         text += hdhC::tf_val(cm);

         (void) notes->operate(capt, text) ;
         notes->setCheckStatus( n_CF, fail );
      }

      var.attValue[jx][0].clear() ;
      return;
    }

    if( isColonFail )
    {
      if( notes->inq(bKey + "73b", var.name) ) // only once
      {
        std::string capt(hdhC::tf_att(var.name, n_cell_methods));
        capt += "requires format name: method, found" + hdhC::tf_val(cm) ;

        (void) notes->operate(capt) ;
        notes->setCheckStatus( n_CF, fail );
      }

      var.attValue[jx][0].clear() ;
      return;
    }

    if( bracketClose != bracketOpen )
       return;

    if( followRecommendations )
    {
      // special: cm="lat: lon: ..."  --> "area: ..."
      int i0 = pIn->getVarIndex( x_cm[0].substr(0, x_cm[0].size()-1) ) ;
      for( size_t i=0 ; i < x_cm.size()-1 ; ++i )
      {
         int i1 = pIn->getVarIndex( x_cm[i+1].substr(0, x_cm[i+1].size()-1) ) ;

         if( i0 > -1 && i1 > -1 )
         {
           if( (pIn->variable[i0].coord.isC[0]
                      || pIn->variable[i0].coord.isC[1] )
                 && (pIn->variable[i1].coord.isC[0]
                      || pIn->variable[i1].coord.isC[1] ) )
           {
             if( notes->inq(bKey + "731a", var.name) )
             {
               std::string capt(n_reco + " for ");
               capt += hdhC::tf_att(var.name, n_cell_methods, hdhC::colon) ;
               capt += "Names" + hdhC::tf_val(pIn->variable[i0].name) + " and" ;
               capt += hdhC::tf_val(pIn->variable[i1].name, hdhC::blank) ;
               capt += "should be replaced by area:";

               (void) notes->operate(capt) ;
               notes->setCheckStatus( n_CF );
             }

             // replace by area:
             cm.clear();
             for( size_t j=0 ; j < x_cm.size() ; ++j )
             {
                if( j )
                   cm += hdhC::blank;

                if( j == i )
                {
                  cm += "area:" ;
                  ++j;
                }
                else
                  cm += x_cm[j] ;
             }

             var.attValue[jx][0].clear();
             x_cm = cm ;

             break;
           }
         }

         i0 = i1;
      }
    }

    // now the total cell_method str is decomposed at first into a separated list and
    // then each list item into a part for dimensions, methods, and comment-bracket
    std::vector<std::string> cm_name;
    std::vector<std::string> cm_method;
    std::string cm_comment;
    bool isNewList=true;

    for( size_t i=0 ; i < x_cm.size() ; ++i )
    {
       size_t lst=x_cm[i].size();
       if(lst)
          --lst;

       // get comments
       if( x_cm[i][0] == '(' )
       {
          cm_comment = x_cm[i++] ;
          for( ; i < x_cm.size() ; ++i )
          {
             cm_comment += hdhC::blank + x_cm[i] ;

             if( x_cm[i].size() && x_cm[i][x_cm[i].size()-1] == ')' )
               break;
          }

          isNewList=true;
       }

       // get methods
       else if( isNewList && x_cm[i][lst] == ':')
       {
          cm_name.push_back( x_cm[i] );
          cm_method.push_back(hdhC::empty);
          isNewList=false;
       }

       // get name:
       else if( x_cm[i][lst] == ':')
          cm_name.back() += hdhC::blank + x_cm[i] ;
       else
       {
          if( isNewList )
             cm_method.back() += hdhC::blank + x_cm[i] ;
          else
          {
             cm_method.back() = x_cm[i] ;
             isNewList=true;
          }
       }
    }

    // Exploit wordLists

    // A given dimension name should only occur once in a cell_methods string,
    // except time:
    std::vector<std::string> multipleDims;

    int sz = static_cast<int>(cm_name.size());
    for( int l=0 ; l < sz-1 ; ++l )
    {
      for( int j=l+1 ; j < sz ; ++j )
      {
        if( cm_name[l] == cm_name[j] )
        {
           if( !hdhC::isAmong(cm_name[l], multipleDims) )
             multipleDims.push_back( cm_name[l] );
        }
      }
    }

    if( cm_comment.size() )
      if( chap73_cellMethods_Comment(cm_comment, var) )
         return ;

    std::string time_colon( timeName + hdhC::colon );

    for( size_t i=0 ; i < cm_name.size() ; ++i )
    {
       if( chap73_cellMethods_Name(cm_name[i], var) )
          return;
       if( chap73_cellMethods_Method(cm_method[i], var) )
          return;

       if( cm_name[i] != time_colon )
         if( chap733(cm_method[i], var, "where") )
           return;
    }

    bool isClim=false;
    if( time_ix > -1 )
      // cm-syntax for climatologies
      isClim = chap74b(var, cm_name, cm_method) ;

    // all names representing non-point variables should have bounds
    chap73_inqBounds(var, cm_name, cm_method, isClim );

    if( var.attValue[jx][0].size() == 0 )
    {
      std::string cm_new;

      for( size_t i=0 ; i < cm_name.size() ; ++i )
      {
        if( i )
          cm_new += hdhC::blank ;

        cm_new += cm_name[i] ;

        if(cm_method[i].size() )
          cm_new += hdhC::blank + cm_method[i] ;
      }

      if( cm_comment.size() )
        cm_new += " (" + cm_comment + ")" ;

      var.attValue[jx][0] += cm_new  ;
    }

    //if( followRecommendations )
    //  chap734b(var, cm_name, cm_method);
  }

  return;
}

bool
CF::chap73_cellMethods_Comment(std::string &par, Variable& var)
{
  //return true for failure

  // Additional information may be specified within parenthesis.
  // a) matching () was checked previously
  // b) check the only standardised one, syntax: (interval: value unit)
  Split x_par(par, " ()", true);  // container of chars

  if( x_par.size() )
  {
    if( x_par[0] == "comment:" )
    {
      if( par.find("interval:") == std::string::npos )
      {
        if( notes->inq(bKey + "732a", var.name) )
        {
           std::string capt(hdhC::tf_att(var.name, n_cell_methods, hdhC::colon) );
           capt += "Key-word" + hdhC::tf_val("comment:", hdhC::blank) ;
           capt += "should be omitted because of absent standardised information" ;

           std::string text("Found " + par);

           (void) notes->operate(capt, text) ;
           notes->setCheckStatus( n_CF, fail );
        }
      }
      else
      {
        if( notes->inq(bKey + "732d", var.name) )
        {
           std::string capt(hdhC::tf_att(var.name, n_cell_methods, hdhC::colon));
           capt += "Swapped order of information, ";
           capt += "required is (interval: val units comment: text)" ;

           std::string text("Found " + par);

           (void) notes->operate(capt, text) ;
           notes->setCheckStatus( n_CF, fail );
        }
      }

      return true ;
    }

    if( x_par[x_par.size()-1] == "comment:" )
    {
      if( notes->inq(bKey + "732c", var.name) )
      {
         std::string capt(hdhC::tf_att(var.name, n_cell_methods));
         capt += "provides incomplete non-standardised information" ;

         (void) notes->operate(capt) ;
         notes->setCheckStatus( n_CF, fail );
      }

      return true ;
    }

    if( x_par[0] != "interval:" )
      return false;  // comment by non-standardised information
  }

  bool is=true;

  if( x_par.size() > 2 && hdhC::isDigit(x_par[1]) )
  {
    if( parseUnits(x_par[2]) )
      is=false;
  }

  if( is )
  {
     if( notes->inq(bKey + "732b", var.name) )
     {
        std::string capt(hdhC::tf_att(var.name, n_cell_methods, hdhC::colon));
        capt += "Interval information requires (interval: value units), found";
        capt += hdhC::tf_val(par);

        (void) notes->operate(capt) ;
        notes->setCheckStatus( n_CF, fail );
     }

     return true;
  }

  return false;
}

bool
CF::chap73_cellMethods_Method(std::string &str0, Variable& var)
{
  // str0 contains only the method item
  std::vector<std::string> term;
  term.push_back("maximum");
  term.push_back("mean");
  term.push_back("median");
  term.push_back("mid_range");
  term.push_back("minimum");
  term.push_back("mode");
  std::string point("point");
  term.push_back(point);
  term.push_back("standard_deviation");
  term.push_back("sum");
  term.push_back("variance");

  // method str0 must begin with one of the terms
  Split x_methods(str0);

  if( x_methods.size() == 0 )
    // should not happen, caught elsewhere
    return true;

  if( !hdhC::isAmong(x_methods[0], term) )
  {
    if( notes->inq(bKey + "73d", var.name) )
    {
      std::string capt(hdhC::tf_att(var.name, n_cell_methods));
      capt += "with invalid " + hdhC::tf_assign("method", x_methods[0]) ;

      (void) notes->operate(capt) ;
      notes->setCheckStatus( n_CF, fail );
    }

    return true;
  }

  // a time-line of instant values without boundaries?
  std::vector<std::string> x_m_Items(x_methods.getItems());
  if( hdhC::isAmong(point, x_m_Items) && time_ix > -1 )
    pIn->variable[time_ix].isInstant=true;

  // check valid mode
  if( x_methods.size() > 1 )
  {
    if( ! (x_methods[1] == "where"
            || x_methods[1] == "over"
              || x_methods[1] == "within" ) )
    {
      if( notes->inq(bKey + "733a", var.name) )
      {
        std::string capt(hdhC::tf_att(var.name, n_cell_methods, hdhC::colon));
        capt += "Invalid condition before";
        capt += hdhC::tf_val(x_methods[1]) ;
        capt += ", expected where|over|within" ;

        (void) notes->operate(capt) ;
        notes->setCheckStatus( n_CF, fail );
      }

      return true;
    }
  }

  return false;
}

bool
CF::chap73_cellMethods_Name(std::string &name, Variable& var)
{
  // valid names:  dimension of the variable, a scalar coordinate variable,
  //               , a standard name indicating the axis, or area.

  Split x_name(name, " :", true);

  for( size_t x=0 ; x < x_name.size() ; ++x )
  {
    size_t ix;

    // dim of a var
    if( hdhC::isAmong(x_name[x], var.dimName) )
    {
      continue;
    }

    for( ix=0 ; ix < pIn->varSz ; ++ix )
    {
      // a scalar coord or standard_name (of any variable)
      if( pIn->variable[ix].isScalar && pIn->variable[ix].name == x_name[x] )
      {
         pIn->variable[ix].addAuxCount();
         var.push_aux(pIn->variable[ix].name);
         break;
      }
      else if( pIn->variable[ix].getAttValue(n_standard_name) == x_name[x] )
        break;
    }

    if( ix < pIn->varSz )
      continue;

    // No specific coordinate: area or a valid standard name that is neither
    // a dimensions nor a coordinate variable.
    if( chap734a(x_name[x]) )
      continue;

    if( notes->inq(bKey + "73c", var.name) )
    {
      std::string capt(hdhC::tf_att(var.name, n_cell_methods, hdhC::colon));
      capt += "Invalid name" + hdhC::tf_val(x_name[x]+hdhC::colon);

      (void) notes->operate(capt) ;
      notes->setCheckStatus( n_CF, fail );
    }
  }

  return false;
}

void
CF::chap73_inqBounds(Variable& var,
  std::vector<std::string>& cm_name,
  std::vector<std::string>& cm_method, bool isClim )
{
  // cell_methods:
  // All names representing non-point variables should have bounds.

  // Get list of unique names without method  point.
  // Note that a cm_name item may contain more than a single dimension,
  // but there is only a single correspponding method item.
  static std::vector<std::string> name;

  for( size_t l=0 ; l < cm_name.size() ; ++l )
  {
    Split x_cm_name(cm_name[l], ": ", true);  // the dimensions in cell_methods
    for( size_t x=0 ; x < x_cm_name.size() ; ++x )
    {
      if( cm_method[l].find("point") == std::string::npos )
      {
        size_t n;
        for(n=0 ; n < name.size() ;++n)
        {
          if( name[n] == x_cm_name[x] )
            break;
        }

        if( n == name.size() )
        {
          // exclude no-coordinate cases area:, longitude:, and latitude:
          int j;
          if( (j=pIn->getVarIndex(x_cm_name[x])) == -1 )
          {
            // find name of lon/lat by finding standard_names (or long_name)
            std::vector<std::string> vs;
            if( x_cm_name[x] == "area" )
            {
              vs.push_back(n_longitude);
              vs.push_back(n_latitude);
            }
            else if( x_cm_name[x] == n_longitude )
              vs.push_back(n_longitude);
            else if( x_cm_name[x] == n_latitude )
              vs.push_back(n_latitude);

            if( vs.size() )
            {
              for(size_t v=0 ; v < pIn->varSz ; ++v)
              {
                Variable& var_t = pIn->variable[v];
                for( size_t k=0 ; k < vs.size() ; ++k)
                {
                  int m;
                  if( var_t.std_name.size() )
                  {
                    if( vs[k] == var_t.std_name )
                      name.push_back(var_t.name);
                  }
                  else if( (m=var_t.getAttIndex(n_long_name)) > -1 )
                  {
                    std::string& s = var_t.attValue[m][0] ;
                    if( vs[k] == s )
                      name.push_back(var_t.name);
                  }
                }
              }
            }
          }
          else
            name.push_back( x_cm_name[x] );
        }
      }
    }
  }

  for( size_t l=0 ; l < name.size() ; ++l )
  {
    int ix ;

    if( (ix=pIn->getVarIndex(name[l])) > -1 )
    {
      Variable& var_n = pIn->variable[ix];

      if( !var_n.bounds.size() && notes->inq(bKey + "73e", var.name, NO_MT) )
      {
        std::string capt(n_reco + ": Variable") ;
        capt += hdhC::tf_val(var_n.name) ;
        capt += " referenced by " + hdhC::tf_att(var.name, n_cell_methods);
        capt += "should have attached ";

        if( isClim )
          capt += n_climatology ;
        else
          capt += "bounds" ;
        capt += "-" + n_variable;

        (void) notes->operate(capt) ;
        notes->setCheckStatus( n_CF );
      }
    }
  }

  return;
}

void
CF::chap73b_reco(Variable& var, std::vector<std::string> &cm_name )
{
  // cell_methods

  // data variables (exception of bounds)
  // should have cell_methods attributes

// the following tests is disabled, because of:
// CF-1.6, p.40: "... it is recommended that every data variable include for
//each of its dimensions and each of its scalar coordinate variables
//the cell_methods information of interest
// [!!!](unless this information would not be meaningful).
//It is especially recommended that cell_methods be explicitly specified
//for each spatio-temporal dimension and each spatio-temporal scalar
//coordinate variable."

//Question: what does meaningful mean?

/*
  if( var.boundsOf.size() )
    return;

  if( var.dimName.size() == 0 )
    return;

  std::vector<size_t> missing;
  std::string item;

  // scan over dimensions of a variable and scalar variables
  // supplied by attribute coordinates
  std::vector<std::string> items;

  // dimensions
  for( size_t i=0 ; i < var.dimName.size() ; ++i )
    items.push_back( var.dimName[i] );

  // scalar coordinate variables
  int jx = vIx[var.name] ;

  for( size_t i=0 ; i < ca_vvs[jx].size() ; ++i )
  {
    int j;
    if( (j=pIn->getVarIndex(ca_vvs[jx][i])) > -1 )
    {
        if( pIn->variable[j].isScalar && pIn->variable[j].isCoordinate() )
          items.push_back( ca_vvs[jx][i] );
    }
  }

  for( size_t i=0 ; i < item.size() ; ++i )
  {
    bool is=true;
    for( size_t l=0 ; l < cm_name.size() ; ++l )
    {
      Split x_cm_name(cm_name[l], ": ", true);

      for( size_t x=0 ; x < x_cm_name.size() ; ++x )
      {
        if( x_cm_name[x] == items[i] || x_cm_name[x] == n_area )
        {
          is=false;
          break;
        }
      }

      if( is )
      {
        // the current dimension is neither listed in cell_methods nor is
        // it a scalar coord var.
        int v ;
        if( (v=pIn->getVarIndex(var.dimName[i]) > -1 ) )
        {
          if( pIn->variable[v].isCoordinate() )
             missing.push_back(jx);
        }
      }
    }
  }

  if( missing.size() )
  {
    if( notes->inq(bKey + "73g", var.name) )
    {
      std::string capt(n_reco + " for " + hdhC::tf_att(var.name, n_cell_methods));
      capt += "should specify an item for each " + n_dimension ;
      capt += ", also for " ;

      for( size_t l=0 ; l < missing.size() ; ++l )
      {
         if( l )
           capt += "," ;

         capt += hdhC::tf_val(var.dimName[missing[l]]) ;
      }

      (void) notes->operate(capt) ;
      notes->setCheckStatus( n_CF );
    }
  }
*/

  return;
}

bool
CF::chap733(std::string &method, Variable& var, std::string mode)
{
  // return true for failure
  // mode: where or over
  std::string where("where");
  std::string over("over");

  // find the word after the mode string, i.e. 'where' or 'over'
  Split x_method(method);
  std::string type;

  // prevent to check a climatology case
  if( mode == where && method.find(where) == std::string::npos )
     return false;

  size_t iw;
  for( iw=0 ; iw < x_method.size() ; ++iw )
  {
     if( x_method[iw] == mode )
     {
       if( (iw+1) < x_method.size() )
         type=x_method[++iw] ;

       break;
     }
  }

  if( type.size() == 0 )
  {
    std::string key(bKey);

    std::string capt(hdhC::tf_att(var.name, n_cell_methods, hdhC::colon));
    if( iw == x_method.size() )
    {
      key += "733a";
      capt += "Undefined condition" + hdhC::tf_val(mode) ;
    }
    else
    {
      key += "733b";
      capt += "Condition" + hdhC::tf_val(mode, hdhC::blank) ;
      capt += "without a type";
    }

    if( notes->inq(key, var.name) )
    {
      (void) notes->operate(capt) ;
      notes->setCheckStatus( n_CF, fail );
    }

    return true;  // no where |over clause specified
  }

  // only read once
  std::vector<std::string> vs_areaType;

  if( ! vs_areaType.size() && region_table.is() )
  {
    ReadLine ifs( area_table.getFile() ) ;

    if( ifs.isOpen() )
    {
      std::string line;
      while( ! ifs.getLine(line) )
        vs_areaType.push_back(line);

      ifs.close();
    }
    else
    {
      if( notes->inq(bKey + "0t") )
      {
        std::string capt("fatal: area-type table could not be opened");

        (void) notes->operate(capt) ;
        notes->setCheckStatus( n_CF, fail );
      }
    }
  }

  // two conventions are available (chap 7.3.3). The one with a
  // type-variable takes precedence

  // typevar: a string-valued auxiliary variable with
  // a standard name of area_type
  int  tv_ix;
  int  sn_ix;
  bool isAreaTypeVar=false;

  // is the second convention given?
  if( (tv_ix = pIn->getVarIndex(type)) > -1 )
  {
     Variable& var_tv = pIn->variable[tv_ix];

     isAreaTypeVar = true;

     std::vector<std::string> txt;

     if( var_tv.isLabel )
     {
       if( var_tv.coord.isCoordVar  )
          txt.push_back("is not an auxiliary coordinate variable") ;

       if( (sn_ix=var_tv.getAttIndex(n_standard_name)) == -1
               || var_tv.attValue[sn_ix][0] != "area_type" )
         txt.push_back("has no " + n_standard_name + "=area_type") ;
     }
     else
       txt.push_back("is not string-valued") ;

     for( size_t i=0 ; i < txt.size() ; ++i )
     {
        if( notes->inq(bKey + "733d", var_tv.name) )
        {
          std::string capt(hdhC::tf_att(var.name, n_cell_methods + ", 2nd convention:"));
          capt += "Condition" + hdhC::tf_val(mode, hdhC::blank) ;
          capt += "declares area-type-";
          capt += hdhC::tf_var(var_tv.name,hdhC::no_blank) + ", which ";
          capt += txt[i] ;

          (void) notes->operate(capt) ;
          notes->setCheckStatus( n_CF, fail );
        }

        return true;
     }
  }
  else
  {
     // check for the first convention
     if( !hdhC::isAmong(type, vs_areaType) )
     {
        if( notes->inq(bKey + "733c", var.name) )
        {
          std::string capt(hdhC::tf_att(var.name, n_cell_methods + ", 1st convention:"));
          capt += "The" + hdhC::tf_val(mode) ;
          capt += "-condition declares invalid area_type" + hdhC::tf_val(type) ;

          (void) notes->operate(capt) ;
          notes->setCheckStatus( n_CF, fail );
        }

        return true;
     }
  }

  if( isAreaTypeVar )
  {
    // check more rules for the second convention
    Variable& tVar = pIn->variable[tv_ix] ;

    if( mode == over )
    {
      // only a single string-valued item is allowed; note that
      // the last dimension is for the maximum string size.
      if( tVar.dimName.size() > 1 )
      {
        if( pIn->nc.getDimSize(tVar.dimName[0]) > 1 )
        {
          if( notes->inq(bKey + "733e", var.name) )
          {
            std::string capt(hdhC::tf_att(var.name, n_cell_methods + ", 2nd convention:"));
            capt += "Variable " + hdhC::tf_assign("type2", tVar.name) ;
            capt += " in <where type1 over type2> should not have more than a single area-type string" ;

            (void) notes->operate(capt) ;
            notes->setCheckStatus( n_CF, fail );
          }

          return true;
        }
      }
    }

    // for both where and over:
    // does the type-var provide valid area-type strings?
    std::vector<std::string> vs;
    std::vector<std::string> tvFail;
    pIn->nc.getData(vs, tVar.name);

    for(size_t l=0 ; l < vs.size() ; ++l)
    {
      vs[l] = hdhC::stripSides(vs[l]);

      if( !hdhC::isAmong(vs[l], vs_areaType) )
        tvFail.push_back(vs[l]);
    }

    if( tvFail.size() )
    {
      if( notes->inq(bKey + "733f", var.name) )
      {
         std::string capt(hdhC::tf_att(var.name, n_cell_methods + ", 2nd convention:"));
         capt += "Type2-" + hdhC::tf_var(tVar.name) ;
         if( tvFail.size() > 5 )
            capt += "with more than five invalid area_type strings";
         else
         {
           capt += "with invalid area_type=" ;
           for( size_t l=0 ; l < tvFail.size() ; ++l)
           {
             if(l)
               capt += ",";
             capt += hdhC::tf_val(tvFail[l]) ;
           }
         }

         (void) notes->operate(capt) ;
         notes->setCheckStatus( n_CF, fail );

         return true;
      }
    }

    // the type-var is an auxiliary coordinate variable
    tVar.coord.isAny=true;

    int j = vIx[var.name];
    if( !hdhC::isAmong(tVar.name, ca_vvs[j]) )
    {
      if( tVar.isFormulaTermsVar )
        return true;

      if( notes->inq(bKey + "5j", var.name, NO_MT) )
      {
         std::string capt(hdhC::tf_att(var.name,n_coordinates, hdhC::colon));
         capt += "Missing auxiliary name" ;
         capt += hdhC::tf_val(tVar.name);

         (void) notes->operate(capt) ;
         notes->setCheckStatus( n_CF, fail );
      }

      return true;
    }
  }

  if( mode == where && method.find(over) < std::string::npos )
     return chap733(method, var, over) ;

  return false ;
}

bool
CF::chap734a(std::string& name)
{
   // no specific coordinate: area or valid standard name instead of
   // coordinate variable, scalar coordinate variable or dimension.
   // Return true if such a case is given.

   // name must not be the name of a dimension
   if( hdhC::isAmong(name, dimensions) )
     return false;

   // name must not be the name of a scalar variable
   for( size_t i=0 ; i < pIn->variable.size() ; ++i )
     if( pIn->variable[i].isScalar && pIn->variable[i].name == name )
       return false;

   if( name == n_area )
     return true;

   // special: a two common valid standard_name
   if( name == n_longitude || name == n_latitude )
        return true;

   // rule: name must not be the name of a dimension or a scalar variable
   // collection of correct names; these have been sorted out before.

  if( std_name_table.is() )  // cf-standard-name-table.xml
     return false;

  ReadLine ifs( std_name_table.getFile() ) ;

  if( ! ifs.isOpen() )
  {
    if( notes->inq(bKey + "0t") )
    {
      std::string capt("fatal: the CF-standarad-name table could not be opened");

      (void) notes->operate(capt) ;
      notes->setCheckStatus( n_CF, fail );
    }

    return false ;
  }

  ifs.clearSurroundingSpaces();

  Variable var;
  var.snTableEntry.found=false;

  scanStdNameTable(ifs, var, name) ;
  ifs.close();

  if( var.snTableEntry.found )
    return true;

  return false;
}

void
CF::chap734b(Variable& var,
  std::vector<std::string> &cm_name,
  std::vector<std::string> &cm_method )
{
  // cell_methods when there are no coordinates.
  // Except for entries whose cell_methods attribute is point, all numeric variables
  // and scalar coordinate vars named by cell_methods should have
  // bounds or climatology attributes. Exception: area: for global
  std::string dim;

  for( size_t l=0 ; l < cm_name.size() ; ++l )
  {
    if( cm_method[l].find("point") < std::string::npos )
      continue;

    std::vector<std::string> vs( pIn->nc.getDimName());

    Split x_cm_name(cm_name[l], ": ", true);  // the dimensions in cell_methods

    for( size_t x=0 ; x < x_cm_name.size() ; ++x )
    {
      int ix = pIn->getVarIndex(x_cm_name[x]) ;

      if( ix == -1 &&
            ( x_cm_name[x] == n_area
                || x_cm_name[x] == n_longitude
                    || x_cm_name[x] == n_latitude) )
      {
        std::string meaning("none");
        std::string *kind=0;  // for preventing compiler warnings when -O2

        int i_lat, i_lon;
        i_lat = i_lon = -1 ;

        // test dimension names
        for( size_t k=0 ; k < vs.size() ; ++k )
        {
          if( vs[k] == n_longitude )
            i_lon = k;

          else if( vs[k] == n_latitude )
            i_lat = k;
        }

        // test coordinate variables
        for( size_t k=0 ; k < pIn->varSz ; ++k )
        {
          if( pIn->variable[k].name == n_longitude )
            i_lon = k;

          else if( pIn->variable[k].name == n_latitude )
            i_lat = k;

          else if( pIn->variable[k].coord.isC[0] )
            i_lon = k;

          else if( pIn->variable[k].coord.isC[1] )
            i_lat = k;
        }

        if( x_cm_name[x] == n_area )
        {
          if( i_lon == -1 && i_lat == -1 )
            meaning=n_global;
          else if( i_lon == -1 )
            meaning="zonal";
          else if( i_lat == -1 )
            meaning="meridional";

          if( meaning.size() )
            kind=&n_area;
        }

        else if( i_lon == -1 && x_cm_name[x] == n_longitude )
        {
          meaning="meridional";
          kind=&n_longitude;
        }

        else if( i_lat == -1 && x_cm_name[x] == n_latitude )
        {
          meaning="zonal";
          kind=&n_latitude;
        }

        if( meaning != n_global && notes->inq(bKey + "734a", var.name) )
        {
          std::string capt(n_reco + " for " + hdhC::tf_att(var.name, n_cell_methods, hdhC::colon)) ;
          capt += hdhC::tf_assign("Name", *kind +hdhC::colon) + " for " + meaning ;
          capt += " scope should have size-one-dimensioned " + n_variable;

          if( meaning == n_global )
            capt += "s <" + n_longitude + "> and <" + n_latitude ;
          else
            capt += " <" + *kind ;

          capt += "> with bounds" ;

          (void) notes->operate(capt) ;
          notes->setCheckStatus( n_CF );
        }
      }
    }
  }

  return;
}

void
CF::chap74a(void)
{
  if( !isCheck )
    return ;

  // index to the climatology variable, i.e. the bounds
  size_t cvIx;
  bool isClimatology=false;

  // climatological statistics. Try for multiple occurrences
  for( cvIx=0 ; cvIx < pIn->varSz ; ++cvIx )
  {
    Variable& var = pIn->variable[cvIx];

    // only time may have a climatology att
    if( var.isValidAtt(n_climatology) )
    {
      // FTV may depend only on a dim without representation
      if( !var.isFormulaTermsVar )
        isClimatology = true;

      if( var.name != timeName )
      {
        if( notes->inq(bKey + "74c", var.name) )
        {
          std::string capt(hdhC::tf_att(n_climatology));
          capt += "must be attached only to " + hdhC::tf_var(timeName, hdhC::no_blank);
          capt += ", but found in " + hdhC::tf_var(var.name);

          (void) notes->operate(capt) ;
          notes->setCheckStatus( n_CF, fail );
        }
      }
    }
  }

  if( !isClimatology )
    return;  // no climatology;

  // When there is a climatology, then at least one variable must have
  // a corresponding cell_methods attribute.
  for( size_t i=0 ; i < pIn->varSz ; ++i )
    if( pIn->variable[i].isValidAtt(n_cell_methods) )
      return;

  if( notes->inq(bKey + "74b", pIn->variable[cvIx].name) )
  {
    std::string capt("a " + n_climatology + hdhC::blank + n_variable) ;
    capt += " is available, but no data " + n_variable ;
    capt += " specifies " + hdhC::tf_att(n_cell_methods);

    (void) notes->operate(capt) ;
    notes->setCheckStatus( n_CF, fail );
  }

  return;
}

bool
CF::chap74b(Variable& var,
  std::vector<std::string> &cm_name,
  std::vector<std::string> &method)
{
  // return true for a valid syntax

  // a climatology must have at least two time: dimensions; find the indices
  std::vector<size_t> t_ix;
  for(size_t i=0 ; i < cm_name.size() ; ++i )
    if( cm_name[i] == (timeName + hdhC::colon) )
       t_ix.push_back(i);

  int j;
  if( (j=pIn->variable[time_ix].getAttIndex(n_climatology)) > -1 )
  {
    int i = pIn->getVarIndex(pIn->variable[time_ix].attValue[j][0]);
    if( i > -1 )
    {
      pIn->variable[i].addAuxCount();
      var.push_aux( pIn->variable[i].name );
    }
  }

  // relation between attributes time:climatology and var:cell_methods

  // a) cell_methods without enough time: instances
  if( t_ix.size() < 2 )
  {
    if( j > -1 )
    {
      if( notes->inq(bKey + "74a", var.name) )
      {
        std::string capt(hdhC::tf_att(var.name, n_cell_methods)) ;
        capt += "is ill-formatted for a " + n_climatology ;

        std::string text("Found " + var.name + hdhC::colon);
        text += hdhC::tf_assign(n_cell_methods, var.getAttValue(n_cell_methods));

        (void) notes->operate(capt, text) ;
        notes->setCheckStatus( n_CF, fail );
      }
    }

    return false;
  }

  // The [within|over days|years] option of cell_methods.
  // Corresponds to climatologies (chapter 7.4).
  // time: is the only dimension; this implies that there is no 'where' clause.
  // Note that str0 is a single blank-separated worid-list.

  std::string nextArg;  // within | over
  std::string within("within");
  std::string over("over");

  std::vector<std::string> period;
  period.push_back("years");
  period.push_back("days");

  if( t_ix.size() == 2 )
  {
     // two valid formats
     // 1) time: method1 within years time: method2 over years
     // 2) time: method1 within days time: method2 over days

     std::string& str0 = method[t_ix[0]];
     std::string& str1 = method[t_ix[1]];

     size_t j;
     std::string what(": was a ");
     what += n_climatology;
     what += "attribute intended? Found ";

     for( j=0 ; j < 2 ; ++j )
     {
       size_t pos;
       if( (pos=str0.find(within)) < std::string::npos)
       {
         what="period after key-word=within,";
         if( str0.find(period[j], pos) < std::string::npos )
         {
            if( (pos=str1.find(over)) < std::string::npos )
            {
              what= "use of key-word=over in";
              if( str1.find(period[j], pos)< std::string::npos )
              {
                what.clear();
                break;
              }
              else
              {
                what="period after key-word=over in";
                break;
              }
            }
         }
       }
     }

     if( what.size() )
     {
        if( notes->inq(bKey + "74d", var.name) )
        {
          std::string capt;
          if( what[0] == ':' )
            capt = hdhC::tf_att(var.name, n_cell_methods);
          else
            capt = hdhC::tf_att(var.name, n_cell_methods + " for a climatology:");
          capt += what  ;
          capt += hdhC::tf_val(var.getAttValue(n_cell_methods));

          (void) notes->operate(capt) ;
          notes->setCheckStatus( n_CF, fail );
        }

        return false;
     }
  }

  else if( t_ix.size() == 3 )
  {
     // valid format
     // time: method1 within days
     //    time: method2 over days
     //       time: method2 over years
     std::string& str0 = method[t_ix[0]] ;
     std::string& str1 = method[t_ix[1]] ;
     std::string& str2 = method[t_ix[2]] ;

     int state;
     size_t pos;

     state=0;
     if( (pos=str0.find(within)) < std::string::npos )
     {
       if( str0.find(period[1], pos) < std::string::npos )
       {
         state=1;
         if( (pos=str1.find(over)) < std::string::npos )
         {
           if( str1.find(period[1], pos) < std::string::npos )
           {
             state=2;
             if( (pos=str2.find(over)) < std::string::npos )
             {
               if( str2.find(period[0], pos) < std::string::npos )
                 state = -1;  // aboslutely valid
             }
           }
         }
       }
     }

     if( state > -1 )
     {
        if( notes->inq(bKey + "74d", var.name) )
        {
          std::string capt( hdhC::tf_att(var.name, n_cell_methods, hdhC::colon)) ;
          capt += "Invalid condition" + hdhC::tf_val(method[t_ix[state]]) ;

          (void) notes->operate(capt) ;
          notes->setCheckStatus( n_CF, fail );
        }

        return false;
     }
  }
  else
    return false;  // don't know what so many time: occurrences could be

  // found valid cell_methods for a climatology,
  // but a corresponding attribute was not defined in the time variable
  if( ! pIn->variable[time_ix].isValidAtt(n_climatology) )
  {
    if( notes->inq(bKey + "74e", var.name) )
    {
      std::string capt(hdhC::tf_att(var.name, n_cell_methods, hdhC::colon));
      capt += " The value " ;
      capt += hdhC::tf_val(timeName) ;
      capt += "suggests " + hdhC::tf_val(n_climatology) ;
      capt += " instead of" ;
      capt += hdhC::tf_val(timeName);
      capt += ":time_bnds" ;

      (void) notes->operate(capt) ;
      notes->setCheckStatus( n_CF, fail );
    }
  }

  return true ;
}

void
CF::chap8(void)
{
  // packed data
  if( ! notes->inq(bKey + "8", hdhC::empty, "INQ_ONLY" ) )
    return ;  // checks are selected to be discarded by 'D'

  for( size_t ix=0 ; ix < pIn->varSz ; ++ix )
  {
    Variable& var = pIn->variable[ix];

    chap81(var) ; // packed data (scale_factor, add_offset)
    chap82(var) ; // compression by gathering
  }

  return ;
}

void
CF::chap81(Variable& var)
{
  if( !isCheck )
    return ;

  // packed data (scale_factor, add_offset)
  // CF doesn't state that both must be available.
  // Default values scale_factor=1 and add_offset=0 are reasonable.

  std::vector<nc_type> attType;
  std::vector<std::string> attStr;
  bool attBool[] = { false, false };
  int att_ix = -1;  // if an att is omitted, then the index to the other one

  attStr.push_back("scale_factor");
  attStr.push_back("add_offset");

  for( size_t i=0 ; i < attStr.size() ; ++i )
    attType.push_back( NC_NAT );

  for( size_t i=0 ; i < attStr.size() ; ++i )
  {
    attBool[i]=false;

    int j;
    if( (j=var.getAttIndex(attStr[i])) > -1 )
    {
      attBool[i]=true;
      att_ix=i;
      attType[i] = var.attType[j] ;
    }
  }

  // not packed
  if( ! (attBool[0] || attBool[1] ) )
    return;

  if( attBool[0] && attBool[1] )
  {
    if( attType[0] != attType[1] )
    {
      if( notes->inq(bKey + "81a", var.name) )
      {
        std::string capt(hdhC::tf_var(var.name, hdhC::colon)) ;
        capt += hdhC::tf_att(hdhC::empty,
                attStr[0], pIn->nc.getTypeStr(attType[0]), hdhC::upper);
        capt += "and " + attStr[1]+ " = <" + pIn->nc.getTypeStr(attType[1]) + ">";
        capt += " have to be the same type";

        (void) notes->operate(capt) ;
        notes->setCheckStatus( n_CF, fail );
      }

      return;
    }
  }

  // var and att of different type
  if( (attBool[att_ix] && (attType[att_ix] != var.type) ) )
  {
    // the variable must be a particular type
    if( ! (var.type == NC_INT || var.type == NC_BYTE || var.type == NC_SHORT) )
    {
      if( notes->inq(bKey + "81b", var.name) )
      {
        std::string capt("type of " + hdhC::tf_var(var.name));
        capt += "should be int, short or byte, because" ;

        capt += " it is different from the type of scale_factor|add_offset," ;

        std::string text(var.name + hdhC::colon);
        if( attBool[0] )
        {
          text += attStr[0] + " type=";
          text += pIn->nc.getTypeStr(attType[0]);
        }
        if( attBool[0] && attBool[1] )
        {
          text += ", " ;
          text += var.name + hdhC::colon;
        }
        if( attBool[1] )
        {
          text += attStr[1] +" type=";
          text += pIn->nc.getTypeStr(attType[1]);
        }
        text += "\n";
        text += var.name;
        text += " type=";
        text += pIn->nc.getTypeStr(var.type);

        (void) notes->operate(capt, text) ;
        notes->setCheckStatus( n_CF, fail );
      }
    }

    // var and att still of different type
    if( !( (attType[att_ix] == NC_FLOAT || attType[att_ix] == NC_DOUBLE ) ) )
    {
       if( notes->inq(bKey + "81c", var.name) )
       {
         std::string capt("types are different between " + hdhC::tf_var(var.name));
         capt += "and " + n_attribute + "s" ;
         capt += hdhC::tf_val(attStr[0], hdhC::blank) ;
         capt += "and" + hdhC::tf_val(attStr[1]) ;
         capt += ", which have to be float or double";

         std::string text("Found " + var.name + hdhC::colon);
         if( attBool[0] )
         {
           text += attStr[0] + " type=";
           text += pIn->nc.getTypeStr(attType[0]);
         }
         if( attBool[0] && attBool[1] )
         {
           text += ", " ;
           text += var.name + hdhC::colon;
         }
         if( attBool[1] )
         {
           text += attStr[1] +" type=";
           text += pIn->nc.getTypeStr(attType[1]);
         }
         text += "\n";
         text += var.name;
         text += " type=";
         text += pIn->nc.getTypeStr(var.type);

         (void) notes->operate(capt, text) ;
         notes->setCheckStatus( n_CF, fail );
       }
     }
  }

  // recommendation
  // if scale_factor and add_offset are type float, the variable should not be int
  if( attType[0] == NC_FLOAT && attType[1] == NC_FLOAT
         && var.type == NC_INT )
  {
     if( notes->inq(bKey + "81d", var.name) )
     {
       std::string capt(hdhC::tf_var(var.name, hdhC::colon));
       capt += "If scale_factor and add_offset are" ;
       capt += hdhC::tf_val("float") + ", then the ";
       capt += n_variable + " should not be" + hdhC::tf_val("int") ;

       (void) notes->operate(capt) ;
       notes->setCheckStatus( n_CF, fail );
     }
  }

  // variable, valid_ min, max and range must be the same type
  if( var.isValidAtt(n_valid_range) || var.isValidAtt(n_valid_min) ||
         var.isValidAtt(n_valid_max) )
  {
    int j;
    bool isNotType[]={ false, false, false };

    if( (j=var.getAttIndex(n_valid_range)) > -1
           && var.attType[j] != var.type )
      isNotType[0] = true;
    if( (j=var.getAttIndex(n_valid_min)) > -1
           && var.attType[j] != var.type )
      isNotType[1] = true;
    if( (j=var.getAttIndex(n_valid_max)) > -1
           && var.attType[j] != var.type )
      isNotType[2] = true;

    if( isNotType[0] || isNotType[1] || isNotType[2] )
    {
      if( notes->inq(bKey + "81e", var.name) )
      {
        std::string capt("types of " + hdhC::tf_var(var.name) );
        capt += "and " + n_attribute ;

        if( (isNotType[0] && isNotType[1])
              || (isNotType[0] && isNotType[2])
                 || (isNotType[1] && isNotType[2]) )
           capt += "s";
        capt += hdhC::blank;

        if( isNotType[0] )
           capt += n_valid_range ;

        if( isNotType[1] )
        {
           if( isNotType[0] )
           {
             if( isNotType[2] )
               capt += ", " ;
             else
               capt += " and " ;
           }
           capt += n_valid_min ;
        }

        if( isNotType[2] )
        {
           if( isNotType[0] && isNotType[1] )
             capt += "," ;
           if( isNotType[0] || isNotType[1] )
             capt += " and " ;
           capt += n_valid_max ;
        }

        capt += " have to be the same" ;

        (void) notes->operate(capt) ;
        notes->setCheckStatus( n_CF, fail );
      }
    }
  }

  return;
}

void
CF::chap82(Variable& var)
{
  // compression by gathering
  size_t sz = pIn->varSz;

  std::string compressVal;
  Split x_cmp;

  // any evidence for a compressed-index variable? Tests also for int-type
  bool isIntType=false;
  bool isEvidence=isCompressEvidence(var, &isIntType);
  compressVal = var.getAttValue(n_compress, lowerCase);
  x_cmp = compressVal;

  bool isFailed=false;

  if( var.isCompress )
  {
    var.addAuxCount() ;

    if(isCheck && !isIntType)
    {
      if( notes->inq(bKey + "82a", var.name) )
      {
        std::string capt(hdhC::tf_att(var.name, n_compress)) ;
        capt += "is specified, but the " + n_variable;
        capt += " is not index-type, found";
        capt += hdhC::tf_val(pIn->nc.getTypeStr(var.type)) ;

        (void) notes->operate(capt) ;
        notes->setCheckStatus( n_CF, fail );
      }

      isFailed=true;
    }

    for( size_t j=0 ; j < x_cmp.size() ; ++j )
    {
      for( size_t jx=0 ; jx < sz ; ++jx )
        if( pIn->variable[jx].name == x_cmp[j] )
          pIn->variable[jx].addAuxCount() ;

      // compress must only declare dims
      if( !hdhC::isAmong(x_cmp[j], dimensions) )
      {
        if( notes->inq(bKey + "82b", var.name) )
        {
          std::string capt(hdhC::tf_att(var.name, n_compress)) ;
          capt += "names a non-existing " + n_dimension ;

          (void) notes->operate(capt) ;
          notes->setCheckStatus( n_CF, fail );
        }

        isFailed=true;
      }
    }
  }

  else if( isCheck && isEvidence )
  {
     if( ! ( cFVal > 15
         && pIn->variable[pIn->varSz].isValidAtt(n_featureType) ) )
     {
       // Note: CF-1.6 allows scalar descriptive variables not in compress context
       if( notes->inq(bKey + "82a", var.name) )
       {
         std::string capt(cFVersion + ": ") ;
         capt += hdhC::tf_val(var.name, hdhC::blank) ;
         capt += "is estimated as compressed-index " + n_variable ;
         capt += ", but a " + hdhC::tf_att(n_compress) + "is missing";

         (void) notes->operate(capt) ;
         notes->setCheckStatus( n_CF, fail );
       }

       isFailed=true;
     }
  }

  if( isFailed )
    return;


  // test for index-range under-|overflow;
  // this is only reasonable, when the checks above didn't fail
  if( compressVal.size() )
  {
    // get dimensions
    std::vector<int> vDims;
    for( size_t j=0 ; j < x_cmp.size() ; ++j )
       vDims.push_back( pIn->nc.getDimSize(x_cmp[j]) );

    int d_max=1;
    for(size_t i=0 ; i < vDims.size() ; ++i )
       d_max *= vDims[i] ;

    //get compressed indices
    MtrxArr<int> ma;
    pIn->nc.getData(ma, var.name, 0 , -1 );

    int ma_min=d_max+1;
    int ma_max=-1;
    for( size_t i=0 ; i < ma.size() ; ++i )
    {
       if( ma_max < ma[i] )
          ma_max = ma[i] ;
       if( ma_min > ma[i] )
          ma_min = ma[i] ;
    }

    bool is=false;

    if( ma_min <= 0 )
    {
       if( ma_min == 0 )
       {
         if( notes->inq(bKey + "82c", var.name) )
         {
           std::string capt("warning for FORTAN: Compressed indices values of ") ;
           capt += n_variable + hdhC::tf_val(var.name, hdhC::blank);
           capt += "apply C index convention";

           (void) notes->operate(capt) ;
           notes->setCheckStatus( n_CF, fail );
         }
       }
       else
         is=true;
    }

    if( ma_max >= d_max )
    {
       if( ma_max == d_max )
       {
         if( notes->inq(bKey + "82d", var.name) )
         {
           std::string capt("warning for C/C++: Compressed indices of ");
           capt += hdhC::tf_var(var.name) ;
           capt += "apply FORTRAN index convention";

           (void) notes->operate(capt) ;
           notes->setCheckStatus( n_CF, fail );
         }
       }
       else
         is=true;
    }

    if(is)
    {
      if( notes->inq(bKey + "82e", var.name) )
      {
        std::string capt("compress-" + hdhC::tf_var(var.name, hdhC::colon)) ;
        capt += "A data index exceeds maximum number of data points, found " ;
        capt += hdhC::tf_assign("index", hdhC::itoa(ma_max));
/*
        for(size_t i=0 ; i < vDims.size() ; ++i )
        {
           if( i )
             capt += " x " ;
           capt += hdhC::itoa(vDims[i]);
        }
*/
        (void) notes->operate(capt) ;
        notes->setCheckStatus( n_CF, fail );
      }
    }
  }

  return ;
}

void
CF::chap9(void)
{
  if( ! notes->inq(bKey + "9", hdhC::empty, "INQ_ONLY" ) )
    return ;  // checks are selected to be discarded by 'D'

  // discrete sampling geometries
  std::vector<std::string*> dsg_att;

  dsg_att.push_back(&n_featureType);
  dsg_att.push_back(&n_cf_role);
  dsg_att.push_back(&n_instance_dimension);
  dsg_att.push_back(&n_sample_dimension);

  if( cFVal < 16 )
  {
    std::vector<std::string> coll;

    for( size_t i=0 ; i < pIn->variable.size() ; ++i )
    {
      Variable& var = pIn->variable[i] ;
      for( size_t j=0 ; j < dsg_att.size() ; ++j )
      {
         if( var.isValidAtt(*(dsg_att[j])) )
         {
            if( !hdhC::isAmong(*(dsg_att[j]), coll) )
               coll.push_back(*(dsg_att[j])) ;
         }
      }
    }

    if( coll.size() )
    {
       if( notes->inq(bKey + "0a") )
       {
         std::string capt("warning: Usage of a CF-1.6 specific attributes in ");
         capt += cFVersion ;
         capt += ", found" ;
         for( size_t i=0 ; i < coll.size() ; ++i )
         {
           if(i)
             capt += ",";
           capt += hdhC::tf_val(coll[i]) ;
         }

         (void) notes->operate(capt) ;
         notes->setCheckStatus( n_CF, fail );
       }
    }

    return;
  }

  // checks are performed for case insensitivity.
  std::vector<std::string> validFT_vs;
  validFT_vs.push_back("point");
  validFT_vs.push_back("timeseries");
  validFT_vs.push_back("profile");
  validFT_vs.push_back("trajectory");
  validFT_vs.push_back(validFT_vs[1]+validFT_vs[2]);
  validFT_vs.push_back(validFT_vs[3]+validFT_vs[2]);

  // collection of strings of found attributes

  // >CF-1.5: global attribute
  std::vector<std::string> vs_featureType;

  Variable& glob = pIn->variable[pIn->varSz] ;

  // Get the featureType(s). Note that only a single one is expected by CF-1.6
  chap9_featureType(validFT_vs, vs_featureType) ;

  // warnings for attributes cf_role|instance_dimension|sample_dimension
  // which are confusing when global
  for( size_t i=1 ; i < dsg_att.size() ; ++i )
  {
     if( glob.isValidAtt(*(dsg_att[i])) )
     {
        if( notes->inq(bKey + "9c", n_global) )
        {
          std::string capt("warning:") ;
          capt += hdhC::tf_val(*(dsg_att[i]), hdhC::blank) ;
          capt += "should not be a global " + n_attribute;

          (void) notes->operate(capt) ;
          notes->setCheckStatus( n_CF, fail );
        }
     }
  }

  // to be used below
  std::vector<int> xyzt_ix;
  std::vector<size_t> dv_ix;
  chap9_getSepVars(xyzt_ix, dv_ix);

  // check n_instance_dimension properties
  chap9_sample_dimension(dv_ix);

  // scan variables for occurrences of attribute cf_role attribute,
  // sample_dimension

  std::vector<std::string> vs_cr;
  vs_cr.push_back("timeseries_id") ;
  vs_cr.push_back("profile_id") ;
  vs_cr.push_back("trajectory_id") ;

  std::vector<std::pair<int, int> > vp_cf_role_ix;

  for( size_t i=0 ; i < pIn->varSz ; ++i )
  {
    // look for an unknown cf_role value
    Variable& var = pIn->variable[i] ;

    int j=-1;
    if( (j=var.getAttIndex(n_cf_role)) > -1 )
    {
      vp_cf_role_ix.push_back( std::pair<int, int>(static_cast<int>(i),j ) );

      if( !hdhC::isAmong(var.attValue[j][0], vs_cr) )
      {
        if( notes->inq(bKey + "9e", var.name) )
        {
          std::string capt(hdhC::tf_att(var.name, n_cf_role, hdhC::colon)) ;
          capt += "Invalid value";
          capt += hdhC::tf_val(var.attValue[j][0]);

          // amend an omitted trailing '_id'
          for( size_t l=0 ; l < vs_cr.size() ; ++l )
          {
            if( (var.attValue[j][0] + "_id") == vs_cr[l] )
            {
               var.attValue[j][0] += "_id" ;
               capt += ", missing trailing _id " ;
               break;
            }
          }

          (void) notes->operate(capt) ;
          notes->setCheckStatus( n_CF, fail );
        }
      }

    }
  }

  // infer the featureType by the layout
  std::vector<std::string> guessedFT(
      chap9_guessFeatureType(validFT_vs, xyzt_ix, dv_ix) );

  if( vs_featureType.size() )
  {
    // do cf_role and featureType comply?
    for( size_t i=0 ; i < vp_cf_role_ix.size() ; ++i )
    {
      std::string str =
          pIn->variable[vp_cf_role_ix[i].first].attValue[vp_cf_role_ix[i].second][0] ;

      str = str.substr(0, str.size() - 3);
      std::string s0( hdhC::Lower()(str));

      size_t j;
      for(j=0 ; j < vs_featureType.size() ; ++j)
        if( vs_featureType[j].find(s0) < std::string::npos )
          break;

      if( j == vs_featureType.size() )
      {
        if( notes->inq(bKey + "9f", n_global) )
        {
          std::string capt(n_global + hdhC::blank +
                  hdhC::tf_att(hdhC::empty, n_featureType, vs_featureType[0]));
          capt += "is not compatible with " ;
          capt += hdhC::tf_att(hdhC::empty, n_cf_role, str +"_id") ;

          (void) notes->operate(capt) ;
          notes->setCheckStatus( n_CF, fail );
        }
      }
    }

    // any label without cf_role?
    bool isPoint=false;
    if( vs_featureType.size()  && hdhC::Lower()(vs_featureType[0]) == "point" )
      isPoint=true;
    else if( guessedFT.size()  && hdhC::Lower()(guessedFT[0]) == "point" )
      isPoint=true;

    if( !isPoint )
    {
      for( size_t i=0 ; i < pIn->varSz ; ++i )
      {
        Variable var_l = pIn->variable[i];

        if( var_l.isLabel && !var_l.isValidAtt(n_cf_role) )
        {
          if( notes->inq(bKey + "9h", n_global) )
          {
            std::string capt(n_reco + ": Variable");
            capt += hdhC::tf_val(var_l.name, hdhC::blank);
            capt += "should have " + hdhC::tf_att(n_cf_role);

            (void) notes->operate(capt) ;
            notes->setCheckStatus( n_CF );
          }
        }
      }
    }

    // compare to a guessed FT
    if( isFeatureType && guessedFT.size() )
    {
      size_t i;
      bool is=true;
      for( i=0 ; i < vs_featureType.size() ; ++i )
      {
        size_t j;
        size_t sz = guessedFT.size() ;

        for( j=0 ; j< sz ; ++j )
        {
          if( guessedFT[j].find(vs_featureType[i]) < std::string::npos )
          {
            is=false;
            break;
          }
        }

        if( j == sz )
        {
          for( j=0 ; j < sz ; ++j )
          {
            if( vs_featureType[i].find(guessedFT[j]) < std::string::npos )
            {
              is=false;
              break;
            }
          }
        }

        if(is)
          break;
      }

      if( is )
      {
        if( notes->inq(bKey + "9g", n_global) )
        {
          std::string capt("mismatch between provided ");
          if( vs_featureType.size() == 1 )
          {
            capt += hdhC::tf_att(hdhC::empty, n_featureType, vs_featureType[0]) ;
          }
          else
          {
            capt += hdhC::tf_att(n_featureType) ;
            for( size_t i=0 ; i < vs_featureType.size() ; ++i )
            {
              if(i)
                capt += ",";
              else
                capt += "<";

              capt += vs_featureType[i];
            }
            capt += ">";
          }
          capt += " and " ;
          if( guessedFT.size() > 1 )
            capt += "ambiguous " ;
          capt += "guess: ";
          for( size_t i=0 ; i < guessedFT.size() ; ++i )
          {
            if(i)
              capt += ",";
            capt += guessedFT[i];
          }

          (void) notes->operate(capt) ;
          notes->setCheckStatus( n_CF, fail );
        }
      }
    }
  }
  else if( guessedFT.size() )
  {
    // reco for orthogonal multidimensions array representation
    for( size_t i=0 ; i < dv_ix.size() ; ++i)
    {
      Variable& var_dv = pIn->variable[ dv_ix[i] ] ;

      if( chap9_orthoMultDimArray(var_dv, xyzt_ix) )
      {
        if( notes->inq(bKey + "94a", n_global, NO_MT) )
        {
          std::string capt(n_reco + ": File with orthogonal multi-dimensional array ");
          capt += hdhC::tf_var(var_dv.name) + "should have global " ;
          capt += hdhC::tf_att(n_featureType) ;

          (void) notes->operate(capt) ;
          notes->setCheckStatus( n_CF );
        }
      }
      else
      {
        if( notes->inq(bKey + "9i", n_global, NO_MT) )
        {
          std::string capt("suspecting missing " );
          capt += hdhC::tf_att(n_featureType) ;
          capt += ", " ;
          if( guessedFT.size() > 1 )
            capt += "ambiguous " ;
          capt += "guess: ";
          for( size_t i=0 ; i < guessedFT.size() ; ++i )
          {
            if(i)
              capt += ",";
            capt += guessedFT[i];
          }

          (void) notes->operate(capt) ;
          notes->setCheckStatus( n_CF, fail );
        }

        isFeatureType = true;
      }
    }
  }


  // missing value
  chap9_MV(dv_ix) ;

  return;
}

void
CF::chap9_featureType(
  std::vector<std::string> &validFT_vs,
  std::vector<std::string> &vs_featureType)
{
  Variable& glob = pIn->variable[pIn->varSz] ;

  // for checking misspelling by blanks or multiple occurrences
  int att_ix;
  if( (att_ix=glob.getAttIndex(n_featureType)) > -1 )
  {
     isFeatureType=true;

     // check for these invalid cases:
     // 1) misspelling by separation
     // 2) CF-v1.6: multiple featureTypes
     // 3) misspelled
     Split x_str;
     x_str.setSeparator(" ,", true);

     for(size_t i=0 ; i < glob.attValue[att_ix].size() ; ++i )
     {
        x_str = hdhC::Lower()(glob.attValue[att_ix][i]) ;

        for( size_t x=0 ; x < x_str.size() ; ++x)
        {
          double editDistance, eDMin;
          size_t eDMin_ix;
          size_t k;
          eDMin=100.;
          for( k=0 ; k < validFT_vs.size() ; ++k)
          {
            std::string s=validFT_vs[k];
            editDistance = wagnerFischerAlgo(x_str[x], validFT_vs[k]);

            if( editDistance < eDMin )
            {
              eDMin = editDistance ;
              eDMin_ix = k;
            }

            if( editDistance == 0 )
              break;
          }

          if( eDMin < 0.25 && x_str[x] != validFT_vs[eDMin_ix] )
          {
            if( notes->inq(bKey + "0h", n_global) )
            {
              std::string capt("is " +
                hdhC::tf_att(hdhC::empty, n_featureType, x_str[x])) ;
              capt += " misspelled? Did you mean " ;
              capt += validFT_vs[eDMin_ix] + "?";

              (void) notes->operate(capt) ;
              notes->setCheckStatus( n_CF, fail );
            }

            // correction for internal use
            vs_featureType.push_back(validFT_vs[eDMin_ix]);
            continue;
          }

          vs_featureType.push_back(x_str[x]);
        }
     }
  }

  if( cFVal > 15 && vs_featureType.size() > 1 )
  {
    if( notes->inq(bKey + "9a", n_global) )
    {
	  std::string capt(cFVersion);
      capt += " expects only a single value for global ";
      capt += hdhC::tf_att(n_featureType) + ", found" ;
      for( size_t i=0 ; i < vs_featureType.size() ; ++i)
      {
        if(i)
          capt += ",";
        capt += hdhC::tf_val(vs_featureType[i]) ;
      }

      (void) notes->operate(capt) ;
      notes->setCheckStatus( n_CF, fail );
    }
  }

  for( size_t j=0 ; j < vs_featureType.size() ; ++j)
  {
    bool is=true;
    for( size_t i=0 ; i < validFT_vs.size() ; ++i)
    {
      if( validFT_vs[i] == vs_featureType[j] )
      {
         is=false;
         break;
      }
    }

    if(is)
    {
      if( notes->inq(bKey + "9d", n_global) )
      {
        std::string capt("invalid " + n_global + hdhC::blank ) ;
        capt += hdhC::tf_att(n_featureType) ;
        capt += hdhC::tf_val(vs_featureType[j]) ;

        (void) notes->operate(capt) ;
        notes->setCheckStatus( n_CF, fail );
      }
    }
  }

  std::vector<std::pair<size_t, int> > cf_role_ix;
  std::vector<std::pair<size_t, int> > instance_dim_ix;
  std::vector<std::pair<size_t, int> > sample_dim_ix;

  for( size_t i=0 ; i < pIn->varSz ; ++i )
  {
    Variable& var = pIn->variable[i] ;

    // featureType should only be a global attribute
    std::string str(var.getAttValue(n_featureType, lowerCase)) ;
    if( str.size() )
    {
       if( notes->inq(bKey + "9b", var.name) )
       {
         std::string capt("warning: ");
         capt += hdhC::tf_att(var.name, n_featureType, hdhC::upper) ;
         capt += "is confusing as " + n_attribute + " of ";
         capt += hdhC::tf_var(var.name);

         (void) notes->operate(capt) ;
         notes->setCheckStatus( n_CF, fail );
       }
    }

    int j;
    if( (j=var.getAttIndex(n_cf_role)) > -1 )
      cf_role_ix.push_back( std::pair<size_t, int>(i, j) ) ;

    if( (j=var.getAttIndex(n_instance_dimension)) > -1 )
      instance_dim_ix.push_back( std::pair<size_t, int>(i, j) ) ;

    if( (j=var.getAttIndex(n_sample_dimension)) > -1 )
      sample_dim_ix.push_back( std::pair<size_t, int>(i, j) ) ;
  }

  return ;
}

void
CF::chap9_getSepVars(std::vector<int>& xyzt_ix, std::vector<size_t>& dv_ix)
{
   for( size_t j=0 ; j < 4 ; ++j )
      xyzt_ix.push_back(-1) ;

   for(size_t i=0 ; i < pIn->varSz ; ++i )
   {
      bool isXYZT[4];
      for( size_t c=0 ; c < 4 ; ++c )
        isXYZT[c] = pIn->variable[i].coord.isC[c] ;

      size_t m;
      for(m=0 ; m < 4 ; ++m)
      {
        // for multiple coordinates for a given type,
        // select the one with the lowest number of dimensions
        if( isXYZT[m] )
        {
          if(xyzt_ix[m] > -1 )
          {
            Variable& var_0 = pIn->variable[ xyzt_ix[m] ] ;
            Variable& var_1 = pIn->variable[i] ;
            if( var_0.dimName.size() < var_1.dimName.size() )
              continue;
          }

          xyzt_ix[m] = i ;
          break;
        }
      }

      if( m == 4 && pIn->variable[i].isDataVar() && !pIn->variable[i].isScalar )
        dv_ix.push_back( i );
   }

   return;
}

std::vector<std::string>
CF::chap9_guessFeatureType(std::vector<std::string> &validFT_vs,
  std::vector<int>& xyzt_ix, std::vector<size_t>& dv_ix )
{
   std::vector<std::string> guessedFT;

   if( dv_ix.size() && chap9_horizontal(xyzt_ix) )
   {
      // for each variant of all featureTypes: when X and Y are given, they
      // share the same dimensions. Only the X-index will be used below.
      // When y s given only, the index was mapped to xyzt[0].

      bool isPoint;
      if( (isPoint=chap9_point(xyzt_ix, dv_ix)) )
        guessedFT.push_back( validFT_vs[0] ) ;

      if( chap9_timeSeries(xyzt_ix, dv_ix) )
        guessedFT.push_back( validFT_vs[1] ) ;

      if( chap9_profile(xyzt_ix, dv_ix) )
        guessedFT.push_back( validFT_vs[2] ) ;

      // note that there could be a case which is point and trajectory
      if( chap9_trajectory(xyzt_ix, dv_ix) )
        guessedFT.push_back( validFT_vs[3] ) ;

      if( chap9_timeSeriesProfile(xyzt_ix, dv_ix) )
        guessedFT.push_back( validFT_vs[4] ) ;

      if( chap9_trajectoryProfile(xyzt_ix, dv_ix) )
        guessedFT.push_back( validFT_vs[5] ) ;
   }

   if( cFVal > 15 && guessedFT.size() > 1 )  // may be adusted in the future
   {
     std::string s(guessedFT[0]); // precedence: top-down first
     guessedFT.clear();
     guessedFT.push_back(s);
   }

   return guessedFT ;
}

bool
CF::chap9_horizontal(std::vector<int>& xyzt_ix)
{
  // when both horizontal dimensions X and Y are given, then
  // they are identical for all featureTypes.
  // If X is not in use, then Y is mapped to X.

  if( xyzt_ix[0] == -1 && xyzt_ix[1] == -1 )
     return false;  // not a featureType

  else if( xyzt_ix[0] == -1)
    xyzt_ix[0] = xyzt_ix[1] ; // and true

  //else if(xyzt_ix[1] == -1) // X is in use
  //  is=true;

  else if( xyzt_ix[0] > -1)
  {
    Variable& var_x = pIn->variable[ xyzt_ix[0] ] ;
    Variable& var_y = pIn->variable[ xyzt_ix[1] ] ;

    for(size_t i=0 ; i < var_x.dimName.size() ; ++i )
      if( var_x.dimName[i] != var_y.dimName[i] )
         return false;
  }

  return true;
}

void
CF::chap9_MV(std::vector<size_t>& dv_ix)
{
  /* the chapter of CF-1.6 about Missing Data is impractical as a guide
     for a check. Thus, a hierachical procedure is used here to annotate
     any combination between auxiliaries and data variables as to MV.
     0) MV of a scalar aux that is named by a coordinate attribute
        ( this has been checked before).
     1) index considerations between 1-D auxiliaries and corresponding data vars.
     2) multi-dim indexes of auxiliaries and corresponding data variables.
  */
/*
  if( ! notes->inq(bKey + "9.6", hdhC::empty, "INQ_ONLY" ) )
    return ;  // checks are selected to be discarded by 'D'

  for( size_t i=0 ; i < dv_ix.size() ; ++i )
  {
    Variable& var = pIn->variable[dv_ix[i]] ;

  }
*/

  return ;
}

bool
CF::chap9_orthoMultDimArray(Variable& var, std::vector<int>& xyzt_ix)
{
  // characteristics of OMDA:
  // a) each coordinate variable depends on only a single dimension,
  // b) the data variable has to depend on all the dimensions of a),
  // c) the data variable has to have more than a single dimension.
  std::vector<std::string> vsd;

  for(size_t j=0 ; j < xyzt_ix.size() ; ++j)
  {
    int c = xyzt_ix[j];
    if( c > -1 )
    {
      Variable& var_c = pIn->variable[c];

      if( var_c.dimName.size() != 1 )
        return false;

      vsd.push_back( var_c.dimName[0] ) ;
    }
  }

  if( var.dimName.size() > 1
        && hdhC::isAmong(vsd, var.dimName, true) )
    return true;

  return false;
}

bool
CF::chap9_point(std::vector<int>& xyzt_ix, std::vector<size_t>& dv_ix)
{
  // dv_ix.size() > 0 was checked before

  // all with a single, identical dimension.

  // Two exceptions with a second special dimension:
  // a) var of char Type,
  // b) bounds with vertices.
  // Note that this is not covered by CF-1.6.

  std::string dName;

  for( size_t i=0 ; i < pIn->varSz ; ++i )
  {
    Variable& var = pIn->variable[i] ;

    size_t sz=1;

    if( var.boundsOf.size() )
      ++sz;
    if( var.type == NC_CHAR )
      ++sz;

    if( var.dimName.size() != sz )
      return false;

    if( !dName.size() )
      dName = var.dimName[0] ;

    if( var.dimName[0] != dName )
      return false;
  }

  return true;
}

bool
CF::chap9_profile(std::vector<int>& xyzt_ix, std::vector<size_t>& dv_ix)
{
  // def.: an ordered set of datapoints along a vertical line at a fixed
  // horizontal position and fixed time.


  // dv_ix.size() > 0 was checked before

  // dv_ix.size() > 0 was checked before
  int x_ix = xyzt_ix[0] ;
//  int y_ix = xyzt_ix[1] ;
  int z_ix = xyzt_ix[2] ;
  int t_ix = xyzt_ix[3] ;

  if( z_ix == -1 )
    return false;

  Variable& var_z = pIn->variable[z_ix] ;

  // a profile should have more than a single Z value
  if( var_z.isScalar )
    return false;
  if(var_z.dimName.size() == 1 && pIn->nc.getDimSize(var_z.dimName[0]) == 1)
    return false;

  // dims of time must be identical to those of the geo-coords
  if( x_ix > -1 )
  {
    if( t_ix == -1 )
      return false;

    Variable& var_x = pIn->variable[x_ix] ;
    Variable& var_t = pIn->variable[t_ix] ;

    if( var_t.dimName.size() > 1 )
      return false;
    else if( var_t.dimName.size() != var_x.dimName.size() )
      return false;
    else
    {
      for(size_t i=0 ; i < var_t.dimName.size() ; ++i)
        if( var_x.dimName[i] != var_t.dimName[i] )
          return false;
    }
  }

  // dims of Z are not shared by X
  if( x_ix > -1 )
  {
    Variable& var_x = pIn->variable[x_ix] ;

    if( hdhC::isAmong(var_z.dimName, var_x.dimName) )
      return false;
  }

  // dims of Z are not shared by T
  if( t_ix > -1 )
  {
    Variable& var_t = pIn->variable[t_ix] ;

    if( hdhC::isAmong(var_z.dimName, var_t.dimName) )
      return false;
  }

  // dims of Z must share those of the data variable
  for( size_t i=0 ; i < dv_ix.size() ; ++i )
  {
    Variable& var = pIn->variable[dv_ix[i]] ;

    // false: all items of the first vector must be in the second one
    if( ! hdhC::isAmong(var_z.dimName, var.dimName, false) )
      return false;
  }

  return true;
}

void
CF::chap9_sample_dimension(std::vector<size_t>& dv_ix)
{
  // Examine attribute sample_dimension and scan for a missing one.
  // Results gained here are used to scan for a missing instance_dimension
  // attribute, too.

  // some simple checks
  std::vector<size_t> vi_IT_ix;  // indices of potential index variables

  for( size_t i=0 ; i < pIn->varSz ; ++i )
  {
    Variable& var = pIn->variable[i];

    if( pIn->nc.isIndexType(var.name) )
    {
      vi_IT_ix.push_back(i);
      continue;
    }

    bool is[2];
    is[0]=false;
    is[1]=false;

    if( var.isValidAtt(n_sample_dimension) )
      is[0] = true;
    if( var.isValidAtt(n_instance_dimension) )
      is[1] = true;

    if( is[0] || is[1] )
    {
      if( notes->inq(bKey + "932c", var.name) )
      {
        std::string capt(hdhC::tf_var(var.name, hdhC::colon));
        if( is[0] && is[1] )
        {
          capt += hdhC::tf_att(n_sample_dimension) ;
          capt += "and " ;
          capt += hdhC::tf_att(n_instance_dimension) ;
        }
        else if( is[0] )
          capt += hdhC::tf_att(n_sample_dimension) ;
        else
          capt += hdhC::tf_att(n_instance_dimension) ;

        capt += "is only allowed for index variables";

        (void) notes->operate(capt) ;
        notes->setCheckStatus( n_CF, fail );
      }
    }
  }

  if( vi_IT_ix.size() == 0 )
    return;

  std::vector<size_t> vi_sD_ix;
  std::vector<int> vi_sum;
  std::vector<int> vi_dSz;

  // scan for attribute sample_dimension
  for( size_t i=0 ; i < vi_IT_ix.size() ; ++i )
  {
    Variable& var = pIn->variable[vi_IT_ix[i]];

    int j=-1;
    if( (j=var.getAttIndex(n_sample_dimension)) > -1 )
    {
      var.addAuxCount();

      vi_sD_ix.push_back(i);
      vi_sum.push_back(0);
      vi_dSz.push_back(0);

      // does the dimension named by sample_dimension exist?
      if( ! pIn->nc.isDimValid(var.attValue[j][0]) )
      {
        if( notes->inq(bKey + "932d", var.name) )
        {
          std::string capt(
            hdhC::tf_att(var.name, n_sample_dimension, var.attValue[j][0]));
          capt += "does not name a valid dimension";

          (void) notes->operate(capt) ;
          notes->setCheckStatus( n_CF, fail );
        }

        continue;
      }

      // does the sum of values var which contains the sample_dimension att
      // match the dimension indicated by att's value?

      // get lenths of sequences and add together; missing of data was
      // checked previously
      MtrxArr<int> ma;
      (void) pIn->nc.getData(ma, var.name, 0, -1);

      vi_sum.back()=0;
      vi_dSz.back() = pIn->nc.getDimSize(var.attValue[j][0]) ;

      if( ma.validRangeBegin.size() == 1 )
      {
        for(size_t i=ma.validRangeBegin[0] ; i < ma.validRangeEnd[0] ; ++i )
          vi_sum.back() += ma[i] ;
      }

      if( vi_sum.back() != vi_dSz.back() )
      {
        if( notes->inq(bKey + "932a", var.name) )
        {
          std::string capt(hdhC::tf_att(var.name, n_sample_dimension, hdhC::colon)) ;
          capt += "The sum of data values=" ;
          capt += hdhC::itoa(vi_sum.back());

          if( vi_sum.back() > vi_dSz.back() )
            capt += " over";
          else
            capt += " under";

          capt += "determines " + n_dimension ;
          capt += hdhC::tf_val(var.attValue[j][0]) ;
          capt += "=" ;
          capt += hdhC::itoa(vi_dSz.back()) ;

          (void) notes->operate(capt) ;
          notes->setCheckStatus( n_CF, fail );
        }
      }
    }
  }

  std::string dVarDimName ;
  for(size_t i=0 ; i < pIn->varSz ; ++i )
  {
    Variable& var = pIn->variable[i];

    if( var.isDataVar() && var.dimName.size() == 1 )
    {
      dVarDimName=pIn->variable[i].dimName[0];
      break;
    }
  }

  // sample_dimension att not found: scan for potential s-d variables
  if( vi_sD_ix.size() == 0 && dVarDimName.size() )
  {
    for( size_t i=0 ; i < vi_IT_ix.size() ; ++i )
    {
      Variable& var = pIn->variable[vi_IT_ix[i]];

      if( var.isUnlimited() )
        continue;
      if( var.coord.isCoordVar )
        continue;
//      if( var.getAttIndex(n_instance_dimension) == -1 )
//       continue;
      if( var.dimName.size() != 1 )
        continue;

      // the sum of values must match the dimension of the data variables

      MtrxArr<int> ma;

      // get lenths of sequences and add together
      (void) pIn->nc.getData(ma, var.name, 0, -1) ;
      int sum = 0;
      int dSz( pIn->nc.getDimSize(dVarDimName) ) ;

      for(size_t j=ma.validRangeBegin[0] ; j < ma.validRangeEnd[0] ; ++j )
        sum += ma[j] ;

      // sum vs. the size of the dimension whose name would be assigned
      // to sample_dimension

      if( sum == dSz )
      {
        var.addAuxCount();

        vi_sum.push_back(sum);
        vi_dSz.push_back(dSz) ;
        vi_sD_ix.push_back(i) ;

        if( notes->inq(bKey + "932b", var.name) )
        {
          std::string capt("index ");
          capt += hdhC::tf_var(var.name, hdhC::colon);
          capt += "suspicion of missing ";
          capt += hdhC::tf_att(n_sample_dimension);

          (void) notes->operate(capt) ;
          notes->setCheckStatus( n_CF, fail );
        }
      }
    }
  }

  // instance_dimension may be used in two modes: sample_dimension is available
  // somewhere else or not.
  bool isFault=false;
  size_t i;

  if( vi_sD_ix.size() == 1 )
  {
    // i) sample_dimension att is available somewhere else
    if( dVarDimName.size() == 0 )
      return;

    for( i=0 ; i < vi_IT_ix.size() ; ++i )
    {
      Variable& var = pIn->variable[vi_IT_ix[i]];

      // ii) the dimName s-d points to is the same as in a data variable
      Variable& sD_var = pIn->variable[vi_sD_ix[0] ];

      if( sD_var.getAttValue(n_sample_dimension) == dVarDimName )
      {
        // iii) the dim of the var the sample_dim att points to is the same
        //      as the dime of the index variable
        if( var.dimName[0] == dVarDimName )
        {
          isFault=true; // attribute is missing
          break;
        }
      }
    }
  }
  else if(vi_sD_ix.size() )
  {
    // too many sample_dimension attributes
    if( notes->inq(bKey + "932f") )
    {
      std::string capt("multiple ");
      capt += n_sample_dimension ;
      capt += "attributes are available, found for variable ";
      for(i=0 ; i < vi_sD_ix.size() ; ++i )
      {
        if(i)
          capt += ", ";
        capt += pIn->variable[vi_sD_ix[i]].name;
      }

      (void) notes->operate(capt) ;
      notes->setCheckStatus( n_CF, fail );
    }

    return;
  }
  else
  {
    // i) no sample_dimension att
    if( dVarDimName.size() == 0 )
      return;

    for( i=0 ; i < vi_IT_ix.size() ; ++i )
    {
      Variable& var = pIn->variable[vi_IT_ix[i]];

      if( var.dimName.size() == 1 )
      {
        int j=-1;
        if( (j=var.getAttIndex(n_instance_dimension)) == -1 )
        {
          // ii) the dim of variable with s-d is the same as in a data variable
          if( var.dimName[0] == dVarDimName )
            isFault = true;
        }
      }
    }
  }

  if( isFault && notes->inq(bKey + "932e", pIn->variable[i].name) )
  {
    std::string capt("index ");
    capt += hdhC::tf_var(pIn->variable[i].name, hdhC::colon);
    capt += "suspicion of missing ";
    capt += hdhC::tf_att(n_instance_dimension);

    (void) notes->operate(capt) ;
    notes->setCheckStatus( n_CF, fail );
  }

  return ;
}

bool
CF::chap9_timeSeries(std::vector<int>& xyzt_ix, std::vector<size_t>& dv_ix)
{
  // dv_ix.size() > 0 was checked before
  int x_ix = xyzt_ix[0] ;
  int z_ix = xyzt_ix[2] ;
  int t_ix = xyzt_ix[3] ;

  if( t_ix == -1 )
    return false;

  Variable& var_t = pIn->variable[t_ix] ;

  // a TS should have more than a single time value
  if( var_t.isScalar )
    return false;
  if(var_t.dimName.size() == 1 && pIn->nc.getDimSize(var_t.dimName[0]) == 1)
    return false;

  // Z should be only scalar (per X or at all)
  if( z_ix > -1 )
  {
    Variable& var_z = pIn->variable[z_ix] ;
    if( !var_z.isScalar )
    {
      if( x_ix > -1 )
      {
        Variable& var_x = pIn->variable[x_ix] ;
        if( var_z.dimName.size() == var_x.dimName.size() )
        {
          for(size_t i=0 ; i < var_x.dimName.size() ; ++i )
            if( var_x.dimName[i] != var_z.dimName[i] )
              return false;
        }
        else
          return false;
      }
      else
        return false;
    }
  }

  // dims of time must be different from those of the geo-coords
  if( x_ix > -1 )
  {
    Variable& var_x = pIn->variable[x_ix] ;

    if( var_t.dimName.size() == var_x.dimName.size() )
    {
      for(size_t i=0 ; i < var_t.dimName.size() ; ++i)
        if( var_x.dimName[i] == var_t.dimName[i] )
          return false; // not a timeSeries
    }
  }

  // data variable vs. time variable
  for( size_t i=0 ; i < dv_ix.size() ; ++i )
  {
    Variable& var = pIn->variable[dv_ix[i]] ;

    //  i) dims are the same
    if( var.dimName.size() == var_t.dimName.size() )
    {
      for( size_t j=0 ; j < var.dimName.size() ; ++j )
        if( var.dimName[j] != var_t.dimName[j] )
            return false;
    }

    // ii) the only dimensions of time is shared by the data variables
    else
    {
      if( var_t.dimName.size() != 1 )
        return false;
      else if( ! hdhC::isAmong(var_t.dimName[0], var.dimName) )
        return false;
    }
  }

  return true;
}

bool
CF::chap9_trajectory(std::vector<int>& xyzt_ix, std::vector<size_t>& dv_ix)
{
  // dv_ix.size() > 0 was checked before
  int x_ix = xyzt_ix[0] ;
  int z_ix = xyzt_ix[2] ;
  int t_ix = xyzt_ix[3] ;

  // a trajectory must have horizontal coordinates, otherwise it
  // would be called profile or single point.
  if( x_ix == -1 )
    return false;

  Variable& var_x = pIn->variable[x_ix] ;

  // dims of T and X must be identical
  if( t_ix > -1 )
  {
    Variable& var_t = pIn->variable[t_ix] ;

    if( var_t.dimName.size() != var_x.dimName.size() )
      return false;

    for(size_t i=0 ; i < var_t.dimName.size() ; ++i)
      if( var_x.dimName[i] != var_t.dimName[i] )
        return false;
  }

  // a set of trajectories may have z() or z(t) per each, but
  // not a profile
  if( z_ix > -1 )
  {
    Variable& var_z = pIn->variable[z_ix] ;

    if( ! (var_z.isScalar
          || (var_z.dimName.size() == 1
                && pIn->nc.getDimSize(var_z.dimName[0])) ) )
    {
      // if Z, then only once per each X, i.e. same dims
      size_t i;
      for(i=0 ; i < var_z.dimName.size() ; ++i)
        if( ! hdhC::isAmong(var_z.dimName[i], var_x.dimName ) )
          break;

      if( i < var_z.dimName.size() )
        return false;
    }
  }

  // data variable vs. X must have identical dims sets
  for( size_t i=0 ; i < dv_ix.size() ; ++i )
  {
    Variable& var = pIn->variable[dv_ix[i]] ;

    if( var.dimName.size() != var_x.dimName.size() )
      return false;

    for(size_t i=0 ; i < var.dimName.size() ; ++i)
      if( var_x.dimName[i] != var.dimName[i] )
        return false;
  }

  return true;
}

bool
CF::chap9_timeSeriesProfile(std::vector<int>& xyzt_ix, std::vector<size_t>& dv_ix)
{
  // dv_ix.size() > 0 was checked before
  int x_ix = xyzt_ix[0] ;
  int z_ix = xyzt_ix[2] ;
  int t_ix = xyzt_ix[3] ;

  if( t_ix == -1 || z_ix == -1 )
    return false;

  Variable& var_t = pIn->variable[t_ix] ;
  Variable& var_z = pIn->variable[z_ix] ;

  // Z should not be scalar or 1-D of size==1
  if( var_z.isScalar )
    return false;
  if( var_z.dimName.size() == 1 && pIn->nc.getDimSize(var_z.dimName[0]) == 1)
    return false;

  // T should not be scalar or 1-D of size==1
  if( var_t.isScalar )
    return false;
  if( var_t.dimName.size() == 1 && pIn->nc.getDimSize(var_t.dimName[0]) == 1)
    return false;

  if( x_ix > -1 )
  {
    Variable& var_x = pIn->variable[x_ix] ;

    // Z should have dim not in X + T
    size_t i;
    for(i=0 ; i < var_z.dimName.size() ; ++i)
    {
      if( ! hdhC::isAmong(var_z.dimName[i], var_x.dimName ) )
        if( ! hdhC::isAmong(var_z.dimName[i], var_t.dimName ) )
          break;
    }
    if( i==var_z.dimName.size() )
      return false;

    // all dims of X and T should be different
    if( hdhC::isAmong(var_t.dimName, var_x.dimName, true ) )
      return false;

    if( var_x.dimSize.size() > 1 )
        return false; // distinguished from a multidimensional array representation
  }

  // data variable vs. Z
  for( size_t i=0 ; i < dv_ix.size() ; ++i )
  {
    Variable& var = pIn->variable[dv_ix[i]] ;

    // if num of var.dims > 1, then these should match the effDims
    if( var.dimName.size() > 1 && var.dimName.size() == effDims_ix.size() )
    {
      for(size_t d=0 ; d < effDims_ix.size() ; ++d )
        if( ! hdhC::isAmong( dimensions[ effDims_ix[d] ], var.dimName) )
          return false;
    }

    if( var.dimName.size() == 1 && var_z.dimName.size() == 1 )
    {
      // dim of var and Z must be the same
      if( var.dimName[0] != var_z.dimName[0] )
        return false;
    }
  }

  return true;
}

bool
CF::chap9_trajectoryProfile(std::vector<int>& xyzt_ix, std::vector<size_t>& dv_ix)
{
  // dv_ix.size() > 0 was checked before
  int x_ix = xyzt_ix[0] ;
  int z_ix = xyzt_ix[2] ;
  int t_ix = xyzt_ix[3] ;

  if( x_ix == -1 || z_ix == -1 )
    return false;

  Variable& var_x = pIn->variable[x_ix] ;

  if( var_x.isScalar )
    return false;

  Variable& var_z = pIn->variable[z_ix] ;

  if( var_z.isScalar )
    return false;

/*
  if( var_z.isScalar
          || (var_z.dimName.size() == 1 && pIn->nc.getDimSize(var_z.dimName[0]) ) )
    return false;
*/

  // dims of time, if given, and horizontal coord must be identical
  if( t_ix > -1 )
  {
    Variable& var_t = pIn->variable[t_ix] ;

    if( var_t.dimName.size() != var_x.dimName.size() )
      return false;

    for(size_t i=0 ; i < var_t.dimName.size() ; ++i)
      if( var_x.dimName[i] != var_t.dimName[i] )
        return false;

    // Z should have a dim not T
    size_t i;
    for(i=0 ; i < var_z.dimName.size() ; ++i)
      if( ! hdhC::isAmong(var_z.dimName[i], var_t.dimName ) )
        break;

    if( i==var_z.dimName.size() )
      return false;

  }

  // Z should have a dim not in X
  size_t i;
  for(i=0 ; i < var_z.dimName.size() ; ++i)
    if( ! hdhC::isAmong(var_z.dimName[i], var_x.dimName ) )
      break;

  if( i==var_z.dimName.size() )
    return false;

  // data variable vs. Z
  for( size_t i=0 ; i < dv_ix.size() ; ++i )
  {
    Variable& var = pIn->variable[dv_ix[i]] ;

    // Z) dims of dv and Z are the same
    if( var.dimName.size() != var_z.dimName.size() )
      return false;

    for( size_t j=0 ; j < var.dimName.size() ; ++j )
      if( var.dimName[j] != var_z.dimName[j] )
        return false;

    //
  }

  return true;
}

// note that the last group are not names of attributes, but commonly used words
// to help the spell checker.
// IMPORTANT: CF_AttType.size() only for real attributes

const char* CF_AttName[] = {
  "standard_error_multiplier"          ,
  "ancillary_variables", "axis"        , "calendar"     , "bounds"       , "cell_measures"
, "cell_methods"       , "climatology" , "comment"      , "compress"     , "Conventions"
, "coordinates"        , "_FillValue"  , "flag_masks"   , "flag_meanings", "flag_values"
, "formula_terms"      , "grid_mapping", "history"      , "institution"  , "leap_month"
, "leap_year"          , "long_name"   , "missing_value", "month_lengths", "positive"
, "references"         , "scale_factor", "source"       , "add_offset"   , "valid_range"
, "standard_name"      , "title"       , "units"        , "valid_max"    , "valid_min"
, "grid_mapping_name"

, "earth_radius"                    , "false_easting"                       , "false_northing"
, "grid_north_pole_latitude"        , "grid_north_pole_longitude"           , "inverse_flattening"
, "latitude_of_projection_origin"   , "longitude_of_central_meridian"       , "longitude_of_prime_meridian"
, "longitude_of_projection_origin"  , "north_pole_grid_longitude"           , "perspective_point_height"
, "scale_factor_at_central_meridian", "scale_factor_at_projection_origin"   , "semi_major_axis"
, "semi_minor_axis"                 , "standard_parallel"   , "straight_vertical_longitude_from_pole"

, "variance"
};

const char CF_AttType[] = {
  'N'
, 'S'                  , 'S'           , 'S'            , 'S'            , 'S'
, 'S'                  , 'S'           , 'S'            , 'S'            , 'S'
, 'S'                  , 'D'           , 'D'            , 'S'            , 'D'
, 'S'                  , 'S'           , 'S'            , 'S'            , 'N'
, 'N'                  , 'S'           , 'D'            , 'N'            , 'S'
, 'S'                  , 'N'           , 'S'            , 'N'            , 'N'
, 'S'                  , 'S'           , 'S'            , 'N'            , 'N'
, 'S'

,'N', 'N', 'N'
,'N', 'N', 'N'
,'N', 'N', 'N'
,'N', 'N', 'N'
,'N', 'N', 'N'
,'N', 'N', 'N'
};

std::vector<std::string> CF::attName(CF_AttName, CF_AttName + 56);
std::vector<char>        CF::attType(CF_AttType, CF_AttType + 55);
