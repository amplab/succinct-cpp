#ifndef REGEX_EXECUTOR_H
#define REGEX_EXECUTOR_H

#include "succinct/SuccinctFile.hpp"

#include <cstdint>
#include <set>
#include <algorithm>
#include <iterator>

class RegExExecutor {
protected:
    typedef std::pair<size_t, size_t> OffsetLength;
    typedef std::set<OffsetLength> RegExResult;
    typedef RegExResult::iterator RegExResultIterator;

    SuccinctFile *s_file;
    RegEx *re;
    std::set<OffsetLength> final_res;

public:
    RegExExecutor(SuccinctFile *s_file, RegEx *re) {
        this->s_file = s_file;
        this->re = re;
    }

    virtual ~RegExExecutor() {}

    virtual void execute() = 0;
    virtual void displayResults(size_t limit) {

        if(limit <= 0)
            limit = final_res.size();
        limit = MIN(limit, final_res.size());
        RegExResultIterator it;
        size_t i;
        fprintf(stderr, "Showing %zu of %zu results.\n", limit, final_res.size());
        fprintf(stderr, "{");
        for(it = final_res.begin(), i = 0; i < limit; i++, it++) {
            fprintf(stderr, "%zu => %zu, ", it->first, it->second);
        }
        fprintf(stderr, "...}\n");
    }

    virtual RegExResult getResults() {
        return final_res;
    }
};

class RegExExecutorNaive: public RegExExecutor {
private:

