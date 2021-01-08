#ifndef QDIMACSParser_h
#define QDIMACSParser_h

#include <vector>
#include <string>
#include <unordered_map>

#include "QBFParser.h"

using std::vector;
using std::string;

class QDIMACSParser: virtual public QBFParser {
  public:
    QDIMACSParser(const string& filename);

  protected:
    QDIMACSParser();
    void readQuantifierBlock(const string& line);
    void readClause(const string& line);
    void addOutputGate();
    vector<string> convertClause(vector<int>& clause);

    virtual void printQDIMACSPrefix(std::ostream& out);
    virtual void addToClauseList(int alias, const GatePolarity polarity, vector<vector<int>>& clause_list);
    virtual void addOutputUnit(bool negate, vector<vector<int>>& clause_list);

    static const string EXISTS_STRING;
    static const string FORALL_STRING;
};

#endif