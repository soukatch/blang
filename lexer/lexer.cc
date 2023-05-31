/*
skipping white space:

for (;; peek = next input character)
    if (peek is a blank or a tab)
        ;
    else if (peek is a newline)
        ++line;
    else
        break;


building integers from digits

if (peek holds a digit) {
    v = 0;
    for (; peek holds a digit;
        v = v * 10 + (int)peek, peek = next input character)
        ;
    return token <num, v>;
}


keywords & identifiers

the expression:

count = count + increment;

corresponds to:

<id, "count"> <=> <id, "count"> <+> <id, "increment"> <;>

need to create reserved set of words


distinguishing keywords from identifiers

create a hash table for strings:

std::unordered_map<string, token> words;

algorithm:

if (peek holds a letter) {
    s = all alnums up until first whitespace;
    w = words.get(s);
    if (w)
        return w;
    words.insert(s, <id, s>);
    return <id, s>;
}


Token scan() {
    skip whitespace
    handle numbers
    handle reserved words and identifiers
    Token t{peek};
    // either get next or set peek to blank
    return t;
}

I'd rather not create a class hierarchy for tokens, and instead have:

using token = std::pair<tag, lexeme>;

where tag is an enumeration (class) and lexeme is a std::string. (this design
requires changing the scan() function for handling numbers)

*/

#include <cctype>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>

enum class tag {
  unkown,
  eof,
  comment,
  identifier,
  numeric_constant,
  char_constant,
  string_literal,
  // punctuators
  l_square,
  r_square,
  l_paren,
  r_paren,
  l_brace,
  r_brace,
  period,
  amp,
  equal,
  star,
  plus,
  minus,
  tilde,
  exclaim,
  slash,
  percent,
  less,
  greater,
  caret,
  pipe,
  question,
  colon,
  semi,
  comma,
  // keywords
  auto_,
  extrn_,
  goto_,
  if_,
  return_,
  switch_,
  while_
};

using token = std::pair<tag, std::optional<std::string>>;

std::unordered_map<std::string, token> words{
    {"auto", {tag::auto_, "auto"}},       {"extrn", {tag::extrn_, "extrn"}},
    {"goto", {tag::goto_, "goto"}},       {"if", {tag::if_, "if"}},
    {"return", {tag::return_, "return"}}, {"switch", {tag::switch_, "switch"}},
    {"while", {tag::while_, "while"}}};

const std::unordered_map<char, tag> tag_mapping{
    {'[', tag::l_square}, {']', tag::r_square}, {'(', tag::l_paren},
    {')', tag::r_paren},  {'{', tag::l_brace},  {'}', tag::r_brace},
    {'.', tag::period},   {'&', tag::amp},      {'=', tag::equal},
    {'*', tag::star},     {'+', tag::plus},     {'-', tag::minus},
    {'~', tag::tilde},    {'!', tag::exclaim},  {'/', tag::slash},
    {'%', tag::percent},  {'<', tag::less},     {'>', tag::greater},
    {'^', tag::caret},    {'|', tag::pipe},     {'?', tag::question},
    {':', tag::colon},    {';', tag::semi},     {',', tag::comma}};

int line{1};

inline tag char_to_tag(int peek) noexcept {
  return tag_mapping.contains(static_cast<char>(peek))
             ? tag_mapping.at(static_cast<char>(peek))
             : tag::unkown;
}

[[noreturn]] inline void err_exit_msg(std::string_view msg = "") {
  std::cout << "lex error on line " << line << ": " << msg << '\n';
  exit(EXIT_FAILURE);
}

