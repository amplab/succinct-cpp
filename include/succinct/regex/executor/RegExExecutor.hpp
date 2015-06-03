#ifndef REGEX_EXECUTOR_H
#define REGEX_EXECUTOR_H

#include "succinct/SuccinctCore.hpp"

#include <cstdint>
#include <set>
#include <algorithm>
#include <iterator>

class RegExExecutor {
protected:
    typedef std::pair<size_t, size_t> OffsetLength;
    typedef std::set<OffsetLength> RegExResult;
    typedef RegExResult::iterator RegExResultIterator;

    SuccinctCore *s_core;
    RegEx *re;
    std::set<OffsetLength> final_res;

public:
    RegExExecutor(SuccinctCore *s_core, RegEx *re) {
        this->s_core = s_core;
        this->re = re;
    }

    virtual ~RegExExecutor() {}

    virtual void execute() = 0;

    virtual void getResults(RegExResult &result) {
        result = final_res;
    }
};

class RegExExecutorBlackBox: public RegExExecutor {
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
    RegExExecutorBlackBox(SuccinctCore *s_core, RegEx *re): RegExExecutor(s_core, re) {
        this->blank = new RegExBlank();

        // Use length of file to represent all positions in the file, since
        // no offset will be >= length of file.
        this->all_pos = s_core->original_size();
    }

    ~RegExExecutorBlackBox() {
        delete blank;
    }

    void execute() {
        compute(regex_res, re);

        // Process final results
        std::vector<std::pair<size_t, size_t>> res;
        for(ResultIterator r_it = regex_res.begin(); r_it != regex_res.end(); r_it++) {
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
        std::string res = "";
        char *alphabet = s_core->getAlphabet();
        uint64_t idx = s_core->lookupISA(offset);
        for (uint64_t k = 0;  k < length; k++) {
            res += alphabet[s_core->lookupC(idx)];
            idx = s_core->lookupNPA(idx);
        }
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
        std::pair<int64_t, int64_t> range = s_core->bw_search(mgram);
        if(range.first > range.second) return;
        offsets.reserve((uint64_t)(range.second - range.first + 1));
        for(int64_t i = range.first; i <= range.second; i++) {
            offsets.push_back((int64_t)s_core->lookupSA(i));
        }
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

class RegExExecutorSuccinct: public RegExExecutor {
protected:
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
    RegExExecutorSuccinct(SuccinctCore *s_core, RegEx *re): RegExExecutor(s_core, re) {}

    size_t count() {
        size_t sum = 0;
        compute(regex_res, re);

        for(ResultIterator r_it = regex_res.begin(); r_it != regex_res.end(); r_it++) {
            Range range = r_it->range;
            if(!isEmpty(range))
                sum += (range.second - range.first + 1);
        }

        return sum;
    }

    void execute() {
        compute(regex_res, re);

        // Process final results
        std::map<int64_t, size_t> sa_buf;
        std::vector<std::pair<size_t, size_t>> res;
        for(ResultIterator r_it = regex_res.begin(); r_it != regex_res.end(); r_it++) {
            Range range = r_it->range;
            if(!isEmpty(range)) {
                for(int64_t i = range.first; i <= range.second; i++) {
                    if(sa_buf.find(i) == sa_buf.end()) {
                        sa_buf[i] = s_core->lookupSA(i);
                    }
                    res.push_back(OffsetLength(sa_buf[i], r_it->length));
                }
            }
        }
        final_res = std::set<std::pair<size_t, size_t>>(res.begin(), res.end());
    }

protected:
    virtual void compute(ResultSet &results, RegEx *r) = 0;

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
        return results.empty();
    }
};

#endif
