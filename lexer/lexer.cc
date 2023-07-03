#include <cassert>
#include <fcntl.h>
#include <unistd.h>

#include "lexer.h"

namespace blang {

token::token(tag _tag, std::string_view _lexeme)
    : tag_{_tag}, lexeme_{_lexeme} {}

token::token(tag _tag, std::string &&_lexeme)
    : tag_{_tag}, lexeme_{std::move(_lexeme)} {}

std::ostream &operator<<(std::ostream &_os, const token &_token) {
  _os << '<' << std::to_string(static_cast<int>(_token.tag_))
      << (_token.lexeme_.empty() ? "" : ",")
      << (_token.lexeme_.empty() ? "" : std::string_view{_token.lexeme_})
      << '>';
  return _os;
}

lexer::lexer(std::string_view _file_name) noexcept
    : fd_{open(_file_name.data(), O_RDONLY)} {
  if (fd_ == -1) {
    perror(_file_name.data());
    exit(errno);
  }

  load(buffer_a_);
  load(buffer_b_);

  begin_ = forward_ = buffer_a_;
}

constexpr int lexer::line() const noexcept { return line_; }

void lexer::load(char *_buffer) noexcept {
  if (const ssize_t bytes_read{read(fd_, _buffer, BUFFER_SIZE - 1)};
      bytes_read >= 0)
    _buffer[bytes_read] = EOF;
  else {
    perror("");
    exit(errno);
  }
}

bool lexer::load_and_switch() noexcept {
  return forward_ == buffer_a_ + BUFFER_SIZE - 1
             ? (load(buffer_b_), forward_ = buffer_b_, true)
         : forward_ == buffer_b_ + BUFFER_SIZE - 1
             ? (load(buffer_a_), forward_ = buffer_a_, true)
             : false;
}

int lexer::comment() noexcept {
  for (++forward_;;) {
    switch (*forward_++) {
    case EOF:
      if (!load_and_switch())
        return false;
      break;
    case '\n':
      ++line_;
      break;
    case '*':
      if (*forward_ == '/')
        return (++forward_, 0);
    }
  }
}

token lexer::operator()() {
  for (; isspace(static_cast<int>(*forward_)); ++forward_)
    if (*forward_ == '\n')
      ++line_;

  begin_ = forward_;
  switch (std::string lexeme; *forward_++) {
  case EOF:
    return load_and_switch() ? (*this)() : tag::eof;
  case '[':
    return tag::l_square;
  case ']':
    return tag::r_square;
  case '(':
    return tag::l_paren;
  case ')':
    return tag::r_paren;
  case '{':
    return tag::l_brace;
  case '}':
    return tag::r_brace;
  case '.':
    return tag::period;
  case '&':
    return *forward_ == '&' ? (++forward_, tag::ampamp) : tag::amp;
  case '=':
    switch (*forward_++) {
    case '&':
      return tag::equalamp;
    case '*':
      return tag::equalstar;
    case '+':
      return tag::equalplus;
    case '-':
      return tag::equalminus;
    case '/':
      return tag::equalslash;
    case '%':
      return tag::percent;
    case '<':
      if (*forward_ == '<')
        return (++forward_, tag::equallessless);
      break;
    case '>':
      if (*forward_ == '>')
        return (++forward_, tag::equalgreatergreater);
      break;
    case '^':
      return tag::equalcaret;
    case '|':
      return tag::equalcaret;
    case '=':
      return tag::equalequal;
    }
    return (--forward_, tag::equal);
  case '*':
    return tag::star;
  case '+':
    return *forward_ == '+' ? (++forward_, tag::plusplus) : tag::plus;
  case '-':
    return *forward_ == '-' ? (++forward_, tag::minusminus) : tag::minus;
  case '~':
    return tag::tilde;
  case '!':
    return *forward_ == '=' ? (++forward_, tag::exclaimequal) : tag::exclaim;
  case '/':
    // we have a comment
    return *forward_ == '*' ? (comment(), (*this)()) : tag::slash;
  case '%':
    return tag::percent;
  case '<':
    return *forward_ == '<'   ? (++forward_, tag::lessless)
           : *forward_ == '=' ? (++forward_, tag::lessequal)
                              : tag::less;
  case '>':
    return *forward_ == '>'   ? (++forward_, tag::greatergreater)
           : *forward_ == '=' ? (++forward_, tag::greaterequal)
                              : tag::greater;
  case '^':
    return tag::caret;
  case '|':
    return *forward_ == '|' ? (++forward_, tag::pipepipe) : tag::pipe;
  case '?':
    return tag::question;
  case ':':
    return tag::colon;
  case ';':
    return tag::semi;
  case ',':
    return tag::comma;
  case '\'':
    for (int i{};;) {
      begin_ = forward_;
      for (; i < 4 && *forward_ != '\''; ++i, ++forward_)
        ;
      assert(!(i == 4 && *forward_ != '\'') &&
             "character literal not terminated with single quote.");
      lexeme.insert(std::end(lexeme), begin_, forward_);
      if (*forward_ == '\'' || (*forward_ == EOF && !load_and_switch()))
        break;
    }
    // need to consume the closing quote.
    return (++forward_, token{tag::char_constant, std::move(lexeme)});
  case '"':
    for (int i{};;) {
      for (begin_ = forward_; *forward_ != '"'; ++forward_)
        ;
      lexeme.insert(std::end(lexeme), begin_, forward_);
      if (*forward_ == '"' || (*forward_ == EOF && !load_and_switch()))
        break;
    }
    return (++forward_, token{tag::string_literal, std::move(lexeme)});
  }

  assert(
      begin_ == --forward_ &&
      "begin and forward should point to same address after switch statement.");

  if (std::isalpha(static_cast<int>(*forward_)) != 0 || *forward_ == '_' ||
      *forward_ == '.') {
    // identifiers
    std::string lexeme;
    for (int i{};; begin_ = forward_) {
      for (; i < MAX_ID_LENGTH &&
             (std::isalnum(static_cast<int>(*forward_)) != 0 ||
              *forward_ == '_' || *forward_ == '.');
           ++i, ++forward_)
        ;
      lexeme.insert(std::end(lexeme), begin_, forward_);
      if (*forward_ != EOF || !load_and_switch())
        break;
    }
    // should we try an assert here?
    return words_.try_emplace(lexeme, tag::name, std::move(lexeme))
        .first->second;
  } else if (isdigit(static_cast<int>(*forward_)) != 0) {
    // numeric constants
    std::string number;
    for (;; begin_ = forward_) {
      for (; std::isdigit(static_cast<int>(*forward_)) != 0; ++forward_)
        ;
      number.insert(std::end(number), begin_, forward_);
      if (*forward_ != EOF || !load_and_switch())
        break;
    }
    return {tag::numeric_constant, std::move(number)};
  }
  return static_cast<tag>(*forward_++);
  // Variable names have one to eight ascii characters, chosen from A-Z, a-z,
  // ., _, 0-9, and start with a non-digit.
}

} // namespace blang