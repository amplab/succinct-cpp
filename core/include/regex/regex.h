#ifndef REGEX_HPP
#define REGEX_HPP

#include <cstdio>

#include "executor/regex_executor.h"
#include "executor/regex_executor_bwd.h"
#include "executor/regex_executor_fwd.h"
#include "parser/regex_parser.h"
#include "planner/regex_planner.h"
#include "regex_types.h"

#define BB_PARTIAL_SCAN

class SRegEx {
 public:
  typedef std::pair<size_t, size_t> OffsetLength;
  typedef std::set<OffsetLength> RegExResults;
  typedef RegExResults::iterator RegExResultsIterator;

  SRegEx(std::string exp, SuccinctCore *s_core, bool opt = true) {
    this->exp = exp;
    this->s_core = s_core;
    this->opt = opt;
    GetSubexpressions();
  }

  void Execute() {
    std::vector<RegExResults> subresults;
    for (auto subexp : subexps) {
      RegExResults subresult;
      Subquery(subresult, subexp);
      subresults.push_back(subresult);
    }

    while (subresults.size() != 1) {
      Wildcard(subresults[0], subresults[1]);
      subresults.erase(subresults.begin());
    }

    r_results = subresults[0];
  }

  void Count(std::vector<size_t> &counts) {
    for (auto subexp : subexps) {
      RegExParser p((char *) subexp.c_str());
      counts.push_back(Subcount(p.Parse()));
    }
  }

  void Wildcard(RegExResults &left, RegExResults &right) {
    RegExResults wildcard_res;
    RegExResultsIterator left_it, right_it;
    for (left_it = left.begin(); left_it != left.end(); left_it++) {
      OffsetLength search_candidate(left_it->first + left_it->second, 0);
      RegExResultsIterator first_entry = right.lower_bound(search_candidate);
      for (right_it = first_entry; right_it != right.end(); right_it++) {
        size_t offset = left_it->first;
        size_t length = right_it->first - left_it->first + right_it->second;
        wildcard_res.insert(OffsetLength(offset, length));
      }
    }
    right = wildcard_res;
  }

