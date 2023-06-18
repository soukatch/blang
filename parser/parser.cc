#include "parser.h"

#include <cassert>

namespace blang {

parser::parser(const std::vector<token> &_tokens) : tokens_{_tokens} {}
parser::parser(std::vector<token> &&_tokens) : tokens_{std::move(_tokens)} {}
parser::parser(std::span<token> _tokens)
    : tokens_{std::cbegin(_tokens), std::cend(_tokens)} {}
parser::parser(token *first, token *last) : tokens_{first, last} {}

// Should the rollback be implemented by the match/sub bodies? probably not...

// program := {definition}_0
bool parser::program() {
  for (; next_ != std::cend(tokens_);)
    if (!definition())
      return false;
  return true;
}

// definition ::=
//  name {[ {constant}01 ]}01 {ival {, ival}0}01 ;
//	name ( {name {, name}0}01 ) statement
bool parser::definition() {
  auto save{next_};
  return definition0() || (next_ = save, definition1());
}

// name {[ {constant}01 ]}01 {ival {, ival}0}01 ;
bool parser::definition0() {
  if (!match(tag::name))
    return false;

  if (match(tag::l_square)) {
    constant();
    if (!match(tag::r_square))
      return false;
  }

  if (ival())
    for (; match(tag::comma);)
      if (!ival())
        return false;

  return match(tag::semi);
}

// name ( {name {, name}0}01 ) statement
bool parser::definition1() {
  if (!match(tag::name) || !match(tag::l_paren))
    return false;

  if (match(tag::name))
    for (; match(tag::comma);)
      if (!match(tag::name))
        return false;
  --next_;

  return (match(tag::r_paren) && statement());
}

/*
statement ::=
        auto name {constant}01 {, name {constant}01}0 ;  statement
        extrn name {, name}0 ; statement
        name : statement
        case constant : statement
        { {statement}0 }
        if ( rvalue ) statement {else statement}01
        while ( rvalue ) statement
        switch rvalue statement
        goto rvalue ;
        return {( rvalue )}01 ;
        {rvalue}01 ;
*/
bool parser::statement() {
  auto save{next_};
  return statement0() || (next_ = save, statement1()) ||
         (next_ = save, statement2()) || (next_ = save, statement3()) ||
         (next_ = save, statement4()) || (next_ = save, statement5()) ||
         (next_ = save, statement5()) || (next_ = save, statement6()) ||
         (next_ = save, statement7()) || (next_ = save, statement8()) ||
         (next_ = save, statement9()) || (next_ = save, statement10());
}

// auto name {constant}01 {, name {constant}01}0 ;  statement
bool parser::statement0() {
  if (!match(tag::auto_) || !match(tag::name))
    return false;

  auto save{next_};
  if (!constant())
    next_ = save;

  for (; match(tag::comma);) {
    if (!match(tag::name))
      return false;
    save = next_;
    if (!constant())
      next_ = save;
  }
  --next_;

  return match(tag::semi) && statement();
}

// extrn name {, name}0 ; statement
bool parser::statement1() {
  if (!match(tag::extrn_) || !match(tag::name))
    return false;

  for (; match(tag::comma);)
    if (!match(tag::name))
      return false;
  --next_;

  return match(tag::semi) && statement();
}

// name : statement
bool parser::statement2() {
  return match(tag::name) && match(tag::colon) && statement();
}

// case constant : statement
bool parser::statement3() {
  return match(tag::case_) && constant() && match(tag::colon) && statement();
}

// { {statement}0 }
bool parser::statement4() {
  if (!match(tag::l_brace))
    return false;

  for (; statement();)
    ;
  --next_;

  return match(tag::r_brace);
}

// if ( rvalue ) statement {else statement}01
bool parser::statement5() {
  if (!match(tag::if_) || !match(tag::l_paren) || !rvalue() ||
      !match(tag::r_paren) || !statement())
    return false;

  if (match(tag::else_) && !statement())
    return false;

  return true;
}

// while ( rvalue ) statement
bool parser::statement6() {
  return match(tag::while_) && match(tag::l_paren) && rvalue() &&
         match(tag::r_paren) && statement();
}

// switch rvalue statement
bool parser::statement7() {
  return match(tag::switch_) && rvalue() && statement();
}

// goto rvalue ;
bool parser::statement8() {
  return match(tag::goto_) && rvalue() && match(tag::comma);
}

// return {( rvalue )}01 ;
bool parser::statement9() {
  if (!match(tag::return_))
    return false;

  if (match(tag::l_paren)) {
    if (!rvalue() || !match(tag::r_paren))
      return false;
  } else
    --next_;

  return true;
}

// {rvalue}01 ;
bool parser::statement10() {
  auto save{next_};
  return rvalue() || (next_ = save, match(tag::semi));
}

/*
constant ::=
        {digit}1
        ' {char}12 '
        " {char}0 "
*/
bool parser::constant() {
  auto save{next_};
  return match(tag::numeric_constant) ||
         (next_ = save, match(tag::char_constant)) ||
         (next_ = save, match(tag::string_literal));
}

/*
rvalue ::=
        ( rvalue )
        lvalue
        constant
        lvalue assign rvalue
        inc-dec lvalue
        lvalue inc-dec
        unary rvalue
        & lvalue
        rvalue binary rvalue
        rvalue ? rvalue : rvalue
        rvalue ( {rvalue {, rvalue}0 }01 )
*/
bool parser::rvalue() {
  auto save{next_};
  return rvalue0() || (next_ = save, rvalue1()) || (next_ = save, rvalue2()) ||
         (next_ = save, rvalue3()) || (next_ = save, rvalue4()) ||
         (next_ = save, rvalue5()) || (next_ = save, rvalue6()) ||
         (next_ = save, rvalue7()) || (next_ = save, rvalue8()) ||
         (next_ = save, rvalue9()) || (next_ = save, rvalue10());
}

// ( rvalue )
bool parser::rvalue0() {
  return match(tag::l_paren) && rvalue() && match(tag::r_paren);
}

// lvalue
bool parser::rvalue1() { return lvalue(); }

// constant
bool parser::rvalue2() { return constant(); }

// lvalue assign rvalue
bool parser::rvalue3() { return lvalue() && assign() && rvalue(); }

// inc-dec lvalue
bool parser::rvalue4() { return inc_dec() && lvalue(); }

// lvalue inc-dec
bool parser::rvalue5() { return lvalue() && inc_dec(); }

// unary rvalue
bool parser::rvalue6() { return unary() && rvalue(); }

// & lvalue
bool parser::rvalue7() { return match(tag::amp) && lvalue(); }

// rvalue binary rvalue
bool parser::rvalue8() { return rvalue() && binary() && rvalue(); }

// rvalue ? rvalue : rvalue
bool parser::rvalue9() {
  return rvalue() && match(tag::question) && rvalue() && match(tag::colon) &&
         rvalue();
}

// rvalue ( {rvalue {, rvalue}0 }01 )
bool parser::rvalue10() {
  if (!rvalue() || !match(tag::l_paren))
    return false;

  if (rvalue())
    for (; match(tag::comma);)
      if (!rvalue())
        return false;
  --next_;

  return match(tag::r_paren);
}

/*
lvalue ::=
        name
        * rvalue
        rvalue [ rvalue ]
*/
bool parser::lvalue() {
  auto save{next_};
  return match(tag::name) || (next_ = save, match(tag::star) && rvalue()) ||
         (next_ = save,
          rvalue() && match(tag::l_square) && rvalue() && match(tag::r_square));
}

/*
assign ::=
        = {binary}01
*/
bool parser::assign() {
  if (!binary())
    --next_;
  return true;
}

/*
inc-dec ::=
        ++
        --
*/
bool parser::inc_dec() {
  auto save{next_};
  return match(tag::plusplus) || (next_ = save, match(tag::minusminus));
}

/*
unary ::=
        -
        !
*/
bool parser::unary() {
  auto save{next_};
  return match(tag::minus) || (next_ = save, match(tag::exclaim));
}

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
bool parser::binary() {
  auto save{next_};
  return match(tag::pipe) || (next_ = save, match(tag::amp)) ||
         (next_ = save, match(tag::equalequal)) ||
         (next_ = save, match(tag::exclaimequal)) ||
         (next_ = save, match(tag::less)) ||
         (next_ = save, match(tag::lessequal)) ||
         (next_ = save, match(tag::greater)) ||
         (next_ = save, match(tag::greaterequal)) ||
         (next_ = save, match(tag::lessless)) ||
         (next_ = save, match(tag::greatergreater)) ||
         (next_ = save, match(tag::minus)) ||
         (next_ = save, match(tag::plus)) ||
         (next_ = save, match(tag::percent)) ||
         (next_ = save, match(tag::star)) || (next_ = save, match(tag::slash));
}

// ival ::=
//	constant
//	name
bool parser::ival() { return constant() || match(tag::name); }
bool parser::match(tag _t) {
  assert(next_ != std::cend(tokens_) && "matching with index out of bounds.");
  return (*next_++).tag_ == _t;
}

bool parser::operator()() { return program(); }
} // namespace blang
