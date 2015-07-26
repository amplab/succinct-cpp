#ifndef REGEX_PARSER_H
#define REGEX_PARSER_H

#include <iostream>
#include <cstring>

#include "../regex_types.h"

// Grammar:
//   <regex> ::= <term> '|' <regex>
//            |  <term>
//
//   <term> ::= { <factor> }
//
//   <factor> ::= <base> { '*' | '+' | '{' <num> ',' <num> ')' }
//
//   <base> ::= <mgram>
//            |  '(' <regex> ')'
//
//   <mgram> ::= <char> | '\' <char> { <mgram> }
//   <num> ::= <digit> { <num> }

class ParseException : public std::exception {
  virtual const char* what() const throw () {
    return std::string("Error parsing regular expression.").c_str();
  }
};

class RegExParser {
 private:
  char *exp;
  RegEx *blank;

 public:
  RegExParser(char *exp) {
    this->exp = exp;
    this->blank = new RegExBlank();
  }

  ~RegExParser() {
    delete blank;
  }

  RegEx *parse() {
    return regex();
  }

 private:
  char peek() {
    return exp[0];
  }

  void eat(char c) {
    if (peek() == c) {
      exp = exp + 1;
    } else {
      throw new ParseException;
    }
  }

  char next() {
    char c = peek();
    eat(c);
    return c;
  }

  bool more() {
    return (strlen(exp) > 0);
  }

  RegEx *regex() {
    RegEx *t = term();
    if (more() && peek() == '|') {
      eat('|');
      RegEx *r = regex();
      return new RegExUnion(t, r);
    }
    return t;
  }

  RegEx *concat(RegEx *a, RegEx *b) {
    if (a->getType() == RegExType::Blank) {
      return b;
    } else if (b->getType() == RegExType::Blank) {
      return a;
    } else if (a->getType() == RegExType::Primitive
        && b->getType() == RegExType::Primitive) {
      if (((RegExPrimitive *) a)->getPrimitiveType()
          == RegExPrimitiveType::Mgram
          && ((RegExPrimitive *) b)->getPrimitiveType()
              == RegExPrimitiveType::Mgram) {
        std::string a_str = ((RegExPrimitive *) a)->getPrimitive();
        std::string b_str = ((RegExPrimitive *) b)->getPrimitive();
        RegEx *ret = new RegExPrimitive(a_str + b_str);
        delete a;
        delete b;
        return ret;
      }
    }
    return new RegExConcat(a, b);
  }

  RegEx *term() {
    RegEx *f = blank;
    while (more() && peek() != ')' && peek() != '|') {
      RegEx *next_f = factor();
      f = concat(f, next_f);
    }
    return f;
  }

  RegEx *factor() {
    RegEx *b = base();

    if (more() && peek() == '*') {
      eat('*');
      b = new RegExRepeat(b, RegExRepeatType::ZeroOrMore);
    } else if (more() && peek() == '+') {
      eat('+');
      b = new RegExRepeat(b, RegExRepeatType::OneOrMore);
    } else if (more() && peek() == '{') {
      eat('{');
      int min = nextInt();
      eat(',');
      int max = nextInt();
      eat('}');
      b = new RegExRepeat(b, RegExRepeatType::MinToMax, min, max);
    }

    return b;
  }

  RegEx *base() {
    if (peek() == '(') {
      eat('(');
      RegEx *r = regex();
      eat(')');
      return r;
    }
    return primitive();
  }

  char nextChar() {
    if (peek() == '\\') {
      eat('\\');
    }
    return next();
  }

  int nextInt() {
    int num = 0;
    while (peek() >= 48 && peek() <= 57) {
      num = num * 10 + (next() - 48);
    }
    return num;
  }

  RegEx* primitive() {
    std::string p = "";
    while (more() && peek() != '|' && peek() != '(' && peek() != ')'
        && peek() != '*' && peek() != '+' && peek() != '{' && peek() != '}') {
      p += nextChar();
    }
    return parsePrimitives(p);
  }

  RegEx* parsePrimitives(std::string p_str) {
    RegEx* p = nextPrimitive(p_str);
    if (p_str.length() != 0) {
      return new RegExConcat(p, parsePrimitives(p_str));
    }
    return p;
  }

  RegEx* nextPrimitive(std::string &p_str) {
    if (p_str[0] == '.') {
      p_str = p_str.substr(1);
      return new RegExPrimitive(".", RegExPrimitiveType::Dot);
    } else if (p_str[0] == '[') {
      size_t p_pos = 1;
      while (p_pos < p_str.length() && p_str[p_pos] != ']') {
        p_pos++;
      }
      if (p_pos == p_str.length())
        throw new ParseException;
      std::string str = p_str.substr(1, p_pos - 1);
      p_str = p_str.substr(p_pos + 1);
      return new RegExPrimitive(str, RegExPrimitiveType::Range);
    }
    size_t p_pos = 0;
    while (p_pos < p_str.length() && p_str[p_pos] != '.' && p_str[p_pos] != '['
        && p_str[p_pos] != ']') {
      p_pos++;
    }
    std::string str = p_str.substr(0, p_pos);
    p_str = p_str.substr(p_pos);
    return new RegExPrimitive(str, RegExPrimitiveType::Mgram);
  }

  RegExPrimitiveType getPrimitiveType(std::string mgram) {
    if (mgram == ".") {
      return RegExPrimitiveType::Dot;
    } else if (mgram[0] == '[') {
      if (mgram[mgram.length() - 1] != ']')
        throw new ParseException;
      return RegExPrimitiveType::Range;
    }
    return RegExPrimitiveType::Mgram;
  }
};

#endif
