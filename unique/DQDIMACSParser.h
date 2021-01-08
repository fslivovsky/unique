#ifndef DQDIMACS_Parser_h
#define DQDIMACS_Parser_h

#include "QDIMACSParser.h"

#include <string>
#include <unordered_map>
#include <vector>
#include <tuple>

using std::string;
using std::unordered_map;
using std::vector;
using std::tuple;

class intVectorHasher {
public:
  std::size_t operator()(vector<int> const& vec) const {
    std::size_t seed = vec.size();
    for(auto& i : vec) {
      seed ^= i + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }
    return seed;
  }
};

class DQDIMACSParser: virtual public QDIMACSParser {

public:
  DQDIMACSParser(const string& filename);

protected:
  DQDIMACSParser();
  virtual void doWriteQCIR(std::ostream& out);
  virtual void doGetDefinitions(Extractor& extractor);
  virtual void printQDIMACSPrefix(std::ostream& out);
  void readDependencyBlock(const string& line);
  void printDependencyBlocks(std::ostream& out);
  void printDQCIRPrefix(std::ostream& out);
  auto getExistentialQuerySets();
  auto getOrdinaryExistentialDefinitions(Extractor& extractor);
  auto getDependentExistentialDefinitions(Extractor& extractor);

  unordered_map<int,vector<int>> dependency_map;
  unordered_map<vector<int>, vector<int>, intVectorHasher> reverse_dependency_map;

  static const string DEPENDENCY_STRING;
};

#endif