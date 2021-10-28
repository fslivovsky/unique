#include "QBFParser.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <assert.h>
#include <algorithm>
#include <iterator>

const string QBFParser::FORALL_STRING = "forall";
const string QBFParser::EXISTS_STRING = "exists";
const string QBFParser::OUTPUT_STRING = "output";
const string QBFParser::AND_STRING = "and";
const string QBFParser::OR_STRING = "or";

using std::make_tuple;

GatePolarity operator-(const GatePolarity& polarity) {
  if (polarity == GatePolarity::None || polarity == GatePolarity::Both) {
    return polarity;
  } else {
    return static_cast<GatePolarity>(static_cast<int>(GatePolarity::Both) - static_cast<int>(polarity));
  }
}

GatePolarity operator+(const GatePolarity& first, const GatePolarity& second) {
  if (first == second) {
    return first;
  } else {
    int first_int = static_cast<int>(first);
    int second_int = static_cast<int>(second);
    int return_int = std::min(first_int + second_int, 3);
    return static_cast<GatePolarity>(return_int);
  }
}

QBFParser::QBFParser(): max_quantifier_depth(0), max_alias(0), output_id(""), variable_gate_boundary(1), number_variables{0, 0}, max_id_number(0), comparator(nullptr) {
  gates.push_back(Gate("")); // Add dummy gate for 1-based indexing.
}

void QBFParser::setComparator(const string& comparator_filename) {
  comparator = new VariableComparator(comparator_filename);
}

QBFParser::~QBFParser() {
  for (auto& gate: gates) {
    delete[] gate.gate_inputs;
  }
  delete comparator;
}


int QBFParser::getAlias(const string& gate_id) {
  if (id_to_alias.find(gate_id) == id_to_alias.end()) {
    max_alias++;
    id_to_alias[gate_id] = max_alias;
    gates.push_back(Gate(gate_id));
    assert(max_alias == gates.size() - 1);
  }
  return id_to_alias[gate_id];
}

int QBFParser::sign(const string& literal_string) {
  assert(literal_string.size());
  return (literal_string.front() == '-') ? -1 : 1;
}

void QBFParser::addVariable(const string& id, const VariableType type) {
  if (isNumber(id)) {
    max_id_number = std::max(max_id_number, std::stoi(id));
  }
  int alias = getAlias(id);
  gates[alias].variable_depth = max_quantifier_depth;
  if (type == VariableType::Existential) {
    gates[alias].gate_type = GateType::Existential;
    number_variables[0]++;
  } else {
    gates[alias].gate_type = GateType::Universal;
    number_variables[1]++;
  }
  variable_gate_boundary++;
}

void QBFParser::addGate(const string& id, const GateType& gate_type, const vector<string>& input_literals) {
  assert(gate_type == GateType::And || gate_type == GateType::Or);
  if (isNumber(id)) {
    max_id_number = std::max(max_id_number, std::stoi(id));
  }
  int alias = getAlias(id);
  assert(alias < gates.size());
  assert(gates[alias].gate_inputs == nullptr);
  assert(gates[alias].gate_type == GateType::None);
  gates[alias].gate_type = gate_type;
  gates[alias].gate_inputs = new int[input_literals.size()];
  for (unsigned i = 0; i < input_literals.size(); i++) {
    auto& literal_string = input_literals[i];
    string input_id = literal_string.front() == '-' ? literal_string.substr(1): literal_string;
    int input_alias = getAlias(input_id);
    gates[alias].gate_inputs[i] = sign(literal_string) * input_alias;
  }
  gates[alias].nr_inputs = input_literals.size();
}

