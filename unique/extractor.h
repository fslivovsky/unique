#ifndef Extractor_HH
#define Extractor_HH

#include <vector>
#include <unordered_set>

#include "InterpolatingSolver.h"

using std::vector;
using std::unordered_set;

using namespace avy::abc;

class Extractor {
public:
  Extractor(int conflict_limit, bool use_same_type);
  ~Extractor();
  void interrupt();
  tuple<vector<int>, vector<tuple<vector<int>, int>>> getDefinitions(vector<vector<int>>& formula, vector<int>& query_variables, vector<int>& shared_variables, vector<bool>& query_mask, int max_variable_int);

protected:
  vector<int> copyClause(vector<int>& clause, unordered_set<int>& shared_variables_set, int offset);
  int miniSatLiteral(int literal);
  void makeMiniSatClause(vector<int>& clause);
  void makeMiniSatFormula(vector<vector<int>>& formula);
  void printFormula(vector<vector<int>>& formula);
  auto definitionsFromCircuit(Aig_Man_t* circuit, vector<int>& defined, vector<int>& input_variables);

  InterpolatingSolver* solver;
  bool signal_caught;
  int auxiliary_start;
  int conflict_limit;
  bool use_same_type;
  
};

#endif