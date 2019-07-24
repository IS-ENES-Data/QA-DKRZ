#ifndef _NC_API_H
#define _NC_API_H

#include <iostream>
#include <string>
#include <vector>
#include <map>

// Most compilers don't know to build libs with templates.
// Thus, sources are included to the header files.
#include "hdhC.h"
#include "matrix_array.h"  // with sources
#include "netcdf.h"  // local instance
#include "brace_op.h"
#include "annotation.h"
#include "readline.h"
#include "split.h"

//using namespace ::netCDF;

/*! \file nc_api.h
 \brief C++ API to meta-data and data of netCDF file; also netCDF C functions.
*/

//! C++ API to the nedCDF C functions.

/*! NcAPI provides a C++ API to netCDF.\n
 Note: netCDF C routines not covered by this class may always
       be used directly by the ncid ID stored within this object.
       An annotation object is connected.

       Annotations are issued with a flag NC_group_currNum :
       1  file handling
       2  inquiries
       3  attributes
       4  NC-4 specific
       5  variables
       6  misc
       7  data (get)
       8  data (out)
*/

class NcAPI
{
public:
   struct ExceptionStruct
   {
      std::ofstream *ofsError;
      std::string strError;
   } ;

//! Default constructor.
    NcAPI();

//! Copy constructor.
    NcAPI( const NcAPI& );

//! Construct and open/create a netCFD file in read-mode by default.
/*! modes: READ, NC_WRITE (both reading and writing),
           NC_NOCLOBBER, NC_SHARE, NC_64BIT_OFFSET,
           NC_NETCDF4, NC_CLASSIC_MODEL
    Multiple modes may be concatenated by '|'.*/
    NcAPI(std::string ncFilename, std::string mode="Read");

//! Destructor.
    ~NcAPI(void);

//! Struct: Properties of an opened netCDF file.
/*! Dimensions, variables with associated or global attributes
 from the define section of an opened netCDF in read-mode.
 In 'Write' mode, layout is invoked successively.
 Analogously for dimensions, variables and global attributes.
 Allocation will take place inside a netCDF method, where
 the caller is responsible for deletion.*/
    struct Layout
    {
      int         format;

// DIMENSIONS
//! Names of the dimensions.
/*! Note: dim IDs are identical to indexes.*/
      std::vector<std::string> dimName;

//! Lengths of the dimensions.
      std::vector<size_t> dimSize;

//! Mapping between dimension name and the index in 'dim'.
      std::map<std::string, int> dimMap;

// VARIABLES
//! Names of variables
      std::vector<std::string> varName;

//! Mapping between varname and the index in 'var'.
      std::map<std::string, int> varMap;
      std::map<int, std::string> varidMap;

//! Mapping between varname and properties (0 --> false).
      std::map<std::string, int>  storage;
      std::map<std::string, int>  deflate;
      std::map<std::string, int>  deflate_level;
      std::map<std::string, int>  endian;
      std::map<std::string, int>  fletcher32;
      std::map<std::string, int>  shuffle;

//! Types of variables
      std::vector<nc_type> varType;

//! Mapping between var type and the index of varName.
      std::map<std::string, nc_type> varTypeMap;

//! Has variable an unlimited dimension?
      std::vector<bool> varIsRecordType;

//! Is the data section empty for a given variable?
      std::vector<bool> noData;

//! Names of variable's dimensions
      std::vector<std::vector<std::string> > varDimName;
      std::vector<std::vector<size_t> > varDimSize;

// Attributes (of vars and then, globally)
//! Vector of vector to the attribute names of a variable.
/*! First vector corresponding to the vector of variables.*/
      std::vector<std::vector<std::string> > varAttName;

//! Vector of mapping of name and index of attributes.
/*! The vector corresponds to the vector of varnames.*/
      std::vector<std::map<std::string, int> > varAttMap;

//! Vector of vector to the attribute type of a variable.
/*! First vector corresponds to the vector of variables.*/
      std::vector<std::vector<nc_type> > varAttType;

//! Vector of vector to the number of values of each attribute.
/*! First vector corresponding to the vector of variables.*/
      std::vector<std::vector<size_t> > varAttValSize;

//! Vector of vector to the attribute names of a variable.
/*! First vector corresponding to the vector of variables.*/
      std::vector<std::string> globalAttName;

//! Mapping between global attribute name and index.
/*! The vector correspopnds to the vector of varnames.*/
      std::map<std::string, int> globalAttMap;

//! Vector of vector to the attribute type of a variable.
/*! First vector corresponds to the vector of variables.*/
      std::vector<nc_type> globalAttType;

//! Vector of vector to the number of values of each attribute.
/*! First vector corresponding to the vector of variables.*/
      std::vector<size_t> globalAttValSize;

//! The number of items in a record.
      std::vector<size_t> recSize;

//! The corner and edge lengths.
      std::vector<size_t*> rec_start;
      std::vector<size_t*> rec_count;
      std::vector<size_t>  rec_index;

