#include "extractor.h"

#include <tuple>
#include <new>

using std::tuple;

Extractor::Extractor(int conflict_limit, bool use_same_type): solver(nullptr), signal_caught(false), auxiliary_start(0), conflict_limit(conflict_limit), use_same_type(use_same_type) {}

Extractor::~Extractor() {
  delete solver;
}

void Extractor::interrupt() {
  if (solver) {
    solver->interrupt();
  }
  signal_caught = true;
}

vector<int> Extractor::copyClause(vector<int>& clause, unordered_set<int>& shared_variables_set, int offset) {
  vector<int> clause_copy;
  for (auto literal: clause) {
    auto variable = abs(literal);
    assert(variable <= offset);
    if (shared_variables_set.find(variable) == shared_variables_set.end()) {
      variable += offset;
      int sign = (literal > 0) ? 1 : -1;
      clause_copy.push_back(sign * variable);
    } else {
      clause_copy.push_back(literal);
    }
  }
  return clause_copy;
}

int Extractor::miniSatLiteral(int literal) {
  return 2 * abs(literal) + (literal < 0);
}

void Extractor::makeMiniSatClause(vector<int>& clause) {
  for (auto& literal: clause) {
    literal = miniSatLiteral(literal);
  }
}

void Extractor::makeMiniSatFormula(vector<vector<int>>& formula) {
  for (auto& clause: formula) {
    makeMiniSatClause(clause);
  }
}

void Extractor::printFormula(vector<vector<int>>& formula) {
  for (auto& clause: formula) {
    for (auto literal: clause) {
      std::cerr << literal << " ";
    }
    std::cerr << std::endl;
  }
}

auto Extractor::definitionsFromCircuit(Aig_Man_t* circuit, vector<int>& defined, vector<int>& input_variables) {
  vector<tuple<vector<int>,int>> definitions;

  assert(circuit != nullptr);
  // Make sure number of inputs and outputs matches expectations
  assert(Aig_ManCiNum(circuit) == input_variables.size() || (Aig_ManCiNum(circuit) == 1 && input_variables.size() == 0));
  assert(Aig_ManCoNum(circuit) == defined.size());
  
  Aig_Obj_t *pObj, *pConst1 = nullptr;
  int i;

  // check if constant is used
  Aig_ManForEachCo(circuit, pObj, i)
    if (Aig_ObjIsConst1(Aig_ObjFanin0(pObj)))
      pConst1 = Aig_ManConst1(circuit);
  Aig_ManConst1(circuit)->iData = ++auxiliary_start;

  // collect nodes in the DFS order
  Vec_Ptr_t * vNodes;
  vNodes = Aig_ManDfs(circuit, 1);

  // assign new variable ids to
  Vec_PtrForEachEntry(Aig_Obj_t*, vNodes, pObj, i) {
    pObj->iData = ++auxiliary_start;
  }
  // ith input/output gets CioId i.
  Aig_ManSetCioIds(circuit);
  // Add definition for constant true.
  if (pConst1) {
    vector<int> empty;
    definitions.push_back(std::make_tuple(empty, pConst1->iData));
  }
  // Add definitions for internal nodes.
  Vec_PtrForEachEntry(Aig_Obj_t *, vNodes, pObj, i) {
    int first_input, second_input, output;
    if (Aig_ObjIsCi(Aig_ObjFanin0(pObj))) {
      first_input = input_variables[Aig_ObjCioId(Aig_ObjFanin0(pObj))];
    } else {
      first_input = Aig_ObjFanin0(pObj)->iData;
    }
    if (Aig_ObjIsCi(Aig_ObjFanin1(pObj))) {
      second_input = input_variables[Aig_ObjCioId(Aig_ObjFanin1(pObj))];
    } else {
      second_input = Aig_ObjFanin1(pObj)->iData;
    }
    output = pObj->iData;
    vector<int> input_literals;
    if (!Aig_ObjFaninC0(pObj)) {
      input_literals.push_back(first_input);
    } else {
      input_literals.push_back(-first_input);
    }
    if (!Aig_ObjFaninC1(pObj)) {
      input_literals.push_back(second_input);
    } else {
      input_literals.push_back(-second_input);
    }
    definitions.push_back(std::make_tuple(input_literals, output));
  }
  // Add definitions for output nodes.
  Aig_ManForEachCo(circuit, pObj, i) {
    int input, output;
    if (Aig_ObjIsCi(Aig_ObjFanin0(pObj))) {
      input = input_variables[Aig_ObjCioId(Aig_ObjFanin0(pObj))];
    } else {
      input = Aig_ObjFanin0(pObj)->iData;
    }
    output = defined[Aig_ObjCioId(pObj)];
    vector<int> input_literals;
    if (!Aig_ObjFaninC0(pObj)) {
      input_literals.push_back(input);
    } else {
      input_literals.push_back(-input);
    }
    definitions.push_back(std::make_tuple(input_literals, output));
  }
  Aig_ManCleanCioIds(circuit);
  Vec_PtrFree(vNodes);
  return definitions;
}

