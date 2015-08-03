#ifndef REGEX_EXECUTOR_BWD
#define REGEX_EXECUTOR_BWD

#include "regex/executor/regex_executor.h"

class RegExExecutorBwd : public RegExExecutorSuccinct {
 public:
  RegExExecutorBwd(SuccinctCore *succinct_core, RegEx *regex)
      : RegExExecutorSuccinct(succinct_core, regex) {
  }

 private:
  void Compute(ResultSet &results, RegEx *regex) {
    switch (regex->GetType()) {
      case RegExType::BLANK: {
        break;
      }
      case RegExType::PRIMITIVE: {
        RegExPrimitive *p = (RegExPrimitive *) regex;
        switch (p->GetPrimitiveType()) {
          case RegExPrimitiveType::MGRAM: {
            Range range = succinct_core_->bw_search(p->GetPrimitive());
            if (!IsEmpty(range))
              results.insert(ResultEntry(range, p->GetPrimitive().length()));
            break;
          }
          case RegExPrimitiveType::DOT: {
            for (char c : std::string(succinct_core_->getAlphabet())) {
              if (c == '\n' || c == (char) 1)
                continue;
              Range range = succinct_core_->bw_search(std::string(1, c));
              if (!IsEmpty(range))
                results.insert(ResultEntry(range, 1));
            }
            break;
          }
          case RegExPrimitiveType::RANGE: {
            for (char c : p->GetPrimitive()) {
              Range range = succinct_core_->bw_search(std::string(1, c));
              if (!IsEmpty(range))
                results.insert(ResultEntry(range, 1));
            }
            break;
          }
        }
        break;
      }
      case RegExType::UNION: {
        ResultSet first_res, second_res;
        Compute(first_res, ((RegExUnion *) regex)->GetFirst());
        Compute(second_res, ((RegExUnion *) regex)->GetSecond());
        Union(results, first_res, second_res);
        break;
      }
      case RegExType::CONCAT: {
        ResultSet right_results;
        Compute(right_results, ((RegExConcat *) regex)->getRight());
        for (auto right_result : right_results) {
          ResultSet temp;
          Concat(temp, ((RegExConcat *) regex)->getLeft(), right_result);
          Union(results, results, temp);
        }
        break;
      }
      case RegExType::REPEAT: {
        RegExRepeat *rep_r = ((RegExRepeat *) regex);
        switch (rep_r->GetRepeatType()) {
          case RegExRepeatType::ZeroOrMore: {
            RepeatOneOrMore(results, rep_r->GetInternal());
            break;
          }
          case RegExRepeatType::OneOrMore: {
            RepeatOneOrMore(results, rep_r->GetInternal());
            break;
          }
          case RegExRepeatType::MinToMax: {
            RepeatMinToMax(results, rep_r->GetInternal(), rep_r->GetMin(),
                           rep_r->GetMax());
          }
        }
        break;
      }
    }
  }

  void Union(ResultSet &union_results, ResultSet first, ResultSet second) {
    std::set_union(first.begin(), first.end(), second.begin(), second.end(),
                   std::inserter(union_results, union_results.begin()), result_comparator);
  }