      std::vector<size_t>  rec_leg_begin;
      std::vector<size_t>  rec_leg_sz;
      std::vector<size_t>  rec_leg_end;

//! NC4: Compression
      std::vector<int> varShuffle;
      std::vector<int> varDeflate;
      std::vector<int> varDeflateLevel;

//! NC4: Chunking (storage==0 yes, ==1 no for NC_CONTIGOUS)
      std::vector<size_t > varStorage;
      std::vector<size_t*> varChunkSize;

//! NC4: Endian (storage==0 yes, ==1 no for NC_CONTIGOUS)
      std::vector<int> varEndian;

//! NC4: Fletcher32
      std::vector<int> varFletcher32;
    };

//! Declaration of a layout object.
    Layout layout;

//! clear layout and close nc-file
    void
      clear(void);

//! Close current ncFile.
/*! Closing an un-opened file is safe.*/
    void
      close(void);

//! Copy attributes from referenced source.
/*! Merging of attributes from different sources is safe.*/
    void
      copyAtts(NcAPI &from, std::string vN_from, std::string vN_to,
          std::vector<std::string> *pvs=0 )
          {copyAtts(from, from.getVarID(vN_from), getVarID(vN_to), pvs);}

    void
      copyAtts(NcAPI &from, std::string vN_from,
          std::vector<std::string> *pvs=0)
          {copyAtts(from, from.getVarID(vN_from), getVarID(vN_from),pvs);}

//! Copy the 'rec'-th record of all (unlimited) variables from source.
/*! The variable type is taken from source.*/
    void
      copyData(NcAPI &from, size_t rec_in, size_t rec_out=UINT_MAX );

//! Copy the 'rec'-th record of variable 'vName' from source.
/*! The variable type is taken from source. The name is changed.*/
    void
      copyData(NcAPI &from, std::string srcVName,
          std::string vName, size_t rec_in, size_t rec_out=UINT_MAX );

//! Copy the 'rec'-th record of variable 'vName' from source.
/*! Keep the name of the variable*/
    void
      copyData(NcAPI &from, std::string vName, size_t rec_in, size_t rec_out=UINT_MAX )
         { copyData(from, vName, vName, rec_in, rec_out);}

//! Copy all records of variable 'vName' from source.
/*! The variable type is taken from source.*/
    void
      copyData(NcAPI &from, std::string srcVName, std::string vName="" );

//! Copy dimension names and sizes from a source.
/*! It is safe to call for identical dimensions.
    If 's' is empty, then all dimensions from '*src' are copied.*/
    void
      copyDim(NcAPI &src, std::string s="");

//! Copy global attributes from referenced source.
/*! Merging of global attributes from different sources is safe.*/
    void
      copyGlobalAtts(NcAPI &from);

//! Copy the entire layout (dims, vars, atts, and non-rec values).
/*! Switches between define and data mode.*/
    void
      copyLayout(NcAPI &);

//! Copy records of all unlimited variables.
/*! The layout of the variables must be defined.*/
    void
      copyRecord(NcAPI &u, size_t rec_in, size_t rec_out=UINT_MAX);

//! Copy variable from source and copy attributes and data.
/*! Dimension are also defined where necessary
    The variable name is that of the source by default,
    but can be changed. The format is taken from the source, if
    both source and this are NETCDF4. If source is classic and this
    is NETCDF, then map appropriately.
    This member is used recursively, if the variable depends on
    dimensions whose name is also a variable name of its own.*/
    bool
      copyVar(NcAPI &, std::string srcVName, std::string vName="", bool isKeepOrder=false);

//! Copy variable from source and copy attributes without data.
    bool
      copyVarDef(NcAPI &, std::string srcVName, std::string vName="", bool keepOrder=false);

//! Create netCDF file.
/*! Modes are described in the constructor description above.
    Here: mode default is empty for NC3; else NC_NETCDF4 . \n
    If creation fails or is inhibited by NC_NOCLOBBER for an
    existing file, then isExitOnError==false returns with false.
    Else: exit*/
    bool
      create(std::string f, std::string mode="",
           bool isExitOnError=true);

//! Define dimension. Return dim-ID.
/*! Define dimension by name and value. If dimsize is omitted,
    the dimension becomes unlimited.*/
    int
      defineDim( std::string dName, size_t dimsize=0 );

//! Copy the entire layout (dimensions, variables and attributes).
    void
      defineLayout(NcAPI &);

//! Define new variable.
/*! Requires an NcType and a vector of valid dimension names.
    Program exits for undefined dimension names.
    Return pointer to the new variable object. The return value is
    the varID. If the variable was not defined, because it exists
    already, returned value == -1. So multiple calls are safe.*/
    int
      defineVar(std::string vName, nc_type);
    int
      defineVar(std::string vName, nc_type, std::vector<std::string>& dims);

//! Delete attribute
/*! Also if a selected attribute wasn't defined, yet.
    Return the index of the attribute or UINT_MAX, if
    there was nothing to delete.*/
    size_t
       delAtt(std::string, int varid=NC_GLOBAL);

//! Leave define mode
    void
       disableDefMode(void);

//! Enter define mode
    void
       enableDefMode(void);