token lex(std::istream &input) {
  static int peek{static_cast<int>(' ')};

  // consume all whitespace
  for (; input.good() && isspace(peek);
       line += static_cast<int>(peek == static_cast<int>('\n')),
       peek = input.get())
    ;

  if (input.eof())
    return {tag::eof, std::nullopt};

  if (!input.good())
    err_exit_msg("stream error");

  if (isalpha(peek) || peek == '.' || peek == '_') {
    // handle identifiers
    // Variable names have one to eight ascii characters, chosen from A-Z, a-z,
    // ., _, 0-9, and start with a non-digit. [1]
    std::string s{static_cast<char>(peek)};
    for (int i{1}; i++ < 8 && input.good() &&
                   (isalnum(peek = input.get()) || peek == '.' || peek == '_');
         s += static_cast<char>(peek))
      ;

    /// TODO: cleaner error handling.
    // error in the stream
    if (!input.good() && !input.eof())
      err_exit_msg("stream error");
    // identifier length exceeds eight bytes.
    if (s.size() == 8 && !isspace(peek))
      err_exit_msg("variable name exceeds maximum length of 8 bytes");

    if (!words.contains(s))
      words.insert({s, {tag::identifier, s}});

    return words[s];
  } else if (isdigit(peek)) {
    std::string s{static_cast<char>(peek)};
    for (; input.good() && isdigit(peek = input.get());
         s += static_cast<char>(peek))
      ;

    /// TODO: cleaner error handling.
    if (!input.good() && !input.eof())
      err_exit_msg("stream error");

    /// TODO: we need to make sure that the lexer hits a whitespace, operator,
    ///       or semi - right now we're just ignoring that syntax error.
    return token{tag::numeric_constant, s};
  } else if (peek == static_cast<int>('\'')) {
    // handle character constant.
    // A "character" is one to four ascii characters, enclosed in single quotes.
    // The characters are stored in a single machine word, right-justified and
    // zero-filled. [1]

    // character constant must be enclosed withing the single quotes.
    if (static_cast<char>(peek = input.get()) == '\'')
      exit(EXIT_FAILURE);

    std::string s{static_cast<char>(peek)};
    for (int i{1}; i++ < 4 && input.good() &&
                   (peek = input.get()) != static_cast<int>('\'');
         s += static_cast<char>(peek))
      ;

    if (s.size() == 4)
      peek = input.get();

    // exceeds maximum length || forgot closing single quote
    if (peek != static_cast<int>('\''))
      err_exit_msg(s.size() == 4
                       ? "character constant exceeds maximum length of 4 bytes"
                       : "expected closing quote");

    // consume the single quote.
    peek = input.get();

    return {tag::char_constant, s};
  } else if (peek == static_cast<int>('"')) {
    // handle string literals.

    std::string s;
    for (; input.good() && (peek = input.get()) != static_cast<int>('"');
         s += static_cast<char>(peek))
      ;

    // forgot closing double quote
    if (input.eof() && peek != static_cast<int>('"'))
      err_exit_msg("expected closing double quote");

    // consume the double quote.
    peek = input.get();

    return {tag::string_literal, s};
  } else if (peek == static_cast<int>('/')) {
    // handle comments.
    if ((peek = input.get()) == '*') {
      // consume all characters until end of comment.
      for (; input.good() && ((peek = input.get()) != static_cast<int>('*') ||
                              (peek = input.get()) != static_cast<int>('/'));
           line += peek == static_cast<int>('\n'))
        ;

      if (!input.good() && !input.eof())
        err_exit_msg("stream error");

      // consume the closing slash.
      peek = input.get();

      return lex(input);
    } else {
      input.putback(peek);
      peek = static_cast<int>('/');
    }
  }

  // should the lexeme be the stringified ascii value? not sure...
  token t{char_to_tag(peek), std::string{static_cast<char>(peek)}};

  if (input.good())
    peek = input.get();

  return t;
}

int main() {
  for (token t; (t = lex(std::cin)).first != tag::eof;
       std::cout << line << ": <" << static_cast<int>(t.first) << ", "
                 << (t.second ? *t.second : "") << ">\n")
    ;
}

///===---References------------------------------------------------------===///
/// 1. bell-labs.com/usr/dmr/www/btut.html
///===-------------------------------------------------------------------===///