tuple<vector<int>, vector<tuple<vector<int>, int>>> Extractor::getDefinitions(vector<vector<int>>& formula, vector<int>& query_variables, vector<int>& shared_variables, vector<bool>& query_mask, int max_variable_int) {

  int nr_variables_to_check = 0;
  for (auto flag: query_mask) {
    nr_variables_to_check += flag;
  }

  // Immediately return if signal has already been received.
  if (nr_variables_to_check == 0 || signal_caught) {
    return std::make_tuple(vector<int>{}, vector<tuple<vector<int>,int>>{});
  }

  auxiliary_start = std::max(max_variable_int, auxiliary_start);

  unordered_set<int> shared_variables_set(shared_variables.begin(), shared_variables.end());
  vector<vector<int>> formula_copy;
  for (auto& clause: formula) {
    formula_copy.push_back(copyClause(clause, shared_variables_set, max_variable_int));
  }

  vector<int> defined;
  vector<tuple<vector<int>,int>> definitions;

  int next_selector_variable = 2 * max_variable_int + 1;

  makeMiniSatFormula(formula);
  makeMiniSatFormula(formula_copy);

  try {
    delete solver;
    solver = new InterpolatingSolver(2 * max_variable_int + 2 * nr_variables_to_check);
    solver->addFormula(formula, formula_copy);

    if (!solver->solve()) {
      std::cerr << "Matrix unsatisfiable." << std::endl;
      return std::make_tuple(defined, definitions);
    }

    next_selector_variable = 2 * max_variable_int + 1;

    int checked = 0;
    for (int i = 0; i < query_variables.size() && !signal_caught; i++) {
      auto variable = query_variables[i];
      bool is_defined = false;
      if (query_mask[i]) {
        auto selector_A = next_selector_variable++;
        auto selector_B = next_selector_variable++;
        vector<int> selector_clause_A = { -selector_A, variable };
        vector<int> selector_clause_B = { -selector_B, -(variable + max_variable_int) };
        makeMiniSatClause(selector_clause_A);
        makeMiniSatClause(selector_clause_B);
        solver->addClause(selector_clause_A, 1);
        solver->addClause(selector_clause_B, 2);
        vector<int> assumptions = { miniSatLiteral(selector_A), miniSatLiteral(selector_B) };
        if (!solver->getInterpolant(variable, assumptions, shared_variables, conflict_limit)) {
          defined.push_back(variable);
          is_defined = true;
        }
        std::cerr << ++checked << "/" << nr_variables_to_check << " checked. \r";
      }
      if (!query_mask[i] || use_same_type || is_defined) {
        vector<int> c1 = { variable, -(variable + max_variable_int) };
        vector<int> c2 = { -variable, (variable + max_variable_int) };
        makeMiniSatClause(c1);
        makeMiniSatClause(c2);
        solver->addClause(c1);
        solver->addClause(c2);
        formula_copy.push_back(c1);
        formula_copy.push_back(c2);
        shared_variables.push_back(variable);
      }
    }
    std::cerr << std::endl;
    auto circuit = solver->getCircuit(shared_variables, !signal_caught);
    if (circuit != nullptr) {
      definitions = definitionsFromCircuit(circuit, defined, shared_variables);
    }
  }
  catch (Minisat::OutOfMemoryException&) {
    std::cerr << "MiniSat out of memory." << std::endl;
  }
  return std::make_tuple(defined, definitions);
}