    void
       exceptionHandling(std::string key,
            std::string capt, std::string text="",
            std::string vName="") ;

//! Error messages are written to a file. Exits immediately.
    void
       exceptionError(std::string);

//! Note: not tested for a long time. Create netCDF file from scratch.
/*! The confFile is formatted almost the same way ncdump formats
    output. Except: if an attribute has values which are not of type
    'char' or a string, then the type (e.g. float) has to precede
    the name of the attribute (separated by a blanc).
    Also, an additional 'write variables:' section can be
    appended that gives instructions to calculate and write limited
    variables on a regular grid (currently only 1-dim). Example: \n
    write variables: \n
    lon: range= -180 to 180,  cells= 720 \n
    lat: range= -90 to 90,  cells= 360 \n
    This section must be introduced by "write variables:". Then
    each line gives range and number of grid-cells (key-word
    'cells'). If a key-word 'bounds' or 'boundaries' precedes or
    follows key-word 'cells', then the boundaries of the cells are
    calculated and 'cells' is increased by one automaticly. \n
    Also a short format is accepted that keeps the sequence: \n
    range_0, range_1, number and optional 'bounds' anywhere. \n
    Note: the 'dimensions'-section must come first. The sequence
    of the others may be arbitrary, even nested (then the variable
    must be defined before putting attributes). \n
    The filename to be generated is specified in the first line.\n
    ToDo: multi-dim variables in the 'write variables' section.
*/
void
      generate(std::string confFile);

    // Return -1 for empty string, and -2 if
    // the string-parameter is not a valid variable.
    // Varid<0 for global attributes
    int
      getAttID(std::string aName, std::string vName="")
         { return getAttID(aName, getVarID(vName) );}

    int
      getAttID(std::string&, int varid);

//! Get names of the attributes of given variable.
/*! Returns empty vector, if no attributes were defined
    or the given variable name is not a netCDF variable.
    If vName is empty, then for global attributes.*/
    std::vector<std::string>
      getAttName(std::string vName);

//! Get number of the attributes of given variable.
/*! If vName is empty, then for global attributes.*/
    size_t
      getAttSize(std::string vName);

//! Get first or only string from attribute with string-value(s).
/*! The boolean is set also true for an empty string.*/
    std::string
      getAttString(std::string attName, std::string varName="");
    std::string
      getAttString(std::string attName, std::string varName,
         bool &isSet);

//! Get the type of an attribute.
    nc_type
      getAttType(std::string attName, std::string varName="");

//! Get string representation of the type of an attribute.
/*! Return value contains the netCDF internal 'nc_type' converted
    to a string representation, i.e. NC_BYTE, NC_CHAR, NC_SHORT,
    NC_INT, NC_FLOAT, and NC_DOUBLE. Empty string for 'no type'.*/
    std::string
      getAttTypeStr(std::string attName, std::string varName="")
        { return getTypeStr(getAttType(attName, varName)); }

//! Get first or only value converted to double.
/*! Returns MAXDOUBLE, if no value was found.*/
    double
      getAttValue(std::string att, std::string var="" );

//! Get value(s) of variable's attribute 'att'.
/*! Returned numerical values are converted to given type of vector
    as well as text-character values and ch**  to string(s).
    If string 'var' is empty, global attributes are looked for.*/
  template <typename Type>
    void
      getAttValues(std::vector<Type> &t, std::string att, std::string var="" );

    void
      getAttValues(std::vector<std::string> &v, std::string att, std::string var="" );

//! Chunking properties from source for netcdf4
    void
      getChunking(NcAPI &u, std::string vName,
         size_t &storage, std::vector<size_t> &chunks)
         {getChunking(u, u.getVarID(vName), storage, chunks);}

//! Get the i-th record of variable vName.
/*! If vName is an invalid variable name, then 0 is returned.
    Get all the data for limited (non-record) variables.
    This member just returns a void* pointer to the data.*/
    void*
      getData(std::string vName, size_t rec=0, int leg=1 )
         { return getData(getVarID(vName), rec, leg); }

//! Access to data by a MtrxArr object.
/*! The user has to take care that type conversion is possible.
    Return value: index of x corresponding to rec; -1 for invalid rec */
  template <typename ToT>
    ToT
      getData(MtrxArr<ToT> &x, std::string vName, int rec=0, int leg=1 );

//! Access of a record by a MtrxArr object.
/*! Note that data is sliced according to an internally specified maximum
    range.*/
    template <typename ToT>
     ToT
      getData(MtrxArr<ToT> &x, int varid, int rec=0, int leg=1);

