#include "succinct/regex/RegEx.hpp"
#include <cstdio>
#include <iostream>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

#include "succinct/regex/executor/RegExExecutor.hpp"
#include "succinct/regex/parser/RegExParser.hpp"
#include "succinct/regex/planner/RegExPlanner.hpp"

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
        RegExPrimitive *p = ((RegExPrimitive *)re);
        std::string p_type;
        switch(p->getPrimitiveType()) {
        case RegExPrimitiveType::Mgram:
        {
            p_type = "mgram";
            break;
        }
        case RegExPrimitiveType::Dot:
        {
            p_type = "dot";
            break;
        }
        case RegExPrimitiveType::Range:
        {
            p_type = "range";
            break;
        }
        default:
            p_type = "unknown";
        }
        fprintf(stderr, "\"%s\":%s", p->getPrimitive().c_str(), p_type.c_str());
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
        display(((RegExConcat *)re)->getLeft());
        fprintf(stderr, ",");
        display(((RegExConcat *)re)->getRight());
        fprintf(stderr, ")");
        break;
    }
    case RegExType::Union:
    {
        fprintf(stderr, "union(");
        display(((RegExUnion *)re)->getFirst());
        fprintf(stderr, ",");
        display(((RegExUnion *)re)->getSecond());
        fprintf(stderr, ")");
        break;
    }
    }
}

void print_usage(char *exec) {
    fprintf(stderr, "Usage: %s [-m mode] [file]\n", exec);
}

typedef unsigned long long int timestamp_t;

static timestamp_t get_timestamp() {
    struct timeval now;
    gettimeofday (&now, NULL);

    return (now.tv_usec + (time_t)now.tv_sec * 1000000) / 1000;
}

int main(int argc, char **argv) {
    if(argc < 2 || argc > 5) {
        print_usage(argv[0]);
        return -1;
    }

    int c;
    uint32_t mode = 0;
    bool opt = false;
    while((c = getopt(argc, argv, "m:o")) != -1) {
        switch(c) {
        case 'm':
            mode = atoi(optarg);
            break;
        case 'o':
            opt = true;
            break;
        default:
            mode = 0;
            opt = false;
        }
    }

    if(optind == argc) {
        print_usage(argv[0]);
        return -1;
    }

    std::string filename = std::string(argv[optind]);
    SuccinctFile *s_file = NULL;
    if(mode == 0) {
        std::cout << "Constructing Succinct data structures...\n";
        s_file = new SuccinctFile(filename);

        std::cout << "Serializing Succinct data structures...\n";
        std::ofstream s_out(filename + ".succinct");
        s_file->serialize();
        s_out.close();
    } else {
        std::cout << "De-serializing Succinct data structures...\n";
        s_file = new SuccinctFile(filename, SuccinctMode::LOAD_IN_MEMORY);
    }

    std::cout << "Done.\n";

    while(true) {
        char exp[100];
        std::cout << "sure> ";
        std::cin.getline(exp, sizeof(exp));


        timestamp_t start = get_timestamp();
        RegExParser parser(exp);
        RegEx *re = parser.parse();

        fprintf(stderr, "Regex: [%s]\nExplanation: [", exp);
        display(re);
        fprintf(stderr, "]\n");

        RegExPlanner *planner = new NaiveRegExPlanner(s_file, re);
        RegEx *re_plan = planner->plan();

        RegExExecutor *rex;
        if(opt) {
            rex = new RegExExecutorOpt(s_file, re_plan);
        } else {
            rex = new RegExExecutorNaive(s_file, re_plan);
        }
        rex->execute();

        timestamp_t tot_time = get_timestamp() - start;

        rex->displayResults(10);
        std::cout << "Query took " << tot_time << " ms\n";
    }

    return 0;
}
