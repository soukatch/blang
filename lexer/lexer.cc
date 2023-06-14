#include <cassert>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unistd.h>
#include <unordered_map>

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
  // misc
  comment,
  identifier,
  numeric_constant,
  char_constant,
  string_literal,
  unknown
};

struct token {
  tag tag_{tag::unknown};
  std::string lexeme_{};

  token(tag _tag, const std::string &_lexeme) : tag_{_tag}, lexeme_{_lexeme} {}

  token(tag _tag = tag::unknown, std::string &&_lexeme = "")
      : tag_{_tag}, lexeme_{std::move(_lexeme)} {}

  friend std::ostream &operator<<(std::ostream &_os, const token &_token) {
    _os << '<' << std::to_string(static_cast<int>(_token.tag_))
        << (_token.lexeme_.empty() ? "" : ",")
        << (_token.lexeme_.empty() ? "" : std::string_view{_token.lexeme_})
        << '>';
    return _os;
  }
};

class lexer final {
  static constexpr int BUFFER_SIZE{4096};
  static constexpr int MAX_ID_LENGTH{8};
  int line_{1};
  char *begin_{nullptr}, *forward_{nullptr};
  char buffer_a_[BUFFER_SIZE]{}, buffer_b_[BUFFER_SIZE]{};
  std::unordered_map<std::string, token> words_{{"auto", {tag::auto_, ""}},
                                                {"extrn", {tag::extrn_, ""}},
                                                {"if", {tag::if_, ""}}};
  int fd_{-1};

public:
  lexer(std::string_view _file_name) noexcept
      : fd_{open(_file_name.data(), O_RDONLY)} {
    if (fd_ == -1) {
      perror(_file_name.data());
      exit(errno);
    }

    load(buffer_a_);
    load(buffer_b_);

    begin_ = forward_ = buffer_a_;
  }

  constexpr int line() const noexcept { return line_; }

  void load(char *_buffer) noexcept {
    if (const ssize_t bytes_read{read(fd_, _buffer, BUFFER_SIZE - 1)};
        bytes_read >= 0)
      _buffer[bytes_read] = EOF;
    else {
      perror("");
      exit(errno);
    }
  }

  bool load_and_switch() noexcept {
    if (forward_ == buffer_a_ + BUFFER_SIZE - 1) {
      load(buffer_b_);
      forward_ = buffer_b_;
      return true;
    } else if (forward_ == buffer_b_ + BUFFER_SIZE - 1) {
      load(buffer_a_);
      forward_ = buffer_a_;
      return true;
    }
    return false;
  }

  int comment() noexcept {
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
        if (*forward_++ == '/')
          return 0;
        --forward_;
      }
    }
  }

  token operator()() {
    for (; isspace(static_cast<int>(*forward_)); ++forward_)
      if (*forward_ == '\n')
        ++line_;

    begin_ = forward_;
    switch (std::string lexeme; *forward_++) {
    case EOF:
      return load_and_switch() ? this->operator()() : tag::eof;
    case '[':
      return tag::l_square;
    case ']':
      return tag::r_square;
    case '(':
      return tag::l_paren;
    case ')':
      return tag::r_square;
    case '{':
      return tag::l_brace;
    case '}':
      return tag::r_brace;
    case '.':
      return tag::period;
    case '&':
      if (*forward_++ == '&')
        return tag::ampamp;
      --forward_;
      return tag::amp;
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
        if (*forward_++ == '<')
          return tag::equallessless;
        --forward_;
        break;
      case '>':
        if (*forward_++ == '>')
          return tag::equalgreatergreater;
        --forward_;
        break;
      case '^':
        return tag::equalcaret;
      case '|':
        return tag::equalcaret;
      case '=':
        return tag::equalequal;
      }
      return tag::equal;
    case '*':
      return tag::star;
    case '+':
      if (*forward_++ == '+')
        return tag::plusplus;
      return tag::plus;
    case '-':
      if (*forward_++ == '-')
        return tag::minusminus;
      --forward_;
      return tag::minus;
    case '~':
      return tag::tilde;
    case '!':
      if (*forward_++ == '=')
        return tag::exclaimequal;
      --forward_;
      return tag::exclaim;
    case '/':
      // we have a comment
      if (*forward_ == '*') {
        comment();
        return this->operator()();
      }
      return tag::slash;
    case '%':
      return tag::percent;
    case '<':
      switch (*forward_++) {
      case '<':
        return tag::lessless;
      case '=':
        return tag::lessequal;
      }
      --forward_;
      return tag::less;
    case '>':
      switch (*forward_++) {
      case '>':
        return tag::greatergreater;
      case '=':
        return tag::greaterequal;
      }
      --forward_;
      return tag::greater;
    case '^':
      return tag::caret;
    case '|':
      if (*forward_++ == '|')
        return tag::pipepipe;
      --forward_;
      return tag::pipe;
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
      ++forward_;
      return {tag::char_constant, std::move(lexeme)};
    case '"':
      for (int i{};;) {
        begin_ = forward_;
        for (; *forward_ != '"'; ++forward_)
          ;
        lexeme.insert(std::end(lexeme), begin_, forward_);
        if (*forward_ == '"' || (*forward_ == EOF && !load_and_switch()))
          break;
      }
      ++forward_;
      return {tag::string_literal, std::move(lexeme)};
    }

    assert(begin_ == --forward_ &&
           "begin and forward should be pointing to same address after switch "
           "statement.");

    if (isalpha(static_cast<int>(*forward_)) != 0 || *forward_ == '_' ||
        *forward_ == '.') {
      // identifiers
      std::string lexeme;
      for (int i{};;) {
        for (;
             i < MAX_ID_LENGTH && (isalnum(static_cast<int>(*forward_)) != 0 ||
                                   *forward_ == '_' || *forward_ == '.');
             ++i, ++forward_)
          ;
        lexeme.insert(std::end(lexeme), begin_, forward_);
        if (*forward_ != EOF || !load_and_switch())
          break;
        begin_ = forward_;
      }
      // should try an assert here?
      return words_.try_emplace(lexeme, tag::identifier, std::move(lexeme))
          .first->second;
    } else if (isdigit(static_cast<int>(*forward_)) != 0) {
      // numeric constants
      std::string number;
      for (;;) {
        for (; isdigit(static_cast<int>(*forward_)) != 0;)
          ;
        number.insert(std::end(number), begin_, forward_);
        if (*forward_ != EOF || !load_and_switch())
          break;
        begin_ = forward_;
      }
      return {tag::numeric_constant, std::move(number)};
    }
    return static_cast<tag>(*forward_++);
    // Variable names have one to eight ascii characters, chosen from A-Z, a-z,
    // ., _, 0-9, and start with a non-digit.
  }
};
} // namespace blang

int main(int argc, char **argv) {
  if (argc != 2) {
    std::cout << "usage: lexer <filename>\n";
    return 1;
  }

  blang::lexer l{argv[1]};
  for (blang::token t{}; (t = l()).tag_ != blang::tag::eof;
       std::cout << l.line() << ':' << t << std::endl)
    ;
}
