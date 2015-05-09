#ifndef REGEX_EXECUTOR_H
#define REGEX_EXECUTOR_H

#include "succinct/SuccinctFile.hpp"

#include <cstdint>
#include <set>
#include <algorithm>
#include <iterator>

class RegExExecutor {
private:
    typedef std::pair<int64_t, int64_t> OffsetLength;
    typedef struct ResultEntry {
        ResultEntry(int64_t offset, int64_t length,
                bool _indef_beg = false, bool _indef_end = false, std::string _legal_chars = "") {
            off_len = OffsetLength(offset, length);
            indef_beg = _indef_beg;
            indef_end = _indef_end;
            legal_chars = _legal_chars;
        }

        ResultEntry(OffsetLength _off_len,
                bool _indef_beg = false, bool _indef_end = false, std::string _legal_chars = "") {
            off_len = _off_len;
            indef_beg = _indef_beg;
            indef_end = _indef_end;
            legal_chars = _legal_chars;
        }

        OffsetLength off_len;
        bool indef_beg;
        bool indef_end;
        std::string legal_chars;
    } ResultEntry;
    struct ResultEntryComparator {
        bool operator() (const ResultEntry &lhs, const ResultEntry &rhs) {
            return lhs.off_len < rhs.off_len;
        }
    };
    ResultEntryComparator comp;
    typedef std::set<ResultEntry, ResultEntryComparator> RegExResult;
    typedef RegExResult::iterator ResultIterator;

    SuccinctFile *s_file;
    RegEx *re;
    RegExResult final_res;

    RegExBlank *blank;
    int64_t all_pos;

public:
    RegExExecutor(SuccinctFile *s_file, RegEx *re) {
        this->s_file = s_file;
        this->re = re;
        this->blank = new RegExBlank();

        // Use length of file to represent all positions in the file, since
        // no offset will be >= length of file.
        this->all_pos = s_file->original_size();
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
        ResultIterator it;
        size_t i;
        for(it = final_res.begin(), i = 0; i < limit; i++, it++) {
            fprintf(stderr, "%lld => %lld, ", it->off_len.first, it->off_len.second);
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
            switch(((RegExPrimitive *)r)->getPrimitiveType()) {
            case RegExPrimitiveType::Mgram:
            {
                mgramSearch(res, (RegExPrimitive *)r);
                break;
            }
            case RegExPrimitiveType::Range:
            case RegExPrimitiveType::Dot:
            {
                res.insert(ResultEntry(all_pos, 1));
                break;
            }
            }
            break;
        }
        case RegExType::Union:
        {
            RegExResult first_res, second_res;
            compute(first_res, ((RegExUnion *)r)->getFirst());
            compute(second_res, ((RegExUnion *)r)->getSecond());
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

    bool isDotRes(RegExResult &res) {
        return res.size() == 1 && res.begin()->off_len.first == all_pos;
    }

    bool isDotResIndef(RegExResult &res) {
        return isDotRes(res) && res.begin()->indef_end;
    }

    void regexUnion(RegExResult &union_res, RegExResult &a, RegExResult &b) {
        if(isDotResIndef(a) && isDotResIndef(b)) {
            union_res.insert(ResultEntry(all_pos, 0, false, true));
            return;
        }
        if(isDotResIndef(a)) {
            union_res = a;
            return;
        }
        if(isDotResIndef(b)) {
            union_res = b;
            return;
        }
        std::set_union(a.begin(), a.end(), b.begin(), b.end(),
                std::inserter(union_res, union_res.begin()), comp);
    }

    void regexConcat(RegExResult &concat_res, RegExResult &a, RegExResult &b) {
        ResultIterator a_it = a.begin(), b_it = b.begin();
        if(isDotRes(a) && isDotRes(b)) {
            if(isDotResIndef(a) || isDotResIndef(b)) {
                concat_res.insert(ResultEntry(all_pos, 0, false, true));
                return;
            }
            size_t dot_len = a_it->off_len.second + b_it->off_len.second;
            concat_res.insert(ResultEntry(all_pos, dot_len));
            return;
        }
        if(isDotRes(a)) {
            if(isDotResIndef(a)) {
                for(; b_it != b.end(); b_it++)
                    concat_res.insert(ResultEntry(b_it->off_len, true, false));
                return;
            }
            for(; b_it != b.end(); b_it++)
                concat_res.insert(ResultEntry(b_it->off_len.first - a_it->off_len.second,
                        a_it->off_len.second + b_it->off_len.second));
            return;
        }
        if(isDotRes(b)) {
            if(isDotResIndef(b)) {
                for(; a_it != a.end(); a_it++)
                    concat_res.insert(ResultEntry(a_it->off_len, false, true));
                return;
            }
            for(; a_it != a.end(); a_it++)
                concat_res.insert(ResultEntry(a_it->off_len.first,
                        a_it->off_len.second + b_it->off_len.second));
            return;
        }
        while(a_it != a.end() && b_it != b.end()) {
            ResultIterator first = a_it;
            while(first != a.end() && first->off_len.first <= b_it->off_len.first) {
                a_it = first; first++;
            }
            if(a_it == a.end()) break;
            while(b_it != b.end() && b_it->off_len.first <= a_it->off_len.first) {
                b_it++;
            }
            if(b_it == b.end()) break;

            bool reg_concat = b_it->off_len.first == (a_it->off_len.first + a_it->off_len.second);
            bool indef_concat = b_it->off_len.first >= (a_it->off_len.first + a_it->off_len.second);
            bool first_indef_end = (a_it->indef_end && indef_concat);
            bool second_indef_beg = (b_it->indef_beg && indef_concat);
            if(reg_concat || first_indef_end || second_indef_beg) {
                int64_t pattern_offset = a_it->off_len.first;
                int64_t pattern_length = (b_it->off_len.first - a_it->off_len.first) + b_it->off_len.second;
                concat_res.insert(ResultEntry(pattern_offset, pattern_length,
                        a_it->indef_beg, b_it->indef_end));
            }
            a_it++;
            b_it++;
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
            if(a.begin()->off_len.first == all_pos) {
                repeat_res.insert(ResultEntry(all_pos, 0, false, true));
                return;
            }
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