  void Concat(ResultSet &concat_results, RegEx *regex,
              ResultEntry right_result) {
    switch (regex->GetType()) {
      case RegExType::BLANK: {
        break;
      }
      case RegExType::PRIMITIVE: {
        RegExPrimitive *p = (RegExPrimitive *) regex;
        switch (p->GetPrimitiveType()) {
          case RegExPrimitiveType::MGRAM: {
            Range range = succinct_core_->continue_bw_search(
                p->GetPrimitive(), right_result.range_);
            if (!IsEmpty(range))
              concat_results.insert(
                  ResultEntry(
                      range, right_result.length_ + p->GetPrimitive().length()));
            break;
          }
          case RegExPrimitiveType::DOT: {
            for (char c : std::string(succinct_core_->getAlphabet())) {
              if (c == '\n' || c == (char) 1)
                continue;
              Range range = succinct_core_->continue_bw_search(
                  std::string(1, c), right_result.range_);
              if (!IsEmpty(range))
                concat_results.insert(
                    ResultEntry(range, right_result.length_ + 1));
            }
            break;
          }
          case RegExPrimitiveType::RANGE: {
            for (char c : p->GetPrimitive()) {
              Range range = succinct_core_->continue_bw_search(
                  std::string(1, c), right_result.range_);
              if (!IsEmpty(range))
                concat_results.insert(
                    ResultEntry(range, right_result.length_ + 1));
            }
            break;
          }
        }
        break;
      }
      case RegExType::UNION: {
        ResultSet res1, res2;
        Concat(res1, ((RegExUnion *) regex)->GetFirst(), right_result);
        Concat(res2, ((RegExUnion *) regex)->GetSecond(), right_result);
        Union(concat_results, res1, res2);
        break;
      }
      case RegExType::CONCAT: {
        ResultSet left_right_results;
        Concat(left_right_results, ((RegExConcat *) regex)->getRight(),
               right_result);
        for (auto left_right_result : left_right_results) {
          ResultSet temp;
          Concat(temp, ((RegExConcat *) regex)->getLeft(), left_right_result);
          Union(concat_results, concat_results, temp);
        }
        break;
      }
      case RegExType::REPEAT: {
        RegExRepeat *rep_r = ((RegExRepeat *) regex);
        switch (rep_r->GetRepeatType()) {
          case RegExRepeatType::ZeroOrMore: {
            RepeatOneOrMore(concat_results, rep_r->GetInternal(), right_result);
            concat_results.insert(right_result);
            break;
          }
          case RegExRepeatType::OneOrMore: {
            RepeatOneOrMore(concat_results, rep_r->GetInternal(), right_result);
            break;
          }
          case RegExRepeatType::MinToMax: {
            RepeatMinToMax(concat_results, rep_r->GetInternal(), right_result,
                           rep_r->GetMin(), rep_r->GetMax());
          }
        }
        break;
      }
    }
  }

  void RepeatOneOrMore(ResultSet &repeat_results, RegEx *regex) {
    ResultSet results;
    Compute(results, regex);
    if (results.empty())
      return;
    Union(repeat_results, repeat_results, results);

    for (auto result : results)
      RepeatOneOrMore(repeat_results, regex, result);
  }

  void RepeatOneOrMore(ResultSet &repeat_results, RegEx *regex,
                       ResultEntry right_result) {
    if (IsEmpty(right_result))
      return;

    ResultSet concat_results;
    Concat(concat_results, regex, right_result);
    if (concat_results.empty())
      return;
    Union(repeat_results, repeat_results, concat_results);

    for (auto result : concat_results)
      RepeatOneOrMore(repeat_results, regex, result);
  }

  void RepeatMinToMax(ResultSet &repeat_results, RegEx *regex, int min,
                      int max) {
    min = (min > 0) ? min - 1 : 0;
    max = (max > 0) ? max - 1 : 0;

    ResultSet results;
    Compute(results, regex);
    if (results.empty())
      return;

    if (!min)
      Union(repeat_results, repeat_results, results);

    if (max)
      for (auto result : results)
        RepeatMinToMax(repeat_results, regex, result, min, max);
  }

  void RepeatMinToMax(ResultSet &repeat_results, RegEx *regex,
                      ResultEntry right_result, int min, int max) {
    min = (min > 0) ? min - 1 : 0;
    max = (max > 0) ? max - 1 : 0;

    if (IsEmpty(right_result))
      return;

    ResultSet concat_results;
    Concat(concat_results, regex, right_result);
    if (concat_results.empty())
      return;

    if (!min)
      Union(repeat_results, repeat_results, concat_results);

    if (max)
      for (auto result : concat_results)
        RepeatMinToMax(repeat_results, regex, result, min, max);
  }
};

#endif
