#include "cfg.hpp"

namespace slang {

void dump_expression(CFG *cfg, Expression *expression, i32 depth) {
  printf("      %*.s%s", depth, " ", ExpressionKind::to_string[expression->kind]);
  switch (expression->kind) {
  case ExpressionKind::False:
  case ExpressionKind::True: printf("\n"); break;
  case ExpressionKind::Not:
    printf(" (\n");
    dump_expression(cfg, expression->unary.inner, depth + 2);
    printf("      %*.s)\n", depth, " ");
    break;
  case ExpressionKind::And:
  case ExpressionKind::Or:
    printf(" (\n");
    dump_expression(cfg, expression->binary.left, depth + 2);
    dump_expression(cfg, expression->binary.right, depth + 2);
    printf("      %*.s)\n", depth, " ");
    break;
  default: assert(!"TODO: unimplemented cfg dump expression"); break;
  }
  (void)cfg;
}

void dump_inst(CFG *cfg, Instruction *inst) {
  printf("  %s\n", InstructionKind::to_string[inst->kind]);
  switch (inst->kind) {
  case InstructionKind::Assign:
    printf("    ");
    for (i32 i = 0; i < inst->assign.variable_name.length; ++i) {
      printf("%c", cfg->file_data[inst->assign.variable_name.index + i]);
    }
    printf(" = \n");
    dump_expression(cfg, inst->assign.right_value_expression, 0);
    break;
  default: break;
  }
}

void dump_block(CFG *cfg, BasicBlock *basic_block) {
  printf("BB:\n");
  for (auto &inst : basic_block->insts) {
    dump_inst(cfg, &inst);
  }
}

void dump_cfg(CFG *cfg) { dump_block(cfg, cfg->entry_bb); }

} // namespace slang