unsigned int QBFParser::removeRedundant() {
  vector<int> nr_outputs(gates.size(), 0);
  for (unsigned i = variable_gate_boundary; i < gates.size(); i++) {
    auto& gate = gates[i];
    if (gate.gate_type == GateType::And || gate.gate_type == GateType::Or) {
      for (unsigned i = 0; i < gate.nr_inputs; i++) {
        auto variable_alias = abs(gate.gate_inputs[i]);
        nr_outputs[variable_alias]++;
      }
    }
  }
  vector<int> queue;
  for (unsigned alias = variable_gate_boundary; alias < gates.size(); alias++) {
    auto& gate = gates[alias];
    if (gate.gate_id != output_id && nr_outputs[alias] == 0 && (gate.gate_type == GateType::And || gate.gate_type == GateType::Or)) {
      queue.push_back(alias);
    }
  }
  auto nr_deleted = 0;
  while (!queue.empty()) {
    auto& gate = gates[queue.back()];
    queue.pop_back();
    nr_deleted++;
    gate.gate_type = GateType::None;
    if (gate.gate_inputs != nullptr) {
      for (unsigned i = 0; i < gate.nr_inputs; i++ ) {
        auto variable_alias = abs(gate.gate_inputs[i]);
        nr_outputs[variable_alias]--;
        if (nr_outputs[variable_alias] == 0 && (gates[variable_alias].gate_type == GateType::And || gates[variable_alias].gate_type == GateType::Or)) {
          // Don't put input gates on the queue (i.e. don't delete input variables).
          queue.push_back(variable_alias);
        }
      }
    }
  }
  return nr_deleted;
}

void QBFParser::getGatePolarities(vector<GatePolarity>& polarity, GatePolarity output_polarity) {
  vector<int> nr_output_gates(gates.size(), 0);
  for (unsigned i = 1; i < gates.size(); i++) {
    auto& gate = gates[i];
    if (gate.gate_type == GateType::And || gate.gate_type == GateType::Or) {
      for (unsigned i = 0; i < gate.nr_inputs; i++) {
        auto variable_alias = abs(gate.gate_inputs[i]);
        nr_output_gates[variable_alias]++;
      }
    }
  }
  polarity.resize(gates.size(), GatePolarity::None);
  auto output_alias = id_to_alias[output_id];
  polarity[output_alias] = output_polarity;
  vector<int> queue = {output_alias};
  while (!queue.empty()) {
    auto alias = queue.back();
    auto& gate = gates[alias];
    assert(polarity[alias] != GatePolarity::None);
    queue.pop_back();
    if (gate.gate_inputs != nullptr) {
      for (unsigned i = 0; i < gate.nr_inputs; i++) {
        auto& input_literal = gate.gate_inputs[i];
        auto variable_alias = abs(input_literal);
        auto child_polarity = input_literal > 0 ? polarity[alias] : -polarity[alias];
        assert(variable_alias < polarity.size());
        polarity[variable_alias] = polarity[variable_alias] + child_polarity;
        assert(polarity[variable_alias] != GatePolarity::None);
        nr_output_gates[variable_alias]--;
        if (nr_output_gates[variable_alias] == 0 && (gates[variable_alias].gate_type == GateType::And || gates[variable_alias].gate_type == GateType::Or)) {
          queue.push_back(variable_alias);
        }
      }
    }
  }
}

void QBFParser::addToClauseList(int alias, const GatePolarity polarity, vector<vector<int>>& clause_list) {
  auto& gate = gates[alias];
  if (gate.gate_type != GateType::And && gate.gate_type != GateType::Or) {
    return;
  }
  for (unsigned i = 0; i < gate.nr_inputs; i++) {
    auto input_literal = gate.gate_inputs[i];
    if (gate.gate_type == GateType::And && polarity != GatePolarity::Negative) {
      // AND: input_literal false forces output false | add if polarity not negative
      clause_list.push_back(vector<int> {input_literal, -alias});
    } else if (gate.gate_type == GateType::Or && polarity != GatePolarity::Positive) {
      // OR: input_literal_true forces output true | add if polarity not positive
      clause_list.push_back(vector<int> {-input_literal, alias});
    }
  }
  // AND: all input_literals true enforces true -> clause -gate_inputs or alias | add if polarity not positive
  if (gate.gate_type == GateType::And && polarity != GatePolarity::Positive) {
    vector<int> large_gate_clause;
    for (unsigned i = 0; i < gate.nr_inputs; i++) {
      auto input_literal = gate.gate_inputs[i];
      large_gate_clause.push_back(-input_literal);
    }
    large_gate_clause.push_back(alias);
    clause_list.push_back(large_gate_clause);
  }
  // OR: all input_literals false enforce false -> clause gate_inputs or -alias | add if polarity not negative
  if (gate.gate_type == GateType::Or && polarity != GatePolarity::Negative) {
    vector<int> large_gate_clause(gate.gate_inputs, gate.gate_inputs + gate.nr_inputs);
    large_gate_clause.push_back(-alias);
    clause_list.push_back(large_gate_clause);
  }
}

