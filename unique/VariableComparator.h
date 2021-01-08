#ifndef Ordering_HH
#define Ordering_HH

#include <string>
#include <unordered_map>
#include <tuple>

class VariableComparator {
public:
  VariableComparator(const std::string& ordering_filename);

  bool operator ()(const std::tuple<int,std::string,bool>& first_tuple, const std::tuple<int,std::string,bool>& second_tuple) {
    const auto& [x, x_name, first_mask] = first_tuple;
    const auto& [y, y_name, second_mask] = second_tuple;
    if (ordering_index.find(x_name) != ordering_index.end() && ordering_index.find(y_name) != ordering_index.end()) {
      // If both variable occur in the specified ordering, stick to that ordering.
      return ordering_index[x_name] < ordering_index[y_name];
    } else if (ordering_index.find(x_name) != ordering_index.end() && ordering_index.find(y_name) == ordering_index.end()) {
      // If x is in the ordering but y is not, order x first.
      return true;
    } else if (ordering_index.find(x_name) != ordering_index.end() && ordering_index.find(y_name) == ordering_index.end()) {
      // Conversely, if y is in the ordering but x is not, order y first.
      return false;
    } else {
      // Otherwise, order according to alias.
      return x < y;
    }
  };
protected:
  std::unordered_map<std::string, int> ordering_index;
};

#endif