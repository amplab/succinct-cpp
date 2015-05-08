#ifndef REGEX_EXECUTOR_H
#define REGEX_EXECUTOR_H

#include "succinct/SuccinctFile.hpp"

#include <cstdint>
#include <set>
#include <algorithm>
#include <iterator>

class RegExExecutor {
private:
    typedef std::pair<size_t, size_t> ResultEntry;
    typedef std::set<ResultEntry> RegExResult;

    SuccinctFile *s_file;
    RegEx *re;
    RegExResult final_res;

    RegExBlank *blank;

public:
    RegExExecutor(SuccinctFile *s_file, RegEx *re) {
        this->s_file = s_file;
        this->re = re;
        this->blank = new RegExBlank();
    }

    ~RegExExecutor() {
        delete blank;
    }

    void execute() {
        compute(final_res, re);
    }

    RegExResult getResults() {
        return final_res;
    }

    void displayResults(size_t limit) {
        fprintf(stderr, "{");
        if(limit <= 0)
            limit = final_res.size();
        limit = MIN(limit, final_res.size());
        RegExResult::iterator it;
        size_t i;
        for(it = final_res.begin(), i = 0; i < limit; i++, it++) {
            fprintf(stderr, "%zd => %zu, ", it->first, it->second);
        }
        fprintf(stderr, "...}\n");
    }

private:

    void compute(RegExResult &res, RegEx *r) {
        switch(r->getType()) {
        case RegExType::Blank:
        {
            // Do nothing
            break;
        }
        case RegExType::Primitive:
        {
            mgramSearch(res, (RegExPrimitive *)r);
            break;
        }
        case RegExType::Or:
        {
            RegExResult first_res, second_res;
            compute(first_res, ((RegExOr *)r)->getFirst());
            compute(second_res, ((RegExOr *)r)->getSecond());
            regexUnion(res, first_res, second_res);
            break;
        }
        case RegExType::Concat:
        {
            RegExResult first_res, second_res;
            compute(first_res, ((RegExConcat *)r)->getFirst());
            compute(second_res, ((RegExConcat *)r)->getSecond());
            regexConcat(res, first_res, second_res);
            break;
        }
        case RegExType::Repeat:
        {
            RegExResult internal_res;
            compute(internal_res, ((RegExRepeat *)r)->getInternal());
            regexRepeat(res, internal_res, ((RegExRepeat *)r)->getRepeatType());
            break;
        }
        }
    }

    void mgramSearch(RegExResult &mgram_res, RegExPrimitive *rp) {
        std::vector<int64_t> offsets;
        std::string mgram = rp->getPrimitive();
        size_t len = mgram.length();
        s_file->search(offsets, mgram);
        for(auto offset: offsets)
            mgram_res.insert(ResultEntry(offset, len));
    }

    void regexUnion(RegExResult &union_res, RegExResult &a, RegExResult &b) {
        std::set_union(a.begin(), a.end(), b.begin(), b.end(),
                std::inserter(union_res, union_res.begin()));
    }

    void regexConcat(RegExResult &concat_res, RegExResult &a, RegExResult &b) {
        typedef RegExResult::iterator res_it;
        res_it a_it, b_it;
        for(a_it = a.begin(), b_it = b.begin(); a_it != a.end() && b_it != b.end(); a_it++) {
            while(b_it != b.end() && b_it->first <= a_it->first) b_it++;
            if(b_it == b.end()) break;

            if(b_it->first == a_it->first + a_it->second) {
                concat_res.insert(ResultEntry(a_it->first, a_it->second + b_it->second));
            }
        }
    }

    void regexRepeat(RegExResult &repeat_res, RegExResult &a, RegExRepeatType r_type) {
        switch(r_type) {
        case RegExRepeatType::ZeroOrMore:
        {
            // TODO: Support
            break;
        }
        case RegExRepeatType::OneOrMore:
        {
            size_t concat_size;
            RegExResult concat_res;
            repeat_res = concat_res = a;

            do {
                RegExResult concat_temp_res;
                regexConcat(concat_temp_res, concat_res, a);
                concat_res = concat_temp_res;

                concat_size = concat_res.size();

                RegExResult repeat_temp_res;
                regexUnion(repeat_temp_res, repeat_res, concat_res);
                repeat_res = repeat_temp_res;

            } while(concat_size);
            break;
        }
        case RegExRepeatType::MinToMax:
        {
            // TODO: Support
            break;
        }
        }
    }

};
#endif
