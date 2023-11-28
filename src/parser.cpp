#include "parser.hpp"
#include "cfg.hpp"

#include <string>
#include <unordered_map>

namespace slang {

// clang-format off
#define TOKEN_KIND(pick) \
  pick(Ident,    "'identifier'"), \
  pick(Dot,      "."), \
  pick(Assign,   "="), \
  pick(Not,      "!"), \
  pick(And,      "&&"), \
  pick(Or,       "||"), \
  pick(LCurl,    "{"), \
  pick(RCurl,    "}"), \
  pick(LParen,   "("), \
  pick(RParen,   ")"), \
  pick(LSquare,   "["), \
  pick(RSquare,   "]"), \
  pick(Intlit,   "'intlit'"), \
  pick(False,    "false"), \
  pick(True,     "true"), \
  pick(Grid,     "grid"), \
  pick(Property, "property"), \
  pick(Function, "function"), \
  pick(If,       "if"), \
  pick(Else,     "else"), \
  pick(For,      "for"), \
  pick(In,       "in"), \
  pick(Return,   "return"), \
  pick(Err,      "'error'"), \
  pick(Eof,      "'end of file'"),
DECLARE_KIND(TOKEN_KIND, TokenKind);
// clang-format on

struct Token {
  TokenKind::Enum kind;
  i32 line;
  union {
    Span value;
    i32 intlit;
  };
};

struct Property {
  std::vector<Span> values;
};

struct Grid {
  std::vector<i32> dimensions;
  i32 variable_start_index;
};

struct Parser {
  std::unordered_map<std::string, i32> property_map;
  std::vector<Property> properties;

  i32 variable_count;
  std::unordered_map<std::string, Grid> grids;

  i32 local_variable_count;
  std::unordered_map<std::string, i32> local_variable_map;

  i32 index_variable_count;
  std::unordered_map<std::string, i32> index_variable_map;

  i32 block_count;

  i32 index;
  i32 tlength;

  i32 line;

  Token token;