    void
      getData(std::vector<std::string> &,std::string, int rec=0);

    std::pair<int, int>
      getDataIndexRange(std::string vName)
        {return getDataIndexRange(getVarID(vName));}

    std::pair<int, int>
      getDataIndexRange(int varid);

//! Get number of values per record of variable 'vName'.
/*! If vName is not a valid variable, 0 is returned.
    Do not confuse with number of records.*/
    size_t
      getDataSize(std::string vName);

//! Compress properties from source for netcdf4
    void
      getDeflate(NcAPI &u, std::string vName,
         int &shuffle, int &deflate, int &level)
         { getDeflate( u, u.getVarID(vName), shuffle, deflate, level); }

//! Get id of dimension.
    int
      getDimID(std::string);

/*! If the variable parameter is empty, all dimension names are returned.
    Else, only those the variable is depending on.*/
    std::vector<std::string>
      getDimName(std::string vName="");

//! Get the size of given dimension.
/*! If dName is invalid, a value of -1 is returned.*/
    int
      getDimSize(int dimID);

    int
      getDimSize(std::string dName)
         { return getDimSize(getDimID(dName)); }

//! Endian properties from source for netcdf4
    void
      getEndian(NcAPI &u, std::string vName, int &endian)
         {getEndian(u, u.getVarID(vName), endian);}

//! Get fill values in vector(_FV, MV, default)
/*! If no attribute defined fill-value is available, then _FV and/or MV
    return the default. Whether _FV and/or MV are set is inidicated by the
    return array. */
    std::vector<bool>
      get_FillValueStr(std::string& vName, std::vector<std::string>& vs )
         { return get_FillValueStr(getVarID(vName), vs); }

    std::vector<bool>
      get_FillValueStr(int varid, std::vector<std::string>& ) ;

  template <typename T>
    bool
      get_FillValue(std::string& vName, std::vector<T>&, std::vector<char>* mode=0 );

//! Fletcher32 properties from source for netcdf4
    void
      getFletcher32(NcAPI &u, std::string vName, int &f)
         {getFletcher32(u, u.getVarID(vName), f);}

//! Get the name of the netCDF file.
    std::string
      getFilename(void){return ncFilename;}

//! Assign layout information from read-opened ncFile.
    void
      getLayout(void);

//! Get those variable names not depending on the unlimited dimension.
    std::vector<std::string>
      getLimitedVarName(void);

//! Get the ncid number of the opened nc.
    int
      getNcid(void){return ncid;}

//! Get the number of current records. Force==true re-checks the number
    size_t
      getNumOfRecords(bool force=false);

    size_t
      getNumOfRows(std::string vName);

//! Get the number of records, if var is unlimited
    size_t
      getNumOfRecords(std::string var, bool force=false);

    size_t
      getRecLegIndex(std::string vName, size_t rec)
      { return getRecLegIndex(getVarID(vName), rec); }

    size_t
      getRecLegIndex(int varid, size_t rec)
        { return rec - layout.rec_leg_begin[varid] ; }

      //! Get number of values per record of variable 'vName'.
/*! If vName is not a valid variable, 0 is returned.
    Do not confuse with number of records.*/
    size_t
      getRecordSize(std::string vName) ;

    std::string
      getTypeStr(nc_type);

      //! Get names of all variables depending on the unlimited dimension.
    std::vector<std::string>
      getUnlimitedVars(void);

//! Get the name of the unlimited dimension; empty if none.
    std::string
      getUnlimitedDimName(void)
        {return unlimitedDimName;}

//! Get name of the variable representation of the unlimited dimension.
/*! If there is none, then try to find a limited time variable as substitute.*/
    std::string
      getUnlimitedDimRepName(void);

//    std::string
//      getUnlimitedDimVarName(void){ return getUnlimitedDimRepName;}

//! Get sizes of the dimensions variable 'vName' depends on.
/*! Return sizes of all dimensions in the sequence getDimName
    would return names. If variable name is empty or invalid,
    return an empty vector.*/
    std::vector<size_t>
      getVarDimSize(std::string vName);

//! Get the netCDF variable id for given string;
/*! Return -1 for empty string, and -2 if parameter is not a valid variable.*/
    int
      getVarID(std::string);

//! Get the netCDF variablename from netcdfd varid.
/*! Return empty string fro invalid varid.*/
    std::string
      getVarnameFromVarID(int) ;

//! Get all names of variables, limited and unlimited).
    std::vector<std::string>
      getVarName(void) { return layout.varName;}

//! Get number of values of variable 'vName'.
/*! In case of unlimited variables, the size of a record.*/
    size_t
      getVarSize(std::string &vName)
         { return layout.recSize[ getVarID(vName) ]; }

//! Get the type of of a variable.
    nc_type
      getVarType(std::string vName);

//! Get the type of of a variable.
    std::string
      getVarTypeStr(std::string vName);