  void Subquery(RegExResults &result, std::string sub_expression) {
    if (opt) {
      RegExParser p((char *) sub_expression.c_str());
      RegEx *r = p.Parse();
      if (IsSuffixed(r) || !IsPrefixed(r)) {
        RegExExecutorBwd executor(s_core, r);
        executor.Execute();
        executor.getResults(result);
      } else {
        RegExExecutorFwd executor(s_core, r);
        executor.Execute();
        executor.getResults(result);
      }
    } else {
#ifdef BB_PARTIAL_SCAN
      std::vector<std::string> sub_sub_expressions;
      std::string sub_sub_expression = "";
      size_t i = 0;
      while (i < sub_expression.length()) {
        if (sub_expression[i] == '[') {
          if (sub_sub_expression != "") {
            sub_sub_expressions.push_back(sub_sub_expression);
            sub_sub_expression = "";
          }
          std::string range = "";
          for (; sub_expression[i] != ']'; i++) {
            if (sub_expression[i] == '-') {
              for (char c = sub_expression[i - 1] + 1;
                  c < sub_expression[i + 1]; c++) {
                range += c;
              }
              i++;
            }
            range += sub_expression[i];
          }
          range += sub_expression[i++];
          if (sub_expression[i] == '+' || sub_expression[i] == '*') {
            range += sub_expression[i++];
          }
          sub_sub_expressions.push_back(range);
        } else if (sub_expression[i] == '.') {
          if (sub_sub_expression != "") {
            sub_sub_expressions.push_back(sub_sub_expression);
            sub_sub_expression = "";
          }
          sub_sub_expressions.push_back(".");
          i++;
        } else {
          sub_sub_expression += sub_expression[i];
          i++;
        }
      }

      if (sub_sub_expression != "") {
        sub_sub_expressions.push_back(sub_sub_expression);
      }

      // Sequentially go through the list of sub-sub-expressions
      std::string last_token = "";
      int32_t last_token_id = -1;
      RegExResults last_results;
      for (size_t i = 0; i < sub_sub_expressions.size(); i++) {
        std::string ssexp = sub_sub_expressions[i];
        if (ssexp[0] == '[' || ssexp[0] == '.') {
          if (last_token_id == -1) {
            continue;
          }
          RegExResults range_results;
          if (ssexp == ".") {
            for (RegExResultsIterator it = last_results.begin();
                it != last_results.end(); it++) {
              range_results.insert(OffsetLength(it->first, it->second + 1));
            }
          } else if (ssexp[ssexp.length() - 1] == '+') {
            std::string range = ssexp.substr(1, ssexp.length() - 3);
            for (RegExResultsIterator it = last_results.begin();
                it != last_results.end(); it++) {
              size_t start_pos = it->first + it->second - 1;
              char c;
              size_t len = 1;
              while (true) {
                c = s_core->CharAt(start_pos + len);
                if (range.find(c) != std::string::npos) {
                  range_results.insert(
                      OffsetLength(it->first, it->second + len));
                } else {
                  break;
                }
                len++;
              }
            }
          } else if (ssexp[ssexp.length() - 1] == '+') {
            std::string range = ssexp.substr(1, ssexp.length() - 3);
            range_results.insert(last_results.begin(), last_results.end());
            for (RegExResultsIterator it = last_results.begin();
                it != last_results.end(); it++) {
              size_t start_pos = it->first + it->second - 1;
              char c;
              size_t len = 1;
              while (true) {
                c = s_core->CharAt(start_pos + len);
                if (range.find(c) != std::string::npos) {
                  range_results.insert(
                      OffsetLength(it->first, it->second + len));
                } else {
                  break;
                }
                len++;
              }
            }
          } else {
            std::string range = ssexp.substr(1, ssexp.length() - 2);
            for (RegExResultsIterator it = last_results.begin();
                it != last_results.end(); it++) {
              size_t cur_pos = it->first + it->second;
              char c = s_core->CharAt(cur_pos);
              if (range.find(c) != std::string::npos) {
                range_results.insert(OffsetLength(it->first, it->second + 1));
              }
            }
          }
          last_results = range_results;
        } else {

          bool backtrack = false;
          if (last_token_id == -1) {
            backtrack = true;
          }

          last_token_id = i;
          last_token = ssexp;

          RegExResults cur_results;
          RegExParser p((char *) ssexp.c_str());
          RegEx *r = p.Parse();
          RegExExecutorBlackBox executor(s_core, r);
          executor.Execute();
          executor.getResults(cur_results);

          if (backtrack) {
            last_results = cur_results;
            for (int32_t j = i - 1; j >= 0; j--) {
              ssexp = sub_sub_expressions[j];
              if (ssexp[ssexp.length() - 1] == '*')
                continue;

              RegExResults range_results;
              if (ssexp == ".") {
                for (RegExResultsIterator it = last_results.begin();
                    it != last_results.end(); it++) {
                  range_results.insert(
                      OffsetLength(it->first - 1, it->second + 1));
                }
              } else if (ssexp[ssexp.length() - 1] == '+') {
                std::string range = ssexp.substr(1, ssexp.length() - 2);
                for (RegExResultsIterator it = last_results.begin();
                    it != last_results.end(); it++) {
                  size_t cur_pos = it->first - 1;
                  char c = s_core->CharAt(cur_pos);
                  if (range.find(c) != std::string::npos) {
                    range_results.insert(
                        OffsetLength(it->first - 1, it->second + 1));
                  }
                }
              } else if (ssexp[ssexp.length() - 1] == '*') {
                std::string range = ssexp.substr(1, ssexp.length() - 2);
                range_results.insert(last_results.begin(), last_results.end());
                for (RegExResultsIterator it = last_results.begin();
                    it != last_results.end(); it++) {
                  size_t cur_pos = it->first - 1;
                  char c = s_core->CharAt(cur_pos);
                  if (range.find(c) != std::string::npos) {
                    range_results.insert(
                        OffsetLength(it->first - 1, it->second + 1));
                  }
                }
              } else {
                std::string range = ssexp.substr(1, ssexp.length() - 2);
                for (RegExResultsIterator it = last_results.begin();
                    it != last_results.end(); it++) {
                  size_t cur_pos = it->first - 1;
                  char c = s_core->CharAt(cur_pos);
                  if (range.find(c) != std::string::npos) {
                    range_results.insert(
                        OffsetLength(it->first - 1, it->second + 1));
                  }
                }
              }
              last_results = range_results;
            }
          } else {
            RegExResults concat_results;
            RegExResultsIterator left_it, right_it;
            for (left_it = last_results.begin(), right_it = cur_results.begin();
                left_it != last_results.end() && right_it != cur_results.end();
                left_it++) {
              while (right_it != cur_results.end()
                  && right_it->first < left_it->first + left_it->second)
                right_it++;
              if (right_it == cur_results.end())
                break;

              if (right_it->first == left_it->first + left_it->second) {
                concat_results.insert(
                    OffsetLength(left_it->first,
                                 left_it->second + right_it->second));
              }
            }
            last_results = concat_results;
          }
        }
      }
      result = last_results;
#else
      RegExParser p((char *)sub_expression.c_str());
      RegExExecutorBlackBox executor(s_core, p.Parse());
      executor.Execute();
      executor.getResults(result);
#endif
    }
  }

