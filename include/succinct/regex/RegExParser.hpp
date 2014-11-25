#ifndef PARSER_H
#define PARSER_H

#include "succinct/regex/RegEx.hpp"
#include <iostream>

// Grammar:
//   <regex> ::= <term> '|' <regex>
//            |  <term>
//
//   <term> ::= { <factor> }
//
//   <factor> ::= <base> { '*' | '+' }
//
//   <base> ::= <mgram>
//            |  '(' <regex> ')'
//
//   <mgram> ::= <char> | <char> { <mgram> }

class ParseException: public std::exception {
    virtual const char* what() const throw() {
        return std::string("Error parsing regular expression.").c_str();
    }
};

class RegExParser {
private:
    char *exp;

public:
    RegExParser(char *exp) {
        this->exp = exp;
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

    RegEx *term() {
        RegEx *f = new RegExBlank();
        while(more() && peek() != ')' && peek() != '|') {
            RegEx *next_f = factor();
            f = new RegExConcat(f, next_f);
        }
        return f;
    }

    RegEx *factor() {
        RegEx *b = base();

        if(more() && peek() == '*') {
            eat('*');
            b = new RegExRepeat(b, RepeatType::ZeroOrMore);
        } else if(more() && peek() == '+') {
            eat('+');
            b = new RegExRepeat(b, RepeatType::OneOrMore);
        }

        return b;
    }

    RegEx *base() {
        switch(peek()) {
        case '(':
        {
            eat('(');
            RegEx *r = regex();
            eat(')');
            return r;
        }
        case '\\':
        {
            eat('\\');
            char esc = next();
            return new RegExPrimitive(esc);
        }
        default:
        {
            return new RegExPrimitive(next());
        }
        }
    }
};

#endif
