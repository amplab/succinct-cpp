  #ifndef REGEX_EXECUTOR_H
#define REGEX_EXECUTOR_H

#include <cstdint>
#include <set>
#include <algorithm>
#include <iterator>

#include "regex/regex_types.h"
#include "succinct_core.h"

class RegExExecutor {
 public:
  typedef std::pair<size_t, size_t> OffsetLength;
  typedef std::set<OffsetLength> RegExResult;
  typedef RegExResult::iterator RegExResultIterator;

  RegExExecutor(SuccinctCore *succinct_core, RegEx *regex) {
    this->succinct_core_ = succinct_core;
    this->regex_ = regex;
  }

  virtual ~RegExExecutor() {
  }

  virtual void Execute() = 0;

  virtual void getResults(RegExResult &results) {
    results = final_results_;
  }

 protected:
  SuccinctCore *succinct_core_;
  RegEx *regex_;
  std::set<OffsetLength> final_results_;

};

class RegExExecutorBlackBox : public RegExExecutor {
 public:
  RegExExecutorBlackBox(SuccinctCore *succinct_core, RegEx *regex)
      : RegExExecutor(succinct_core, regex) {
  }

  ~RegExExecutorBlackBox() {
  }

  void Execute() {
    Compute(final_results_, regex_);
  }

 private:
  void Compute(RegExResult &results, RegEx *regex) {
    switch (regex->GetType()) {
      case RegExType::BLANK: {
        fprintf(
            stderr,
            "RegEx blank is a place-holder and should not appear in the expression!\n");
        exit(0);
        break;
      }
      case RegExType::PRIMITIVE: {
        switch (((RegExPrimitive *) regex)->GetPrimitiveType()) {
          case RegExPrimitiveType::MGRAM: {
            MgramSearch(results, (RegExPrimitive *) regex);
            break;
          }
          case RegExPrimitiveType::RANGE:
          case RegExPrimitiveType::DOT: {
            std::string primitive = ((RegExPrimitive *) regex)->GetPrimitive();
            if (primitive == ".") {
              primitive = std::string(succinct_core_->getAlphabet());
            }
            for (auto c : primitive) {
              RegExPrimitive char_primitive(std::string(1, c));
              MgramSearch(results, &char_primitive);
            }
            break;
          }
        }
        break;
      }
      case RegExType::UNION: {
        RegExResult first_res, second_res;
        Compute(first_res, ((RegExUnion *) regex)->GetFirst());
        Compute(second_res, ((RegExUnion *) regex)->GetSecond());
        Union(results, first_res, second_res);
        break;
      }
      case RegExType::CONCAT: {
        RegExResult first_res, second_res;
        Compute(first_res, ((RegExConcat *) regex)->getLeft());
        Compute(second_res, ((RegExConcat *) regex)->getRight());
        Concat(results, first_res, second_res);
        break;
      }
      case RegExType::REPEAT: {
        RegExResult internal_res;
        Compute(internal_res, ((RegExRepeat *) regex)->GetInternal());
        Repeat(results, internal_res, ((RegExRepeat *) regex)->GetRepeatType());
        break;
      }
    }
  }

  void MgramSearch(RegExResult &mgram_results,
                   RegExPrimitive *regex_primitive) {
    std::vector<int64_t> offsets;
    std::string mgram = regex_primitive->GetPrimitive();
    size_t len = mgram.length();
    std::pair<int64_t, int64_t> range = succinct_core_->bw_search(mgram);
    if (range.first > range.second)
      return;
    offsets.reserve((uint64_t) (range.second - range.first + 1));
    for (int64_t i = range.first; i <= range.second; i++) {
      offsets.push_back((int64_t) succinct_core_->lookupSA(i));
    }
    for (auto offset : offsets)
      mgram_results.insert(OffsetLength(offset, len));
  }

  void Union(RegExResult &union_results, RegExResult &first,
             RegExResult &second) {
    std::set_union(first.begin(), first.end(), second.begin(), second.end(),
                   std::inserter(union_results, union_results.begin()));
  }

