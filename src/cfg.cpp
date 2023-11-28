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
  case ExpressionKind::LVar: printf("lv%d", expression->lvar); break;
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
  case ExpressionKind::GridRef: printf("g%d", expression->grid_start_variable); break;
  case ExpressionKind::Index:
    dump_expression(expression->index.inner);
    if (expression->index.is_constant) {
      printf("[%d]", expression->index.constant_index);
    } else {
      printf("[i%d]", expression->index.indexvar);
    }
    break;
  default: assert(!"TODO: unimplemented cfg dump expression straight"); break;
  }
}

struct LoopTarget {
  i32 block_id;
  i32 iterator_id;
};

void dump_cfg(CFG *cfg) {
  printf("digraph {\n");
  std::vector<BasicBlock *> visited;
  std::vector<BasicBlock *> worklist;
  worklist.push_back(cfg->entry_bb);
  while (worklist.size()) {
    auto *bb = worklist.back();
    worklist.pop_back();
    visited.push_back(bb);

    std::vector<LoopTarget> loop_targets;
    printf("  %d [shape=record,label=\"bb%d", bb->id, bb->id);
    for (auto &inst : bb->insts) {
      switch (inst.kind) {
      case InstructionKind::Assign:
        printf("\\nlv%d = ", inst.assign.localvar);
        dump_expression(inst.assign.right_value_expression);
        break;
      case InstructionKind::Loop:
        printf("\\nfor lv%d in %d", inst.loop.indexvar, inst.loop.length);
        worklist.push_back(inst.loop.inner_bb);
        loop_targets.push_back({inst.loop.inner_bb->id, inst.loop.indexvar});
        break;
      default: assert(!"Unreachable"); break;
      }
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
      printf("\"]\n");
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
    case TerminatorKind::End: printf("\"]\n"); break;
    default: printf("\"]\n"); break;
    }
    for (auto target : loop_targets) {
      printf("  %d->%d [color=red,label=\"lv%d\"]\n", bb->id, target.block_id, target.iterator_id);
    }
  }
  printf("}\n");
}

struct IndexVariable {
  i32 id;
  i32 value;
};

struct Scope {
  Scope *parent;
  std::vector<AssignInstruction> local_variable_context;
  std::vector<IndexVariable> index_variable_context;
};

SAT_Expression literal_1{1};
SAT_Expression not_literal_1{Operator::NOT, nullptr, &literal_1};
SAT_Expression false_sat{Operator::AND, &literal_1, &not_literal_1};
SAT_Expression true_sat{Operator::OR, &literal_1, &not_literal_1};

SAT_Expression *new_not(SAT_Expression *inner) {
  if (inner == &false_sat) return &true_sat;
  if (inner == &true_sat) return &false_sat;
  return new SAT_Expression(Operator::NOT, nullptr, inner);
}

SAT_Expression *new_and(SAT_Expression *left, SAT_Expression *right) {
  if (left == &false_sat || right == &false_sat) return &false_sat;
  if (left == &true_sat) return right;
  if (right == &true_sat) return left;
  if (left == right) return left;
  if (*left == *right) return left;
  return new SAT_Expression(Operator::AND, left, right);
}

SAT_Expression *new_or(SAT_Expression *left, SAT_Expression *right) {
  if (left == &true_sat || right == &true_sat) return &true_sat;
  if (left == &false_sat) return right;
  if (right == &false_sat) return left;
  if (left == right) return left;
  return new SAT_Expression(Operator::OR, left, right);
}

Expression *find_local_variable_value(Scope *scope, i32 variable_id) {
  if (!scope) return nullptr;
  for (auto lvar : scope->local_variable_context) {
    if (lvar.localvar == variable_id) return lvar.right_value_expression;
  }
  return find_local_variable_value(scope->parent, variable_id);
}

i32 find_index_variable_value(Scope *scope, i32 variable_id, bool increment) {
  if (!scope) return -1;
  for (i32 i = 0; i < (i32)scope->index_variable_context.size(); ++i) {
    auto ivar = scope->index_variable_context[(u32)i];
    if (ivar.id == variable_id) {
      i32 old_value = ivar.value;
      if (increment) scope->index_variable_context[(u32)i].value++;
      return old_value;
    }
  }
  return find_index_variable_value(scope->parent, variable_id, increment);
}