void QBFParser::addOutputUnit(bool negate, vector<vector<int>>& clause_list) {
 if (negate) {
    clause_list.push_back({-id_to_alias[output_id]});
  } else {
    clause_list.push_back({id_to_alias[output_id]});
  }
}

vector<vector<int>> QBFParser::getMatrix(bool negate, bool tseitin) {
  GatePolarity output_polarity;
  if (tseitin) {
    output_polarity = GatePolarity::Both;
  } else if (negate) {
    output_polarity = GatePolarity::Negative;
  } else {
    output_polarity = GatePolarity::Positive;
  }
  vector<GatePolarity> polarity;
  getGatePolarities(polarity, output_polarity);
  vector<vector<int>> clause_list;
  for (unsigned alias = 1; alias < gates.size(); alias++) {
    addToClauseList(alias, polarity[alias], clause_list);
  }
  addOutputUnit(negate, clause_list);
  assert(clausesOK(clause_list));
  return clause_list;
}

bool QBFParser::clausesOK(vector<vector<int>>& clause_list) {
  for (auto& clause: clause_list) {
    for (auto l: clause) {
      if (abs(l) > getMaxVariableInt()) {
        return false;
      }
    }
  }
  return true;
}

vector<vector<int>> QBFParser::getDefinitionClauses() {
  vector<vector<int>> definition_clauses;
  for (int alias: definition_aliases) {
    addToClauseList(alias, GatePolarity::Both, definition_clauses);
  }
  return definition_clauses;
}

int QBFParser::getMaxVariableInt() {
  return gates.size() - 1;
}

int QBFParser::numberVariables(VariableType type) {
  return number_variables[static_cast<int>(type)];
}

void QBFParser::addDefinitions(vector<tuple<vector<int>,int>>& definitions, vector<int>& defined_variables) {
  for (const auto& defined_alias: defined_variables) {
    defined_ids.push_back(gates[defined_alias].gate_id);
  }
  for (auto& [input_literals, output_alias]: definitions) {
    addDefinition(input_literals, output_alias);
  }
}

auto QBFParser::getDefinitionsFor(Extractor& extractor, VariableType type) {
  bool negate = (type == VariableType::Universal);
  auto propositional_matrix = getMatrix(negate);
  int max_variable_int = getMaxVariableInt();
  auto [shared_variables, query_variables, query_mask] = getQueryVariableSets(type);
  auto [defined, definitions] = extractor.getDefinitions(propositional_matrix, query_variables, shared_variables, query_mask, max_variable_int);
  float fraction = float(defined.size()) / float(numberVariables(type));
  string qtype_string_long = (type == VariableType::Universal) ? "universal" : "existential";
  std::cerr << "Found " << defined.size() << " out of " << numberVariables(type) << " " << qtype_string_long << " variables uniquely determined (" << fraction << ")." << std::endl;
  return std::make_tuple(defined, definitions);
}

tuple<vector<int>, vector<int>, vector<bool>> QBFParser::getQueryVariableSets(VariableType type) {
  vector<int> defining_variables;
  GateType variable_type = (type == VariableType::Universal) ? GateType::Universal : GateType::Existential;
  unsigned int alias = 1;
  if (type == VariableType::Universal) {
    // Don't look for unique Herbrand functions of outermost universals.
    for (; alias < variable_gate_boundary && gates[alias].gate_type == GateType::Universal; alias++) {
      defining_variables.push_back(alias);
    }
  }
  for (; alias < variable_gate_boundary && gates[alias].gate_type != variable_type; alias++) {
    defining_variables.push_back(alias);
  }
  // All remaining variables go into the query variables.
  vector<std::tuple<int, string, bool>> query_tuples;

  for (; alias < variable_gate_boundary; alias++) {
    query_tuples.emplace_back(alias, gates[alias].gate_id, gates[alias].gate_type == variable_type);
  }

  if (comparator != nullptr) {
    std::sort(query_tuples.begin(), query_tuples.end(), *comparator);
  }

  vector<int> query_variables;
  vector<bool> query_mask;

  for (const auto& [alias, _, mask_value]: query_tuples) {
    query_variables.push_back(alias);
    query_mask.push_back(mask_value);
  }

  return make_tuple(defining_variables, query_variables, query_mask);
}

