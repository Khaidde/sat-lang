#ifndef CFG_HPP
#define CFG_HPP

#include "general.hpp"
#include "sat_syntax_tree.hpp"
#include <vector>

namespace slang {

// clang-format off
#define EXPRESSION_KIND(pick) \
  pick(None,    "'None'"), \
  pick(False,   "False"), \
  pick(True,    "True"), \
  pick(LVar,    "LVar"), \
  pick(Not,     "Not"), \
  pick(And,     "And"), \
  pick(Or,      "Or"), \
  pick(GridRef, "GridRef"), \
  pick(Index,   "Index"),
DECLARE_KIND(EXPRESSION_KIND, ExpressionKind);

struct Expression;

struct UnaryExpression {
  Expression *inner;
};

struct BinaryExpression {
  Expression *left;
  Expression *right;
};

struct IndexExpression {
  i32 dimension_size;

  Expression *inner;

  bool is_constant;
  union {
    i32 indexvar;
    i32 constant_index;
  };
};

struct Expression {
  ExpressionKind::Enum kind;
  union {
    i32 lvar;
    UnaryExpression unary;
    BinaryExpression binary;
    i32 grid_start_variable;
    IndexExpression index;
  };
};

// clang-format off
#define INSTRUCTION_KIND(pick) \
  pick(Assign, "Assign"), \
  pick(Loop,   "Loop"),
DECLARE_KIND(INSTRUCTION_KIND, InstructionKind);
// clang-format on

struct BasicBlock;
struct Instruction;

struct AssignInstruction {
  i32 localvar;
  Expression *right_value_expression;
};

struct LoopInstruction {
  i32 indexvar;
  i32 length;
  BasicBlock *inner_bb;
};

struct Instruction {
  InstructionKind::Enum kind;
  union {
    AssignInstruction assign;
    LoopInstruction loop;
  };
};

// clang-format off
#define TERMINATOR_KIND(pick) \
  pick(None,   "None"), \
  pick(Goto,   "Goto"), \
  pick(Branch, "Branch"), \
  pick(Return, "Return"), \
  pick(End,    "End"),
DECLARE_KIND(TERMINATOR_KIND, TerminatorKind);
// clang-format on

struct GotoTerminator {
  BasicBlock *goto_bb;
};

struct BranchTerminator {
  Expression *condition_expression;

  BasicBlock *then_bb;
  BasicBlock *else_bb;
};

struct ReturnTerminator {
  Expression *return_expression;
};

struct BasicBlock {
  i32 id;
  std::vector<Instruction> insts;

  TerminatorKind::Enum terminator_kind;
  union {
    GotoTerminator go;
    BranchTerminator branch;
    ReturnTerminator ret;
  };
};

struct CFG {
  BasicBlock *entry_bb;
  char *file_data;
};

void dump_cfg(CFG *cfg);

SAT_Expression *generate_sat(CFG *cfg);

} // namespace slang

#endif
