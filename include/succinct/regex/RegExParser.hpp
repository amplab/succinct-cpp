#ifndef REGEX_PARSER_H
#define REGEX_PARSER_H

#include "succinct/regex/RegEx.hpp"
#include <iostream>

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

class ParseException: public std::exception {
    virtual const char* what() const throw() {
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
        if(peek() == c) {
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
        return strlen(exp) > 0;
    }

    RegEx *regex() {
        RegEx *t = term();
        if(more() && peek() == '|') {
            eat('|');
            RegEx *r = regex();
            return new RegExOr(t, r);
        }
        return t;
    }

    RegEx *concat(RegEx *a, RegEx *b) {
        if(a->getType() == RegExType::Blank) {
            return b;
        } else if(a->getType() == RegExType::Primitive &&
                b->getType() == RegExType::Primitive) {
            std::string a_str = ((RegExPrimitive *)a)->getMgram();
            std::string b_str = ((RegExPrimitive *)b)->getMgram();
            RegEx *ret = new RegExPrimitive(a_str + b_str);
            delete a;
            delete b;
            return ret;
        }
        return new RegExConcat(a, b);
    }

    RegEx *term() {
        RegEx *f = blank;
        while(more() && peek() != ')' && peek() != '|') {
            RegEx *next_f = factor();
            f = concat(f, next_f);
        }
        return f;
    }

    RegEx *factor() {
        RegEx *b = base();

        if(more() && peek() == '*') {
            eat('*');
            b = new RegExRepeat(b, RegExRepeatType::ZeroOrMore);
        } else if(more() && peek() == '+') {
            eat('+');
            b = new RegExRepeat(b, RegExRepeatType::OneOrMore);
        } else if(more() && peek() == '{') {
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
        if(peek() == '(') {
            eat('(');
            RegEx *r = regex();
            eat(')');
            return r;
        }
        return mgram();
    }

    char nextChar() {
        if(peek() == '\\') {
            eat('\\');
        }
        return next();
    }

    int nextInt() {
        int num = 0;
        while(peek() >= 48 && peek() <= 57) {
            num = num * 10 + (next() - 48);
        }
        return num;
    }

    RegEx* mgram() {
        std::string m = "";
        while(more() && peek() != '|' && peek() != '(' &&
                peek() != ')' && peek() != '*' && peek() != '+' &&
                peek() != '{' && peek() != '}') {
            m += nextChar();
        }
        return new RegExPrimitive(m);
    }
};

#endif
