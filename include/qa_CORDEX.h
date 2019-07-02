#ifndef _QA_CORDEX_H
#define _QA_CORDEX_H

#include "hdhC.h"
#include "date.h"
#include "annotation.h"
#include "qa.h"
#include "qa_data.h"
#include "qa_time.h"
#include "qa_cnsty.h"

//! Quality Control Program Unit for CORDEX.
/*! All the QA considerations specific to CORDEX are covered by this class.\n
Requested properties from the cordex_archive_specifications.pdf
from http://cordex.dmi.dk/joomla/images/CORDEX are checked as well
as specifications in CORDEX_variables_requirement_table.pdf.\n
Results of the QA are written to a netCDF file
(to the directory where the main program was started) with filename
qa_<data-filename>.nc. Outlier test and such for replicated records
are performed. Annotations are supplied via the Annotation class
linked by a pointer.
*/

struct DRS_CV
{
  DRS_CV(QA*);

  void   applyOptions(std::vector<std::string>&);
  void   checkModelName(std::string &aN, std::string &aV,
            char model, std::string iN="", std::string iV="");

  //! Match filename components and global attributes of the file.
  void   checkFilename(std::string& fName, struct DRS_CV_Table&);
  void   checkFilenameEncoding(Split&, struct DRS_CV_Table& );

  //! Test optional global attribute
  void   checkDrivingExperiment(void);

  //! Is it NetCDF-4, is it compressed?
  void   checkNetCDF(NcAPI* p=0);
  void   checkPath(std::string&, struct DRS_CV_Table&);
  void   checkProductName(std::string& drs_product,
                   std::string prod_choice,
                   std::map<std::string, std::string>& gM);
  void   findFN_faults(Split&, Split&,
                   std::map<std::string, std::string>&,
                   std::map<std::string, std::string>&,
                   std::string& text);
  void   findPath_faults(Split&, Split&,
                   std::map<std::string, std::string>&,
                   std::map<std::string, std::string>&,
                   std::string& text);

  int    getPathBegIndex( Split& drs, Split& x_e,
            std::map<std::string, std::string>& gM );
  std::string
         getInstantAtt(void);

  void run(void);

  hdhC::FileSplit GCM_ModelnameTable;
  hdhC::FileSplit RCM_ModelnameTable;

  bool enabledCompletenessCheck;

  std::string drsF;
  std::string drsP;

  Annotation* notes;
  QA*         pQA;
};

//! Struct containing dimensional properties to cross-check with table information.
struct DimensionMetaData
{
  std::map<std::string, std::string> attMap;

  bool         isUnitsDefined;
  uint32_t     checksum;  //fletcher32
  size_t       size;
};

//! Properties of the variable to cross-check with table information.
class VariableMetaData
{
  public:
  VariableMetaData(QA*, Variable *var=0);
  ~VariableMetaData();

  std::map<std::string, std::string> attMap;

  bool        isUnitsDefined;

  std::string name;

  //! store results temporarily
  DataOutputBuffer dataOutputBuffer;

  Annotation*  notes;
  Variable*    var;
  QA*          pQA;
  QA_Data      qaData;

  int  finally(int errCode=0);
  void forkAnnotation(Annotation *p);
  void setAnnotation(Annotation *p);
  void setParent(QA *p){pQA=p;}

  //! Verify units % or 1 by data range
  void verifyPercent(void);
};

class QA_Exp
{
  public:

  //! Default constructor.
  QA_Exp();

  void   applyOptions(std::vector<std::string>&);

  bool   checkDateFormat(std::string requested, std::string available) ;

  void   checkDimStandardTable(ReadLine &tbl, InFile &in,
            VariableMetaData &var,
            std::vector<struct DimensionMetaData>&,
            std::map<std::string, size_t> &col,
            std::string dName, size_t colMax);

  //! Check dimensions
  void   checkDimTableEntry(InFile &in,
            VariableMetaData &var,
            struct DimensionMetaData &nc,
            struct DimensionMetaData &tbl) ;

  void   checkDimAxis(InFile &in,
           VariableMetaData &vMD,
           struct DimensionMetaData &nc_entry,
           struct DimensionMetaData &tbl_entry);

  void   checkDimChecksum(InFile &in,
           VariableMetaData &vMD,
           struct DimensionMetaData &nc_entry,
           struct DimensionMetaData &tbl_entry);