void QBFParser::getDefinitions(Extractor& extractor) {
  doGetDefinitions(extractor);
}

void QBFParser::doGetDefinitions(Extractor& extractor) {
  auto [defined_existentials, definitions_existentials] = getDefinitionsFor(extractor, VariableType::Existential);
  auto [defined_universals, definitions_universals] = getDefinitionsFor(extractor, VariableType::Universal);

  if (defined_existentials.size() > 0) {
    std::cerr << "Processing existential definitions. " << std::endl;
    addDefinitions(definitions_existentials, defined_existentials);
  }

  if (defined_universals.size() > 0) {
    std::cerr << "Processing universal definitions. " << std::endl;
    addDefinitions(definitions_universals, defined_universals);
  }
}

bool QBFParser::startsWith(const string& line, const string& pattern) {
  return line.compare(0, pattern.length(), pattern) == 0;
}

vector<string> QBFParser::split(const string& s, char delimiter)
{
   vector<string> tokens;
   string token;
   std::istringstream tokenStream(s);
   while (std::getline(tokenStream, token, delimiter))
   {
      tokens.push_back(token);
   }
   return tokens;
}

void QBFParser::addDefinition(vector<int>& input_literals, int output_alias) {
  definition_aliases.push_back(output_alias);
  auto gate_type = GateType::And;
  if (output_alias >= gates.size()) {
    gates.resize(output_alias + 1, Gate(""));
  }
  auto& gate = gates[output_alias];
  gate.gate_type = gate_type;
  if (gate.gate_id == "") {
    gate.gate_id = std::to_string(++max_id_number);
    assert(id_to_alias.find(gate.gate_id) == id_to_alias.end());
    id_to_alias[gate.gate_id] = output_alias;
  }
  assert(gate.gate_inputs == nullptr);
  gate.gate_inputs = new int[input_literals.size()];
  std::copy(input_literals.begin(), input_literals.end(), gate.gate_inputs);
  gate.nr_inputs = input_literals.size();
}

bool QBFParser::isNumber(const string& s)
{
  return !s.empty() && std::find_if(s.begin(), s.end(), [](char c) { return !std::isdigit(c); }) == s.end();
}

vector<int> QBFParser::gateTopologicalOrdering() {
  vector<int> nr_output_gates(gates.size(), 0);
  for (unsigned i = 1; i < gates.size(); i++) {
    auto& gate = gates[i];
    if (gate.gate_type == GateType::And || gate.gate_type == GateType::Or) {
      for (unsigned i = 0; i < gate.nr_inputs; i++) {
        auto variable_alias = abs(gate.gate_inputs[i]);
        nr_output_gates[variable_alias]++;
      }
    }
  }
  vector<int> gates_ordered;
  auto output_alias = id_to_alias[output_id];
  vector<int> queue = {output_alias};
  while (!queue.empty()) {
    auto alias = queue.back();
    queue.pop_back();
    gates_ordered.push_back(alias);
    auto& gate = gates[alias];
    if (gate.gate_inputs != nullptr) {
      for (unsigned i = 0; i < gate.nr_inputs; i++) {
        auto variable_alias = abs(gate.gate_inputs[i]);
        nr_output_gates[variable_alias]--;
        if (nr_output_gates[variable_alias] == 0 && (gates[variable_alias].gate_type == GateType::And || gates[variable_alias].gate_type == GateType::Or)) {
          queue.push_back(variable_alias);
        }
      }
    }
  }
  std::reverse(gates_ordered.begin(), gates_ordered.end());
  return gates_ordered;
}

void QBFParser::writeQCIR(const string& filename) {
  std::ofstream out(filename);
  if (out) {
    doWriteQCIR(out);
  } else {
    std::cerr << "Error opening file: " << filename << std::endl;
  }
}

void QBFParser::writeQCIR() {
  doWriteQCIR(std::cout);
}

