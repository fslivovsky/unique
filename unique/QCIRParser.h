#ifndef QCIRParser_h
#define QCIRParser_h

#include <vector>
#include <string>
#include "QBFParser.h"

using std::vector;
using std::string;

class QCIRParser: virtual public QBFParser {
  public:
    QCIRParser(const string& filename);

  protected:
    QCIRParser();
    void readQuantifierBlock(const string& line);
    void readGate(const string& line);
    void readOutput(const string& line);

    static const string EXISTS_STRING;
    static const string FORALL_STRING;
    static const string OUTPUT_STRING;
    static const string AND_STRING;
    static const string OR_STRING;
};

#endif