    typedef struct ResultEntry {
    public:
        ResultEntry(size_t offset, size_t length,
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

        size_t begin() const {
            return off_len.first;
        }

        size_t end() const {
            return off_len.first + off_len.second;
        }

        bool exactConcatCheck(ResultEntry right) const {
            return off_len.first + off_len.second == right.off_len.first;
        }

        bool wildcardConcatCheck(ResultEntry right) const {
            return off_len.first + off_len.second < right.off_len.first;
        }

        OffsetLength off_len;
        bool indef_beg;
        bool indef_end;
        std::string legal_chars;
    } ResultEntry;

    struct ResultEntryComparator {
        bool operator() (const ResultEntry &lhs, const ResultEntry &rhs) {
            if(lhs.off_len == rhs.off_len)
                return lhs.legal_chars < rhs.legal_chars;
            return lhs.off_len < rhs.off_len;
        }
    };

    ResultEntryComparator comp;
    typedef std::set<ResultEntry, ResultEntryComparator> ResultSet;
    typedef ResultSet::iterator ResultIterator;
    typedef ResultSet::reverse_iterator ReverseResultIterator;

    ResultSet regex_res;

    RegExBlank *blank;
    size_t all_pos;

public:
    RegExExecutorNaive(SuccinctFile *s_file, RegEx *re): RegExExecutor(s_file, re) {
        this->blank = new RegExBlank();

        // Use length of file to represent all positions in the file, since
        // no offset will be >= length of file.
        this->all_pos = s_file->original_size();
    }

    ~RegExExecutorNaive() {
        delete blank;
    }

    void execute() {
        compute(regex_res, re);

        // Process final results
        std::vector<std::pair<size_t, size_t>> res;
        for(ResultIterator r_it = regex_res.begin(); r_it != regex_res.end(); r_it++) {
            // fprintf(stderr, "%zu,%zu\n", r_it->off_len.first, r_it->off_len.second);
            if(r_it->off_len.first == all_pos) {
                for(size_t offset = 0; offset < all_pos; offset++) {
                    if(r_it->off_len.second && rangeCheck(r_it->legal_chars, offset, r_it->off_len.second))
                        res.push_back(std::pair<size_t, size_t>(offset, r_it->off_len.second));
                }
            } else {
                res.push_back(r_it->off_len);
            }
        }
        final_res = std::set<std::pair<size_t, size_t>>(res.begin(), res.end());
    }

private:

    inline bool isDotRes(ResultSet &res) {
        return res.size() == 1 && res.begin()->off_len.first == all_pos;
    }

    inline bool isDotResIndef(ResultSet &res) {
        return isDotRes(res) && res.begin()->indef_end;
    }

    inline bool isLegal(std::string legal_chars, char c) {
        return legal_chars.find(c) != std::string::npos;
    }

    inline bool rangeCheck(std::string legal_chars, size_t offset, size_t length) {
        if(legal_chars == "." || legal_chars == "") return true;
        std::string res;
        s_file->extract(res, offset, length);
        for(size_t pos = 0; pos < res.length(); pos++) {
            if(!isLegal(legal_chars, res[pos])) return false;
        }
        return true;
    }

    void compute(ResultSet &res, RegEx *r) {
        switch(r->getType()) {
        case RegExType::Blank:
        {
            res.insert(ResultEntry(all_pos, 0));
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
                res.insert(ResultEntry(all_pos, 1, false, false,
                        ((RegExPrimitive *)r)->getPrimitive()));
                break;
            }
            }
            break;
        }
        case RegExType::Union:
        {
            ResultSet first_res, second_res;
            compute(first_res, ((RegExUnion *)r)->getFirst());
            compute(second_res, ((RegExUnion *)r)->getSecond());
            regexUnion(res, first_res, second_res);
            break;
        }
        case RegExType::Concat:
        {
            ResultSet first_res, second_res;
            compute(first_res, ((RegExConcat *)r)->getLeft());
            compute(second_res, ((RegExConcat *)r)->getRight());
            regexConcat(res, first_res, second_res);
            break;
        }
        case RegExType::Repeat:
        {
            ResultSet internal_res;
            compute(internal_res, ((RegExRepeat *)r)->getInternal());
            regexRepeat(res, internal_res, ((RegExRepeat *)r)->getRepeatType());
            break;
        }
        }
    }

    void mgramSearch(ResultSet &mgram_res, RegExPrimitive *rp) {
        std::vector<int64_t> offsets;
        std::string mgram = rp->getPrimitive();
        size_t len = mgram.length();
        s_file->search(offsets, mgram);
        for(auto offset: offsets)
            mgram_res.insert(ResultEntry(offset, len));
    }

    void regexUnion(ResultSet &union_res, ResultSet &a, ResultSet &b) {
        std::set_union(a.begin(), a.end(), b.begin(), b.end(),
                std::inserter(union_res, union_res.begin()), comp);
    }

    void regexConcat(ResultSet &concat_res, ResultSet &a, ResultSet &b) {

        ResultIterator a_it = a.begin(), b_it = b.begin();

        if(isDotRes(a) && isDotRes(b)) {
            // TODO: Take care of character ranges
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
                    concat_res.insert(ResultEntry(b_it->off_len, true, false, b_it->legal_chars));
                return;
            }
            for(; b_it != b.end(); b_it++)
                if(rangeCheck(a_it->legal_chars, b_it->off_len.first - a_it->off_len.second,
                        a_it->off_len.second))
                    concat_res.insert(ResultEntry(b_it->off_len.first - a_it->off_len.second,
                            a_it->off_len.second + b_it->off_len.second));
            return;
        }

        if(isDotRes(b)) {
            if(isDotResIndef(b)) {
                for(; a_it != a.end(); a_it++)
                    concat_res.insert(ResultEntry(a_it->off_len, false, true, a_it->legal_chars));
                return;
            }
            for(; a_it != a.end(); a_it++) {
                if(rangeCheck(b_it->legal_chars, a_it->off_len.first + a_it->off_len.second,
                        b_it->off_len.second))
                    concat_res.insert(ResultEntry(a_it->off_len.first,
                            a_it->off_len.second + b_it->off_len.second));
            }
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

            bool first_indef_end = (a_it->indef_end && a_it->wildcardConcatCheck(*b_it) && rangeCheck(a_it->legal_chars, a_it->end(), (b_it->begin() - a_it->end())));
            bool second_indef_beg = (b_it->indef_beg && a_it->wildcardConcatCheck(*b_it) && rangeCheck(b_it->legal_chars, a_it->end(), (b_it->begin() - a_it->end())));
            if(a_it->exactConcatCheck(*b_it) || first_indef_end || second_indef_beg) {
                size_t pattern_offset = a_it->off_len.first;
                size_t pattern_length = (b_it->off_len.first - a_it->off_len.first) + b_it->off_len.second;
                concat_res.insert(ResultEntry(pattern_offset, pattern_length,
                        a_it->indef_beg, b_it->indef_end));
            }

            a_it++;
            b_it++;
        }

    }

    void regexRepeat(ResultSet &repeat_res, ResultSet &a, RegExRepeatType r_type, int min = -1, int max = -1) {
        switch(r_type) {
        case RegExRepeatType::ZeroOrMore:
        {
            size_t concat_size;
            ResultSet concat_res;
            if(a.begin()->off_len.first == all_pos) {
                repeat_res.insert(ResultEntry(all_pos, 1, false, true, a.begin()->legal_chars));
                return;
            }
            repeat_res = concat_res = a;

            do {
                ResultSet concat_temp_res;
                regexConcat(concat_temp_res, concat_res, a);
                concat_res = concat_temp_res;

                concat_size = concat_res.size();

                ResultSet repeat_temp_res;
                regexUnion(repeat_temp_res, repeat_res, concat_res);
                repeat_res = repeat_temp_res;

            } while(concat_size);

            repeat_res.insert(ResultEntry(all_pos, 0));

            break;
        }
        case RegExRepeatType::OneOrMore:
        {
            size_t concat_size;
            ResultSet concat_res;
            if(a.begin()->off_len.first == all_pos) {
                // FIXME: .* is equivalent to .+ for now.
                repeat_res.insert(ResultEntry(all_pos, 1, false, true, a.begin()->legal_chars));
                return;
            }
            repeat_res = concat_res = a;

            do {
                ResultSet concat_temp_res;
                regexConcat(concat_temp_res, concat_res, a);
                concat_res = concat_temp_res;

                concat_size = concat_res.size();

                ResultSet repeat_temp_res;
                regexUnion(repeat_temp_res, repeat_res, concat_res);
                repeat_res = repeat_temp_res;

            } while(concat_size);
            break;
        }
        case RegExRepeatType::MinToMax:
        {
            size_t concat_size;
            ResultSet concat_res, min_res;
            if(a.begin()->off_len.first == all_pos) {
                for(int i = min; i <= max; i++) {
                    repeat_res.insert(ResultEntry(all_pos, i, false, false, a.begin()->legal_chars));
                }
                return;
            }
            min_res = concat_res = a;
            size_t num_repeats = 1;

            // Get to min repeats
            while(num_repeats < min) {
                ResultSet concat_temp_res;
                regexConcat(concat_temp_res, concat_res, a);
                concat_res = concat_temp_res;

                num_repeats++;

                if(concat_res.size() == 0) return;
            }

            do {
                ResultSet concat_temp_res;
                regexConcat(concat_temp_res, concat_res, a);
                concat_res = concat_temp_res;

                concat_size = concat_res.size();

                ResultSet repeat_temp_res;
                regexUnion(repeat_temp_res, repeat_res, concat_res);
                repeat_res = repeat_temp_res;

                num_repeats++;
            } while(concat_size && num_repeats < max);

            if(min == 0) {
                repeat_res.insert(ResultEntry(all_pos, 0));
            }

            break;
        }
        }
    }
};