    bool
      inqDeflate(std::string var="");

    int
      inqNetcdfFormat(void){return ncFormat;}

//! Is there any current record
    bool   isAnyRecord(void);

//! Is there any current record for given variable
    bool   isAnyRecord(std::string);

//! Is the name obf the attribute valid?
    bool
      isAttValid(std::string aName, std::string vName="")
         { return (getAttID(aName, vName) == -2) ? false : true ; }

//! Return true, if dimension 'dName' is unlimited.
    bool
      isDimUnlimited(std::string &dName);

//! Return true, if any dimension is unlimited.
    bool
      isDimUnlimited(void);

    bool
      isDimUnlimitedGenuine(void){ return isGenuineUnlimitedDim ;}

//! Return true if string is a valid dimension
    bool
      isDimValid(std::string);

//! Is the most recent data body empty
    bool isEmptyData(std::string vName)
         { return isEmptyData(getVarID(vName)) ;}

    bool isEmptyData(int vid);

    bool is_global(void){ return is_NC_GLOBAL;}

//! Return true if the type is index compatible
    bool
      isIndexType(std::string vName);

//! Return true for an empty data section of the given variable name
    bool
      isRecordType(std::string varName) ;

    bool
      isRecordType(int id){ return layout.varIsRecordType[id] ;}

//! Return true if the type is index compatible
    bool
      isNumericType(std::string vName, std::string aName="");

//! Return true, if the netCDF file is open.
    bool
      isOpen(void){ return isThisOpen;}

//! Return true, if variable 'vName' depends on unlimited dimension.
    bool
      isVarUnlimited(std::string vName);
    bool
      isVarUnlimited(int varid);

//! Return true, if parameter vName is a valid variable name.
    bool
      isVariableValid(std::string);
    bool
      isVariableValid(std::string, std::string &descript);

//! Open netCDF file.
/*! Modes are described in the constructor description above.
    Here: mode default: READ \n
    If creation fails or is inhibited by NC_NOCLOBBER for an
    existing file, then isExitOnError==false returns with false.
    Else: exit .*/
    bool
      open(std::string f="", std::string="Read",
           bool isExitOnError=true );

//! Set values of 'arr' to 'rec'ord number of variable 'vName'.
/*! The number of elements in 'arr' must be identical to the size
    of a record for 'vName'. The type of 'arr' must be identical to
    the one defined for the variable 'vName'; no cast!*/
  template <typename Type>
    void
      putData(size_t rec, std::string vName, Type *arr )
         { putData(rec, getVarID(vName), arr ); }

  template <typename Type>
    void
      putData(size_t rec_beg, size_t rec_count, std::string vName, Type *arr )
         { putData(rec_beg, rec_count, getVarID(vName), arr ); }

//! Special: for a single value
  template <typename Type>
    void
      putData(size_t rec, std::string vName, Type arr )
         { putData(rec, getVarID(vName), &arr ); }

//! Special: for a single string
    void
      putData(size_t rec, std::string vName, std::string arr )
         { putData(rec, getVarID(vName), arr.c_str() ); }

//! Version for data stored in a MtrxArr object.
  template <typename Type>
    void
      putData(size_t rec, std::string vName, MtrxArr<Type> &mv)
         { putData(rec, getVarID(vName), mv.begin() ) ; }

//! Set values of 'arr' to 'rec'ord number of variable 'vName'.
/*! This overloaded version performs a static_cast from the data type
    'FromT' to the type of the variable 'x' that must be given
    (value of x does not matter).
    The number of elements in 'arr' must be identical to the size
    of a record for 'vName'. Modification of data by a function.
    If 'recOut' is given, then records are shifted (Note: then,
    'func' must be given explicity as 0, if there is none).
    Note: also for non-record variables (internally checked).*/
  template <typename ToT, typename FromT>
    void
      putData(size_t rec, std::string vName, FromT *arr,
            ToT x, ToT (*func)(double,int,size_t)=0,
              size_t outRec=UINT_MAX )
         { putData(rec, getVarID(vName), arr, x, func, outRec); }

//! Version for data of a record stored in a Matval object.
  template <typename ToT, typename FromT>
    void
      putData(size_t rec, std::string vName, MtrxArr<FromT> &mv,
         ToT x, ToT (*func)(double,int,size_t)=0,
              size_t outRec=UINT_MAX )
         { putData(rec, getVarID(vName), mv.begin(), x, func, outRec); }

//! Set all values of a limited variable (non-record).
  template <typename ToT, typename FromT>
    void
      putData(std::string vName, FromT *arr,
         ToT x, ToT (*func)(double,int,size_t)=0,
              size_t outRec=UINT_MAX )
         { putData(getVarID(vName), arr, x, func, outRec); }

//! For data stored in a MtrxArr object.
  template <typename ToT, typename FromT>
    void
      putData(MtrxArr<FromT> &mv, std::string vName,
         ToT x, ToT (*func)(double,int,size_t)=0,
              size_t outRec=UINT_MAX )
         { putData(getVarID(vName), mv.begin(), x, func, outRec); }

//! For a single value
  template <typename Type>
    void
      putDat1(size_t rec_beg, std::string vName, Type *arr )
         { putDat1(rec_beg, getVarID(vName), arr ); }

