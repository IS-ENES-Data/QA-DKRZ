#ifndef _QA_CMIP6_H
#define _QA_CMIP6_H

#include "hdhC.h"
#include "date.h"
#include "annotation.h"
#include "json/json-forwards.h"
#include "json/json.h"
#include "qa.h"
#include "qa_data.h"
#include "qa_time.h"
#include "qa_cnsty.h"

#include "json/json-forwards.h"
#include "json/json.h"

//! Quality Control Program Unit for CMIP6.
/*! All the QA considerations are covered by this class.\n
Properties specified in standard_output.xlsx document
from http://cmip-pcmdi.llnl.gov/cmip5/docs
are checked as well as the Data Reference Syntax given in cmip5_data_reference_syntax.pdf.\n
The netCDF data-file is linked by a pointer to the InFile class
instance. Results of the QA are written to a netCDF file
(to the directory where the main program was started) with filename qa_<data-filename>.nc. Outlier test and such for replicated records
are performed. Annotations are supplied via the Annotation class
linked by a pointer.
*/

class QA_Exp;

class CMIP6_CV
{
  public:
  CMIP6_CV(QA*, Variable*);

  void  add_requ_glob_att(std::string&);

  bool  check_array(std::string& id);
  void  check_string(std::string& id);
  void  check_experiment_id(std::string id);
  void  check_source_id(std::string id);

  void  check_existence(void);

  void check_experiment_id_value(
      std::string& id, std::vector<std::string>&, std::string& f_aVal) ;

  void check_source_id_value(
      std::string& id, std::vector<std::string>&, std::string& f_aVal) ;

  void check_exp_activity_id(
      std::string& id, std::vector<std::string>&, std::string& f_aVal) ;
/*
  void check_exp_additional_allowed_model_components(
      std::string& id, std::vector<std::string>&, std::string& f_aVal) ;
  void check_exp_description(
      std::string& id, std::vector<std::string>&, std::string& f_aVal) ;
  void check_exp_end_year(
      std::string& id, std::vector<std::string>&, std::string& f_aVal) ;
  void check_exp_experiment(
      std::string& id, std::vector<std::string>&, std::string& f_aVal) ;
  void check_exp_min_number_yrs_per_sim(
      std::string& id, std::vector<std::string>&, std::string& f_aVal) ;
  void check_exp_parent_activity_id(
      std::string& id, std::vector<std::string>&, std::string& f_aVal) ;
  void check_exp_parent_experiment_id(
      std::string& id, std::vector<std::string>&, std::string& f_aVal) ;
  void check_exp_required_model_components(
      std::string& id, std::vector<std::string>&, std::string& f_aVal) ;
  void check_exp_start_year(
      std::string& id, std::vector<std::string>&, std::string& f_aVal) ;
  void check_exp_sub_experiment(
      std::string& id, std::vector<std::string>&, std::string& f_aVal) ;
  void check_exp_sub_experiment_id(
      std::string& id, std::vector<std::string>&, std::string& f_aVal) ;

  void check_sid_aerosol(
      std::string& id, std::vector<std::string>&, std::string& f_aVal) ;
  void check_sid_atmosphere(
      std::string& id, std::vector<std::string>&, std::string& f_aVal) ;
  void check_sid_atmospheric_chemistry(
      std::string& id, std::vector<std::string>&, std::string& f_aVal) ;
  void check_sid_cohort(
      std::string& id, std::vector<std::string>&, std::string& f_aVal) ;
  void check_sid_institution_id(
      std::string& id, std::vector<std::string>&, std::string& f_aVal) ;
  void check_sid_label(
      std::string& id, std::vector<std::string>&, std::string& f_aVal) ;
  void check_sid_label_extended(
      std::string& id, std::vector<std::string>&, std::string& f_aVal) ;
  void check_sid_land_ice(
      std::string& id, std::vector<std::string>&, std::string& f_aVal) ;
  void check_sid_land_surface(
      std::string& id, std::vector<std::string>&, std::string& f_aVal) ;
  void check_sid_ocean(
      std::string& id, std::vector<std::string>&, std::string& f_aVal) ;
  void check_sid_ocean_biogeochemistry(
      std::string& id, std::vector<std::string>&, std::string& f_aVal) ;
  void check_sid_release_year(
      std::string& id, std::vector<std::string>&, std::string& f_aVal) ;
  void check_sid_sea_ice(
      std::string& id, std::vector<std::string>&, std::string& f_aVal) ;
  void check_sid_source_id(
      std::string& id, std::vector<std::string>&, std::string& f_aVal) ;
*/

