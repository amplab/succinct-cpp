#ifndef REGEX_EXECUTOR_FWD
#define REGEX_EXECUTOR_FWD

#include "regex/executor/regex_executor.h"

class RegExExecutorFwd : public RegExExecutorSuccinct {
 public:
  RegExExecutorFwd(SuccinctCore *succinct_core, RegEx *regex)
      : RegExExecutorSuccinct(succinct_core, regex) {
  }

 private:
  void Compute(ResultSet &results, RegEx *regex) {
    switch (regex->GetType()) {
      case RegExType::BLANK: {
        break;
      }
      case RegExType::PRIMITIVE: {
        RegExPrimitive *primitive = (RegExPrimitive *) regex;
        switch (primitive->GetPrimitiveType()) {
          case RegExPrimitiveType::MGRAM: {
            Range range = succinct_core_->FwdSearch(primitive->GetPrimitive());
            if (!IsEmpty(range))
              results.insert(
                  ResultEntry(range, primitive->GetPrimitive().length()));
            break;
          }
          case RegExPrimitiveType::DOT: {
            for (char c : std::string(succinct_core_->GetAlphabet())) {
              if (c == '\n' || c == (char) 1)
                continue;
              Range range = succinct_core_->FwdSearch(std::string(1, c));
              if (!IsEmpty(range))
                results.insert(ResultEntry(range, 1));
            }
            break;
          }
          case RegExPrimitiveType::RANGE: {
            for (char c : primitive->GetPrimitive()) {
              Range range = succinct_core_->FwdSearch(std::string(1, c));
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
        ResultSet left_results;
        Compute(left_results, ((RegExConcat *) regex)->getLeft());
        for (auto left_result : left_results) {
          ResultSet temp;
          Concat(temp, ((RegExConcat *) regex)->getRight(), left_result);
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
                   std::inserter(union_results, union_results.begin()),
                   result_comparator);
  }

  void Concat(ResultSet &concat_results, RegEx *r, ResultEntry left_result) {
    switch (r->GetType()) {
      case RegExType::BLANK: {
        break;
      }
      case RegExType::PRIMITIVE: {
        RegExPrimitive *p = (RegExPrimitive *) r;
        switch (p->GetPrimitiveType()) {
          case RegExPrimitiveType::MGRAM: {
            Range range = succinct_core_->ContinueFwdSearch(
                p->GetPrimitive(), left_result.range_, left_result.length_);
            if (!IsEmpty(range))
              concat_results.insert(
                  ResultEntry(
                      range, left_result.length_ + p->GetPrimitive().length()));
            break;
          }
          case RegExPrimitiveType::DOT: {
            for (char c : std::string(succinct_core_->GetAlphabet())) {
              if (c == '\n' || c == (char) 1)
                continue;
              Range range = succinct_core_->ContinueFwdSearch(
                  std::string(1, c), left_result.range_, left_result.length_);
              if (!IsEmpty(range))
                concat_results.insert(
                    ResultEntry(range, left_result.length_ + 1));
            }
            break;
          }
          case RegExPrimitiveType::RANGE: {
            for (char c : p->GetPrimitive()) {
              Range range = succinct_core_->ContinueFwdSearch(
                  std::string(1, c), left_result.range_, left_result.length_);
              if (!IsEmpty(range))
                concat_results.insert(
                    ResultEntry(range, left_result.length_ + 1));
            }
            break;
          }
        }
        break;
      }
      case RegExType::UNION: {
        ResultSet res1, res2;
        Concat(res1, ((RegExUnion *) r)->GetFirst(), left_result);
        Concat(res2, ((RegExUnion *) r)->GetSecond(), left_result);
        Union(concat_results, res1, res2);
        break;
      }
      case RegExType::CONCAT: {
        ResultSet right_left_results;
        Concat(right_left_results, ((RegExConcat *) r)->getLeft(), left_result);
        for (auto right_left_result : right_left_results) {
          ResultSet temp;
          Concat(temp, ((RegExConcat *) r)->getRight(), right_left_result);
          Union(concat_results, concat_results, temp);
        }
        break;
      }
      case RegExType::REPEAT: {
        RegExRepeat *rep_r = ((RegExRepeat *) r);
        switch (rep_r->GetRepeatType()) {
          case RegExRepeatType::ZeroOrMore: {
            RepeatOneOrMore(concat_results, rep_r->GetInternal(), left_result);
            concat_results.insert(left_result);
            break;
          }
          case RegExRepeatType::OneOrMore: {
            RepeatOneOrMore(concat_results, rep_r->GetInternal(), left_result);
            break;
          }
          case RegExRepeatType::MinToMax: {
            RepeatMinToMax(concat_results, rep_r->GetInternal(), left_result,
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
                       ResultEntry left_result) {
    if (IsEmpty(left_result))
      return;

    ResultSet concat_results;
    Concat(concat_results, regex, left_result);
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
                      ResultEntry left_result, int min, int max) {
    min = (min > 0) ? min - 1 : 0;
    max = (max > 0) ? max - 1 : 0;

    if (IsEmpty(left_result))
      return;

    ResultSet concat_results;
    Concat(concat_results, regex, left_result);
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
