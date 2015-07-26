#ifndef REGEX_EXECUTOR_BWD
#define REGEX_EXECUTOR_BWD

#include "regex/executor/RegExExecutor.hpp"

class RegExExecutorBwd : public RegExExecutorSuccinct {
 public:
  RegExExecutorBwd(SuccinctCore *s_core, RegEx *re)
      : RegExExecutorSuccinct(s_core, re) {
  }

 private:
  void compute(ResultSet &results, RegEx *r) {
    switch (r->getType()) {
      case RegExType::Blank: {
        break;
      }
      case RegExType::Primitive: {
        RegExPrimitive *p = (RegExPrimitive *) r;
        switch (p->getPrimitiveType()) {
          case RegExPrimitiveType::Mgram: {
            Range range = s_core->bw_search(p->getPrimitive());
            if (!isEmpty(range))
              results.insert(ResultEntry(range, p->getPrimitive().length()));
            break;
          }
          case RegExPrimitiveType::Dot: {
            for (char c : std::string(s_core->getAlphabet())) {
              if (c == '\n' || c == (char) 1)
                continue;
              Range range = s_core->bw_search(std::string(1, c));
              if (!isEmpty(range))
                results.insert(ResultEntry(range, 1));
            }
            break;
          }
          case RegExPrimitiveType::Range: {
            for (char c : p->getPrimitive()) {
              Range range = s_core->bw_search(std::string(1, c));
              if (!isEmpty(range))
                results.insert(ResultEntry(range, 1));
            }
            break;
          }
        }
        break;
      }
      case RegExType::Union: {
        ResultSet first_res, second_res;
        compute(first_res, ((RegExUnion *) r)->getFirst());
        compute(second_res, ((RegExUnion *) r)->getSecond());
        regexUnion(results, first_res, second_res);
        break;
      }
      case RegExType::Concat: {
        ResultSet right_results;
        compute(right_results, ((RegExConcat *) r)->getRight());
        for (auto right_result : right_results) {
          ResultSet temp;
          regexConcat(temp, ((RegExConcat *) r)->getLeft(), right_result);
          regexUnion(results, results, temp);
        }
        break;
      }
      case RegExType::Repeat: {
        RegExRepeat *rep_r = ((RegExRepeat *) r);
        switch (rep_r->getRepeatType()) {
          case RegExRepeatType::ZeroOrMore: {
            regexRepeatOneOrMore(results, rep_r->getInternal());
            break;
          }
          case RegExRepeatType::OneOrMore: {
            regexRepeatOneOrMore(results, rep_r->getInternal());
            break;
          }
          case RegExRepeatType::MinToMax: {
            regexRepeatMinToMax(results, rep_r->getInternal(), rep_r->getMin(),
                                rep_r->getMax());
          }
        }
        break;
      }
    }
  }

  void regexUnion(ResultSet &union_results, ResultSet a, ResultSet b) {
    std::set_union(a.begin(), a.end(), b.begin(), b.end(),
                   std::inserter(union_results, union_results.begin()), comp);
  }