 //! Set or change attribute to (of) a variable.
/*! If parameter vName is omitted, then global
    attributes are added. Variables may be provided as
    vectors, arrays with additional size, or plain.
    Some methods are with type conversion.*/

//  vector ------------------
    template <typename Type>
    void
      setAtt(std::string vName, std::string aName, std::vector<Type> &values)
         {setAtt(getVarID(vName), aName, values) ;}

    template <typename To, typename From>
    void
      setAtt(std::string vName, std::string aName,
             std::vector<From> &values, To x)
         {setAtt(getVarID(vName), aName, values, x) ;}

    // special
    void
      setAtt(std::string vName, std::string aName,
         std::vector<std::string> &text)
         {setAttString(getVarID(vName), aName, text);}

    void
      setGlobalAtt(std::string aName, std::vector<std::string> &text)
         {setAttString(NC_GLOBAL, aName, text);}

//  array ------------------
    template <typename Type>
    void
      setAtt(std::string vName, std::string aName,
             Type *arr, size_t sz)
         {setAtt(getVarID(vName), aName, arr, sz) ;}

    template <typename To, typename From>
    void
      setAtt(std::string vName, std::string aName,
             From *arr, size_t sz, To x)
         {setAtt(getVarID(vName), aName, arr, sz, x) ;}

//  plain value, but also a const char*. So , take special care.
    template <typename Type>
    void
      setAtt(std::string vName, std::string aName, Type *val)
         {setAttString(getVarID(vName), aName, val ) ;}

    template <typename Type>
    void
      setAtt(std::string vName, std::string aName, Type val)
         {setAtt(getVarID(vName), aName, val ) ;}

    template <typename To, typename From>
    void
      setAtt(std::string vName, std::string aName, From value, To x)
         {setAtt(getVarID(vName), aName, value, x) ;}

    // special for a string value
    void
      setAtt(std::string vName, std::string aName, std::string str)
         {setAttString(getVarID(vName), aName, str);}

//  Specialisation for generateParseAtt(): convert strings to type
    template <typename T>
    void
      setAtt(std::string vName, std::string aName,
         std::vector<std::string> &vstr, T x);

    // again for global attributes
//  vector ------------------
    template <typename Type>
    void
      setGlobalAtt(std::string aName, std::vector<Type> &values)
         {setAtt(NC_GLOBAL, aName, values) ;}

    template <typename To, typename From>
    void
      setGlobalAtt(std::string aName, std::vector<From> &values, To x)
         {setAtt(NC_GLOBAL, aName, values, x) ;}

//  array ------------------
    template <typename Type>
    void
      setGlobalAtt(std::string aName, Type *arr, size_t sz)
         {setAtt(NC_GLOBAL, aName, arr, sz) ;}

    template <typename To, typename From>
    void
      setGlobalAtt(std::string aName, From *arr, size_t sz, To x)
         {setAtt(NC_GLOBAL, aName, sz, x) ;}

//  plain value, but also a const char*. So , take special care.
    template <typename Type>
    void
      setGlobalAtt(std::string aName, Type *val)
         {setAttString(NC_GLOBAL, aName, val ) ;}

    template <typename Type>
    void
      setGlobalAtt(std::string aName, Type val)
         {setAtt(NC_GLOBAL, aName, val ) ;}

    template <typename To, typename From>
    void
      setGlobalAtt(std::string aName, From value, To x)
         {setAtt(NC_GLOBAL, aName, value, x) ;}

// special for a string value
    void
      setGlobalAtt(std::string aName, std::string str)
         {setAttString(NC_GLOBAL, aName, str);}

//! Compress for netcdf4
    void
      setDeflate(std::string vName, int shuffle, int deflate, int deflate_level)
         { setDeflate(getVarID(vName), shuffle, deflate, deflate_level); }

