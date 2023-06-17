#pragma once

#include <cstdio>
#include <iostream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

namespace blang {
enum class tag {
  eof = EOF,
  // punctuators
  l_square = 256,
  r_square,
  l_paren,
  r_paren,
  l_brace,
  r_brace,
  period,
  amp,
  ampamp,
  equalamp,
  star,
  equalstar,
  plus,
  plusplus,
  equalplus,
  minus,
  minusminus,
  equalminus,
  tilde,
  exclaim,
  exclaimequal,
  slash,
  equalslash,
  percent,
  equalpercent,
  less,
  lessless,
  lessequal,
  equallessless,
  greater,
  greatergreater,
  greaterequal,
  equalgreatergreater,
  caret,
  equalcaret,
  pipe,
  pipepipe,
  equalpipe,
  question,
  colon,
  semi,
  equal,
  equalequal,
  comma,
  // keywords
  auto_,
  extrn_,
  if_,
  else_,
  goto_,
  switch_,
  case_,
  while_,
  return_,
  // misc
  comment,
  name,
  numeric_constant,
  char_constant,
  string_literal,
  unknown
};

struct token {
  tag tag_{tag::unknown};
  std::string lexeme_{};

  token(tag _tag = tag::unknown, std::string_view _lexeme = "");
  token(tag _tag, std::string &&_lexeme);
  friend std::ostream &operator<<(std::ostream &_os, const token &_token);
};

class lexer final {
  static inline constexpr int BUFFER_SIZE{4096};
  static inline constexpr int MAX_ID_LENGTH{8};
  int line_{1};
  char *begin_{nullptr}, *forward_{nullptr};
  char buffer_a_[BUFFER_SIZE]{}, buffer_b_[BUFFER_SIZE]{};
  std::unordered_map<std::string, token> words_{
      {"auto", {tag::auto_}},     {"extrn", {tag::extrn_}},
      {"if", {tag::if_}},         {"case", {tag::case_}},
      {"else", {tag::else_}},     {"while", {tag::while_}},
      {"switch", {tag::switch_}}, {"goto", {tag::goto_}},
      {"return", {tag::return_}}};
  int fd_{-1};

public:
  lexer(std::string_view _file_name) noexcept;
  constexpr int line() const noexcept;
  void load(char *_buffer) noexcept;
  bool load_and_switch() noexcept;
  int comment() noexcept;
  token operator()();
};
} // namespace blang