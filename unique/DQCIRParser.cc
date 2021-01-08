#include "DQCIRParser.h"

DQCIRParser::DQCIRParser(const std::string& filename) {
  std::ifstream file(filename.c_str());
  string line;
  while (std::getline(file, line)) {
    auto line_with_withspaces = line; // Hack for dependency lines.
    line.erase(remove_if(line.begin(), line.end(), [] (char c) { return isspace(int(c)); }), line.end()); // Remove whitespaces.
    if (line.length() == 0 || line.front() == '#') {
      continue;
    } else if (QCIRParser::startsWith(line, QBFParser::FORALL_STRING) || startsWith(line, QBFParser::EXISTS_STRING)) {
      QCIRParser::readQuantifierBlock(line);
    } else if (QCIRParser::startsWith(line, QBFParser::OUTPUT_STRING)) {
      QCIRParser::readOutput(line);
    } else if (QCIRParser::startsWith(line, DQDIMACSParser::DEPENDENCY_STRING)) {
      readDependencyBlock(line_with_withspaces);
    } else {
      readGate(line);
    }
  }
  assert(output_id.size());
  assert(id_to_alias.find(output_id) != id_to_alias.end());
  std::cerr << "Done parsing " << gates.size() << " gates." << std::endl;

  /* Remove redundant gates (optional). */
  std::cerr << "Removed " << removeRedundant() << " redundant gates." << std::endl;
};

void DQCIRParser::doWriteQCIR(std::ostream& out) {
  DQDIMACSParser::doWriteQCIR(out);
}

void DQCIRParser::doGetDefinitions(Extractor& extractor) {
  DQDIMACSParser::doGetDefinitions(extractor);
}