    void
      setDeflate(int varid, int shuffle, int deflate, int deflate_level);

//! Endian for netcdf4
    void
      setEndian(int varid, int endian);

    void
      setFileAccessMode(std::string fam);

//! Fletcher32 for netcdf4
    void
      setFletcher32(int varid, int fletcher32);

//! Set netCDF filename.
    void
      setFilename(std::string f, std::string mode="ReadOnly");

   // entry function for book-keeping and setting fillValues
    void
      setFillValue(std::string aName, std::string &vName,
            nc_type, int id);

    //! get access to the global exception and annotation handling
    void
      setNotes(Annotation *n) {notes = n; }

//! Set the path to the netCDF filename (without filename).
    void
      setPath(std::string &p){ncPath=p;}

//    void
//      setPseudoUnlimitedDim(std::string vName)
//          {pseudoUnlimitedDim.push_back(vName);}

//! Set the name of the unlimited dimension.
/*! If there is a limited dim that acts as an unlimited one.*/
    std::string
      setUnlimitedDimName(std::string s)
        {return unlimitedDimName=s;}

    void
      temporarily_disable_annotation(void){with_annotation=false;}

private:
    int ncid;
    int status ;
    int ncFormat;
    bool isDefineMode;
    bool is_NC_GLOBAL ;
    bool with_annotation;

    bool isChunking;
    bool isDeflate;
    bool isEndianess;
    bool isFletcher32;

    int         unlimitedDimID;
    std::string unlimitedDimName;
    std::string unlimitedDimRepName;
    bool isGenuineUnlimitedDim ;

    struct ExternalSetting
    {
      bool isChunking;
      bool isDeflate;
      bool isDefineMode;
      bool isEndianess;
      bool isFletcher;

      std::string variable;

      int shuffle;
      int deflate;
      int deflate_level;

    };
    ExternalSetting xtrnlSet;


    std::string ncPath;
    std::string ncFilename;
    std::string ncFileDirection;  // "in" || "out"
    std::string fileAccessMode;

    std::vector<bool>        hasEffVarUnlimitedDim;
//    std::vector<std::string> pseudoUnlimitedDim;
    std::vector<bool>        isDataEmpty;
    std::vector<bool>        isDataEmptyChecked;

    int    numOfRecords;
    size_t max_read_sz;

    // Spaces for records of various types
    std::vector<MtrxArr<signed char> >        rec_val_schar;
    std::vector<MtrxArr<unsigned char> >      rec_val_uchar ;
    std::vector<MtrxArr<char> >               rec_val_text ;
    std::vector<MtrxArr<short> >              rec_val_short ;
    std::vector<MtrxArr<unsigned short> >     rec_val_ushort ;
    std::vector<MtrxArr<int> >                rec_val_int ;
    std::vector<MtrxArr<unsigned int> >       rec_val_uint ;
    std::vector<MtrxArr<unsigned long long> > rec_val_ulonglong ;
    std::vector<MtrxArr<long long> >          rec_val_longlong ;
    std::vector<MtrxArr<float> >              rec_val_float ;
    std::vector<MtrxArr<double> >             rec_val_double ;
    std::vector<MtrxArr<char*> >              rec_val_string ;

    // used in templates
    signed char        *v_schar;
    unsigned char      *v_uchar ;
    char               *v_text ;
    short              *v_short ;
    unsigned short     *v_ushort ;
    int                *v_int ;
    unsigned int       *v_uint ;
    unsigned long long *v_ulonglong ;
    long long          *v_longlong ;
    float              *v_float ;
    double             *v_double ;
    char**             *v_string ;

    signed char        *p_schar;
    unsigned char      *p_uchar ;
    char               *p_text ;
    short              *p_short ;
    unsigned short     *p_ushort ;
    int                *p_int ;
    unsigned int       *p_uint ;
    unsigned long long *p_ulonglong ;
    long long          *p_longlong ;
    float              *p_float ;
    double             *p_double ;
    char**             *p_string ;

    bool isThisOpen;
    bool isNC4;
    bool isNcFilenameChanged;

    // messaging in case of exceptions.
    struct ExceptionStruct xcptn;

    //! Pointer to the annotation object.
    Annotation *notes;

/*
    void
      addAtt(int varid, std::string aName, std::string &val);

    template <typename Type>
    void
      addAtt(int varid, std::string aName, Type &val);

    template <typename Type>
    void
      addAtt(int varid, std::string aName, Type *p, size_t size);

    template <typename Type>
    void
      addAtt(int varid, std::string aName, std::vector<Type> &val);
*/

