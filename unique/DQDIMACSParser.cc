#include "DQDIMACSParser.h"

#include <fstream>
#include <stdexcept>
#include <iterator>

using std::make_tuple;

const string DQDIMACSParser::DEPENDENCY_STRING = "d";

DQDIMACSParser::DQDIMACSParser(const string& filename) {
  std::ifstream file(filename.c_str());
  string line;
  while (std::getline(file, line)) {
    if (line.length() == 0 || line.front() == 'c' || line.front() == 'p') { // Ignore preamble as well.
      continue;
    } else if (startsWith(line, FORALL_STRING) || startsWith(line, EXISTS_STRING)) {
      readQuantifierBlock(line);
    } else if (startsWith(line, DEPENDENCY_STRING)) {
      readDependencyBlock(line);
    } else {
      readClause(line);
    }
  }
  // Add dummy output gate.
  addOutputGate();
}

DQDIMACSParser::DQDIMACSParser() {};

auto DQDIMACSParser::getExistentialQuerySets() {
  vector<int> defining_variables;
  vector<int> query_variables;
  vector<bool> query_mask;
  unsigned int alias;
  for (alias = 1; alias < variable_gate_boundary && gates[alias].gate_type != GateType::Existential; alias++) {
    defining_variables.push_back(alias);
  }
  // All remaining variables without explicit dependencies go into the query variables.
  for (; alias < variable_gate_boundary; alias++) {
    if (dependency_map.find(alias) == dependency_map.end()) {
      query_variables.push_back(alias);
      query_mask.push_back(gates[alias].gate_type == GateType::Existential);
    }
  }
  return make_tuple(defining_variables, query_variables, query_mask);
}

auto DQDIMACSParser::getOrdinaryExistentialDefinitions(Extractor& extractor) {
  auto propositional_matrix = getMatrix(false);
  auto [shared_variables, query_variables, query_mask] = getExistentialQuerySets();
  return extractor.getDefinitions(propositional_matrix, query_variables, shared_variables, query_mask, getMaxVariableInt());
}

auto DQDIMACSParser::getDependentExistentialDefinitions(Extractor& extractor) {
  vector<int> all_defined;
  vector<definition> all_definitions;
  for (auto& [dependencies_const, variables_const]: reverse_dependency_map) {
    if (dependencies_const.size()) {
      // Ignore existentials with empty dependency sets.
      vector<int> dependencies = dependencies_const;
      vector<int> variables = variables_const;
      vector<bool> query_mask(variables.size(), true);
      auto propositional_matrix = getMatrix(false); // Called separately for each dependency set, can this be avoided?
      auto [defined, definitions] = extractor.getDefinitions(propositional_matrix, variables, dependencies, query_mask, getMaxVariableInt());
      all_defined.insert(all_defined.end(), std::make_move_iterator(defined.begin()), std::make_move_iterator(defined.end()));
      all_definitions.insert(all_definitions.end(), std::make_move_iterator(definitions.begin()), std::make_move_iterator(definitions.end()));
    }
  }
  return std::make_tuple(all_defined, all_definitions);
}

void DQDIMACSParser::doGetDefinitions(Extractor& extractor) {
  auto [defined, definitions] = getOrdinaryExistentialDefinitions(extractor);
  auto [dependent_defined, dependent_definitions] = getDependentExistentialDefinitions(extractor);
  std::cerr << dependent_defined.size() << " of " << dependency_map.size() << " variables with explicit dependencies uniquely determined." << std::endl;

  defined.insert(defined.end(), std::make_move_iterator(dependent_defined.begin()), std::make_move_iterator(dependent_defined.end()));
  definitions.insert(definitions.end(), std::make_move_iterator(dependent_definitions.begin()), std::make_move_iterator(dependent_definitions.end()));

  float fraction = float(defined.size()) / float(numberVariables(VariableType::Existential));
  std::cerr << "Found " << defined.size() << " out of " << numberVariables(VariableType::Existential) << " existential variables uniquely determined (" << fraction << ")." << std::endl;

  if (defined.size() > 0) {
    std::cerr << "Processing definitions. " << std::endl;
    addDefinitions(definitions, defined);
  }
}

void DQDIMACSParser::readDependencyBlock(const string& line) {
  vector<string> line_split = split(line, ' ');
  line_split.erase(remove_if(line_split.begin(), line_split.end(), [](string& x) { return x.empty(); } ), line_split.end());

  auto dependency_string = line_split.front();
  assert(dependency_string == DEPENDENCY_STRING);
  assert(line_split.back() == "0");

  auto& dependent_variable_id = line_split[1];
  addVariable(dependent_variable_id, VariableType::Existential);
  int dependent_variable_alias = getAlias(dependent_variable_id);
  vector<int> dependencies;
  for (int i = 2; i < line_split.size() - 1; i++) {
    string& dependency_id = line_split[i];
    assert(id_to_alias.find(dependency_id) != id_to_alias.end());
    int dependency_alias = getAlias(dependency_id);
    dependencies.push_back(dependency_alias);
  }
  dependency_map[dependent_variable_alias] = dependencies;
  reverse_dependency_map[dependencies].push_back(dependent_variable_alias);
}

void DQDIMACSParser::doWriteQCIR(std::ostream& out) {
  printDQCIRPrefix(out);
  printDependencyBlocks(out);
  printQCIRGates(out);
}

void DQDIMACSParser::printDQCIRPrefix(std::ostream& out) {
  out << "#QCIR-G14" << std::endl; // Print "preamble".
  GateType last_block_type = GateType::None;
  bool first_variable_seen = false;
  for (unsigned i = 1; i < variable_gate_boundary; i++) {
    auto& gate = gates[i];
    if ((gate.gate_type == GateType::Existential && dependency_map.find(i) == dependency_map.end()) || gate.gate_type == GateType::Universal) {
      if (gate.gate_type != last_block_type) { 
        last_block_type = gate.gate_type;
        if (first_variable_seen) {
          out << ')' << std::endl; // Close last block unless this is the first variable.
        }
        auto& block_start_string = (gate.gate_type == GateType::Existential) ? QBFParser::EXISTS_STRING : QBFParser::FORALL_STRING;
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

void DQDIMACSParser::printDependencyBlocks(std::ostream& out) {
  for (unsigned alias = 1; alias < variable_gate_boundary; alias++) {
    auto& gate = gates[alias];
    if (gate.gate_type == GateType::Existential && dependency_map.find(alias) != dependency_map.end()) {
      out << DEPENDENCY_STRING << " " << gate.gate_id << " ";
      for (auto& dependency: dependency_map[alias]) {
        auto& gate_dependency = gates[dependency];
        out << gate_dependency.gate_id << " ";
      }
      out << "0" << std::endl;
    }
  }
}

void DQDIMACSParser::printQDIMACSPrefix(std::ostream& out) {
  GateType last_block_type = GateType::None;
  bool first_variable_seen = false;
  for (unsigned i = 1; i < variable_gate_boundary; i++) {
    auto& gate = gates[i];
    if (dependency_map.find(i) == dependency_map.end() && (gate.gate_type == GateType::Existential || gate.gate_type == GateType::Universal)) {
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
    // All AND-Gates are definitions.
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
  // Finally, print explicit dependencies.
  printDependencyBlocks(out);
}