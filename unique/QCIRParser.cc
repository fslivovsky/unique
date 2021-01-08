#include "QCIRParser.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <assert.h>
#include <algorithm>

using std::make_tuple;

QCIRParser::QCIRParser() {};

QCIRParser::QCIRParser(const string& filename) {
  std::ifstream file(filename.c_str());
  string line;
  while (std::getline(file, line)) {
    line.erase(remove_if(line.begin(), line.end(), [] (char c) { return isspace(int(c)); }), line.end()); // Remove whitespaces.
    if (line.length() == 0 || line.front() == '#') {
      continue;
    } else if (startsWith(line, QBFParser::FORALL_STRING) || startsWith(line, QBFParser::EXISTS_STRING)) {
      readQuantifierBlock(line);
    } else if (startsWith(line, QBFParser::OUTPUT_STRING)) {
      readOutput(line);
    } else {
      readGate(line);
    }
  }
  assert(output_id.size());
  assert(id_to_alias.find(output_id) != id_to_alias.end());
  std::cerr << "Done parsing " << gates.size() << " gates." << std::endl;

  /* Remove redundant gates (optional). */
  std::cerr << "Removed " << removeRedundant() << " redundant gates." << std::endl;
}

void QCIRParser::readQuantifierBlock(const string& line) {
  max_quantifier_depth++;
  assert(line.back() == ')');
  auto opening_pos = line.find('(');
  assert(opening_pos != string::npos);
  auto quantifier_string = line.substr(0, opening_pos);
  VariableType type = (quantifier_string == QBFParser::EXISTS_STRING) ? VariableType::Existential : VariableType::Universal;
  auto variables_string = line.substr(opening_pos + 1, line.size() - opening_pos  - 2);
  for (auto& v: split(variables_string, ',')) {
    addVariable(v, type);
  }
}

void QCIRParser::readGate(const string& line) {
  assert(line.back() == ')');
  auto equals_pos = line.find('=');
  assert(equals_pos != string::npos);
  auto opening_pos = line.find('(');
  assert(opening_pos != string::npos);
  auto gate_type_string = line.substr(equals_pos + 1, opening_pos - equals_pos - 1);
  auto gate_id = line.substr(0, equals_pos);
  assert(gate_type_string == QBFParser::AND_STRING || gate_type_string == QBFParser::OR_STRING);
  GateType gate_type = (gate_type_string == QBFParser::AND_STRING) ? GateType::And : GateType::Or;
  auto gate_inputs_string = line.substr(opening_pos + 1, line.length() - opening_pos - 2);
  auto input_literals = split(gate_inputs_string, ',');
  addGate(gate_id, gate_type, input_literals);
}

void QCIRParser::readOutput(const string& line) {
  assert(line.back() == ')');
  auto opening_pos = line.find('(');
  assert(opening_pos == QBFParser::OUTPUT_STRING.size());
  output_id = line.substr(opening_pos + 1, line.length() - opening_pos - 2);
}
