#include "parser.hpp"
#include "cfg.hpp"

#include <string>
#include <unordered_map>

namespace slang {

// clang-format off
#define TOKEN_KIND(pick) \
  pick(Ident,    "'identifier'"), \
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
  pick(Property, "property"), \
  pick(Object,   "object"), \
  pick(Function, "function"), \
  pick(If,       "if"), \
  pick(Else,     "else"), \
  pick(For,      "for"), \
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

struct ValueList {
  std::vector<Span> values;
};

struct ValuesDatabase {
  std::unordered_map<std::string, i32> map;
  std::vector<ValueList> list;
};

struct Grid {
  i32 object_id;
  std::vector<i32> dimensions;

  i32 object_instance_start_index;
};

struct Lexer {
  ValuesDatabase properties;
  ValuesDatabase objects;

  i32 object_count;
  std::unordered_map<std::string, Grid> grids;

  i32 index;
  i32 tlength;

  i32 line;

  Token token;

  char *data;
  i32 file_length;
};

bool is_whitespace(char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; }

bool is_letter(char c) { return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z'); }

bool is_letter_or_underscore(char c) { return is_letter(c) || c == '_'; }

bool is_number(char c) { return '0' <= c && c <= '9'; }

bool is_number_or_underscore(char c) { return is_number(c) || c == '_'; }

bool is_eof(Lexer *l) { return l->index + l->tlength >= l->file_length; }

void end_token(Lexer *l) {
  l->index += l->tlength;
  l->tlength = 0;
}

Token *create_token(Lexer *l, TokenKind::Enum kind) {
  l->token.kind         = kind;
  l->token.line         = l->line;
  l->token.value.index  = l->index;
  l->token.value.length = l->tlength;
  end_token(l);
  return &l->token;
}

char peek_char(Lexer *l) {
  if (is_eof(l)) return 0;
  return l->data[l->index + l->tlength];
}

// clang-format off
TokenKind::Enum keywords[] {
  TokenKind::False,
  TokenKind::True,
  TokenKind::Property,
  TokenKind::Object,
  TokenKind::Function,
  TokenKind::If,
  TokenKind::Else,
  TokenKind::For,
  TokenKind::Return,
};
const i32 num_keywords = sizeof(keywords) / sizeof(TokenKind::Enum);
// clang-format on

Token *next_keyword_or_identifier(Lexer *l) {
  while (char ch = peek_char(l)) {
    if (!is_letter_or_underscore(ch) && !is_number(ch)) break;
    ++l->tlength;
  }

  for (i32 i = 0; i < num_keywords; ++i) {
    cstr ptr = TokenKind::to_string[keywords[i]];

    i32 k = 0;
    for (; k < l->tlength; ++k) {
      if (*ptr != l->data[l->index + k]) break;
      ++ptr;
    }

    if (*ptr == '\0' && k == l->tlength) return create_token(l, keywords[i]);
  }
  return create_token(l, TokenKind::Ident);
}

Token *next_integer(Lexer *l) {
  u64 val = 0;
  while (char c = peek_char(l)) {
    if (!c || !is_number_or_underscore(c)) break;
    ++l->tlength;
    if (c != '_') val = val * 10 + (u8)(c - '0');
  }
  auto *token   = create_token(l, TokenKind::Intlit);
  token->intlit = (i32)val;
  return token;
}

Token *next(Lexer *l) {
  while (is_whitespace(peek_char(l))) {
    if (peek_char(l) == '\n') l->line++;
    l->index++;
  }

  switch (char c = peek_char(l)) {
  case '=': ++l->tlength; return create_token(l, TokenKind::Assign);
  case '!': ++l->tlength; return create_token(l, TokenKind::Not);
  case '&':
    ++l->tlength;
    if (peek_char(l) == '&') {
      ++l->tlength;
      return create_token(l, TokenKind::And);
    } else {
      error("line %d: expected && instead of &\n", l->line, c);
      create_token(l, TokenKind::Err);
      return nullptr;
    }
  case '|':
    ++l->tlength;
    if (peek_char(l) == '|') {
      ++l->tlength;
      return create_token(l, TokenKind::Or);
    } else {
      error("line %d: expected || instead of |\n", l->line, c);
      create_token(l, TokenKind::Err);
      return nullptr;
    }
  case '{': ++l->tlength; return create_token(l, TokenKind::LCurl);
  case '}': ++l->tlength; return create_token(l, TokenKind::RCurl);
  case '(': ++l->tlength; return create_token(l, TokenKind::LParen);
  case ')': ++l->tlength; return create_token(l, TokenKind::RParen);
  case '[': ++l->tlength; return create_token(l, TokenKind::LSquare);
  case ']': ++l->tlength; return create_token(l, TokenKind::RSquare);
  case '\0':
    l->token.kind         = TokenKind::Eof;
    l->token.value.index  = l->index;
    l->token.value.length = 1;
    return &l->token;
  default:
    if (is_letter_or_underscore(c)) return next_keyword_or_identifier(l);
    if (is_number(c)) return next_integer(l);

    error("line %d: unknown character %c\n", l->line, c);
    create_token(l, TokenKind::Err);
    return nullptr;
  }

  return nullptr;
}

Token *peek(Lexer *l) { return &l->token; }

bool check_peek(Lexer *l, TokenKind::Enum kind) { return peek(l)->kind == kind; }

bool is_span_equal_cstr(char *file_data, Span span, cstr str) {
  for (i32 i = 0; i < span.length; ++i) {
    if (*str == '\0') return false;
    if (file_data[span.index + i] != *str) return false;
    ++str;
  }
  return *str == '\0';
}

Expression *parse_expression(Lexer *l);

Expression *parse_operand(Lexer *l) {
  switch (peek(l)->kind) {
  case TokenKind::False: {
    if (!next(l)) return nullptr; // next false

    Expression *false_expression = new Expression();
    false_expression->kind       = ExpressionKind::False;
    return false_expression;
  }
  case TokenKind::True: {
    if (!next(l)) return nullptr; // next true

    Expression *true_expression = new Expression();
    true_expression->kind       = ExpressionKind::True;
    return true_expression;
  }
  case TokenKind::Not: {
    if (!next(l)) return nullptr; // next !

    Expression *not_expression  = new Expression();
    not_expression->kind        = ExpressionKind::Not;
    not_expression->unary.inner = parse_expression(l);
    if (!not_expression->unary.inner) return nullptr;
    return not_expression;
  }
  default: error("line %d: unexpected expression operand parsing\n", l->line); return nullptr;
  }
}

Expression *parse_operator(Lexer *l, Expression *left_expression, i32 left_precedence) {
  for (;;) {
    i32 right_precedence                          = 0;
    ExpressionKind::Enum operator_expression_kind = ExpressionKind::None;
    switch (peek(l)->kind) {
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

    switch (peek(l)->kind) {
    case TokenKind::And:
    case TokenKind::Or: {
      Expression *binary_expression  = new Expression();
      binary_expression->kind        = operator_expression_kind;
      binary_expression->binary.left = left_expression;
      if (!binary_expression->binary.left) return nullptr;

      if (!next(l)) return nullptr; // next 'binary operator'

      binary_expression->binary.right = parse_operator(l, parse_operand(l), right_precedence);
      if (!binary_expression->binary.right) return nullptr;

      left_expression = binary_expression;

      break;
    }
    default: assert(!"Unexpected operator parsing"); break;
    }
  }
  return left_expression;
}

Expression *parse_expression(Lexer *l) { return parse_operator(l, parse_operand(l), 0); }

BasicBlock *parse_block(Lexer *l) {
  assert(check_peek(l, TokenKind::LCurl));
  if (!next(l)) return nullptr; // next {

  BasicBlock *current_bb = new BasicBlock();

  for (;;) {
    switch (peek(l)->kind) {
    case TokenKind::Ident: {
      Instruction assignment_inst;
      assignment_inst.kind                 = InstructionKind::Assign;
      assignment_inst.assign.variable_name = peek(l)->value;
      if (!next(l)) return nullptr; // next 'ident'

      if (!check_peek(l, TokenKind::Assign)) {
        cstr unexpected_kind = TokenKind::to_string[peek(l)->kind];
        error("line %d: expected '=' after identifier but found %s instead\n", l->line, unexpected_kind);
        return nullptr;
      }
      if (!next(l)) return nullptr; // next =

      assignment_inst.assign.right_value_expression = parse_expression(l);
      if (!assignment_inst.assign.right_value_expression) return nullptr;

      current_bb->insts.push_back(assignment_inst);
      break;
    }
    case TokenKind::If: assert(!"TODO: parse if statement"); break;
    case TokenKind::For: assert(!"TODO: parse loop"); break;
    case TokenKind::Return: assert(!"TODO: parse return"); break;
    case TokenKind::RCurl: break;
    default: error("line %d: unimplemented statement parse", l->line); return nullptr;
    }
    if (peek(l)->kind == TokenKind::RCurl) break;
  }

  assert(check_peek(l, TokenKind::RCurl));
  if (!next(l)) return nullptr; // next }

  return current_bb;
}

BasicBlock *parse_function(Lexer *l) {
  assert(check_peek(l, TokenKind::Function));
  if (!next(l)) return nullptr; // next function

  auto *function_name = peek(l);
  if (!is_span_equal_cstr(l->data, function_name->value, "is_sat")) {
    error("expected function name to be 'is_sat'\n");
    return nullptr;
  }
  if (!next(l)) return nullptr; // next 'function name'

  if (!check_peek(l, TokenKind::LParen)) {
    error("expected ( after function name\n");
    return nullptr;
  }
  if (!next(l)) return nullptr; // next (

  if (!check_peek(l, TokenKind::RParen)) {
    error("expected ) after function name\n");
    return nullptr;
  }
  if (!next(l)) return nullptr; // next )

  if (!check_peek(l, TokenKind::LCurl)) {
    error("expected { to define function body\n");
    return nullptr;
  }

  return parse_block(l);
}

Result parse_value_list(Lexer *l, ValuesDatabase *value_db) {
  if (!check_peek(l, TokenKind::Ident)) {
    error("line %d: expected name for global\n", l->line);
    return err;
  }
  Span name = peek(l)->value;
  if (!next(l)) return err; // next 'ident'

  std::string name_string(&l->data[name.index], (u32)name.length);

  if (value_db->map.find(name_string) != value_db->map.end()) {
    error("line %d: duplicate name found for %s\n", l->line, name_string.c_str());
    return err;
  }

  ValueList new_value_list;
  value_db->map.insert(std::make_pair(name_string, value_db->list.size()));
  value_db->list.push_back(new_value_list);

  ValueList *value_list_ptr = &value_db->list.back();
  if (!check_peek(l, TokenKind::LCurl)) {
    error("line %d: expected { after global name\n", l->line);
    return err;
  }
  if (!next(l)) return err; // next {

  while (!check_peek(l, TokenKind::RCurl)) {
    if (check_peek(l, TokenKind::Err)) return err;
    if (!check_peek(l, TokenKind::Ident)) {
      error("line %d: expected another name in value list\n", l->line);
      return err;
    }

    value_list_ptr->values.push_back(peek(l)->value);

    if (!next(l)) return err; // next 'ident'
  }
  assert(check_peek(l, TokenKind::RCurl));
  if (!next(l)) return err; // next '}'

  return ok;
}

BasicBlock *parse_file(Lexer *l) {
  if (!next(l)) return nullptr; // consume the first token

  BasicBlock *entry_bb = nullptr;
  for (;;) {
    auto *token = peek(l);
    if (token->kind == TokenKind::Err || token->kind == TokenKind::Eof) break;

    switch (token->kind) {
    case TokenKind::Function:
      if (entry_bb) {
        error("line %d: expected one function but found another here\n", l->line);
        return nullptr;
      }
      entry_bb = parse_function(l);
      if (!entry_bb) return nullptr;
      break;
    case TokenKind::Property: {
      if (!next(l)) return nullptr; // next property
      if (parse_value_list(l, &l->properties)) return nullptr;
      break;
    }
    case TokenKind::Object:
      if (!next(l)) return nullptr; // next object
      if (parse_value_list(l, &l->objects)) return nullptr;
      break;
    case TokenKind::Ident: {
      Span object_name = peek(l)->value;
      std::string object_name_string(&l->data[object_name.index], (u32)object_name.length);
      if (!next(l)) return nullptr; // next 'ident'

      auto it = l->objects.map.find(object_name_string);
      if (it == l->objects.map.end()) {
        error("line %d: unknown object name %s\n", l->line, object_name_string.c_str());
        return nullptr;
      }

      auto object_id = it->second;

      if (!check_peek(l, TokenKind::Ident)) {
        error("line %d: expected name for grid\n");
        return nullptr;
      }
      if (!next(l)) return nullptr; // next object

      Span grid_name = peek(l)->value;
      std::string grid_name_string(&l->data[grid_name.index], (u32)grid_name.length);

      if (l->grids.find(grid_name_string) != l->grids.end()) {
        error("line %d: found duplicate grid definition for %s\n", grid_name_string.c_str());
        return nullptr;
      }

      l->grids.insert(std::make_pair(grid_name_string, Grid{}));
      Grid *new_grid_ptr                        = &l->grids[grid_name_string];
      new_grid_ptr->object_id                   = object_id;
      new_grid_ptr->object_instance_start_index = l->object_count;

      i32 grid_size = 1;
      for (;;) {
        if (check_peek(l, TokenKind::Err)) return nullptr;
        if (!check_peek(l, TokenKind::LSquare)) break;
        if (!next(l)) return nullptr; // next [

        if (!check_peek(l, TokenKind::Intlit)) {
          error("line %d: expected integer literal for grid dimensions\n");
          return nullptr;
        }

        i32 dimension_value = peek(l)->intlit;
        grid_size *= dimension_value;
        new_grid_ptr->dimensions.push_back(dimension_value);

        if (!next(l)) return nullptr; // next 'intlit'

        if (!check_peek(l, TokenKind::RSquare)) {
          error("line %d: expected ] for grid dimensions\n");
          return nullptr;
        }
        if (!next(l)) return nullptr; // next ]
      }

      if (new_grid_ptr->dimensions.size() == 0) {
        error("line %d: expected grid to have at least one dimension\n");
        return nullptr;
      }
      l->object_count += grid_size;
      debug("created grid %s with %d variables\n", grid_name_string.c_str(), grid_size);

      break;
    }
    default:
      error("line %d: expected global statement but got %s\n", l->line, TokenKind::to_string[token->kind]);
      return nullptr;
    }
  }

  return entry_bb;
}

Result parse_to_cfg(CFG *out_cfg, cstr filepath) {
  Lexer lex;
  lex.object_count = 0;
  lex.index        = 0;
  lex.tlength      = 0;
  lex.line         = 1;
  lex.data         = nullptr;
  lex.file_length  = 0;

  auto *fstream = fopen(filepath, "rb");
  if (!fstream) {
    error("could not open file %s\n", filepath);
    return err;
  }

  i32 capacity = 0x100;
  i32 length   = 0;

  lex.data = new char[u32(capacity)];
  for (;;) {
    i32 remaining_capacity = capacity - length;
    lex.file_length += fread(lex.data + lex.file_length, 1, u32(remaining_capacity), fstream);

    if (length != capacity) {
      fclose(fstream);
      break;
    }

    capacity         = (capacity << 1) - (capacity >> 1) + 8;
    char *new_buffer = new char[u32(capacity)];
    memcpy(new_buffer, lex.data, u32(lex.file_length));
    delete[] lex.data;
    lex.data = new_buffer;
  }

  out_cfg->entry_bb  = parse_file(&lex);
  out_cfg->file_data = lex.data;
  if (!out_cfg->entry_bb) {
    error("failed to generate CFG\n");
    return err;
  }

  return ok;
}

} // namespace slang