class RegExExecutorOpt: public RegExExecutor {
private:
    typedef std::pair<int64_t, int64_t> Range;
    typedef struct ResultEntry {
        ResultEntry(Range _range, size_t _length) {
            range = _range;
            length = _length;
        }
        Range range;
        size_t length;
    } ResultEntry;
    struct ResultEntryComparator {
        bool operator() (const ResultEntry &lhs, const ResultEntry &rhs) {
            if(lhs.range == rhs.range)
                return lhs.length < rhs.length;
            return lhs.range < rhs.range;
        }
    } comp;
    typedef std::set<ResultEntry, ResultEntryComparator> ResultSet;
    typedef ResultSet::iterator ResultIterator;

    ResultSet regex_res;

public:
    RegExExecutorOpt(SuccinctFile *s_file, RegEx *re): RegExExecutor(s_file, re) {}

    void execute() {
        compute(regex_res, re);

        // Process final results
        std::vector<std::pair<size_t, size_t>> res;
        for(ResultIterator r_it = regex_res.begin(); r_it != regex_res.end(); r_it++) {
            Range range = r_it->range;
            if(!isEmpty(range)) {
                for(int64_t i = range.first; i <= range.second; i++) {
                    res.push_back(OffsetLength(s_file->lookupSA(i), r_it->length));
                }
            }
        }
        final_res = std::set<std::pair<size_t, size_t>>(res.begin(), res.end());
    }

private:
    bool isEmpty(Range range) {
        return range.first > range.second;
    }