  void Concat(RegExResult &concat_results, RegExResult &left,
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
        concat_results.insert(
            OffsetLength(left_it->first, left_it->second + right_it->second));
    }
  }

  void Repeat(RegExResult &repeat_results, RegExResult &internal,
              RegExRepeatType repeat_type, int min = -1, int max = -1) {
    switch (repeat_type) {
      case RegExRepeatType::ZeroOrMore: {
        size_t concat_size;
        RegExResult concat_res;
        repeat_results = concat_res = internal;

        do {
          RegExResult concat_temp_res;
          Concat(concat_temp_res, concat_res, internal);
          concat_res = concat_temp_res;

          concat_size = concat_res.size();

          RegExResult repeat_temp_res;
          Union(repeat_temp_res, repeat_results, concat_res);
          repeat_results = repeat_temp_res;

        } while (concat_size);
        break;
      }
      case RegExRepeatType::OneOrMore: {
        // FIXME: .* is equivalent to .+ for now.
        size_t concat_size;
        RegExResult concat_res;
        repeat_results = concat_res = internal;

        do {
          RegExResult concat_temp_res;
          Concat(concat_temp_res, concat_res, internal);
          concat_res = concat_temp_res;

          concat_size = concat_res.size();

          RegExResult repeat_temp_res;
          Union(repeat_temp_res, repeat_results, concat_res);
          repeat_results = repeat_temp_res;

        } while (concat_size);
        break;
      }
      case RegExRepeatType::MinToMax: {
        size_t concat_size;
        RegExResult concat_res, min_res;
        min_res = concat_res = internal;
        size_t num_repeats = 1;

        // Get to min repeats
        while (num_repeats < min) {
          RegExResult concat_temp_res;
          Concat(concat_temp_res, concat_res, internal);
          concat_res = concat_temp_res;

          num_repeats++;

          if (concat_res.size() == 0)
            return;
        }

        do {
          RegExResult concat_temp_res;
          Concat(concat_temp_res, concat_res, internal);
          concat_res = concat_temp_res;

          concat_size = concat_res.size();

          RegExResult repeat_temp_res;
          Union(repeat_temp_res, repeat_results, concat_res);
          repeat_results = repeat_temp_res;

          num_repeats++;
        } while (concat_size && num_repeats < max);

        break;
      }
    }
  }
};

class RegExExecutorSuccinct : public RegExExecutor {
 public:
  typedef std::pair<int64_t, int64_t> Range;
  typedef struct ResultEntry {
    ResultEntry(Range range, size_t length) {
      range_ = range;
      length_ = length;
    }
    Range range_;
    size_t length_;
  } ResultEntry;
  struct ResultEntryComparator {
    bool operator()(const ResultEntry &lhs, const ResultEntry &rhs) {
      if (lhs.range_ == rhs.range_)
        return lhs.length_ < rhs.length_;
      return lhs.range_ < rhs.range_;
    }
  } result_comparator;
  typedef std::set<ResultEntry, ResultEntryComparator> ResultSet;
  typedef ResultSet::iterator ResultIterator;

  RegExExecutorSuccinct(SuccinctCore *succinct_core, RegEx *regex)
      : RegExExecutor(succinct_core, regex) {
  }

  size_t Count() {
    size_t sum = 0;
    Compute(regex_results_, regex_);

    for (ResultIterator r_it = regex_results_.begin();
        r_it != regex_results_.end(); r_it++) {
      Range range = r_it->range_;
      if (!IsEmpty(range))
        sum += (range.second - range.first + 1);
    }

    return sum;
  }

  void Execute() {
    Compute(regex_results_, regex_);

    // Process final results
    std::map<int64_t, size_t> sa_buf;
    std::vector<std::pair<size_t, size_t>> results;
    for (ResultIterator r_it = regex_results_.begin();
        r_it != regex_results_.end(); r_it++) {
      Range range = r_it->range_;
      if (!IsEmpty(range)) {
        for (int64_t i = range.first; i <= range.second; i++) {
          if (sa_buf.find(i) == sa_buf.end()) {
            sa_buf[i] = succinct_core_->lookupSA(i);
          }
          results.push_back(OffsetLength(sa_buf[i], r_it->length_));
        }
      }
    }
    final_results_ = std::set<std::pair<size_t, size_t>>(results.begin(),
                                                         results.end());
  }

 protected:
  virtual void Compute(ResultSet &results, RegEx *regex) = 0;

  bool IsEmpty(Range range) {
    return range.first > range.second;
  }

  bool IsEmpty(ResultEntry entry) {
    return IsEmpty(entry.range_);
  }

  bool IsEmpty(ResultSet results) {
    for (auto result : results) {
      if (!IsEmpty(result.range_))
        return false;
    }
    return results.empty();
  }

  ResultSet regex_results_;
};

#endif
