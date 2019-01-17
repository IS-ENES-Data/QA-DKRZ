#ifndef _CF_17_H
#define _CF_17_H

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
  ~CF(){if( ifs_std_name.isOpen()) ifs_std_name.close();}

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
  // Check of standard_name vs. formula_terms (case: dimless vertical coord).
  // The chapter ordering has changed; this is valid sind v1.7
  void   chap432(Variable&, std::string &units);
  void   chap433(void);       // vertical dimensionless coord
  bool   chap433(Variable&,
                 std::vector<std::string>&,
                 std::vector<std::string>&,
                 std::vector<std::string>&,
                 std::vector<std::string>&,
                 int& valid_ix, int ft_jx, int sn_jx,
                 std::vector<std::pair<std::string, std::string> > &) ;
  bool   chap433_checkSNvsFT( Variable& var,
            std::vector<std::string> &valid_sn,
            std::vector<std::string> &valid_ft,
            std::vector<std::string> &valid_units,
            int& valid_ix, int& ft_ix, int& sn_ix, std::string &units );
  void   chap433_computedStdName( Variable&,
            std::string &valid_sn, std::string &valid_ft, std::string &valid_cmp_sn,
            int valid_ix, int att_ft_ix,
            std::vector<std::pair<std::string, std::string> > &att_ft_pv) ;
  void   chap433_consistencyTableD1( Variable &var,
            std::string &valid_sn, std::string &valid_ft, std::string &valid_cmp_sn,
            int valid_ix, int att_ft_ix,
            std::vector<std::pair<std::string, std::string> > &vp_param);
  void   chap433_getParamVars( Variable&,
            std::vector<std::string> &valid_sn,
            std::vector<std::string> &valid_ft,
            int& valid_ix, int att_ft_ix,
            std::vector<std::pair<std::string, std::string> > &att_ft_pv) ;
  void   chap433_verify_FT(Variable&,
            std::string &valid_ft, std::string &valid_sn, std::string &valid_units,
            int valid_ix, int att_ft_ix, std::vector<std::string> &fTerms,
            std::vector<std::pair<std::string, std::string> > &p_found_ft);
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
  void   chap56_simple(Variable&, size_t,  // grid mapping att naming a grid-mapping variable
            std::vector<std::pair<std::string, std::string> >& );
  void   chap56_expanded(Variable&, size_t);  // expanded form of the grid mapping att
  int    chap56_gridMappingVar(Variable& dv, std::string &, std::string);
  void   chap56_gridMappingCoords(Variable& dataVar, std::string mCV[]);
  void   chap56_gridMappingParams(Variable &var, std::string &gm_name)   ;

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
  bool   chap734a(std::string&) ;
  void   chap73b_reco(Variable&, std::vector<std::string> &dim );
  bool   chap733(std::string& method, Variable&, std::string mode) ;
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

  void   chapA_useCase(void) ;

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
  bool   check_standard_name(Variable&, std::string&, std::string mode="") ;
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
  bool   scanStdNameTable(Variable&, std::string);
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

  ReadLine ifs_std_name ;
  bool is_ifs_std_name={true};
  //std::vector<std::string> vs_std_name; // contains also computed_standard_names
  //std::vector<size_t> vs_std_name_ix ; // index for vector Variable
  std::vector<size_t> vs_SNT_ix ; // index for variable.SN_TableEntry objects; not necessarily varSz
  size_t SNT_ix ;                 // 0: std_name, 1: computed_std_name

  // a few names of attributes used throughout the checks
  const std::string n_actual_range = {"actual_range"};
  const std::string n_add_offset = {"add_offset"};
  const std::string n_ancillary_variables = {"ancillary_variables"};
  const std::string n_area = {"area"};
  const std::string n_attribute = {"attribute"};
  const std::string n_axis = {"axis"};
  const std::string n_bounds = {"bounds"};
  const std::string n_calendar = {"calendar"};
  const std::string n_cell_measures = {"cell_measures"};
  const std::string n_cell_methods = {"cell_methods"};
  const std::string n_cf_role = {"cf_role"};
  const std::string n_CF = {"CF"};
  const std::string n_climatology = {"climatology"};
  const std::string n_comment = {"comment"};
  const std::string n_compress = {"compress"};
  const std::string n_computed_std_name = {"computed_standard_name"};
  const std::string n_Conventions = {"Conventions"};
  const std::string n_coordinates = {"coordinates"};
  const std::string n_dimension = {"dimension"};
  const std::string n_external_variables = {"external_variables"};
  const std::string n_featureType = {"featureType"};
  const std::string n_FillValue = {"_FillValue"};
  const std::string n_flag_masks = {"flag_masks"};
  const std::string n_flag_meanings = {"flag_meanings"};
  const std::string n_flag_values = {"flag_values"};
  const std::string n_formula_terms = {"formula_terms"};
  const std::string n_global = {"global"};
  const std::string n_grid_latitude = {"grid_latitude"};
  const std::string n_grid_longitude = {"grid_longitude"};
  const std::string n_grid_mapping = {"grid_mapping"};
  const std::string n_grid_mapping_name = {"grid_mapping_name"};
  const std::string n_history = {"history"};
  const std::string n_instance_dimension = {"instance_dimension"};
  const std::string n_institution = {"institution"};
  const std::string n_long_name = {"long_name"};
  const std::string n_latitude = {"latitude"};
  const std::string n_leap_month = {"leap_month"};
  const std::string n_leap_year = {"leap_year"};
  const std::string n_longitude = {"longitude"};
  const std::string n_missing_value = {"missing_value"};
  const std::string n_month_lengths = {"month_lengths"};
  const std::string n_NC_GLOBAL = {"NC_GLOBAL"};
  const std::string n_number_of_observations = {"number_of_observations"};
  const std::string n_positive = {"positive"};
  const std::string n_reco = {"CF recommendation"};
  const std::string n_references = {"references"};
  const std::string n_sample_dimension = {"sample_dimension"};
  const std::string n_scale_factor = {"scale_factor"};
  const std::string n_source = {"source"};
  const std::string n_standard_error_multiplier = {"standard_error_multiplier"};
  const std::string n_standard_name = {"standard_name"};
  const std::string n_time = {"time"};
  const std::string n_title = {"title"};
  const std::string n_units = {"units"};
  const std::string n_valid_max = {"valid_max"};
  const std::string n_valid_min = {"valid_min"};
  const std::string n_valid_range = {"valid_range"};
  const std::string n_variable = {"variable"};

  const bool lowerCase = {true};
  std::string bKey;
  std::string fail;
  std::string NO_MT;

  std::vector<std::string>        associatedGroups;
  std::vector<std::string>        dimensions;
  std::vector<size_t>             effDims_ix;

  // properties of coordinates attributes
  std::vector<std::pair<int, int> > ca_pij;
  std::vector<std::vector<std::string> > ca_vvs ;

  // grid mapping parameters
  std::string n_earth_radius = { "earth_radius" };
  std::string n_false_easting = { "false_easting" };
  std::string n_false_northing = { "false_northing" };
  std::string n_fixed_angle_axis = { "fixed_angle_axis" };
  std::string n_grid_north_pole_latitude = { "grid_north_pole_latitude" };
  std::string n_grid_north_pole_longitude = { "grid_north_pole_longitude" };
  std::string n_inverse_flattening = { "inverse_flattening" };
  std::string n_latitude_of_projection_origin = { "latitude_of_projection_origin" };
  std::string n_longitude_of_central_meridian = { "longitude_of_central_meridian" };
  std::string n_longitude_of_prime_meridian = { "longitude_of_prime_meridian" };
  std::string n_longitude_of_projection_origin = { "longitude_of_projection_origin" };
  std::string n_north_pole_grid_longitude = { "north_pole_grid_longitude" };
  std::string n_north_pole_longitude = { "north_pole_grid_longitude" };
  std::string n_perspective_point_height = { "perspective_point_height" };
  std::string n_scale_factor_at_central_meridian = { "scale_factor_at_central_meridian" };
  std::string n_scale_factor_at_projection_origin = { "scale_factor_at_projection_origin" };
  std::string n_semi_major_axis = { "semi_major_axis" };
  std::string n_semi_minor_axis = { "semi_minor_axis" };
  std::string n_sweep_angle_axis = { "sweep_angle_axis" };
  std::string n_standard_parallel = { "standard_parallel" };
  std::string n_straight_vertical_longitude_from_pole = { "straight_vertical_longitude_from_pole" };
  std::string n_azimuth_of_central_line = { "azimuth_of_central_line" };
  std::string n_geographic_crs_name = { "geographic_crs_name" };
  std::string n_crs_wkt = { "crs_wkt" };
  std::string n_geoid_name = { "geoid_name" };
  std::string n_geopotential_datum_name = { "geopotential_datum_name" };
  std::string n_horizontal_datum_name = { "horizontal_datum_name" };
  std::string n_prime_meridian_name = { "prime_meridian_name" };
  std::string n_projected_crs_name = { "projected_crs_name" };
  std::string n_reference_ellipsoid_name = { "reference_ellipsoid_name" };
  std::string n_towgs84 = { "towgs84" };

  // Attributes and value type
  const std::string n_N={"N"} ;
  const std::string n_S={"S"} ;
  const std::string n_C={"C"} ;
  const std::string n_D={"D"} ;
  const std::string n_G={"G"} ;

  const std::vector<std::string> CF_Attribute = {
  n_actual_range, n_N, n_C, n_D,
  n_add_offset, n_N, n_D,
  n_ancillary_variables, n_S, n_D,
  n_axis, n_S, n_C,
  n_azimuth_of_central_line, n_N,
  n_bounds, n_S, n_C,
  n_calendar, n_S, n_C,
  n_cell_measures, n_S, n_D,
  n_cell_methods, n_S, n_D,
  n_cf_role, n_C, n_C,
  n_climatology, n_S, n_C,
  n_comment, n_S, n_G ,n_D,
  n_compress, n_S, n_C,
  n_computed_std_name, n_S, n_C,
  n_Conventions, n_S, n_G,
  n_coordinates, n_S, n_D,
  n_crs_wkt, n_S,
  n_earth_radius, n_N,
  n_external_variables, n_S, n_G,
  n_false_easting, n_N,
  n_false_northing, n_N,
  n_featureType, n_C, n_G,
  n_FillValue, n_D, n_D,
  n_flag_masks, n_D, n_D,
  n_flag_meanings, n_S, n_D,
  n_flag_values, n_D, n_D,
  n_formula_terms, n_S, n_C,
  n_geographic_crs_name, n_S,
  n_geoid_name, n_S,
  n_geopotential_datum_name, n_S,
  n_grid_mapping_name, n_N,
  n_grid_mapping, n_S, n_D,
  n_grid_north_pole_latitude, n_N,
  n_grid_north_pole_longitude, n_N,
  n_horizontal_datum_name, n_S,
  n_history, n_S, n_G,
  n_instance_dimension, n_N, n_D,
  n_institution, n_S, n_G, n_D,
  n_inverse_flattening, n_N,
  n_latitude_of_projection_origin, n_N,
  n_leap_month, n_N, n_C,
  n_leap_year, n_N, n_C,
  n_long_name, n_S, n_C, n_D,
  n_longitude_of_central_meridian, n_N,
  n_longitude_of_prime_meridian, n_N,
  n_longitude_of_projection_origin, n_N,
  n_missing_value, n_D, n_D,
  n_month_lengths, n_N, n_C,
  n_north_pole_grid_longitude, n_N,
  n_perspective_point_height, n_N,
  n_positive, n_S, n_C,
  n_prime_meridian_name, n_S,
  n_projected_crs_name, n_S,
  n_references, n_S, n_G, n_D,
  n_reference_ellipsoid_name, n_S,
  n_sample_dimension, n_N, n_D,
  n_scale_factor, n_N, n_D,
  n_scale_factor_at_central_meridian, n_N,
  n_scale_factor_at_projection_origin, n_N,
  n_semi_major_axis, n_N,
  n_semi_minor_axis, n_N,
  n_source, n_S, n_G, n_D,
  n_standard_error_multiplier, n_N, n_D,
  n_standard_name, n_S, n_C, n_D,
  n_standard_parallel, n_N,
  n_straight_vertical_longitude_from_pole, n_N,
  n_title, n_S, n_G,
  n_towgs84, n_N,
  n_units, n_S, n_C, n_D,
  n_valid_max, n_N, n_C, n_D,
  n_valid_min, n_N, n_C, n_D,
  n_valid_range, n_N, n_C, n_D,
  };

  // grid mapping names
  const std::string n_albers_conical_equal_area = {"albers_conical_equal_area"};
  const std::string n_azimuthal_equidistant = {"azimuthal_equidistant"};
  const std::string n_lambert_azimuthal_equal_area = {"lambert_azimuthal_equal_area"};
  const std::string n_lambert_conformal_conic = {"lambert_conformal_conic"};
  const std::string n_lambert_cylindrical_equal_area = {"lambert_cylindrical_equal_area"};
  const std::string n_latitude_longitude = {"latitude_longitude"};
  const std::string n_mercator = {"mercator"};
  const std::string n_orthographic = {"orthographic"};
  const std::string n_polar_stereographic = {"polar_stereographic"};
  const std::string n_rotated_latitude_longitude = {"rotated_latitude_longitude"};
  const std::string n_stereographic = {"stereographic"};
  const std::string n_transverse_mercator = {"transverse_mercator"};
  const std::string n_vertical_perspective = {"vertical_perspective"};
  const std::string n_geostationary = {"geostationary"};
  const std::string n_oblique_mercator = {"oblique_mercator"};
  const std::string n_sinusodial = {"sinusodial"};

  // parametric vertical coordinates
  // valid comuted_standard names
  const std::string n_Table_D1 = {"Table_D1"};
  const std::string n_air_pressure = {"air_pressure"};
  const std::string n_altitude = {"altitude"};
  const std::string n_hagd = {"height_above_geopotential_datum"};

};

#endif