i32 get_xvariable_from_index_expression(Scope *scope, i32 accumulator, Expression *index_expression) {
  switch (index_expression->kind) {
  case ExpressionKind::GridRef: return accumulator + index_expression->grid_start_variable;
  case ExpressionKind::Index:
    if (index_expression->index.is_constant) {
      accumulator += index_expression->index.constant_index * index_expression->index.dimension_size;
    } else {
      i32 index_value = find_index_variable_value(scope, index_expression->index.indexvar, false);
      accumulator += index_value * index_expression->index.dimension_size;
    }
    return get_xvariable_from_index_expression(scope, accumulator, index_expression->index.inner);
  default: assert(!"cannot get xvariable from expression kind"); return -1;
  }
}

SAT_Expression *translate_expression_to_sat(Scope *scope, Expression *expression) {
  switch (expression->kind) {
  case ExpressionKind::False: return &false_sat;
  case ExpressionKind::True: return &true_sat;
  case ExpressionKind::LVar:
    return translate_expression_to_sat(scope, find_local_variable_value(scope, expression->lvar));
  case ExpressionKind::Not: return new_not(translate_expression_to_sat(scope, expression->unary.inner));
  case ExpressionKind::And:
    return new_and(translate_expression_to_sat(scope, expression->binary.left),
                   translate_expression_to_sat(scope, expression->binary.right));
  case ExpressionKind::Or:
    return new_or(translate_expression_to_sat(scope, expression->binary.left),
                  translate_expression_to_sat(scope, expression->binary.right));
  case ExpressionKind::GridRef: assert(!"cannot translate gridref directly"); break;
  case ExpressionKind::Index:
    // increase by 1 so that variable 0 is never created
    return new SAT_Expression(get_xvariable_from_index_expression(scope, 0, expression) + 1);
  default: assert(!"TODO: unimplemented translation of expression to sat"); return nullptr;
  }
}

SAT_Expression *translate_block_to_sat(Scope *parent_scope, BasicBlock *bb) {
  Scope scope;
  scope.parent = parent_scope;

  SAT_Expression *statement_result = &true_sat;

  for (auto &inst : bb->insts) {
    switch (inst.kind) {
    case InstructionKind::Assign: scope.local_variable_context.push_back(inst.assign); break;
    case InstructionKind::Loop: {
      scope.index_variable_context.push_back({inst.loop.indexvar, 0});
      SAT_Expression *loop = translate_block_to_sat(&scope, inst.loop.inner_bb);
      for (i32 i = 1; i < inst.loop.length; ++i) {
        find_index_variable_value(&scope, inst.loop.indexvar, true);
        loop = new_or(loop, translate_block_to_sat(&scope, inst.loop.inner_bb));
      }
      statement_result = new_and(statement_result, loop);
      break;
    }
    default: assert(!"TODO: unimplemented translation of block statement"); break;
    }
  }

  SAT_Expression *terminator_result;
  switch (bb->terminator_kind) {
  case TerminatorKind::Goto: terminator_result = translate_block_to_sat(&scope, bb->go.goto_bb); break;
  case TerminatorKind::Branch: {
    SAT_Expression *cond     = translate_expression_to_sat(&scope, bb->branch.condition_expression);
    SAT_Expression *not_cond = new_not(cond);
    SAT_Expression *then_sat = new_and(cond, translate_block_to_sat(&scope, bb->branch.then_bb));
    SAT_Expression *else_sat = new_and(not_cond, translate_block_to_sat(&scope, bb->branch.else_bb));
    terminator_result        = new_or(then_sat, else_sat);
    break;
  }
  case TerminatorKind::Return:
    terminator_result = translate_expression_to_sat(&scope, bb->ret.return_expression);
    break;
  case TerminatorKind::End: terminator_result = &true_sat; break;
  default: assert(!"Unreachable"); break;
  }

  return new_and(statement_result, terminator_result);
}

SAT_Expression *generate_sat(CFG *cfg) { return translate_block_to_sat(nullptr, cfg->entry_bb); }

} // namespace slang
