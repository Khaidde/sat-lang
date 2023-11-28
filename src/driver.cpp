#include "general.hpp"

#include "cfg.hpp"
#include "parser.hpp"

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

  return ok;
}
