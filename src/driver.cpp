#include "general.hpp"

#include "cfg.hpp"
#include "parser.hpp"
#include "tseitin_transform.hpp"

int main(int argc, char **argv) {
  if (argc != 2) {
    error("expected argument for file name");
    return err;
  }

  slang::CFG cfg;
  if (slang::parse_to_cfg(&cfg, argv[1])) return err;

  slang::dump_cfg(&cfg);

  auto *sat_expression = slang::generate_sat(&cfg);
  sat_expression->display();
  std::cout << std::endl;

  SAT_Expression *expression3 = new SAT_Expression(Operator::AND, new SAT_Expression(3), new SAT_Expression(1));
  SAT_Expression expression_combined3(Operator::AND, expression3, new SAT_Expression(2));

  std::vector<std::vector<int>> clauses = slang::to_cnf(sat_expression);
  slang::output_dimacs(clauses, "output.dimacs");

  std::cout << "CNF:\n";
  for (const auto &row : clauses) {
    // Print each element of the row with spaces
    for (const auto &element : row) {
      std::cout << element << " ";
    }
    // Print a newline after each row
    std::cout << std::endl;
  }

  return ok;
}
