#ifndef QBFParser_h
#define QBFParser_h

#include <unordered_map>
#include <vector>
#include <string>
#include <tuple>
#include <iostream>
#include <tuple>

#include "extractor.h"
#include "VariableComparator.h"

using std::vector;
using std::string;
using std::tuple;
using std::unordered_map;

typedef tuple<vector<int>,int> definition;

enum class VariableType: int8_t { Existential = 0, Universal = 1};
enum class GateType { None, Existential, Universal, And, Or };
enum class GatePolarity: int { None = 0, Positive = 1, Negative = 2, Both = 3};

struct Gate {
  string gate_id;
  GateType gate_type;
  unsigned int variable_depth;
  int* gate_inputs;
  unsigned int nr_inputs;

  Gate(string gate_id): gate_id(gate_id), gate_type(GateType::None), variable_depth(0), gate_inputs(nullptr), nr_inputs(0) {}
};

class QBFParser {
public:
  QBFParser();
  virtual ~QBFParser();
  void setComparator(const string& comparator_filename);
  void getDefinitions(Extractor& extractor);
  void writeQCIR(const string& filename);
  void writeQCIR();
  void writeQDIMACS(const string& filename);
  void writeQDIMACS();
  void writeDIMACS(const string& filename);
  void writeDIMACS();
  void writeVerilog();
  void writeVerilog(const string& filename);

protected:
  virtual void doGetDefinitions(Extractor& extractor);
  virtual void printQDIMACSPrefix(std::ostream& out);
  virtual void doWriteQCIR(std::ostream& out);
  virtual void doWriteVerilog(std::ostream& out);
  void doWriteQDIMACS(std::ostream& out);
  void doWriteDIMACS(std::ostream& out);
  void printClauselist(vector<vector<int>>& clause_list, std::ostream& out);
  void printQCIRPrefix(std::ostream& out);
  void printQCIRGate(Gate& gate, std::ostream& out);
  void printQCIRGates(std::ostream& out);
  void printAndOrGateVerilog(std::ostream& out, const int alias);
  template<typename T> void paste(std::ostream& out, vector<T>& arguments, const string& separator);

  bool startsWith(const string& line, const string& pattern);
  vector<string> split(const string& s, char delimiter);

  void addDefinition(vector<int>& input_literals, int output_alias);
  vector<vector<int>> getMatrix(bool negate, bool tseitin=false);
  vector<vector<int>> getDefinitionClauses();
  int getMaxVariableInt();
  int numberVariables(VariableType type);
  tuple<vector<int>, vector<int>, vector<bool>> getQueryVariableSets(VariableType type);
  void addDefinitions(vector<definition>& definitions, vector<int>& defined_variables);
  auto getDefinitionsFor(Extractor& extractor, VariableType type);
  void addVariable(const string& id, const VariableType type);
  void addGate(const string& id, const GateType& gate_type, const vector<string>& input_literals);
  unsigned int removeRedundant();
  int getAlias(const string& gate_id);
  void getGatePolarities(vector<GatePolarity>& polarities, GatePolarity output_polarity);
  virtual void addToClauseList(int alias, const GatePolarity polarity, vector<vector<int>>& clause_list);
  virtual void addOutputUnit(bool negate, vector<vector<int>>& clause_list);
  int sign(const string& literal_string);
  bool isNumber(const string& s);
  vector<int> gateTopologicalOrdering();
  bool clausesOK(vector<vector<int>>& clause_list);

  unsigned int max_quantifier_depth;
  int max_alias;
  string output_id;
  unordered_map<string, int> id_to_alias;
  vector<Gate> gates;
  int variable_gate_boundary;
  int number_variables[2];
  int max_id_number;
  vector<string> defined_ids;
  vector<int> definition_aliases;
  VariableComparator* comparator;

  static const string EXISTS_STRING;
  static const string FORALL_STRING;
  static const string OUTPUT_STRING;
  static const string AND_STRING;
  static const string OR_STRING;

};

#endif