  size_t Subcount(RegEx *r) {
    if (IsSuffixed(r) || !IsPrefixed(r)) {
      RegExExecutorBwd executor(s_core, r);
      return executor.Count();
    } else {
      RegExExecutorFwd executor(s_core, r);
      return executor.Count();
    }
  }

  void Explain() {
    fprintf(stderr, "***");
    for (auto subexp : subexps) {
      RegExParser p((char *) subexp.c_str());
      ExplainSubexpression(p.Parse());
      fprintf(stderr, "***");
    }
  }

  void ShowResults(size_t limit) {
    if (limit <= 0)
      limit = r_results.size();
    limit = MIN(limit, r_results.size());
    RegExResultsIterator it;
    size_t i;
    fprintf(stderr, "Showing %zu of %zu results.\n", limit, r_results.size());
    fprintf(stderr, "{");
    for (it = r_results.begin(), i = 0; i < limit; i++, it++) {
      fprintf(stderr, "%zu => %zu, ", it->first, it->second);
    }
    fprintf(stderr, "...}\n");
  }

  void GetResults(RegExResults &results) {
    results = r_results;
  }

 private:
  void ExplainSubexpression(RegEx *re) {
    switch (re->GetType()) {
      case RegExType::BLANK: {
        fprintf(stderr, "<blank>");
        break;
      }
      case RegExType::PRIMITIVE: {
        RegExPrimitive *p = ((RegExPrimitive *) re);
        fprintf(stderr, "\"%s\"", p->GetPrimitive().c_str());
        break;
      }
      case RegExType::REPEAT: {
        fprintf(stderr, "REPEAT(");
        ExplainSubexpression(((RegExRepeat *) re)->GetInternal());
        fprintf(stderr, ")");
        break;
      }
      case RegExType::CONCAT: {
        fprintf(stderr, "(");
        ExplainSubexpression(((RegExConcat *) re)->getLeft());
        fprintf(stderr, " CONCAT ");
        ExplainSubexpression(((RegExConcat *) re)->getRight());
        fprintf(stderr, ")");
        break;
      }
      case RegExType::UNION: {
        fprintf(stderr, "(");
        ExplainSubexpression(((RegExUnion *) re)->GetFirst());
        fprintf(stderr, " OR ");
        ExplainSubexpression(((RegExUnion *) re)->GetSecond());
        fprintf(stderr, ")");
        break;
      }
    }
  }

  bool IsPrefixed(RegEx *re) {
    switch (re->GetType()) {
      case RegExType::BLANK:
        return false;
      case RegExType::PRIMITIVE:
        return (((RegExPrimitive *) re)->GetPrimitiveType()
            == RegExPrimitiveType::MGRAM);
      case RegExType::REPEAT:
        return IsPrefixed(((RegExRepeat *) re)->GetInternal());
      case RegExType::CONCAT:
        return IsPrefixed(((RegExConcat *) re)->getLeft());
      case RegExType::UNION:
        return IsPrefixed(((RegExUnion *) re)->GetFirst())
            && IsPrefixed(((RegExUnion *) re)->GetSecond());
    }
  }

  bool IsSuffixed(RegEx *re) {
    switch (re->GetType()) {
      case RegExType::BLANK:
        return false;
      case RegExType::PRIMITIVE:
        return (((RegExPrimitive *) re)->GetPrimitiveType()
            == RegExPrimitiveType::MGRAM);
      case RegExType::REPEAT:
        return IsSuffixed(((RegExRepeat *) re)->GetInternal());
      case RegExType::CONCAT:
        return IsSuffixed(((RegExConcat *) re)->getRight());
      case RegExType::UNION:
        return IsSuffixed(((RegExUnion *) re)->GetFirst())
            && IsPrefixed(((RegExUnion *) re)->GetSecond());
    }
  }

  void GetSubexpressions() {
    // TODO: Right now this assumes we don't have nested ".*" operators
    // It would be nice to allow .* anywhere.
    std::string delimiter = ".*";

    size_t pos = 0;
    std::string subexp;

    while ((pos = exp.find(delimiter)) != std::string::npos) {
      subexp = exp.substr(0, pos);
      subexps.push_back(subexp);
      exp.erase(0, pos + delimiter.length());
    }

    subexp = exp;
    subexps.push_back(subexp);
  }

  std::string exp;
  std::vector<std::string> subexps;
  SuccinctCore *s_core;
  bool opt;

  RegExResults r_results;
};

#endif