void QBFParser::doWriteQDIMACS(std::ostream& out) {
  auto matrix_clauses = getMatrix(false);
  out << "c defined variables: ";
  std::copy(defined_ids.begin(), defined_ids.end(), std::ostream_iterator<string>(out, " "));
  out << std::endl;
  out << "p cnf " << max_id_number << " " << matrix_clauses.size() << std::endl;
  printQDIMACSPrefix(out);
  printClauselist(matrix_clauses, out);
}

void QBFParser::doWriteDIMACS(std::ostream& out) {
  auto definition_clauses = getDefinitionClauses();
  out << "c defined variables: ";
  std::copy(defined_ids.begin(), defined_ids.end(), std::ostream_iterator<string>(out, " "));
  out << std::endl;
  out << "p cnf " << max_id_number << " " << definition_clauses.size() << std::endl;
  printClauselist(definition_clauses, out);
}

void QBFParser::printClauselist(vector<vector<int>>& clause_list, std::ostream& out) {
  for (auto& clause: clause_list) {
    for (auto& literal: clause) {
      auto alias = abs(literal);
      auto sign = (literal > 0) ? "" : "-";
      out << sign << gates[alias].gate_id << " ";
    }
    out << "0" << std::endl;
  }
}

void QBFParser::printQDIMACSPrefix(std::ostream& out) {
  GateType last_block_type = GateType::None;
  bool first_variable_seen = false;
  for (unsigned i = 1; i < variable_gate_boundary; i++) {
    auto& gate = gates[i];
    if (gate.gate_type == GateType::Existential || gate.gate_type == GateType::Universal) {
      if (gate.gate_type != last_block_type) { 
        last_block_type = gate.gate_type;
        if (first_variable_seen) {
          out << "0" << std::endl; // Close last block unless this is the first variable.
        }
        auto block_start_string = (gate.gate_type == GateType::Existential) ? 'e' : 'a';
        out << block_start_string << " ";  // "Open" a new quantifier block.
      }
      out << gate.gate_id << " ";
      first_variable_seen = true;
    }
  }
  if (last_block_type == GateType::Universal) {
    out << "0" << std::endl;
    out << "e "; // Open new quantifier block for Tseitin variables.
  }
  for (unsigned alias = 1; alias < gates.size(); alias++) {
    auto& gate = gates[alias];
    if (gate.gate_type == GateType::And || gate.gate_type == GateType::Or) {
      out << gate.gate_id << " ";
    }
  }
  if (first_variable_seen) {
    out << "0" << std::endl;
  }
}

void QBFParser::writeQDIMACS(const string& filename) {
  std::ofstream out(filename);
  if (out) {
    doWriteQDIMACS(out);
  } else {
    std::cerr << "Error opening file: " << filename << std::endl;
  }
}

void QBFParser::writeQDIMACS() {
  doWriteQDIMACS(std::cout);
}

void QBFParser::writeDIMACS(const string& filename) {
  std::ofstream out(filename);
  if (out) {
    doWriteDIMACS(out);
  } else {
    std::cerr << "Error opening file: " << filename << std::endl;
  }
}

void QBFParser::writeDIMACS() {
  doWriteDIMACS(std::cout);
}

void QBFParser::printQCIRPrefix(std::ostream& out) {
  out << "#QCIR-G14" << std::endl; // Print "preamble".
  GateType last_block_type = GateType::None;
  bool first_variable_seen = false;
  for (unsigned i = 1; i < variable_gate_boundary; i++) {
    auto& gate = gates[i];
    if (gate.gate_type == GateType::Existential || gate.gate_type == GateType::Universal) {
      if (gate.gate_type != last_block_type) { 
        last_block_type = gate.gate_type;
        if (first_variable_seen) {
          out << ')' << std::endl; // Close last block unless this is the first variable.
        }
        auto& block_start_string = (gate.gate_type == GateType::Existential) ? EXISTS_STRING : FORALL_STRING;
        out << block_start_string << '(' << gate.gate_id; // "Open" a new quantifier block.
      } else {
        out << ", " << gate.gate_id;
      }
      first_variable_seen = true;
    }
  }
  if (first_variable_seen) { // Close last block (if there were any quantified variables).
    out << ')' << std::endl; 
  }
  out << "output(" << output_id << ')' << std::endl;
}