  char *data;
  i32 file_length;
};

bool is_span_equal(Parser *p, Span span1, Span span2) {
  if (span1.length != span2.length) return false;
  for (i32 i = 0; i < span1.length; ++i) {
    if (p->data[span1.index + i] != p->data[span2.index + i]) return false;
  }
  return true;
}

std::string span_to_string(Parser *p, Span span) { return std::string(&p->data[span.index], (u32)span.length); }

bool is_whitespace(char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; }

bool is_letter(char c) { return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z'); }

bool is_letter_or_underscore(char c) { return is_letter(c) || c == '_'; }

bool is_number(char c) { return '0' <= c && c <= '9'; }

bool is_number_or_underscore(char c) { return is_number(c) || c == '_'; }

bool is_eof(Parser *p) { return p->index + p->tlength >= p->file_length; }

void end_token(Parser *p) {
  p->index += p->tlength;
  p->tlength = 0;
}

Token *create_token(Parser *p, TokenKind::Enum kind) {
  p->token.kind         = kind;
  p->token.line         = p->line;
  p->token.value.index  = p->index;
  p->token.value.length = p->tlength;
  end_token(p);
  return &p->token;
}

char peek_char(Parser *p) {
  if (is_eof(p)) return 0;
  return p->data[p->index + p->tlength];
}

// clang-format off
TokenKind::Enum keywords[] {
  TokenKind::False,
  TokenKind::True,
  TokenKind::Grid,
  TokenKind::Property,
  TokenKind::Function,
  TokenKind::If,
  TokenKind::Else,
  TokenKind::For,
  TokenKind::In,
  TokenKind::Return,
};
const i32 num_keywords = sizeof(keywords) / sizeof(TokenKind::Enum);
// clang-format on

Token *next_keyword_or_identifier(Parser *p) {
  while (char ch = peek_char(p)) {
    if (!is_letter_or_underscore(ch) && !is_number(ch)) break;
    ++p->tlength;
  }

  for (i32 i = 0; i < num_keywords; ++i) {
    cstr ptr = TokenKind::to_string[keywords[i]];

    i32 k = 0;
    for (; k < p->tlength; ++k) {
      if (*ptr != p->data[p->index + k]) break;
      ++ptr;
    }

    if (*ptr == '\0' && k == p->tlength) return create_token(p, keywords[i]);
  }
  return create_token(p, TokenKind::Ident);
}

Token *next_integer(Parser *p) {
  u64 val = 0;
  while (char c = peek_char(p)) {
    if (!c || !is_number_or_underscore(c)) break;
    ++p->tlength;
    if (c != '_') val = val * 10 + (u8)(c - '0');
  }
  auto *token   = create_token(p, TokenKind::Intlit);
  token->intlit = (i32)val;
  return token;
}

Token *next(Parser *p) {
  while (is_whitespace(peek_char(p))) {
    if (peek_char(p) == '\n') p->line++;
    p->index++;
  }

  switch (char c = peek_char(p)) {
  case '.': ++p->tlength; return create_token(p, TokenKind::Dot);
  case '=': ++p->tlength; return create_token(p, TokenKind::Assign);
  case '!': ++p->tlength; return create_token(p, TokenKind::Not);
  case '&':
    ++p->tlength;
    if (peek_char(p) == '&') {
      ++p->tlength;
      return create_token(p, TokenKind::And);
    } else {
      error("line %d: expected && instead of &\n", p->line, c);
      create_token(p, TokenKind::Err);
      return nullptr;
    }
  case '|':
    ++p->tlength;
    if (peek_char(p) == '|') {
      ++p->tlength;
      return create_token(p, TokenKind::Or);
    } else {
      error("line %d: expected || instead of |\n", p->line, c);
      create_token(p, TokenKind::Err);
      return nullptr;
    }
  case '{': ++p->tlength; return create_token(p, TokenKind::LCurl);
  case '}': ++p->tlength; return create_token(p, TokenKind::RCurl);
  case '(': ++p->tlength; return create_token(p, TokenKind::LParen);
  case ')': ++p->tlength; return create_token(p, TokenKind::RParen);
  case '[': ++p->tlength; return create_token(p, TokenKind::LSquare);
  case ']': ++p->tlength; return create_token(p, TokenKind::RSquare);
  case '\0':
    p->token.kind         = TokenKind::Eof;
    p->token.value.index  = p->index;
    p->token.value.length = 1;
    return &p->token;
  default:
    if (is_letter_or_underscore(c)) return next_keyword_or_identifier(p);
    if (is_number(c)) return next_integer(p);

    error("line %d: unknown character %c\n", p->line, c);
    create_token(p, TokenKind::Err);
    return nullptr;
  }

  return nullptr;
}

Token *peek(Parser *p) { return &p->token; }

bool check_peek(Parser *p, TokenKind::Enum kind) { return peek(p)->kind == kind; }

bool is_span_equal_cstr(char *file_data, Span span, cstr str) {
  for (i32 i = 0; i < span.length; ++i) {
    if (*str == '\0') return false;
    if (file_data[span.index + i] != *str) return false;
    ++str;
  }
  return *str == '\0';
}

Expression *parse_expression(Parser *p);

Expression *parse_operand(Parser *p) {
  switch (peek(p)->kind) {
  case TokenKind::False: {
    if (!next(p)) return nullptr; // next false

    Expression *false_expression = new Expression();
    false_expression->kind       = ExpressionKind::False;
    return false_expression;
  }
  case TokenKind::True: {
    if (!next(p)) return nullptr; // next true

    Expression *true_expression = new Expression();
    true_expression->kind       = ExpressionKind::True;
    return true_expression;
  }
  case TokenKind::Not: {
    if (!next(p)) return nullptr; // next !

    Expression *not_expression  = new Expression();
    not_expression->kind        = ExpressionKind::Not;
    not_expression->unary.inner = parse_operand(p);
    if (!not_expression->unary.inner) return nullptr;
    return not_expression;
  }
  case TokenKind::Ident: {
    std::string name_string = span_to_string(p, peek(p)->value);
    if (!next(p)) return nullptr; // next 'ident'

    if (peek(p)->kind == TokenKind::LSquare) {
      if (p->grids.find(name_string) == p->grids.end()) {
        error("line %d: unknown grid with name %s\n", p->line, name_string.c_str());
        return nullptr;
      }
      Grid *grid_ptr = &p->grids[name_string];

      i32 expected_dimensions = (i32)grid_ptr->dimensions.size();

      Expression *grid_ref          = new Expression();
      grid_ref->kind                = ExpressionKind::GridRef;
      grid_ref->grid_start_variable = grid_ptr->variable_start_index;

      Expression *result = grid_ref;

      i32 accumulated_dimension_size = 1;
      i32 dimension_index            = 0;
      for (;;) {
        if (check_peek(p, TokenKind::Err)) return nullptr;
        if (!check_peek(p, TokenKind::LSquare)) break;
        if (!next(p)) return nullptr; // next [

        if (dimension_index >= expected_dimensions) {
          error("line %d: indexing with more dimensions than the expected of %d\n", p->line, expected_dimensions);
          return nullptr;
        }

        Expression *index_expression           = new Expression();
        index_expression->kind                 = ExpressionKind::Index;
        index_expression->index.dimension_size = accumulated_dimension_size;
        index_expression->index.inner          = result;
        result                                 = index_expression;

        switch (peek(p)->kind) {
        case TokenKind::Intlit:
          index_expression->index.constant_index = peek(p)->intlit;
          index_expression->index.is_constant    = true;
          if (!next(p)) return nullptr; // next 'intlit'
          break;
        case TokenKind::Ident: {
          Span index_name               = peek(p)->value;
          std::string index_name_string = span_to_string(p, index_name);
          if (!next(p)) return nullptr; // next 'intlit'

          if (check_peek(p, TokenKind::Dot)) {
            if (!next(p)) return nullptr; // next .

            auto it = p->property_map.find(index_name_string);
            if (it == p->property_map.end()) {
              error("line %d: could not find property %s\n", p->line, index_name_string.c_str());
              return nullptr;
            }

            Property *property = &p->properties[(u32)it->second];

            if (!check_peek(p, TokenKind::Ident)) {
              error("line %d: expected property value name after .", p->line);
              return nullptr;
            }
            Span value_name               = peek(p)->value;
            std::string value_name_string = span_to_string(p, value_name);
            if (!next(p)) return nullptr; // next 'ident'

            for (i32 i = 0; i < (i32)property->values.size(); ++i) {
              if (is_span_equal(p, value_name, property->values[(u32)i])) {
                index_expression->index.constant_index = i;
                break;
              }
            }
            if (index_expression->index.constant_index == -1) {
              error("line %d: could not find value %s in property %s \n", p->line, value_name_string.c_str(),
                    index_name_string.c_str());
              return nullptr;
            }
            index_expression->index.is_constant = true;
          } else {

            auto it = p->index_variable_map.find(index_name_string);
            if (it == p->index_variable_map.end()) {
              error("line %d: could not find index variable %s\n", p->line, index_name_string.c_str());
              return nullptr;
            }

            index_expression->index.indexvar    = it->second;
            index_expression->index.is_constant = false;
          }
          break;
        }
        default: error("line %d: expected integer literal or property value for grid index\n", p->line); return nullptr;
        }

        i32 dimension_size = grid_ptr->dimensions[(u32)dimension_index];
        if (index_expression->index.is_constant && index_expression->index.constant_index >= dimension_size) {
          error("line %d: access of %d out of bounds of dimension size %d\n", p->line,
                index_expression->index.constant_index, dimension_size);
          return nullptr;
        }
        accumulated_dimension_size *= dimension_size;

        if (!check_peek(p, TokenKind::RSquare)) {
          error("line %d: expected ] for grid index\n", p->line);
          return nullptr;
        }
        if (!next(p)) return nullptr; // next ]

        ++dimension_index;
      }

      if (dimension_index != expected_dimensions) {
        error("line %d: expected %d accesses into grid but found %d\n", p->line, expected_dimensions, dimension_index);
        return nullptr;
      }

      return result;
    } else {
      Expression *lvar_expression = new Expression();
      lvar_expression->kind       = ExpressionKind::LVar;

      auto it = p->local_variable_map.find(name_string);
      if (it == p->local_variable_map.end()) {
        error("line %d: could not find local variable definition for %s\n", p->line, name_string.c_str());
        return nullptr;
      }
      lvar_expression->lvar = it->second;
      return lvar_expression;
    }
    break;
  }
  default:
    error("line %d: unexpected expression operand parsing of %s\n", p->line, TokenKind::to_string[peek(p)->kind]);
    return nullptr;
  }
}

Expression *parse_operator(Parser *p, Expression *left_expression, i32 left_precedence) {
  for (;;) {
    i32 right_precedence                          = 0;
    ExpressionKind::Enum operator_expression_kind = ExpressionKind::None;
    switch (peek(p)->kind) {
    case TokenKind::And:
      operator_expression_kind = ExpressionKind::And;
      right_precedence         = 1;
      break;
    case TokenKind::Or:
      operator_expression_kind = ExpressionKind::Or;
      right_precedence         = 1;
      break;
    default: break;
    }

    if (left_precedence >= right_precedence) break;

    assert(operator_expression_kind != ExpressionKind::None);

    switch (peek(p)->kind) {
    case TokenKind::And:
    case TokenKind::Or: {
      Expression *binary_expression  = new Expression();
      binary_expression->kind        = operator_expression_kind;
      binary_expression->binary.left = left_expression;
      if (!binary_expression->binary.left) return nullptr;

      if (!next(p)) return nullptr; // next 'binary operator'

      binary_expression->binary.right = parse_operator(p, parse_operand(p), right_precedence);
      if (!binary_expression->binary.right) return nullptr;

      left_expression = binary_expression;

      break;
    }
    default: assert(!"Unexpected operator parsing"); break;
    }
  }
  return left_expression;
}

Expression *parse_expression(Parser *p) { return parse_operator(p, parse_operand(p), 0); }

BasicBlock *parse_block(Parser *p, BasicBlock *entry_bb);

BasicBlock *new_block(Parser *p) {
  BasicBlock *bb      = new BasicBlock();
  bb->id              = p->block_count++;
  bb->terminator_kind = TerminatorKind::None;
  return bb;
}

BasicBlock *parse_if(Parser *p, BasicBlock *entry_bb) {
  assert(check_peek(p, TokenKind::If));
  if (!next(p)) return nullptr; // next if

  entry_bb->terminator_kind = TerminatorKind::Branch;

  BasicBlock *exit_bb = new_block(p);

  entry_bb->branch.condition_expression = parse_expression(p);
  if (!entry_bb->branch.condition_expression) return nullptr;

  entry_bb->branch.then_bb = new_block(p);
  BasicBlock *then_exit_bb = parse_block(p, entry_bb->branch.then_bb);
  if (!then_exit_bb) return nullptr;
  if (then_exit_bb->terminator_kind != TerminatorKind::Return) {
    then_exit_bb->terminator_kind = TerminatorKind::Goto;
    then_exit_bb->go.goto_bb      = exit_bb;
  }

  if (check_peek(p, TokenKind::Else)) {
    if (!next(p)) return nullptr; // next else

    entry_bb->branch.else_bb = new_block(p);
    BasicBlock *else_exit_bb = parse_block(p, entry_bb->branch.else_bb);
    if (!else_exit_bb) return nullptr;
    if (else_exit_bb->terminator_kind != TerminatorKind::Return) {
      else_exit_bb->terminator_kind = TerminatorKind::Goto;
      else_exit_bb->go.goto_bb      = exit_bb;
    }
  } else {
    entry_bb->branch.else_bb = exit_bb;
  }

  return exit_bb;
}

Result parse_for(Parser *p, Instruction *instruction) {
  assert(check_peek(p, TokenKind::For));
  if (!next(p)) return err; // next for

  instruction->kind = InstructionKind::Loop;

  if (!check_peek(p, TokenKind::Ident)) {
    error("line %d: expected name for iterator variable\n", p->line);
    return err;
  }
  Span iterator_name               = peek(p)->value;
  std::string iterator_name_string = span_to_string(p, iterator_name);
  if (!next(p)) return err; // next 'ident'
  auto it = p->index_variable_map.find(iterator_name_string);
  if (it == p->index_variable_map.end()) {
    instruction->loop.indexvar = p->index_variable_count;
    p->index_variable_map.insert(std::make_pair(iterator_name_string, p->index_variable_count++));
  } else {
    instruction->loop.indexvar = it->second;
  }

  if (!check_peek(p, TokenKind::In)) {
    error("line %d: expected 'in' keyword after for loop index variable\n", p->line);
    return err;
  }
  if (!next(p)) return err; // next in

  if (!check_peek(p, TokenKind::Intlit)) {
    error("line %d: expected number for for loop range\n", p->line);
    return err;
  }
  instruction->loop.length = peek(p)->intlit;
  if (!next(p)) return err; // next 'intlit'

  instruction->loop.inner_bb = new_block(p);
  BasicBlock *exit_bb        = parse_block(p, instruction->loop.inner_bb);
  if (!exit_bb) return err;

  if (exit_bb->terminator_kind == TerminatorKind::None) {
    exit_bb->terminator_kind = TerminatorKind::End;
  }
  return ok;
}

BasicBlock *parse_block(Parser *p, BasicBlock *entry_bb) {
  assert(check_peek(p, TokenKind::LCurl));
  if (!next(p)) return nullptr; // next {

  BasicBlock *current_bb = entry_bb;

  for (;;) {
    if (current_bb->terminator_kind == TerminatorKind::Return) {
      error("line %d: statement cannot exist after return\n", p->line);
      return nullptr;
    }
    switch (peek(p)->kind) {
    case TokenKind::Ident: {
      Instruction assignment_inst;
      assignment_inst.kind = InstructionKind::Assign;

      Span local_variable_name = peek(p)->value;
      if (!next(p)) return nullptr; // next 'ident'

      std::string local_variable_name_string = span_to_string(p, local_variable_name);
      auto it                                = p->local_variable_map.find(local_variable_name_string);
      if (it == p->local_variable_map.end()) {
        assignment_inst.assign.localvar = p->local_variable_count;
        p->local_variable_map.insert(std::make_pair(local_variable_name_string, p->local_variable_count++));
      } else {
        assignment_inst.assign.localvar = it->second;
      }

      if (!check_peek(p, TokenKind::Assign)) {
        cstr unexpected_kind = TokenKind::to_string[peek(p)->kind];
        error("line %d: expected '=' after identifier but found %s instead\n", p->line, unexpected_kind);
        return nullptr;
      }
      if (!next(p)) return nullptr; // next =

      assignment_inst.assign.right_value_expression = parse_expression(p);
      if (!assignment_inst.assign.right_value_expression) return nullptr;

      current_bb->insts.push_back(assignment_inst);
      break;
    }
    case TokenKind::If: {
      BasicBlock *if_exit_bb = parse_if(p, current_bb);
      if (!if_exit_bb) return nullptr;
      current_bb = if_exit_bb;
      break;
    }
    case TokenKind::For:
      current_bb->insts.push_back({});
      if (parse_for(p, &current_bb->insts.back())) return nullptr;
      break;
    case TokenKind::Return:
      if (!next(p)) return nullptr; // next return

      current_bb->terminator_kind       = TerminatorKind::Return;
      current_bb->ret.return_expression = parse_expression(p);
      if (!current_bb->ret.return_expression) return nullptr;
      break;
    case TokenKind::RCurl: break;
    default: error("line %d: unimplemented statement parse", p->line); return nullptr;
    }
    if (peek(p)->kind == TokenKind::RCurl) break;
  }

  assert(check_peek(p, TokenKind::RCurl));
  if (!next(p)) return nullptr; // next }

  return current_bb;
}

BasicBlock *parse_function(Parser *p) {
  assert(check_peek(p, TokenKind::Function));
  if (!next(p)) return nullptr; // next function

  auto *function_name = peek(p);
  if (!is_span_equal_cstr(p->data, function_name->value, "is_sat")) {
    error("expected function name to be 'is_sat'\n");
    return nullptr;
  }
  if (!next(p)) return nullptr; // next 'function name'

  if (!check_peek(p, TokenKind::LCurl)) {
    error("expected { to define function body\n");
    return nullptr;
  }

  BasicBlock *entry_bb = new_block(p);
  BasicBlock *exit_bb  = parse_block(p, entry_bb);
  if (!exit_bb) return nullptr;
  if (exit_bb->terminator_kind != TerminatorKind::Return) {
    error("expected function return at end as safeguard\n");
    return nullptr;
  }
  return entry_bb;
}

BasicBlock *parse_file(Parser *p) {
  if (!next(p)) return nullptr; // consume the first token

  BasicBlock *entry_bb = nullptr;
  for (;;) {
    auto *token = peek(p);
    if (token->kind == TokenKind::Err || token->kind == TokenKind::Eof) break;

    switch (token->kind) {
    case TokenKind::Function:
      if (entry_bb) {
        error("line %d: expected one function but found another here\n", p->line);
        return nullptr;
      }
      entry_bb = parse_function(p);
      if (!entry_bb) return nullptr;
      break;
    case TokenKind::Property: {
      if (!next(p)) return nullptr; // next property

      if (!check_peek(p, TokenKind::Ident)) {
        error("line %d: expected name for property\n", p->line);
        return nullptr;
      }
      std::string name_string = span_to_string(p, peek(p)->value);
      if (!next(p)) return nullptr; // next 'ident'

      if (p->property_map.find(name_string) != p->property_map.end()) {
        error("line %d: duplicate property name found for %s\n", p->line, name_string.c_str());
        return nullptr;
      }

      Property new_property;
      p->property_map.insert(std::make_pair(name_string, p->properties.size()));
      p->properties.push_back(new_property);

      Property *new_property_ptr = &p->properties.back();
      if (!check_peek(p, TokenKind::LCurl)) {
        error("line %d: expected { after property name\n", p->line);
        return nullptr;
      }
      if (!next(p)) return nullptr; // next {

      while (!check_peek(p, TokenKind::RCurl)) {
        if (check_peek(p, TokenKind::Err)) return nullptr;
        if (!check_peek(p, TokenKind::Ident)) {
          error("line %d: expected another name in property value list\n", p->line);
          return nullptr;
        }

        new_property_ptr->values.push_back(peek(p)->value);

        if (!next(p)) return nullptr; // next 'ident'
      }
      assert(check_peek(p, TokenKind::RCurl));
      if (!next(p)) return nullptr; // next '}'

      break;
    }
    case TokenKind::Grid: {
      if (!next(p)) return nullptr; // next 'grid'

      if (!check_peek(p, TokenKind::Ident)) {
        error("line %d: expected name for grid\n", p->line);
        return nullptr;
      }
      std::string grid_name_string = span_to_string(p, peek(p)->value);
      if (!next(p)) return nullptr; // next 'ident'

      if (p->grids.find(grid_name_string) != p->grids.end()) {
        error("line %d: found duplicate grid definition for %s\n", p->line, grid_name_string.c_str());
        return nullptr;
      }

      p->grids.insert(std::make_pair(grid_name_string, Grid{}));
      Grid *new_grid_ptr                 = &p->grids[grid_name_string];
      new_grid_ptr->variable_start_index = p->variable_count;

      i32 grid_size = 1;
      for (;;) {
        if (check_peek(p, TokenKind::Err)) return nullptr;
        if (!check_peek(p, TokenKind::LSquare)) break;
        if (!next(p)) return nullptr; // next [

        if (!check_peek(p, TokenKind::Intlit)) {
          error("line %d: expected integer literal for grid dimensions\n", p->line);
          return nullptr;
        }

        i32 dimension_value = peek(p)->intlit;
        grid_size *= dimension_value;
        new_grid_ptr->dimensions.push_back(dimension_value);

        if (!next(p)) return nullptr; // next 'intlit'

        if (!check_peek(p, TokenKind::RSquare)) {
          error("line %d: expected ] for grid dimensions\n", p->line);
          return nullptr;
        }
        if (!next(p)) return nullptr; // next ]
      }

      if (new_grid_ptr->dimensions.size() == 0) {
        error("line %d: expected grid to have at least one dimension\n", p->line);
        return nullptr;
      }
      p->variable_count += grid_size;
      debug("created grid %s with %d variables\n", grid_name_string.c_str(), grid_size);

      break;
    }
    default:
      error("line %d: expected global statement but got %s\n", p->line, TokenKind::to_string[token->kind]);
      return nullptr;
    }
  }

  return entry_bb;
}

Result parse_to_cfg(CFG *out_cfg, cstr filepath) {
  Parser lex;
  lex.variable_count       = 0;
  lex.local_variable_count = 0;
  lex.index_variable_count = 0;
  lex.block_count          = 0;
  lex.index                = 0;
  lex.tlength              = 0;
  lex.line                 = 1;
  lex.data                 = nullptr;
  lex.file_length          = 0;

  auto *fstream = fopen(filepath, "rb");
  if (!fstream) {
    error("could not open file %s\n", filepath);
    return err;
  }

  i32 capacity = 0x100;
  lex.data     = new char[u32(capacity)];
  for (;;) {
    i32 remaining_capacity = capacity - lex.file_length;
    lex.file_length += fread(lex.data + lex.file_length, 1, u32(remaining_capacity), fstream);

    if (lex.file_length != capacity) {
      fclose(fstream);
      break;
    }

    capacity         = (capacity << 1) - (capacity >> 1) + 8;
    char *new_buffer = new char[u32(capacity)];
    memcpy(new_buffer, lex.data, u32(lex.file_length));
    delete[] lex.data;
    lex.data = new_buffer;
  }

  debug("Parsing %d bytes from file %s\n", lex.file_length, filepath);

  out_cfg->entry_bb  = parse_file(&lex);
  out_cfg->file_data = lex.data;
  if (!out_cfg->entry_bb) {
    error("failed to generate CFG\n");
    return err;
  }

  return ok;
}

} // namespace slang