  void  disable_requ_glob_att(std::string&);

  // type: 'a' array, 'o' obj, 's' string, 'E' experiment_id, 'S' source_id
  char  get_CV_tableType(std::string& id);

  Json::Value
        getJsonValue(std::string&);
  void  readJsonFiles(void);

  void  run(void);

//  void  check(Variable& t_glob, std::string& id, std::string& sVal,
//                  std::vector<std::string>& aVal);

  // names of global atts
  std::vector<std::string> cv_table;
  std::string cv_path;
  std::string n_CV;

  std::vector<std::string> vs_requiredGlobalAtts;

  Json::Value jObj_activity_id;
  // Json::Value jObj_experiment_id;  // instantiated temporarilly
  Json::Value jObj_frequency;
  Json::Value jObj_grid_label;
  Json::Value jObj_institution_id;
  Json::Value jObj_nominal_resolution;
  Json::Value jObj_realm;
  // Json::Value jObj_source_id;      // instantiated temporarilly
  Json::Value jObj_required_global_attributes ;
  Json::Value jObj_source_type;
  Json::Value jObj_table_id;
  Json::Value jObj_mip_era;

  Annotation* notes;
  Variable*   pGlob;
  QA*         pQA;
};

struct CMOR
{
  CMOR(QA*);

  void   applyOptions(std::vector<std::string>&);

  //! Comparison of dimensions between file and the var-requirements table
  /*! Cross-checks with the standard table are performed only once for
   each variable at first time encounter in the CMIP Project ensuring
   conformance.*/

  void   checkEnsembleMemItem(std::string& rqName, std::string& attVal);
  void   checkForcing(std::vector<std::string>&, std::string&);

  //! Check dimensions
  void   checkMIPT_dim( std::vector<std::string>&,
            VariableMetaData&,
            std::map<std::string, size_t>& col, std::string& CMORdimName);

  void   checkMIPT_dimEntry(
            VariableMetaData& var,
            struct DimensionMetaData& f_DMD,
            struct DimensionMetaData& t_DMD) ;

  //! Compare dimensional properties from the table to the file
  void   checkMIPT_dim_axis(
           VariableMetaData& vMD,
           struct DimensionMetaData& f_DMD,
           struct DimensionMetaData& t_DMD);

  void   checkMIPT_dim_boundsQuest(
            VariableMetaData& var,
            struct DimensionMetaData& f_DMD,
            struct DimensionMetaData& t_DMD) ;

  void   checkMIPT_dim_boundsRequested(
            VariableMetaData& var,
            struct DimensionMetaData& f_DMD,
            struct DimensionMetaData& t_DMD) ;

  void   checkMIPT_dim_boundsValues(
            VariableMetaData& var,
            struct DimensionMetaData& f_DMD,
            struct DimensionMetaData& t_DMD) ;

  void   checkMIPT_dim_coordsAtt(
            VariableMetaData& var,
            struct DimensionMetaData& f_DMD,
            struct DimensionMetaData& t_DMD) ;

  void   checkMIPT_dim_indexAxis(
            VariableMetaData& var,
            struct DimensionMetaData& f_DMD,
            struct DimensionMetaData& t_DMD) ;

  void   checkMIPT_dim_longName(
            VariableMetaData& var,
            struct DimensionMetaData& f_DMD,
            struct DimensionMetaData& t_DMD) ;

  bool   checkMIPT_dim_outname(
           VariableMetaData& vMD,
           struct DimensionMetaData& f_DMD,
           struct DimensionMetaData& t_DMD);

  void   checkMIPT_dim_positive(
           VariableMetaData& vMD,
           struct DimensionMetaData& f_DMD,
           struct DimensionMetaData& t_DMD);

  void   checkMIPT_dim_requested(
            VariableMetaData& var,
            struct DimensionMetaData& f_DMD,
            struct DimensionMetaData& t_DMD) ;

  void   checkMIPT_dim_stdName(
           VariableMetaData& vMD,
           struct DimensionMetaData& f_DMD,
           struct DimensionMetaData& t_DMD);

  void   checkMIPT_dim_type(
           VariableMetaData& vMD,
           struct DimensionMetaData& f_DMD,
           struct DimensionMetaData& t_DMD);

