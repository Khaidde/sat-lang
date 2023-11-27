#ifndef CFG_HPP
#define CFG_HPP

#include "general.hpp"
#include <vector>

namespace slang {

// clang-format off
#define EXPRESSION_KIND(pick) \
  pick(None,  "'None'"), \
  pick(False, "False"), \
  pick(True,  "True"), \
  pick(XVar,  "XVar"), \
  pick(Not,   "Not"), \
  pick(And,   "And"), \
  pick(Or,    "Or"),
DECLARE_KIND(EXPRESSION_KIND, ExpressionKind);

struct Expression;

struct UnaryExpression {
  Expression *inner;
};

struct BinaryExpression {
  Expression *left;
  Expression *right;
};

struct Expression {
  ExpressionKind::Enum kind;
  union {
    i32 xvar;
    UnaryExpression unary;
    BinaryExpression binary;
  };
};

// clang-format off
#define INSTRUCTION_KIND(pick) \
  pick(Assign, "Assign"),
DECLARE_KIND(INSTRUCTION_KIND, InstructionKind);
// clang-format on

struct Instruction;

struct AssignInstruction {
  Span variable_name;
  Expression *right_value_expression;
};

struct Instruction {
  InstructionKind::Enum kind;
  union {
    AssignInstruction assign;
  };
};

// clang-format off
#define TERMINATOR_KIND(pick) \
  pick(None,   "None"), \
  pick(Goto,   "Goto"), \
  pick(Branch, "Branch"), \
  pick(Return, "Return"),
DECLARE_KIND(TERMINATOR_KIND, TerminatorKind);
// clang-format on

struct BasicBlock;

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

} // namespace slang

#endif
