#ifndef _QA_CONSISTENCY_H
#define _QA_CONSISTENCY_H

#include "hdhC.h"
#include "date.h"
#include "annotation.h"

class QA;
class InFile;

class Consistency
{
  public:
  Consistency(QA *, InFile*, std::vector<std::string> &opt,
              std::string tPath="" );

  void   applyOptions(std::vector<std::string> &opt, std::string& tPath);

  //! Prepare the comparison of dimensions between file and project table.
  /*! This is checked for each chunk or atomic data set in each
  experiment ensuring conformance.*/
  bool   check(void);
  bool   check(Variable&, std::string entryID);

  //! Put Attributes of given variable to string
  void   getAtts(Variable&, std::string &) ;

  //! Put all meta-data to string.
  void   getMetaData(Variable&, std::string& entryID, std::string &md);

  //! Get the checksum of all non-unlimited variables
  void   getValues(Variable&, std::string &) ;

  void   getVarType(Variable&, std::string &);

  bool   isEnabled(void){ return checkEnabled; }
  bool   lockFile(std::string &fName);

  void   setAnnotation(Annotation *p){ notes=p;}
  void   setExcludedAttributes(std::vector<std::string> &);
  void   setTable(std::string &p, std::string &t);

  void   testAux(std::string mode,
            std::vector<std::vector<std::string> >& vvs_1st_aName,
            std::vector<std::vector<std::string> >& vvs_1st_aVal,
            std::vector<std::vector<std::string> >& vvs_2nd_aName,
            std::vector<std::vector<std::string> >& vvs_2nd_aVal);

  void   testAttributes( std::string& vName,
            std::vector<std::string>& file_aName,
            std::vector<std::string>& file_aVal,
            std::vector<std::string>& table_aName,
            std::vector<std::string>& table_aVal);

  bool   unlockFile(std::string &fName);

  //! Add meta data of each variable to the project table.
  /*! Meta-data of the data variable and the auxiliaries.*/
  void   write(Variable&, std::string& entryID);

  std::vector<std::string> excludedAttributes;

  hdhC::FileSplit consistencyTableFile;

  bool checkEnabled;

  Annotation *notes;
  InFile     *pIn;
  QA         *pQA;  //the parent
} ;

#endif
