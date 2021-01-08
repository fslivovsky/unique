#ifndef DQCIR_Parser_h
#define DQCIR_Parser_h

#include <fstream>
#include "DQDIMACSParser.h"
#include "QCIRParser.h"

class DQCIRParser: public DQDIMACSParser, public QCIRParser {
public:
  DQCIRParser(const std::string& filename);

protected:
  virtual void doWriteQCIR(std::ostream& out);
  virtual void doGetDefinitions(Extractor& extractor);

};

#endif