void QBFParser::printQCIRGate(Gate& gate, std::ostream& out) {
  if (gate.gate_type == GateType::And || gate.gate_type == GateType::Or) {
    auto& gate_type_string = (gate.gate_type == GateType::And) ? AND_STRING : OR_STRING;
    out << gate.gate_id << " = " << gate_type_string << '(';
    for (unsigned i = 0; i < gate.nr_inputs; i++) {
      if (i) {
        out << ", ";
      }
      int input_literal = gate.gate_inputs[i];
      int input_alias = abs(input_literal);
      auto sign_char = input_literal > 0 ? "" : "-";
      out << sign_char << gates[input_alias].gate_id;
    }
    out << ')' << std::endl;
  }
}

void QBFParser::printQCIRGates(std::ostream& out) {
  vector<int> gates_ordered = gateTopologicalOrdering();
  for (auto alias: gates_ordered) {
    printQCIRGate(gates[alias], out);
  }
}

void QBFParser::doWriteQCIR(std::ostream& out) {
  printQCIRPrefix(out);
  printQCIRGates(out);
}

void QBFParser::writeVerilog(const string& filename) {
  std::ofstream out(filename);
  if (out) {
    doWriteVerilog(out);
  } else {
    std::cerr << "Error opening file: " << filename << std::endl;
  }
}

void QBFParser::writeVerilog() {
  doWriteVerilog(std::cout);
}

void QBFParser::doWriteVerilog(std::ostream& out) {
  // Only works for 2QBF at the moment.
  vector<string> input_ids, output_ids, auxiliary_ids;
  for (unsigned i = 1; i < variable_gate_boundary; i++) {
    const auto& gate = gates[i];
    if (gate.gate_type == GateType::Universal || gate.gate_type == GateType::Existential) {
      input_ids.push_back("v_" + gate.gate_id);
    } else {
      // A definition of this variable was found.
      output_ids.push_back("v_" + gate.gate_id);
    }
  }
  for (const int& alias: definition_aliases) {
    const auto& gate = gates[alias];
    if (alias >= variable_gate_boundary) {
      // This is an auxiliary variable/gate.
      auxiliary_ids.push_back("v_" + gate.gate_id);
    }
  }
  out << "module definitions(";
  paste(out, input_ids, ", ");
  out << ", ";
  paste(out, output_ids, ", ");
  out << ");" << std::endl;
  if (input_ids.size()) {
    out << "input ";
    paste(out, input_ids, ", ");
    out << ";" << std::endl;
  }
  out << "output ";
  paste(out, output_ids, ", ");
  out << ";" << std::endl;
  if (auxiliary_ids.size()) {
    out << "wire ";
    paste(out, auxiliary_ids, ", ");
    out << ";" << std::endl;
  }
  for (const int& alias: definition_aliases) {
    printAndOrGateVerilog(out, alias);
  }
  out << "endmodule" << std::endl;
}

void QBFParser::printAndOrGateVerilog(std::ostream& out, const int alias) {
  const auto& gate = gates[alias];
  assert(gate.gate_type == GateType::Or || gate.gate_type == GateType::And);
  out << "assign " << "v_" + gate.gate_id << " = ";
  if (gate.nr_inputs > 0) {
    vector<string> gate_input_strings;
    for (unsigned i = 0; i < gate.nr_inputs; i++) {
      int input_literal = gate.gate_inputs[i];
      int input_alias = abs(input_literal);
      string sign_string = input_literal > 0 ? "" : "~";
      gate_input_strings.push_back(sign_string + "v_" + gates[input_alias].gate_id);
    }
    string separator = (gate.gate_type == GateType::And ? " & ": " | ");
    paste(out, gate_input_strings, separator);
  } else { // No inputs, simplifies to constant;
    out << (gate.gate_type == GateType::And ? "1": "0");
  }
  out << ";" << std::endl;
}

template<typename T> void QBFParser::paste(std::ostream& out, vector<T>& arguments, const string& separator) {
  for (unsigned i = 0; i < arguments.size(); i++) {
    if (i > 0) {
      out << separator;
    }
    out << arguments[i];
  }
}
