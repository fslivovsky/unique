#include "QDIMACSParser.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <assert.h>
#include <algorithm>

const string QDIMACSParser::FORALL_STRING = "a";
const string QDIMACSParser::EXISTS_STRING = "e";

using std::make_tuple;

QDIMACSParser::QDIMACSParser() {}

QDIMACSParser::QDIMACSParser(const string& filename) {
  std::ifstream file(filename.c_str());
  string line;
  while (std::getline(file, line)) {
    if (line.length() == 0 || line.front() == 'c' || line.front() == 'p') { // Ignore preamble as well.
      continue;
    } else if (startsWith(line, FORALL_STRING) || startsWith(line, EXISTS_STRING)) {
      readQuantifierBlock(line);
    } else {
      readClause(line);
    }
  }
  // Add dummy output gate.
  addOutputGate();
}

void QDIMACSParser::addOutputGate() {
  vector<int> clause_aliases;
  for (unsigned i = variable_gate_boundary; i < gates.size(); i++) {
    clause_aliases.push_back(i);
  }
  max_id_number++;
  output_id = std::to_string(max_id_number);
  int output_alias = getAlias(output_id);
  Gate& output_gate = gates[output_alias];
  output_gate.gate_type = GateType::And;
  output_gate.gate_inputs = new int[clause_aliases.size()];
  output_gate.nr_inputs = clause_aliases.size();
  for (unsigned i = 0; i < clause_aliases.size(); i++) {
    output_gate.gate_inputs[i] = clause_aliases[i];
  }
}

void QDIMACSParser::readQuantifierBlock(const string& line) {
  max_quantifier_depth++;
  vector<string> line_split = split(line, ' ');
  line_split.erase(remove_if(line_split.begin(), line_split.end(), [](string& x) { return x.empty(); } ), line_split.end());
  auto quantifier_string = line_split.front();
  assert(quantifier_string == EXISTS_STRING || quantifier_string == FORALL_STRING);
  VariableType type = (quantifier_string == EXISTS_STRING) ? VariableType::Existential : VariableType::Universal;
  assert(line_split.back() == "0");
  for (unsigned i = 1; i < line_split.size() - 1; i++) {
    auto& v = line_split[i];
    addVariable(v, type);
  }
}

void QDIMACSParser::readClause(const string& line) {
  auto line_split = split(line, ' ');
  line_split.erase(remove_if(line_split.begin(), line_split.end(), [](string& x) { return x.empty(); } ), line_split.end());
  assert(line_split.back() == "0");
  line_split.pop_back();
  addGate(std::to_string(max_id_number+1), GateType::Or, line_split);
}

void QDIMACSParser::printQDIMACSPrefix(std::ostream& out) {
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
  // Check if any definitions were added and include Tseitin variables for them.
  vector<int> and_gates;
  for (unsigned alias = 1; alias < gates.size(); alias++) {
    auto& gate = gates[alias];
    if (gate.gate_type == GateType::And && gate.gate_id != output_id) {
      and_gates.push_back(alias);
    }
  }
  if (and_gates.size() > 0) {
    if (last_block_type == GateType::Universal) { // Open new quantifier block if necessary.
      out << "0" << std::endl << "e ";
    }
    for (auto& alias: and_gates) {
      auto& gate = gates[alias];
      out << gate.gate_id << " ";
    }
  }
  if (first_variable_seen) {  // Close last quantifier block.
    out << "0" << std::endl;
  }
}

void QDIMACSParser::addToClauseList(int alias, const GatePolarity polarity, vector<vector<int>>& clause_list) {
  auto& gate = gates[alias];
  // Do not introduce auxiliary variables/clauses for input clauses and the output gate.
  if (gate.gate_type == GateType::Or) {
    clause_list.emplace_back(vector<int>(gate.gate_inputs, gate.gate_inputs + gate.nr_inputs));
  } else if (gate.gate_id != output_id) {
    QBFParser::addToClauseList(alias, polarity, clause_list);
  }
}

void QDIMACSParser::addOutputUnit(bool negate, vector<vector<int>>& clause_list) {
  if (negate) {
    clause_list.push_back({-id_to_alias[output_id]});
  }
}