  //! Check the units of a dimension's variable representation.
  void   checkMIPT_dim_units(
            VariableMetaData& var,
            struct DimensionMetaData& f_DMD,
            struct DimensionMetaData& t_DMD);

  void   checkMIPT_dim_validMax(
            VariableMetaData& var,
            struct DimensionMetaData& f_DMD,
            struct DimensionMetaData& t_DMD) ;

  void   checkMIPT_dim_validMin(
            VariableMetaData& var,
            struct DimensionMetaData& f_DMD,
            struct DimensionMetaData& t_DMD) ;

  void   checkMIPT_dim_value(
            VariableMetaData& var,
            struct DimensionMetaData& f_DMD,
            struct DimensionMetaData& t_DMD) ;

  void   checkMIPT_var(
            VariableMetaData& vMD, VariableMetaData& tEntry,
            std::map<std::string, size_t>&);

  void   checkMIPT_var_cellMeasures(
            VariableMetaData& vMD,
            VariableMetaData& tEntry);

  void   checkMIPT_var_cellMethods(
            VariableMetaData& vMD,
            VariableMetaData& tEntry);

  void   checkMIPT_var_flagMeanings(
            VariableMetaData& vMD,
            VariableMetaData& tEntry);

  void   checkMIPT_var_flagValues(
            VariableMetaData& vMD,
            VariableMetaData& tEntry);

  void   checkMIPT_var_frequency(
            VariableMetaData& vMD,
            VariableMetaData& tEntry);

  void   checkMIPT_var_longName(
            VariableMetaData& vMD,
            VariableMetaData& tEntry);

  bool   checkMIPT_var_positive(
            VariableMetaData& vMD,
            VariableMetaData& tEntry);

  void   checkMIPT_var_realm(
            VariableMetaData& vMD,
            VariableMetaData& tEntry);

  void   checkMIPT_var_stdName(
            VariableMetaData& vMD,
            VariableMetaData& tEntry);

  void   checkMIPT_var_type(
            VariableMetaData& vMD,
            VariableMetaData& tEntry);

  void   checkMIPT_var_Units(
            VariableMetaData& vMD,
            VariableMetaData& tEntry);

  //! Check variables' properties
  /*! Prepare the check for dimensions and variable.*/
  bool   checkMIP_table(InFile& in,
            VariableMetaData& var,
            std::vector<struct DimensionMetaData>&);

  void   checkRequestedAttributes(void);
  void   checkReqAtt_global(void);
  void   checkReqAtt_variable(Variable&);

  void   checkSource(void);

  // the next one is applied to several checks
  void   checkStringValues(
            struct DimensionMetaData& f_DMD,
            struct DimensionMetaData& t_DMD,
            std::string&, size_t maxSz=0 );

  void   checkTrackingID(std::string& rV, std::string& aV);

  //! Apply the cross-check for the variable.
  void   checkVarReqTableEntry(
             VariableMetaData& ,
             VariableMetaData& tbl_entry);

  // the next one is applied to several checks
  void   checkWithTolerance(
            struct DimensionMetaData& f_DMD,
            struct DimensionMetaData& t_DMD,
            std::string&, size_t maxSz=0 );

  //! Find occurrence of a heading line in string str0.
  /*! Returns true, if not found or the maximum number of columns needed.
      col maps required column titles to their index.*/
  //! Find entry of a requested variable in the standard table.
  bool   findVarReqTableEntry(ReadLine&, std::string&, VariableMetaData& var,
            std::map<std::string, size_t>& col, VariableMetaData& tEntry);

  //! Store properties of a dimension in the struct.
  /*! Note: the name of the dimension is passed by the struct.
      dim-name is returned; accounts for a switch to a label-name*/
  bool   getDimMetaData(InFile& in, VariableMetaData& ,
             struct DimensionMetaData& , std::string dName, size_t);

  // read dim columns for a given CMOR_dim; if column == empty, then all columns
  void   getDimSheetEntry(std::string& CMOR_dim, std::vector<std::string>&,
                          std::map<std::string, size_t>& col,
                          DimensionMetaData&, std::string column="");

  void   getMIPT_var(std::string& entry,
            VariableMetaData& tEntry,
            std::map<std::string, size_t>&);

  void   getVarReqTableSubSheetName(std::string& str0, VariableMetaData& vMD );

  void   initDefaults(void);

  //! Read the headlines for dimensions and variable.
  /*! Read from the standard table; used to identify columns.*/
  bool   readHeadline(ReadLine&,
            VariableMetaData&, std::string& line,
            std::map<std::string, size_t>& v_col);

