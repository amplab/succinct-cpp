#ifndef REGEX_EXECUTOR_H
#define REGEX_EXECUTOR_H

#include <cstdint>
#include <set>
#include <algorithm>
#include <iterator>

#include "regex/regex_types.h"
#include "succinct_core.h"

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

  virtual ~RegExExecutor() {
  }

  virtual void execute() = 0;

  virtual void getResults(RegExResult &result) {
    result = final_res;
  }
};

class RegExExecutorBlackBox : public RegExExecutor {
 private:

 public:
  RegExExecutorBlackBox(SuccinctCore *s_core, RegEx *re)
      : RegExExecutor(s_core, re) {
  }

  ~RegExExecutorBlackBox() {
  }

  void execute() {
    compute(final_res, re);
  }

 private:
  void compute(RegExResult &res, RegEx *r) {
    switch (r->getType()) {
      case RegExType::Blank: {
        fprintf(
            stderr,
            "RegEx blank is a place-holder and should not appear in the expression!\n");
        exit(0);
        break;
      }
      case RegExType::Primitive: {
        switch (((RegExPrimitive *) r)->getPrimitiveType()) {
          case RegExPrimitiveType::Mgram: {
            regexMgram(res, (RegExPrimitive *) r);
            break;
          }
          case RegExPrimitiveType::Range:
          case RegExPrimitiveType::Dot: {
            std::string primitive = ((RegExPrimitive *) r)->getPrimitive();
            if (primitive == ".") {
              primitive = std::string(s_core->getAlphabet());
            }
            for (auto c : primitive) {
              RegExPrimitive char_primitive(std::string(1, c));
              regexMgram(res, &char_primitive);
            }
            break;
          }
        }
        break;
      }
      case RegExType::Union: {
        RegExResult first_res, second_res;
        compute(first_res, ((RegExUnion *) r)->getFirst());
        compute(second_res, ((RegExUnion *) r)->getSecond());
        regexUnion(res, first_res, second_res);
        break;
      }
      case RegExType::Concat: {
        RegExResult first_res, second_res;
        compute(first_res, ((RegExConcat *) r)->getLeft());
        compute(second_res, ((RegExConcat *) r)->getRight());
        regexConcat(res, first_res, second_res);
        break;
      }
      case RegExType::Repeat: {
        RegExResult internal_res;
        compute(internal_res, ((RegExRepeat *) r)->getInternal());
        regexRepeat(res, internal_res, ((RegExRepeat *) r)->getRepeatType());
        break;
      }
    }
  }

  void regexMgram(RegExResult &mgram_res, RegExPrimitive *rp) {
    std::vector<int64_t> offsets;
    std::string mgram = rp->getPrimitive();
    size_t len = mgram.length();
    std::pair<int64_t, int64_t> range = s_core->bw_search(mgram);
    if (range.first > range.second)
      return;
    offsets.reserve((uint64_t) (range.second - range.first + 1));
    for (int64_t i = range.first; i <= range.second; i++) {
      offsets.push_back((int64_t) s_core->lookupSA(i));
    }
    for (auto offset : offsets)
      mgram_res.insert(OffsetLength(offset, len));
  }

  void regexUnion(RegExResult &union_res, RegExResult &a, RegExResult &b) {
    std::set_union(a.begin(), a.end(), b.begin(), b.end(),
                   std::inserter(union_res, union_res.begin()));
  }

  void regexConcat(RegExResult &concat_res, RegExResult &left,
                   RegExResult &right) {

    RegExResultIterator left_it, right_it;
    for (left_it = left.begin(), right_it = right.begin();
        left_it != left.end() && right_it != right.end(); left_it++) {
      while (right_it != right.end()
          && right_it->first < left_it->first + left_it->second)
        right_it++;
      if (right_it == right.end())
        break;

      if (right_it->first == left_it->first + left_it->second)
        concat_res.insert(
            OffsetLength(left_it->first, left_it->second + right_it->second));
    }
  }

  void regexRepeat(RegExResult &repeat_res, RegExResult &a,
                   RegExRepeatType r_type, int min = -1, int max = -1) {
    switch (r_type) {
      case RegExRepeatType::ZeroOrMore: {
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

        } while (concat_size);
        break;
      }
      case RegExRepeatType::OneOrMore: {
        // FIXME: .* is equivalent to .+ for now.
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

        } while (concat_size);
        break;
      }
      case RegExRepeatType::MinToMax: {
        size_t concat_size;
        RegExResult concat_res, min_res;
        min_res = concat_res = a;
        size_t num_repeats = 1;

        // Get to min repeats
        while (num_repeats < min) {
          RegExResult concat_temp_res;
          regexConcat(concat_temp_res, concat_res, a);
          concat_res = concat_temp_res;

          num_repeats++;

          if (concat_res.size() == 0)
            return;
        }

        do {
          RegExResult concat_temp_res;
          regexConcat(concat_temp_res, concat_res, a);
          concat_res = concat_temp_res;

          concat_size = concat_res.size();

          RegExResult repeat_temp_res;
          regexUnion(repeat_temp_res, repeat_res, concat_res);
          repeat_res = repeat_temp_res;

          num_repeats++;
        } while (concat_size && num_repeats < max);

        break;
      }
    }
  }
};

class RegExExecutorSuccinct : public RegExExecutor {
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
    bool operator()(const ResultEntry &lhs, const ResultEntry &rhs) {
      if (lhs.range == rhs.range)
        return lhs.length < rhs.length;
      return lhs.range < rhs.range;
    }
  } comp;
  typedef std::set<ResultEntry, ResultEntryComparator> ResultSet;
  typedef ResultSet::iterator ResultIterator;

  ResultSet regex_res;

 public:
  RegExExecutorSuccinct(SuccinctCore *s_core, RegEx *re)
      : RegExExecutor(s_core, re) {
  }

  size_t count() {
    size_t sum = 0;
    compute(regex_res, re);

    for (ResultIterator r_it = regex_res.begin(); r_it != regex_res.end();
        r_it++) {
      Range range = r_it->range;
      if (!isEmpty(range))
        sum += (range.second - range.first + 1);
    }

    return sum;
  }

  void execute() {
    compute(regex_res, re);

    // Process final results
    std::map<int64_t, size_t> sa_buf;
    std::vector<std::pair<size_t, size_t>> res;
    for (ResultIterator r_it = regex_res.begin(); r_it != regex_res.end();
        r_it++) {
      Range range = r_it->range;
      if (!isEmpty(range)) {
        for (int64_t i = range.first; i <= range.second; i++) {
          if (sa_buf.find(i) == sa_buf.end()) {
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
    for (auto result : results) {
      if (!isEmpty(result.range))
        return false;
    }
    return results.empty();
  }
};

#endif
