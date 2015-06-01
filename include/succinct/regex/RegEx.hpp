#ifndef REGEX_HPP
#define REGEX_HPP

#include <cstdio>

#include "succinct/regex/RegExTypes.hpp"
#include "succinct/regex/parser/RegExParser.hpp"
#include "succinct/regex/planner/RegExPlanner.hpp"
#include "succinct/regex/executor/RegExExecutor.hpp"

class SRegEx {
private:
    typedef std::pair<size_t, size_t> OffLen;
    typedef std::set<OffLen> RRes;
    typedef RRes::iterator RResIt;

    void explain_subexp(RegEx *re) {
        switch(re->getType()) {
        case RegExType::Blank:
        {
            fprintf(stderr, "<blank>");
            break;
        }
        case RegExType::Primitive:
        {
            RegExPrimitive *p = ((RegExPrimitive *)re);
            fprintf(stderr, "\"%s\"", p->getPrimitive().c_str());
            break;
        }
        case RegExType::Repeat:
        {
            fprintf(stderr, "REPEAT(");
            explain_subexp(((RegExRepeat *)re)->getInternal());
            fprintf(stderr, ")");
            break;
        }
        case RegExType::Concat:
        {
            fprintf(stderr, "(");
            explain_subexp(((RegExConcat *)re)->getLeft());
            fprintf(stderr, " CONCAT ");
            explain_subexp(((RegExConcat *)re)->getRight());
            fprintf(stderr, ")");
            break;
        }
        case RegExType::Union:
        {
            fprintf(stderr, "(");
            explain_subexp(((RegExUnion *)re)->getFirst());
            fprintf(stderr, " OR ");
            explain_subexp(((RegExUnion *)re)->getSecond());
            fprintf(stderr, ")");
            break;
        }
        }
    }

    void parse() {
        // TODO: Right now this assumes we don't have nested ".*" operators
        // It would be nice to allow .* anywhere.
        std::string delimiter = ".*";

        size_t pos = 0;
        std::string subexp;

        while ((pos = exp.find(delimiter)) != std::string::npos) {
            subexp = exp.substr(0, pos);
            RegExParser parser((char *)subexp.c_str());
            subexps.push_back(parser.parse());
            exp.erase(0, pos + delimiter.length());
        }

        subexp = exp;
        RegExParser parser((char *)subexp.c_str());
        subexps.push_back(parser.parse());
    }

public:
    SRegEx(std::string exp, SuccinctCore *s_core) {
        this->exp = exp;
        this->s_core = s_core;
        parse();
    }

    void execute() {
        std::vector<RRes> subresults;
        for(auto subexp: subexps) {
            RRes subresult;
            subquery(subresult, subexp);
            subresults.push_back(subresult);
        }

        while(subresults.size() != 1) {
            wildcard(subresults[0], subresults[1]);
            subresults.erase(subresults.begin());
        }

        r_results = subresults[0];
    }

    void wildcard(RRes &left, RRes &right) {
        RRes wildcard_res;
        RResIt left_it, right_it;
        for(left_it = left.begin(); left_it != left.end(); left_it++) {
            OffLen search_candidate(left_it->first + left_it->second, 0);
            RResIt first_entry = right.lower_bound(search_candidate);
            for(right_it = first_entry; right_it != right.end(); right_it++) {
                size_t offset = left_it->first;
                size_t length = right_it->first - left_it->first + right_it->second;
                wildcard_res.insert(OffLen(offset, length));
            }
        }
        right = wildcard_res;
    }

    void subquery(RRes &result, RegEx *r) {
        RegExExecutorOpt executor(s_core, r);
        executor.execute();
        executor.getResults(result);
    }

    void explain() {
        fprintf(stderr, "***");
        for(auto subexp: subexps) {
            explain_subexp(subexp);
            fprintf(stderr, "***");
        }
    }

    void show_results(size_t limit) {
        if(limit <= 0)
            limit = r_results.size();
        limit = MIN(limit, r_results.size());
        RResIt it;
        size_t i;
        fprintf(stderr, "Showing %zu of %zu results.\n", limit, r_results.size());
        fprintf(stderr, "{");
        for(it = r_results.begin(), i = 0; i < limit; i++, it++) {
            fprintf(stderr, "%zu => %zu, ", it->first, it->second);
        }
        fprintf(stderr, "...}\n");
    }

    void get_results(RRes &results) {
        results = r_results;
    }

private:
    std::string exp;
    std::vector<RegEx *> subexps;
    SuccinctCore *s_core;

    RRes r_results;
};


#endif