  //! Starting function for all table cross-checks.
  void   run(InFile&, std::vector<VariableMetaData>&);

  // a little bit intricated, because of Omon-3D tracers, cf3hr, and cfSites
  void                     bufTableSheets(VariableMetaData&);
  std::vector<std::string> tableIDBuf;
  size_t                   tableIDBufIx;

  std::string parentExpID;
  std::string parentExpRIP;

  static std::string n_activity;
  static std::string n_axis;
  static std::string n_bounds_quest;
  static std::string n_bounds_requested;
  static std::string n_bounds_values;
  static std::string n_cell_measures;
  static std::string n_cell_methods;
  static std::string n_CMOR_dimension;
  static std::string n_CMOR_tables;
  static std::string n_CMOR_varName;
  static std::string n_coordinates;
  static std::string n_flag_meanings;
  static std::string n_flag_values;
  static std::string n_forcing;
  static std::string n_frequency;
  static std::string n_global;
  static std::string n_index_axis;
  static std::string n_mip_era;
  static std::string n_long_name;
  static std::string n_outname;
  static std::string n_output;
  static std::string n_output_dim_name;
  static std::string n_positive;
  static std::string n_priority;
  static std::string n_product;
  static std::string n_realm;
  static std::string n_requested;
  static std::string n_standard_name;
  static std::string n_table_id;
  static std::string n_tol_on_requests;
  static std::string n_tracking_id;
  static std::string n_type;
  static std::string n_units;
  static std::string n_valid_max;
  static std::string n_valid_min;
  static std::string n_value;
  static std::string n_varName;

  Annotation* notes;
  QA*         pQA;
  QA_Exp*     pExp;
};

std::string CMOR::n_activity            ="activity";
std::string CMOR::n_axis                ="axis";
std::string CMOR::n_bounds_quest        ="bnds_quest";
std::string CMOR::n_bounds_requested    ="bounds_requested";
std::string CMOR::n_bounds_values       ="bounds_values";
std::string CMOR::n_cell_measures       ="cell_measures";
std::string CMOR::n_cell_methods        ="cell_methods";
std::string CMOR::n_CMOR_dimension      ="CMOR_dimension";
std::string CMOR::n_CMOR_tables         ="CMOR_tables";
std::string CMOR::n_CMOR_varName        ="CMOR_varName";
std::string CMOR::n_coordinates         ="coordinates";
std::string CMOR::n_flag_meanings       ="flag_meanings";
std::string CMOR::n_flag_values         ="flag_values";
std::string CMOR::n_forcing             ="forcing";
std::string CMOR::n_frequency           ="frequency";
std::string CMOR::n_global              ="global";
std::string CMOR::n_index_axis          ="index_axis";
std::string CMOR::n_mip_era             ="mip_era";
std::string CMOR::n_long_name           ="long_name";
std::string CMOR::n_output              ="output";
std::string CMOR::n_output_dim_name     ="output_dimension_name";
std::string CMOR::n_positive            ="positive";
std::string CMOR::n_priority            ="priority";
std::string CMOR::n_product             ="product";
std::string CMOR::n_realm               ="realm";
std::string CMOR::n_requested           ="requested";
std::string CMOR::n_standard_name       ="standard_name";
std::string CMOR::n_table_id            ="table_id";
std::string CMOR::n_tol_on_requests     ="tol_on_requests";
std::string CMOR::n_tracking_id         ="tracking_id";
std::string CMOR::n_type                ="type";
std::string CMOR::n_units               ="units";
std::string CMOR::n_valid_max           ="valid_max";
std::string CMOR::n_valid_min           ="valid_min";
std::string CMOR::n_value               ="value";
std::string CMOR::n_varName             ="Variable Name";

struct DRS_CV
{
  DRS_CV(QA*);

  void   applyOptions(std::vector<std::string>&);
  void   checkFilename(std::string& fName, struct DRS_CV_Table&);
  void   checkFilenameEncoding(Split&, struct DRS_CV_Table&);
  void   checkFilenameGeographic(Split&);
  void   checkMIPT_tableName(Split&);
  void   checkNetCDF(NcAPI* p=0);

