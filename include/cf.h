#ifndef _CF_14_H
#define _CF_14_H

#include "hdhC.h"
#include "iobj.h"
#include "annotation.h"
#include "variable.h"
#include "udunits2.h"
#include "converter.h"

class CF : public IObj
{
  public:

  std::map<std::string, int> vIx;

  struct SN_Entry
  {
//     SN_Entry(std::string &sn){name = sn; found=false;}

     std::string name;
     std::string std_name;
     std::string remainder;
     bool        found;
     std::string alias;
     std::string canonical_units;
     std::string amip;
     std::string grib;
  };

  CF();
  ~CF(){;}

  //! coresponding to virtual methods in IObj
  bool   closeEntryTime(void){return false;}
  bool   entry(void);
  bool   init(void) ;
  void   linkObject(IObj *);

  void   setFilename(hdhC::FileSplit& f) {fSplit=f;}
  void   setFilename(std::string f) {fSplit.setFile(f);}
  void   setTablePath(std::string p) {tablePath=p;}

  void   chap(void);
  void   chap_reco(void);

  void   chap2(void);       // names
  void   chap2_reco(void);      // NetCDF files and components
  void   chap21(void);     // filename
  void   chap22(void);     // dimensions
  void   chap23(void);     // names
  void   chap23_reco(void);    // names
  void   chap24(void);     // dimensions
  void   chap24_reco(void);    // dimensions of a variable
  void   chap251(void);   // missing data
  void   chap26(void);     // attributes
  bool   chap261(void);   // convention
  void   chap262_reco(void);  // title and history

  void   chap3(void);                  //
  void   chap3_reco(void);      // description of data
  void   chap33(void);     // standard_name etc.
  void   chap34(void);     // ancillary_variables
  void   chap35(void);     // flags
  void   chap35_reco(void);    // description of data

  void   chap4(void);                  // Coordinate Types
  bool   chap4(Variable&);      // Coordinate Types
  bool   chap41(Variable&);    // lat/lon coordinate
  bool   chap43(Variable&);    // vertical coordinate
  bool   chap431(Variable&);  // vertical dimensional coord
  void   chap432(void);       // vertical dimensionless coord
  bool   chap432(Variable&, std::vector<std::string>&, std::vector<std::string>&,
                 int valid_ft_ix, int valid_sn_ix, int ft_jx, int sn_jx,
                 std::vector<std::pair<std::string, std::string> > &) ;
  // check of standard_name vs. formula_terms (case: dimless vertical coord)
  bool   chap432_checkSNvsFT( Variable& var,
            std::vector<std::string>& valid_sn,
            std::vector<std::string>& valid_ft,
            int& valid_sn_ix,
            int& ft_ix, int& sn_ix, std::string& units );
  void   chap432_deprecatedUnits(Variable&, std::string &units);
  void   chap432_getParamVars( Variable&,
            std::vector<std::string>& valid_sn,
            std::vector<std::string>& valid_ft,
            int& valid_ft_ix, int& valid_sn_ix, int att_ft_ix,
            std::vector<std::pair<std::string, std::string> >& att_ft_pv) ;
  void   chap432_verify_FT(Variable&, int, std::string &reqFormTerms,
            int att_ft_ix, std::vector<std::string> &fTerms,
            std::vector<std::pair<std::string, std::string> >& p_found_ft);
  bool   chap44(Variable&);    // time
  void   chap44a_reco(Variable&);   // ref time 0
  bool   chap441(Variable&);  // calendar

  void   chap5(void);    // Coordinate Systems
  void   chap5_reco(void);      // coordinate types
  void   chap50(void);  // dim-list of variables; post-poned
  void   chap51(Variable&);  // coordinate variables
  void   chap52(void);  // coordinate attribute(s)
  void   chap53(void);  // reduced horizontal grid:
//  void   chap54(void);  // time series of station data
//  void   chap55(void);  // trajectories: checked implicitely
  void   chap56(void);  // grid mapping
  int    chap56_gridMappingVar(Variable& dv, std::string &, std::string);
  void   chap56_attProps(Variable& dataVar,
            std::string mCV[]);       // map coordinates

  void   chap6(void);    // labels

