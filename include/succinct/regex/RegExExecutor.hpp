#ifndef REGEX_EXECUTOR_H
#define REGEX_EXECUTOR_H

#include "succinct/SuccinctFile.hpp"

#include <cstdint>

class RegExExecutor {
private:
    enum RegExResultType {
        Union,
        Concat,
        Repeat,
        Mgram
    };

    struct LengthsComparator {
        const std::vector<int64_t> & offsets;

        LengthsComparator(const std::vector<int64_t> & val_vec):
            offsets(val_vec) {}

        bool operator()(int i1, int i2)
        {
            return offsets[i1] < offsets[i2];
        }
    };

    typedef struct {
        std::vector<int64_t> offsets;
        std::vector<size_t> lengths;
        RegExResultType type;
    } RegExResult;

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

    void displayResults() {
        for(size_t i = 0; i < final_res.offsets.size(); i++) {
            fprintf(stderr, "%lld => %lu\n", final_res.offsets[i], final_res.lengths[i]);
        }
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
        s_file->search(mgram_res.offsets, rp->getMgram());
        mgram_res.lengths.resize(mgram_res.offsets.size(), rp->getMgram().length());
        mgram_res.type = RegExResultType::Mgram;
    }

    void regexUnion(RegExResult &union_res, RegExResult &a, RegExResult &b) {
        union_res.offsets.insert(union_res.offsets.end(), a.offsets.begin(), a.offsets.end());
        union_res.lengths.insert(union_res.lengths.end(), a.lengths.begin(), a.lengths.end());
        union_res.offsets.insert(union_res.offsets.end(), b.offsets.begin(), b.offsets.end());
        union_res.lengths.insert(union_res.lengths.end(), b.lengths.begin(), b.lengths.end());
        union_res.type = RegExResultType::Union;
    }

    void regexConcat(RegExResult &concat_res, RegExResult &a, RegExResult &b) {
        std::sort(a.lengths.begin(), a.lengths.end(), LengthsComparator(a.offsets));
        std::sort(a.offsets.begin(), a.offsets.end());
        std::sort(b.lengths.begin(), b.lengths.end(), LengthsComparator(b.offsets));
        std::sort(b.offsets.begin(), b.offsets.end());
        size_t b_pos = 0;
        for(size_t i = 0; i < a.offsets.size() && b_pos < b.offsets.size(); i++) {
            int64_t cur_a = a.offsets.at(i);
            size_t cur_a_len = a.lengths.at(i);
            while(b_pos < b.offsets.size() && b.offsets.at(b_pos) <= cur_a) b_pos++;
            if(b_pos == b.offsets.size()) break;
            if(b.offsets.at(b_pos) == cur_a + (int64_t)cur_a_len) {
                concat_res.offsets.push_back(cur_a);
                concat_res.lengths.push_back(cur_a_len + b.lengths.at(b_pos));
            }
        }
        concat_res.type = RegExResultType::Concat;
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

                concat_size = concat_res.offsets.size();

                // fprintf(stderr, "Size of intermediate result = %lu\n", concat_temp_res.offsets.size());

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