  void   checkPath(std::string&, struct DRS_CV_Table&);
  void   findFN_faults(Split&, Split&,
                   std::map<std::string, std::string>&,
                   std::map<std::string, std::string>&,
                   std::string& text);
  void   findPath_faults(Split&, Split&,
                   std::map<std::string, std::string>&,
                   std::map<std::string, std::string>&,
                   std::string& text);
  void   checkProductName(std::string& drs_product,
                   std::string prod_choice,
                   std::map<std::string, std::string>& gM);
  void   checkVariableName(std::string& f_vName);

  std::string
         getEnsembleMember(void);
  int    getPathBegIndex( Split& drs, Split& x_e,
            std::map<std::string, std::string>& gM );
  std::string
         getInstantAtt(void);

  void   run(void);

  bool enabledCompletenessCheck;

  std::string ensembleMember;
  std::string member_id;
  std::string drsF;
  std::string drsP;

  Annotation* notes;
  QA*         pQA;
};

//! Struct containing dimensional properties to cross-check with table information.
struct DimensionMetaData
{
  DimensionMetaData(Variable* p=0){var=p;}

  std::map<std::string, std::string> attMap;
  Variable*    var;
};

//! Properties of the variable to cross-check with table information.
class VariableMetaData
{
  public:
  VariableMetaData(QA*, Variable *v=0);
  ~VariableMetaData();

  int  finally(int errCode=0);
  void forkAnnotation(Annotation *p);
  void setAnnotation(Annotation *p);
  void setParent(QA* p){pQA=p;}

  //! Verify units % or 1 by data range
  void verifyPercent(void);

  // buffer results
  DataOutputBuffer dataOutputBuffer;

  std::map<std::string, std::string> attMap;
  bool isNewVar;

  Annotation* notes;
  Variable*   var;
  QA*         pQA ;
  QA_Data     qaData;

};

class QA_Exp
{
public:
  //! Default constructor.
  QA_Exp();

  void   applyOptions(std::vector<std::string>&);

  //! Only a single data variable is permitted
  void   checkDataVarNum(void);

  //! Checks meta-data
  void   checkMetaData(InFile& ) ;

  void   checkVariableType(void);

//! Variable objects for netcdf meta-data
  /*! Only the objects are created.*/
  void   createVarMetaData(void);

  //! Get global attribute 'frequency'
  std::string
         getFrequency(void);

  //! get and check MIP table name; s would indicate a name to be tried
  std::string
         getTableID(std::string s="");

  std::string
         getTableEntryID(std::string vN="");

  std::string
         getVarnameFromFilename(std::string str="");

  //! Check the path to the tables;
  bool   inqTables(std::string tName);

  void   init(std::vector<std::string>&);

  //! Initialisation of flushing gathered results to netCDF file.
  /*! Parameter indicates the number of variables. */
  void   initDataOutputBuffer(void);

  //! Set default values.
  void   initDefaults(void);

  //! Initialisiation of a resumed session.
  /*! Happens for non-atomic data sets and those that are yet incomplete. */
  void   initResumeSession(std::vector<std::string>& prevTargets);

  //! Connect this with the object to be checked
//  void   setInFilePointer(InFile *p) { pIn = p; }

  void   run(void);

  void setParent(QA*);

  //! Set properties of variable from netcdf file
  void   setVarMetaData(VariableMetaData& );

  //! Name of the netCDF file with results of the quality control
  hdhC::FileSplit varReqTable;

  std::vector<VariableMetaData> varMeDa;

  NcAPI* nc;
  QA*    pQA;

  MtrxArr<double> tmp_mv;

  // the same buf-size for all buffer is required for testing replicated records
  size_t bufferSize;

  // init for test about times
  bool isUseStrict;
  bool isClearBits;
  bool notMIP_table_avail;

  int mipPosition;
  int varnamePosition;

  std::vector<std::string> excludedAttribute;
  std::vector<std::string> overruleAllFlagsOption;

  std::vector<std::string> constValueOption;
  std::vector<std::string> fillValueOption;

  std::string cfStndNames;
  std::string currMIP_tableName;
  std::string frequency;
  std::vector<std::string> MIP_tableNames;
  std::vector<int>         MIP_FNameTimeSz;

  std::string experiment_id;
  std::string fVarname;

  static std::string getCaptionIntroDim(
                   struct DimensionMetaData& f_entry,
                   struct DimensionMetaData& t_entry, std::string att="");
  static std::string getCaptionIntroVar(
                   std::string table,
                   struct VariableMetaData& f_entry, std::string att="");
  bool        not_equal(double x1, double x2, double epsilon);
  void        pushBackVarMeDa(Variable*);
};

#endif

