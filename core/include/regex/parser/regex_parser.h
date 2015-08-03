#ifndef REGEX_PARSER_H
#define REGEX_PARSER_H

#include <iostream>
#include <cstring>

#include "regex/regex_types.h"

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
 public:
  RegExParser(char *exp) {
    this->exp_ = exp;
    this->blank_ = new RegExBlank();
  }

  ~RegExParser() {
    delete blank_;
  }

  RegEx *Parse() {
    return Regex();
  }

 private:
  char Peek() {
    return exp_[0];
  }

  void Eat(char c) {
    if (Peek() == c) {
      exp_ = exp_ + 1;
    } else {
      throw new ParseException;
    }
  }

  char Next() {
    char c = Peek();
    Eat(c);
    return c;
  }

  bool More() {
    return (strlen(exp_) > 0);
  }

  RegEx *Regex() {
    RegEx *t = Term();
    if (More() && Peek() == '|') {
      Eat('|');
      RegEx *r = Regex();
      return new RegExUnion(t, r);
    }
    return t;
  }

  RegEx *Concat(RegEx *a, RegEx *b) {
    if (a->GetType() == RegExType::BLANK) {
      return b;
    } else if (b->GetType() == RegExType::BLANK) {
      return a;
    } else if (a->GetType() == RegExType::PRIMITIVE
        && b->GetType() == RegExType::PRIMITIVE) {
      if (((RegExPrimitive *) a)->GetPrimitiveType()
          == RegExPrimitiveType::MGRAM
          && ((RegExPrimitive *) b)->GetPrimitiveType()
              == RegExPrimitiveType::MGRAM) {
        std::string a_str = ((RegExPrimitive *) a)->GetPrimitive();
        std::string b_str = ((RegExPrimitive *) b)->GetPrimitive();
        RegEx *ret = new RegExPrimitive(a_str + b_str);
        delete a;
        delete b;
        return ret;
      }
    }
    return new RegExConcat(a, b);
  }

  RegEx *Term() {
    RegEx *f = blank_;
    while (More() && Peek() != ')' && Peek() != '|') {
      RegEx *next_f = Factor();
      f = Concat(f, next_f);
    }
    return f;
  }

  RegEx *Factor() {
    RegEx *b = Base();

    if (More() && Peek() == '*') {
      Eat('*');
      b = new RegExRepeat(b, RegExRepeatType::ZeroOrMore);
    } else if (More() && Peek() == '+') {
      Eat('+');
      b = new RegExRepeat(b, RegExRepeatType::OneOrMore);
    } else if (More() && Peek() == '{') {
      Eat('{');
      int min = NextInt();
      Eat(',');
      int max = NextInt();
      Eat('}');
      b = new RegExRepeat(b, RegExRepeatType::MinToMax, min, max);
    }

    return b;
  }

  RegEx *Base() {
    if (Peek() == '(') {
      Eat('(');
      RegEx *r = Regex();
      Eat(')');
      return r;
    }
    return Primitive();
  }

  char NextChar() {
    if (Peek() == '\\') {
      Eat('\\');
    }
    return Next();
  }

  int NextInt() {
    int num = 0;
    while (Peek() >= 48 && Peek() <= 57) {
      num = num * 10 + (Next() - 48);
    }
    return num;
  }

  RegEx* Primitive() {
    std::string p = "";
    while (More() && Peek() != '|' && Peek() != '(' && Peek() != ')'
        && Peek() != '*' && Peek() != '+' && Peek() != '{' && Peek() != '}') {
      p += NextChar();
    }
    return ParsePrimitives(p);
  }

  RegEx* ParsePrimitives(std::string p_str) {
    RegEx* p = NextPrimitive(p_str);
    if (p_str.length() != 0) {
      return new RegExConcat(p, ParsePrimitives(p_str));
    }
    return p;
  }

  RegEx* NextPrimitive(std::string &p_str) {
    if (p_str[0] == '.') {
      p_str = p_str.substr(1);
      return new RegExPrimitive(".", RegExPrimitiveType::DOT);
    } else if (p_str[0] == '[') {
      size_t p_pos = 1;
      while (p_pos < p_str.length() && p_str[p_pos] != ']') {
        p_pos++;
      }
      if (p_pos == p_str.length())
        throw new ParseException;
      std::string str = p_str.substr(1, p_pos - 1);
      p_str = p_str.substr(p_pos + 1);
      return new RegExPrimitive(str, RegExPrimitiveType::RANGE);
    }
    size_t p_pos = 0;
    while (p_pos < p_str.length() && p_str[p_pos] != '.' && p_str[p_pos] != '['
        && p_str[p_pos] != ']') {
      p_pos++;
    }
    std::string str = p_str.substr(0, p_pos);
    p_str = p_str.substr(p_pos);
    return new RegExPrimitive(str, RegExPrimitiveType::MGRAM);
  }

  RegExPrimitiveType GetPrimitiveType(std::string mgram) {
    if (mgram == ".") {
      return RegExPrimitiveType::DOT;
    } else if (mgram[0] == '[') {
      if (mgram[mgram.length() - 1] != ']')
        throw new ParseException;
      return RegExPrimitiveType::RANGE;
    }
    return RegExPrimitiveType::MGRAM;
  }

  char *exp_;
  RegEx *blank_;
};

#endif