  void   chap7(void);    // cells
  void   chap71(void);  // pertains to both boundaries and climatologies
  void   chap71_reco(Variable&);   // cell boundaries
  void   chap72(void);  // cell measure
  void   chap73(void);  // cell methods
  bool   chap73_cellMethods_Comment(std::string&, Variable&) ;
  bool   chap73_cellMethods_Method(std::string&, Variable&) ;
  bool   chap73_cellMethods_Name(std::string&, Variable&) ;
  void   chap73_inqBounds(Variable&,  std::vector<std::string>& name,
                          std::vector<std::string>& method, bool );
  void   chap73b_reco(Variable&, std::vector<std::string> &dim );
  bool   chap733(std::string& method, Variable&, std::string mode) ;
  bool   chap734a(std::string&) ;
  void   chap734b(Variable&, std::vector<std::string> &dim, std::vector<std::string> &method);
  void   chap74a(void);  // climatological statistics
  bool   chap74b(Variable&, std::vector<std::string> &name, std::vector<std::string> &method) ;

  void   chap8(void);    // reduction of data size
  void   chap81(Variable&);  // packed data
  void   chap82(Variable&);  // compression by gathering (scal_factor, offset)

  void   chap9(void);    // discrete sampling geometries (CF-1.6)
  void   chap9_featureType(std::vector<std::string> &validFeatureType,
            std::vector<std::string> &featureType) ;
  void   chap9_getSepVars(std::vector<int>& xyzt_ix, std::vector<size_t>& dv_ix);
  std::vector<std::string>
         chap9_guessFeatureType(std::vector<std::string> &featureType,
           std::vector<int>& xyzt_ix, std::vector<size_t>& dv_ix) ;
  bool   chap9_horizontal(std::vector<int>& xyzt_ix);
  void   chap9_MV(std::vector<size_t>& dv_ix);
  bool   chap9_orthoMultDimArray(Variable&, std::vector<int>& xyzt_ix);
  bool   chap9_point(std::vector<int>& xyzt_ix, std::vector<size_t>& dv_ix);
  bool   chap9_profile(std::vector<int>& xyzt_ix, std::vector<size_t>& dv_ix);
  void   chap9_sample_dimension(std::vector<size_t>& dv_ix);
  bool   chap9_timeSeries(std::vector<int>& xyzt_ix, std::vector<size_t>& dv_ix);
  bool   chap9_timeSeriesProfile(std::vector<int>& xyzt_ix, std::vector<size_t>& dv_ix);
  bool   chap9_trajectory(std::vector<int>& xyzt_ix, std::vector<size_t>& dv_ix);
  bool   chap9_trajectoryProfile(std::vector<int>& xyzt_ix, std::vector<size_t>& dv_ix);

  void   applyOptions(void);
  void   analyseCoordWeights(void);
  void   attributeSpellCheck(void);

  void   checkCoordinateValues(Variable&, bool isFormTermAux=false) ;
template <typename T>
  void   checkCoordinateValues(Variable&, bool, T);
  void   checkCoordinateFillValueAtt(Variable&) ;
  void   checkGroupRelation(void);

  // variables are dimension of another variable
  bool   checkRelationAsDimension(std::vector<bool>&);
  // a dimension of a variable is shared by another variable
  bool   checkRelationByDimension(std::vector<bool>&);
  bool   checkRelationCF16(std::vector<bool>&);
  bool   checkRelationScalar(std::vector<bool>&);
  void   checkSN_Modifier(Variable &);
  bool   cmpUnits( std::string s, std::string ref);
  void   enableCheck(void){isCheck=true;}
  int    finally(int eCode=0);
  void   finalAtt(void) ;
  void   finalAtt_axis(void);
  void   finalAtt_coordinates(void);
  void   finalAtt_coordinates_A(void);
  void   finalAtt_coordinates_B(void);
  void   finalAtt_coordinates_C(void);
  void   finalAtt_coordinates_D(void);
  void   finalAtt_positive(void);
  void   finalAtt_units(void);
  void   final_dataVar(void);
  void   findAmbiguousCoords(void);
  void   findCellMeasures(Variable&);
  void   findIndexVar(void);
  bool   findLabel(Variable&);
  void   getAssociatedGroups(void);
  void   getDims(void);
  std::vector<std::string>
         // mode: "key", "arg", "1st", "2nd". tail=last char of key
         getKeyWordArgs(std::string&, std::string mode="arg", char tail='\0');
  void   getSN_TableEntry(void);
  void   getVarStateEvidences(Variable&);
  void   hasBounds(Variable&);
  void   initDefaults(void);
  bool   isBounds(Variable&);
  bool   isChap9_specialLabel(Variable& label, Variable& var);
  bool   isCompressAux(Variable&);
  bool   isCompressEvidence(Variable&, bool*) ; // bool* for int-type
//  bool   isCompliant(void){ return isCF;}
  bool   isLatitude(void);
  bool   isLongitude(void);
  bool   isXYZinCoordAtt(size_t v_ix);
  bool   parseUnits( std::string s);
  void   postAnnotations(void);

