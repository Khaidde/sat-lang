#include "cfg.hpp"

#include <vector>

namespace slang {

bool has_visisted(BasicBlock *bb, std::vector<BasicBlock *> *visited) {
  for (u32 i = 0; i < visited->size(); ++i) {
    if (visited->at(i) == bb) return true;
  }
  return false;
}

void dump_expression(Expression *expression) {
  switch (expression->kind) {
  case ExpressionKind::False: printf("false"); break;
  case ExpressionKind::True: printf("true"); break;
  case ExpressionKind::XVar: printf("x%d", expression->xvar); break;
  case ExpressionKind::Not:
    printf("!");
    dump_expression(expression->unary.inner);
    break;
  case ExpressionKind::And:
  case ExpressionKind::Or:
    printf("(");
    dump_expression(expression->binary.left);
    printf(" %c ", expression->kind == ExpressionKind::And ? '^' : 'v');
    dump_expression(expression->binary.right);
    printf(")");
    break;
  default: assert(!"TODO: unimplemented cfg dump expression straight"); break;
  }
}

void dump_cfg(CFG *cfg) {
  printf("digraph {\n");
  std::vector<BasicBlock *> visited;
  std::vector<BasicBlock *> worklist;
  worklist.push_back(cfg->entry_bb);
  while (worklist.size()) {
    auto *bb = worklist.back();
    worklist.pop_back();
    visited.push_back(bb);

    printf("  %d [shape=record,label=\"bb%d", bb->id, bb->id);
    for (auto &inst : bb->insts) {
      assert(inst.kind == InstructionKind::Assign);
      printf("\\n");
      for (i32 i = 0; i < inst.assign.variable_name.length; ++i) {
        printf("%c", cfg->file_data[inst.assign.variable_name.index + i]);
      }
      printf(" = ");
      dump_expression(inst.assign.right_value_expression);
    }
    switch (bb->terminator_kind) {
    case TerminatorKind::Goto:
      printf("\"]\n");
      if (!has_visisted(bb->go.goto_bb, &visited)) worklist.push_back(bb->go.goto_bb);
      printf("  %d->%d\n", bb->id, bb->go.goto_bb->id);
      break;
    case TerminatorKind::Branch:
      printf("\\nbr ");
      dump_expression(bb->branch.condition_expression);
      printf("}\"]\n");
      if (!has_visisted(bb->branch.then_bb, &visited)) worklist.push_back(bb->branch.then_bb);
      printf("  %d->%d [label=\"1\"]\n", bb->id, bb->branch.then_bb->id);
      if (!has_visisted(bb->branch.else_bb, &visited)) worklist.push_back(bb->branch.else_bb);
      printf("  %d->%d\n", bb->id, bb->branch.else_bb->id);
      break;
    case TerminatorKind::Return:
      printf("\\nreturn ");
      dump_expression(bb->ret.return_expression);
      printf("\"]\n");
      break;
    default: assert(!"Unreachable"); break;
    }
  }
  printf("}\n");
}

} // namespace slang