  void   checkDimLongName(InFile &in,
           VariableMetaData &vMD,
           struct DimensionMetaData &nc_entry,
           struct DimensionMetaData &tbl_entry);

  void   checkDimOutName(InFile &in,
           VariableMetaData &vMD,
           struct DimensionMetaData &nc_entry,
           struct DimensionMetaData &tbl_entry);

  void   checkDimSize(InFile &in,
           VariableMetaData &vMD,
           struct DimensionMetaData &nc_entry,
           struct DimensionMetaData &tbl_entry);

  void   checkDimStndName(InFile &in,
           VariableMetaData &vMD,
           struct DimensionMetaData &nc_entry,
           struct DimensionMetaData &tbl_entry);

  //! Check the (transient) time dimension.
  void   checkDimULD(
            VariableMetaData &var,
            struct DimensionMetaData &nc,
            struct DimensionMetaData &tbl);

  //! Check the units of a dimension's variable representation.
  void   checkDimUnits(InFile &in,
            VariableMetaData &var,
            struct DimensionMetaData &nc,
            struct DimensionMetaData &tbl);

  //! Check near-surface height value between 0 - 10m
  void   checkHeightValue(InFile &);

//! Checks meta-data
  void   checkMetaData(InFile &) ;

  //! Consistency between filename, plev variable and data value
  /*! For variables defined for a specific pressure level, eg.ta850*/
  void   checkPressureCoord(InFile&);

  void   checkTrackingID(std::string& rV, std::string& aV);

  void   checkVarTableEntry(
             VariableMetaData &,
             VariableMetaData &tbl_entry);

  //! Consistency test between file and table: cell_methods.
  void   checkVarTableEntry_cell_methods(
             VariableMetaData &,
             VariableMetaData &tbl_entry);

  //! Consistency test between file and table: long name.
  void   checkVarTableEntry_longName(
             VariableMetaData &,
             VariableMetaData &tbl_entry);

  //! Consistency test between file and table: (output) name.
  void   checkVarTableEntry_name(
             VariableMetaData &,
             VariableMetaData &tbl_entry);

  //! Consistency test between file and table: standard name.
  void   checkVarTableEntry_standardName(
             VariableMetaData &,
             VariableMetaData &tbl_entry);

  //! Consistency test between file and table: units.
  /*! If one of table or CF standard units matches, then the check will pass.*/
  void   checkVarTableEntry_units(
             VariableMetaData &,
             VariableMetaData &tbl_entry);

  //! Make VarMetaData objects.
  void   createVarMetaData(void);

  void   checkVariableType(void);
  void   checkVariableTypeX(size_t, size_t, size_t, std::string&);

  //! Check time properties.
  void   domainCheck(void);
  void   domainCheckData(std::string &var_lon, std::string &var_lat,
            std::vector<std::string>&, std::string tbl_id);
  void   domainCheckDims(std::string item, std::string &t_Nlon,
            std::string &f_Nlon, std::string tble_id);
  void   domainCheckPole(std::string item,
            std::string &t_Nlon, std::string &f_Nlon);
  void   domainFindTableType( std::vector<std::vector<std::string> > &tbl1,
            std::vector<std::vector<std::string> > &tbl2,
            int &ix1, int &ix2 );
  bool   domainFindTableTypeByRange(
            std::vector<std::vector<std::string> > &T1,
            std::vector<std::vector<std::string> > &T2,
           int &table_id, size_t &row );

  //! Find entry of a requested variable in the standard table.
  bool   findTableEntry(ReadLine &, std::string &,
            VariableMetaData &tble_entry);

  bool   findTableEntry(ReadLine &, std::string &name,
            size_t col_outName, std::string &str0);

  //! Find the name of requested sub-table in string str0.
//  bool   find_CORDEX_SubTable(ReadLine &, std::string &str0,
//            VariableMetaData &var);

  std::string
         getAttValue(size_t v_ix, size_t a_ix);

  //! Store properties of a dimension in the struct.
  /*! Note: the name of the dimension is passed by the struct.*/
  bool   getDimMetaData(InFile &in,
             VariableMetaData &,
             struct DimensionMetaData &, std::string &);

  std::string
         getFrequency(void);

