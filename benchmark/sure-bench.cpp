#include "succinct/regex/RegEx.hpp"
#include "succinct/regex/RegExParser.hpp"

#include <cstdio>
#include <iostream>

// Debug
void display(RegEx *re) {
    switch(re->getType()) {
    case RegExType::Blank:
    {
        fprintf(stderr, "<blank>");
        break;
    }
    case RegExType::Primitive:
    {
        fprintf(stderr, "[%c]", ((RegExPrimitive *)re)->getCharacter());
        break;
    }
    case RegExType::Repeat:
    {
        fprintf(stderr, "repeat(");
        display(((RegExRepeat *)re)->getInternal());
        fprintf(stderr, ")");
        break;
    }
    case RegExType::Concat:
    {
        fprintf(stderr, "concat(");
        display(((RegExConcat *)re)->getFirst());
        fprintf(stderr, ",");
        display(((RegExConcat *)re)->getSecond());
        fprintf(stderr, ")");
        break;
    }
    case RegExType::Or:
    {
        fprintf(stderr, "or(");
        display(((RegExOr *)re)->getFirst());
        fprintf(stderr, ",");
        display(((RegExOr *)re)->getSecond());
        fprintf(stderr, ")");
        break;
    }
    }
}

int main(int argc, char **argv) {
    if(argc != 1) {
        fprintf(stderr, "Usage: %s\n", argv[0]);
        return -1;
    }

    char exp[100];
    std::cin >> exp;
    fprintf(stderr, "Regex:[%s]\n", exp);

    RegExParser parser(exp);
    RegEx *re = parser.parse();

    display(re);
    fprintf(stderr, "\n");

    return 0;
}