    void
      addAttToLayout(int varid, std::string attName, nc_type, size_t len);

    void
      clearLayout(void);

    template <typename T_in, typename T_out>
    void
      convert( size_t N, T_in *arr_in, T_out *&arr_out);

    template <typename T>
    nc_type
      convTypeID(T x);

    void
      copyAtt(NcAPI &, int varid_from, int varid_to,
            std::string aName);
    void
      copyAtts(NcAPI &, int varid_from, int varid_to,
            std::vector<std::string> *pvs=0);

    void
      copyChunking(NcAPI &, int varid_from, int varid_to);

    void
      copyData(NcAPI &from, int varid_from, int varid,
          size_t rec_in, size_t rec_out=UINT_MAX);

    void
      copyDeflate(NcAPI &, int varid_from, int varid_to);

    void
      copyEndian(NcAPI &, int varid_from, int varid_to);

    void
      copyFillValue(NcAPI &, int varid_from, int varid_to);

    void
      copyFletcher32(NcAPI &, int varid_from, int varid_to);

    void
      copyGlobalAtts(NcAPI &u, int varid_u, int varid,
           std::vector<std::string> *paN) ;

    void
      generateParseAtt(ReadLine &rC, std::string vName="");

    void
      generateParseVar(ReadLine &rC,std::vector<std::string> &dimStrs);
/*
    void
      generateWriteLimited(std::string s, std::string &vName,
            NcType &nctype);
*/
/*
    template <typename Type>
    void
      get(std::string name, Type *x, long *counts, long *start=0);
*/

    void
      getChunking(NcAPI &u, int varid,
         size_t &storage, std::vector<size_t> &chunks);

    // size without the trailing '\0'. Return -1, if not terminating
    // char was found within the size of trySz
    template <typename T>
    int
      getConstCharSize(T v, size_t trySz=250);

    void*
      getData(int varid, size_t rec, int leg=1);

// Get number of values per record of variable 'vName'.
    size_t
      getDataSize(int varid);

    void
      getDeflate(NcAPI &, int varid,
         int &shuffle, int &deflate, int &level);

    template <typename T>
    void
      getDefaultFillValue(nc_type type, T& x);

    void
      getEndian(NcAPI &u, int varid, int &endian)
         {endian =u.layout.varEndian[varid] ;}

    void
      getFletcher32(NcAPI &u, int varid, int &f)
         {f =u.layout.varFletcher32[varid] ;}

    size_t
      getRecordSize(int varid) ;

    void
      init(void);

    void
      layoutVarAttPushes(void);

    void
      layoutVarDataPushes(std::string& vName, nc_type type);

    template <typename Type>
    void
      layoutVarDataPushesT(MtrxArr<Type>&, std::string& vName );
    void
      layoutVarDataPushesStr(MtrxArr<char*>&, std::string& vName );

    void
      layoutVarDataPushesVoid(nc_type type);

    void
      print_error(std::string method="", std::string message="");

    template <typename Type>
    void
      putData(size_t rec, int varid, Type *arr);

    template <typename Type>
    void
      putData(size_t rec_beg, size_t rec_end, int varid, Type *arr );

    template <typename Type>
    void
      putDat1(size_t rec_beg, int varid, Type *arr );

    template <typename ToT, typename FromT>
    void
      putData(size_t rec, int varid, FromT *from,
        ToT x, ToT (*func)(double,int,size_t), size_t recOut );

// --------- specialisations
    void
      setAtt(int varid, std::string aName, std::string &str);

    void
      setAtt(int varid, std::string aName,
         std::vector<std::string> &text) ;

// --------- no conversion
// vector
    template <typename Type>
    void
      setAtt(int varid, std::string aName, std::vector<Type> &values);

// ---------- with conversion
// vector and conversion
    template <typename To, typename From>
    void
      setAtt(int varid, std::string aName,
              std::vector<From> &values, To x);

// array
    template <typename T>
    void
      setAtt(int varid, std::string aName, T *arr, size_t sz);

// array and conversion
    template <typename To, typename From>
    void
      setAtt(int varid, std::string aName, From *arr, size_t sz, To x);

// plain value
    template <typename From>
    void
      setAtt(int varid, std::string aName, From val);

// plain value and conversion
    template <typename From, typename To>
    void
      setAtt(int varid, std::string aName, From val, To to);

    // convert string(s) to char *.
    void
      setAttString(int varid, std::string aName,
       std::vector<std::string> &text);

    void
      setAttString(int varid, std::string aName, std::string text);

//! Chunking for netcdf4
    void
      setChunking(int varid, size_t storage, size_t *chunkSize, size_t sz);

    void
      updateAtts(int varid, std::string aName, size_t aIndex,
            nc_type ncType, size_t len);
};

#endif
