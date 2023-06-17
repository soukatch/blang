#include "parser.h"

#include <vector>

namespace blang {

parser::parser(const std::vector<token> &_v) : v_{_v} {}
parser::parser(std::vector<token> &&_v) noexcept : v_{std::move(_v)} {}

// Should the rollback be implemented by the match/sub bodies? probably not...

// program := {definition}_0
bool parser::program() {
  for (; next_ < v_.size();)
    if (!definition())
      return false;
  return true;
}

// definition ::=
//  name {[ {constant}01 ]}01 {ival {, ival}0}01 ;
//	name ( {name {, name}0}01 ) statement
bool parser::definition() {
  int save{next_};
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
  if (!match(tag::name) || !match(tag::semi))
    return false;

  if (match(tag::name)) {
    for (; match(tag::comma);)
      if (!match(tag::name))
        return false;
  }

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
  int save{next_};
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

  constant();

  for (; match(tag::comma); constant())
    if (!match(tag::name))
      return false;

  return match(tag::comma) && statement();
}

// extrn name {, name}0 ; statement
bool parser::statement1() {
  if (!match(tag::extrn_) || !match(tag::name))
    return false;

  for (; match(tag::comma);)
    if (!match(tag::name))
      return false;

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

  return match(tag::r_brace);
}

// if ( rvalue ) statement {else statement}01
bool parser::statement5() {
  if (match(tag::if_) && match(tag::l_paren) && rvalue() &&
      match(tag::r_paren) && statement())
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
    if (!rvalue())
      return false;
    if (!match(tag::r_paren))
      return false;
  }

  return true;
}

// {rvalue}01 ;
bool parser::statement10() { return rvalue() || match(tag::semi); }

/*
constant ::=
        {digit}1
        ' {char}12 '
        " {char}0 "
*/
bool parser::constant() {
  return match(tag::numeric_constant) || match(tag::char_constant) ||
         match(tag::string_literal);
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
  int save{next_};
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

  return match(tag::r_paren);
}

/*
lvalue ::=
        name
        * rvalue
        rvalue [ rvalue ]
*/
bool parser::lvalue() {
  return match(tag::name) || (match(tag::star) && rvalue()) ||
         (rvalue() && match(tag::l_square) && rvalue() && match(tag::r_square));
}

/*
assign ::=
        = {binary}01
*/
bool parser::assign() { return binary() || true; }

/*
inc-dec ::=
        ++
        --
*/
bool parser::inc_dec() {
  return match(tag::plusplus) || match(tag::minusminus);
}

/*
unary ::=
        -
        !
*/
bool parser::unary() { return match(tag::minus) || match(tag::exclaim); }

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
  return match(tag::pipe) || match(tag::amp) || match(tag::equalequal) ||
         match(tag::exclaimequal) || match(tag::less) ||
         match(tag::lessequal) || match(tag::greater) ||
         match(tag::greaterequal) || match(tag::lessless) ||
         match(tag::greatergreater) || match(tag::minus) || match(tag::plus) ||
         match(tag::percent) || match(tag::star) || match(tag::slash);
}

// ival ::=
//	constant
//	name
bool parser::ival() { return constant() || match(tag::name); }
bool parser::match(tag _t) { return v_[next_++].tag_ == _t; }

bool parser::operator()() { return program(); }
} // namespace blang