  void regexConcat(ResultSet &concat_results, RegEx *r,
                   ResultEntry right_result) {
    switch (r->getType()) {
      case RegExType::Blank: {
        break;
      }
      case RegExType::Primitive: {
        RegExPrimitive *p = (RegExPrimitive *) r;
        switch (p->getPrimitiveType()) {
          case RegExPrimitiveType::Mgram: {
            Range range = s_core->continue_bw_search(p->getPrimitive(),
                                                     right_result.range);
            if (!isEmpty(range))
              concat_results.insert(
                  ResultEntry(
                      range, right_result.length + p->getPrimitive().length()));
            break;
          }
          case RegExPrimitiveType::Dot: {
            for (char c : std::string(s_core->getAlphabet())) {
              if (c == '\n' || c == (char) 1)
                continue;
              Range range = s_core->continue_bw_search(std::string(1, c),
                                                       right_result.range);
              if (!isEmpty(range))
                concat_results.insert(
                    ResultEntry(range, right_result.length + 1));
            }
            break;
          }
          case RegExPrimitiveType::Range: {
            for (char c : p->getPrimitive()) {
              Range range = s_core->continue_bw_search(std::string(1, c),
                                                       right_result.range);
              if (!isEmpty(range))
                concat_results.insert(
                    ResultEntry(range, right_result.length + 1));
            }
            break;
          }
        }
        break;
      }
      case RegExType::Union: {
        ResultSet res1, res2;
        regexConcat(res1, ((RegExUnion *) r)->getFirst(), right_result);
        regexConcat(res2, ((RegExUnion *) r)->getSecond(), right_result);
        regexUnion(concat_results, res1, res2);
        break;
      }
      case RegExType::Concat: {
        ResultSet left_right_results;
        regexConcat(left_right_results, ((RegExConcat *) r)->getRight(),
                    right_result);
        for (auto left_right_result : left_right_results) {
          ResultSet temp;
          regexConcat(temp, ((RegExConcat *) r)->getLeft(), left_right_result);
          regexUnion(concat_results, concat_results, temp);
        }
        break;
      }
      case RegExType::Repeat: {
        RegExRepeat *rep_r = ((RegExRepeat *) r);
        switch (rep_r->getRepeatType()) {
          case RegExRepeatType::ZeroOrMore: {
            regexRepeatOneOrMore(concat_results, rep_r->getInternal(),
                                 right_result);
            concat_results.insert(right_result);
            break;
          }
          case RegExRepeatType::OneOrMore: {
            regexRepeatOneOrMore(concat_results, rep_r->getInternal(),
                                 right_result);
            break;
          }
          case RegExRepeatType::MinToMax: {
            regexRepeatMinToMax(concat_results, rep_r->getInternal(),
                                right_result, rep_r->getMin(), rep_r->getMax());
          }
        }
        break;
      }
    }
  }

  void regexRepeatOneOrMore(ResultSet &repeat_results, RegEx *r) {
    ResultSet results;
    compute(results, r);
    if (results.empty())
      return;
    regexUnion(repeat_results, repeat_results, results);

    for (auto result : results)
      regexRepeatOneOrMore(repeat_results, r, result);
  }

  void regexRepeatOneOrMore(ResultSet &repeat_results, RegEx *r,
                            ResultEntry right_result) {
    if (isEmpty(right_result))
      return;

    ResultSet concat_results;
    regexConcat(concat_results, r, right_result);
    if (concat_results.empty())
      return;
    regexUnion(repeat_results, repeat_results, concat_results);

    for (auto result : concat_results)
      regexRepeatOneOrMore(repeat_results, r, result);
  }

  void regexRepeatMinToMax(ResultSet &repeat_results, RegEx *r, int min,
                           int max) {
    min = (min > 0) ? min - 1 : 0;
    max = (max > 0) ? max - 1 : 0;

    ResultSet results;
    compute(results, r);
    if (results.empty())
      return;

    if (!min)
      regexUnion(repeat_results, repeat_results, results);

    if (max)
      for (auto result : results)
        regexRepeatMinToMax(repeat_results, r, result, min, max);
  }

  void regexRepeatMinToMax(ResultSet &repeat_results, RegEx *r,
                           ResultEntry right_result, int min, int max) {
    min = (min > 0) ? min - 1 : 0;
    max = (max > 0) ? max - 1 : 0;

    if (isEmpty(right_result))
      return;

    ResultSet concat_results;
    regexConcat(concat_results, r, right_result);
    if (concat_results.empty())
      return;

    if (!min)
      regexUnion(repeat_results, repeat_results, concat_results);

    if (max)
      for (auto result : concat_results)
        regexRepeatMinToMax(repeat_results, r, result, min, max);
  }
};

#endif
