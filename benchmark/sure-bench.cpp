#include "succinct/regex/RegEx.hpp"
#include "succinct/regex/RegExParser.hpp"
#include "succinct/regex/RegExPlanner.hpp"
#include "succinct/regex/RegExExecutor.hpp"

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
        fprintf(stderr, "[%s]", ((RegExPrimitive *)re)->getMgram().c_str());
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
    if(argc != 2) {
        fprintf(stderr, "Usage: %s [filename]\n", argv[0]);
        return -1;
    }

    std::string filename = std::string(argv[1]);

    char exp[100];
    std::cin >> exp;
    fprintf(stderr, "Regex:[%s]\n", exp);

    SuccinctFile *s_file = new SuccinctFile(filename);

    RegExParser parser(exp);
    RegEx *re = parser.parse();

    display(re);
    fprintf(stderr, "\n");

    RegExPlanner *planner = new NaiveRegExPlanner(s_file, re);
    RegEx *re_plan = planner->plan();

    RegExExecutor rex(s_file, re_plan);
    rex.execute();

    rex.displayResults();

    return 0;
}
