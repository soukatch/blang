#pragma once

#include "../lexer/lexer.h"
#include <vector>

namespace blang {

class parser final {
  const std::vector<token> tokens_{};
  std::vector<token>::const_iterator next_{std::cbegin(tokens_)};
  std::vector<token>::difference_type max_next_{0};

  // program := {definition}_0
  bool program();

  // definition ::=
  //  name {[ {constant}01 ]}01 {ival {, ival}0}01 ;
  //	name ( {name {, name}0}01 ) statement
  bool definition();
  // name {[ {constant}01 ]}01 {ival {, ival}0}01 ;
  bool definition0();
  // name ( {name {, name}0}01 ) statement
  bool definition1();

  bool statement();
  // auto name {constant}01 {, name {constant}01}0 ;  statement
  bool statement0();
  // extrn name {, name}0 ; statement
  bool statement1();
  // name : statement
  bool statement2();
  // case constant : statement
  bool statement3();
  // { {statement}0 }
  bool statement4();
  // if ( rvalue ) statement {else statement}01
  bool statement5();
  // while ( rvalue ) statement
  bool statement6();
  // switch rvalue statement
  bool statement7();
  // goto rvalue ;
  bool statement8();
  // return {( rvalue )}01 ;
  bool statement9();
  // {rvalue}01 ;
  bool statement10();

  /*
  constant ::=
          {digit}1
          ' {char}12 '
          " {char}0 "
  */
  bool constant();

  bool rvalue();
  // ( rvalue )
  bool rvalue0();
  // lvalue
  bool rvalue1();
  // constant
  bool rvalue2();
  // lvalue assign rvalue
  bool rvalue3();
  // inc-dec lvalue
  bool rvalue4();
  // lvalue inc-dec
  bool rvalue5();
  // unary rvalue
  bool rvalue6();
  // & lvalue
  bool rvalue7();
  //   // rvalue binary rvalue
  //   bool rvalue8();
  //   // rvalue ? rvalue : rvalue
  //   bool rvalue9();
  //   // rvalue ( {rvalue {, rvalue}0 }01 )
  bool rvalue10();
  bool rprime();

  /*
  lvalue ::=
          name
          * rvalue
          rvalue [ rvalue ]
  */
  bool lvalue();
  bool lprime();

  /*
  assign ::=
          = {binary}01
  */
  bool assign();

  /*
  inc-dec ::=
          ++
          --
  */
  bool inc_dec();

  /*
  unary ::=
          -
          !
  */
  bool unary();

  /*
  binary ::=
          |
          &
          ==
          !=
          <
          <=
          >
          >=
          <<
          >>
          -
          +
          %
          *
          /
  */
  bool binary();

  /*
  ival ::=
          constant
          name
  */
  bool ival();

  bool match(tag _t);

public:
  parser(const std::vector<token> &_tokens);
  parser(std::vector<token> &&_tokens);
  parser(token *_first, token *_last);
  bool operator()();
  constexpr inline decltype(auto) next() const {
    return std::distance(std::cbegin(tokens_), next_);
  }

  constexpr inline const decltype(max_next_) &max_next() const & noexcept {
    return max_next_;
  }

  constexpr inline decltype(auto) max_next() && noexcept {
    return std::move(max_next_);
  }
};

// Should the rollback be implemented by the match/sub bodies? probably not...

} // namespace blang