  bool   run(void);
  bool   scanStdNameTable(std::vector<int>& zx);
  bool   scanStdNameTable(ReadLine&, Variable&, std::string);
  bool   setCheck(std::string&);
  void   setCheckStatus(std::string);
  void   setDataVarName(std::string s){ dataVarName = s;}
  void   setFollowRecommendations(bool b){followRecommendations=b;}
//  void   setTable(std::string p){ std_name_table=p; }

  bool   timeUnitsFormat(Variable&, bool annot=true);
  bool   timeUnitsFormat_frq(std::string, std::vector<std::string>& capt,
                             std::vector<std::string>& text);
  bool   timeUnitsFormat_key(std::string,  std::vector<std::string>& capt,
                             std::vector<std::string>& text);
  bool   timeUnitsFormat_date(Variable&, std::string,
                             std::vector<std::string>& capt,
                             std::vector<std::string>& text, bool annot=true);
  bool   timeUnitsFormat_time(std::string,  std::vector<std::string>& capt,
                             std::vector<std::string>& text);
  bool   timeUnitsFormat_TZ(std::string,  std::vector<std::string>& capt,
                             std::vector<std::string>& text);

  std::string
         units_lon_lat(Variable&, std::string units="");
  double wagnerFischerAlgo(std::string&, std::string&);

  bool isCF14_timeSeries;

  bool isCheck;  // true: perform checks, false: only set variable values
  size_t cFVal;  // e.g. CF-1.6 --> cFVal=16
  bool followRecommendations; // false by default
  bool isFeatureType;
  std::string cFVersion;

  std::string tablePath;
  hdhC::FileSplit fSplit;
  hdhC::FileSplit std_name_table;
  hdhC::FileSplit area_table;
  hdhC::FileSplit region_table;

  ut_system*   unitSystem;

  std::string dataVarName;  // only when provided by setDataVarName()
  std::string timeName;  // the name of the unlimited/time variable
  int         time_ix;
  int         compress_ix;

  // a few names of attributes used throughout the checks
  std::string n_ancillary_variables;
  std::string n_area;
  std::string n_attribute;
  std::string n_axis;
  std::string n_bounds;
  std::string n_calendar;
  std::string n_cell_measures;
  std::string n_cell_methods;
  std::string n_cf_role;
  std::string n_CF;
  std::string n_climatology;
  std::string n_compress;
  std::string n_Conventions;
  std::string n_coordinates;
  std::string n_dimension;
  std::string n_featureType;
  std::string n_FillValue;
  std::string n_flag_masks;
  std::string n_flag_values;
  std::string n_formula_terms;
  std::string n_global;
  std::string n_grid_latitude;
  std::string n_grid_longitude;
  std::string n_grid_mapping;
  std::string n_instance_dimension;
  std::string n_long_name;
  std::string n_latitude;
  std::string n_leap_month;
  std::string n_leap_year;
  std::string n_longitude;
  std::string n_missing_value;
  std::string n_month_lengths;
  std::string n_NC_GLOBAL;
  std::string n_number_of_observations;
  std::string n_positive;
  std::string n_reco;
  std::string n_standard_name;
  std::string n_sample_dimension;
  std::string n_time;
  std::string n_units;
  std::string n_valid_max;
  std::string n_valid_min;
  std::string n_valid_range;
  std::string n_variable;

  static const bool lowerCase = true;
  std::string bKey;
  std::string fail;
  std::string NO_MT;

  std::vector<std::string>        associatedGroups;
  static std::vector<std::string> attName;
  static std::vector<char>        attType;  // S: string, N: numeric, D: data type
  std::vector<std::string>        dimensions;
  std::vector<size_t>             effDims_ix;

  // properties of coordinates attributes
  std::vector<std::pair<int, int> > ca_pij;
  std::vector<std::vector<std::string> > ca_vvs ;
};

#endif