    bool isEmpty(ResultEntry entry) {
        return isEmpty(entry.range);
    }

    bool isEmpty(ResultSet results) {
        for(auto result: results) {
            if(!isEmpty(result.range)) return false;
        }
        return true;
    }

    void compute(ResultSet &results, RegEx *r) {
        switch(r->getType()) {
        case RegExType::Blank:
        {
            break;
        }
        case RegExType::Primitive:
        {
            RegExPrimitive *p = (RegExPrimitive *)r;
            switch(p->getPrimitiveType()) {
            case RegExPrimitiveType::Mgram:
            {
                Range range = s_file->bwSearch(p->getPrimitive());
                results.insert(ResultEntry(range, p->getPrimitive().length()));
                break;
            }
            case RegExPrimitiveType::Dot:
            {
                assert(0);
                break;
            }
            case RegExPrimitiveType::Range:
            {
                assert(0);
                break;
            }
            }
            break;
        }
        case RegExType::Union:
        {
            ResultSet first_res, second_res;
            compute(first_res, ((RegExUnion *)r)->getFirst());
            compute(second_res, ((RegExUnion *)r)->getSecond());
            regexUnion(results, first_res, second_res);
            break;
        }
        case RegExType::Concat:
        {
            ResultSet right_results;
            compute(right_results, ((RegExConcat *)r)->getRight());
            for(auto right_result: right_results) {
                ResultSet temp;
                regexConcat(temp, ((RegExConcat *)r)->getLeft(), right_result);
                regexUnion(results, results, temp);
            }
            break;
        }
        case RegExType::Repeat:
        {
            regexRepeat(results, ((RegExRepeat *)r)->getInternal());
            break;
        }
        }
    }

    void regexUnion(ResultSet &union_results, ResultSet a, ResultSet b) {
        std::set_union(a.begin(), a.end(), b.begin(), b.end(),
                std::inserter(union_results, union_results.begin()), comp);
    }

    void regexConcat(ResultSet &concat_results, RegEx *r, ResultEntry right_result) {
        switch(r->getType()) {
        case RegExType::Blank:
        {
            break;
        }
        case RegExType::Primitive:
        {
            RegExPrimitive *p = (RegExPrimitive *)r;
            switch(p->getPrimitiveType()) {
            case RegExPrimitiveType::Mgram:
            {
                Range range = s_file->continueBwSearch(p->getPrimitive(), right_result.range);
                concat_results.insert(ResultEntry(range, right_result.length + p->getPrimitive().length()));
                break;
            }
            case RegExPrimitiveType::Dot:
            {
                assert(0);
                break;
            }
            case RegExPrimitiveType::Range:
            {
                assert(0);
                break;
            }
            }
            break;
        }
        case RegExType::Union:
        {
            ResultSet res1, res2;
            regexConcat(res1, ((RegExUnion *)r)->getFirst(), right_result);
            regexConcat(res2, ((RegExUnion *)r)->getSecond(), right_result);
            regexUnion(concat_results, res1, res2);
            break;
        }
        case RegExType::Concat:
        {
            ResultSet left_right_results;
            regexConcat(left_right_results, ((RegExConcat *)r)->getRight(), right_result);
            for(auto left_right_result: left_right_results) {
                ResultSet temp;
                regexConcat(temp, ((RegExConcat *)r)->getLeft(), left_right_result);
                regexUnion(concat_results, concat_results, temp);
            }
            break;
        }
        case RegExType::Repeat:
        {
            regexRepeat(concat_results, ((RegExRepeat *)r)->getInternal(), right_result);
            break;
        }
        }
    }

    void regexRepeat(ResultSet &repeat_results, RegEx *r) {
        ResultSet results;
        compute(results, r);
        regexUnion(repeat_results, repeat_results, results);

        for(auto result: results)
            regexRepeat(repeat_results, r, result);
    }

    void regexRepeat(ResultSet &repeat_results, RegEx *r, ResultEntry right_result) {

        if(isEmpty(right_result)) return;

        ResultSet concat_results;
        regexConcat(concat_results, r, right_result);
        regexUnion(repeat_results, repeat_results, concat_results);

        for(auto result: concat_results)
            regexRepeat(repeat_results, r, result);
    }
};
#endif
