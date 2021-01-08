#include <fstream>
#include <sstream>

#include "VariableComparator.h"

VariableComparator::VariableComparator(const std::string& ordering_filename) {
  std::ifstream ifs;
  ifs.open(ordering_filename.c_str(), std::ifstream::in);
  std::string line, token;
  std::getline(ifs, line);
  std::istringstream line_stream(line);
  int position = 0;
  while (line_stream >> token) {
    ordering_index[token] = position++;
  }
  ifs.close();
}