  std::string
         getVarnameFromFilename(std::string str="");

//  std::string
//         getStandardTable(void){ return varReqTable.file ; }

  //! Get the Standard table name from the global attributes
  void   getSubTable(void);

  std::string
         getTableEntryID(std::string vN="");

  void   init(std::vector<std::string>&);

  //! Initialisation of flushing gathered results to netCDF file.
  /*! Parameter indicates the number of variables. */
  void   initDataOutputBuffer(void);
  void   initDefaults(void);

  //! Initialisiation of a resumed session.
  /*! For non-atomic data sets and such being incomplete, yet. */
  void   initResumeSession(std::vector<std::string>& prevTargets);

  //! Check existence of tables
  bool   getVarReqTables(void);

  //! Read the headlines for dimensions and variable.
  /*! Read from the standard table; used to identify columns.*/
  bool   readTableCaptions(ReadLine &, std::string freq,
            std::map<std::string, size_t> &col, std::string &str0);

  void   reqAttCheck(void);
  void   reqAttCheckCloudValues(std::string &aux, std::string &vals) ;
  void   reqAttCheckGlobal(Variable&);
  void   reqAttCheckVariable(Variable&);

  void   run(void);

  void   setParent(QA*);

  //! Cross-check with standard table.
  /*! Prepare the check for dimensions and variable.*/
  void   varReqTableCheck(InFile &in,
            VariableMetaData &var,
            std::vector<struct DimensionMetaData>& );

  //! Check dimensional bounds: layout and size
  /*! Number of values and checksum of the bounds*/
/*
  void   varReqTableCheckDimBounds(InFile &in, Split &splt_line,
            VariableMetaData &var,
            struct DimensionMetaData &dimFE,
            struct DimensionMetaData &dimTE,
            std::map<std::string, size_t> &col) ;
*/
  //! Check dimensional values: layout and size
  /*! Number of values and checksum*/
/*
  void   varReqTableCheckDimValues(InFile &in, Split &splt_line,
            VariableMetaData &var,
            struct DimensionMetaData &file,
            struct DimensionMetaData &table,
            std::map<std::string, size_t> &col) ;
*/

  hdhC::FileSplit varReqTable;

  std::vector<VariableMetaData> varMeDa;

  NcAPI *nc;
  QA* pQA;

  MtrxArr<double> tmp_mv;

  // the same buf-size for all buffer is required for testing replicated records
  size_t bufferSize;

  // init for test about times
  bool enabledCompletenessCheck;
  bool isUseStrict;
  bool isCheckParentExpID;
  bool isCheckParentExpRIP;
  bool isClearBits;
  bool isRotated;

  int varnamePosition;

  std::vector<std::string> excludedAttribute;
  std::vector<std::string> overruleAllFlagsOption;

  std::vector<std::string> constValueOption;
  std::vector<std::string> fillValueOption;
  std::vector<std::string> reqAttOption;

  std::string cfStndNames;
  std::string GCMModelName;
  std::string RCMModelName;
  std::string institute_id;

  std::vector<std::string> possibleFrequencies;
  std::string frequency;
  std::string parentExpID;
  std::string parentExpRIP;
  std::string experiment_id;
  std::string subTable ;

  std::string fVarname;

  std::string n_axis;
  std::string n_bnds_name;
  std::string n_cmor_name;
  std::string n_coordinates;
  std::string n_index_axis;
  std::string n_long_name;
  std::string n_outname;
  std::string n_requested;
  std::string n_standard_name;
  std::string n_type;
  std::string n_units;
  std::string n_value;

  std::string n_cell_measures;
  std::string n_cell_methods;
  std::string n_cell_methods_opt;
  std::string n_positive;
  std::string n_tracking_id;

  // a dummy
  std::vector<std::string> MIP_tableNames;
  std::vector<int>         MIP_FNameTimeSz;

  std::string getCaptIntroDim(VariableMetaData &vMD,
                   struct DimensionMetaData &nc_entry,
                   struct DimensionMetaData &tbl_entry, std::string att="");
  std::string getSubjectsIntroDim(VariableMetaData &vMD,
                   struct DimensionMetaData &nc_entry,
                   struct DimensionMetaData &tbl_entry, bool isColon=true);
  void        pushBackVarMeDa(Variable*);
  void        setCheckMode(std::string);
};

#endif
