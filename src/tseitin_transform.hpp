#ifndef TSEITIN_TRANSFORM_HPP
#define TSEITIN_TRANSFORM_HPP

#include "sat_syntax_tree.hpp"
#include <vector>

namespace slang {

std::vector<std::vector<int>> to_cnf(SAT_Expression *expression);

void output_dimacs(const std::vector<std::vector<int>> &cnf, std::string filename);

} // namespace slang

#endif
