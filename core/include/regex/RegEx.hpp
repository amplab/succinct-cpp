#ifndef REGEX_HPP
#define REGEX_HPP

#include <cstdio>

#include "regex/RegExTypes.hpp"
#include "regex/parser/RegExParser.hpp"
#include "regex/planner/RegExPlanner.hpp"
#include "regex/executor/RegExExecutor.hpp"
#include "regex/executor/RegExExecutorBwd.hpp"
#include "regex/executor/RegExExecutorFwd.hpp"

#define BB_PARTIAL_SCAN
class SRegEx {
 private:
  typedef std::pair<size_t, size_t> OffsetLength;
  typedef std::set<OffsetLength> RegExResults;
  typedef RegExResults::iterator RegExResultsIterator;

  void explain_subexp(RegEx *re) {
    switch (re->getType()) {
      case RegExType::Blank: {
        fprintf(stderr, "<blank>");
        break;
      }
      case RegExType::Primitive: {
        RegExPrimitive *p = ((RegExPrimitive *) re);
        fprintf(stderr, "\"%s\"", p->getPrimitive().c_str());
        break;
      }
      case RegExType::Repeat: {
        fprintf(stderr, "REPEAT(");
        explain_subexp(((RegExRepeat *) re)->getInternal());
        fprintf(stderr, ")");
        break;
      }
      case RegExType::Concat: {
        fprintf(stderr, "(");
        explain_subexp(((RegExConcat *) re)->getLeft());
        fprintf(stderr, " CONCAT ");
        explain_subexp(((RegExConcat *) re)->getRight());
        fprintf(stderr, ")");
        break;
      }
      case RegExType::Union: {
        fprintf(stderr, "(");
        explain_subexp(((RegExUnion *) re)->getFirst());
        fprintf(stderr, " OR ");
        explain_subexp(((RegExUnion *) re)->getSecond());
        fprintf(stderr, ")");
        break;
      }
    }
  }

  bool is_prefixed(RegEx *re) {
    switch (re->getType()) {
      case RegExType::Blank:
        return false;
      case RegExType::Primitive:
        return (((RegExPrimitive *) re)->getPrimitiveType()
            == RegExPrimitiveType::Mgram);
      case RegExType::Repeat:
        return is_prefixed(((RegExRepeat *) re)->getInternal());
      case RegExType::Concat:
        return is_prefixed(((RegExConcat *) re)->getLeft());
      case RegExType::Union:
        return is_prefixed(((RegExUnion *) re)->getFirst())
            && is_prefixed(((RegExUnion *) re)->getSecond());
    }
  }

  bool is_suffixed(RegEx *re) {
    switch (re->getType()) {
      case RegExType::Blank:
        return false;
      case RegExType::Primitive:
        return (((RegExPrimitive *) re)->getPrimitiveType()
            == RegExPrimitiveType::Mgram);
      case RegExType::Repeat:
        return is_suffixed(((RegExRepeat *) re)->getInternal());
      case RegExType::Concat:
        return is_suffixed(((RegExConcat *) re)->getRight());
      case RegExType::Union:
        return is_suffixed(((RegExUnion *) re)->getFirst())
            && is_prefixed(((RegExUnion *) re)->getSecond());
    }
  }

  void get_subexpressions() {
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

 public:
  SRegEx(std::string exp, SuccinctCore *s_core, bool opt = true) {
    this->exp = exp;
    this->s_core = s_core;
    this->opt = opt;
    get_subexpressions();
  }

  void execute() {
    std::vector<RegExResults> subresults;
    for (auto subexp : subexps) {
      RegExResults subresult;
      subquery(subresult, subexp);
      subresults.push_back(subresult);
    }

    while (subresults.size() != 1) {
      wildcard(subresults[0], subresults[1]);
      subresults.erase(subresults.begin());
    }

    r_results = subresults[0];
  }

  void count(std::vector<size_t> &counts) {
    for (auto subexp : subexps) {
      RegExParser p((char *) subexp.c_str());
      counts.push_back(subcount(p.parse()));
    }
  }

  void wildcard(RegExResults &left, RegExResults &right) {
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

  void subquery(RegExResults &result, std::string sub_expression) {
    if (opt) {
      RegExParser p((char *) sub_expression.c_str());
      RegEx *r = p.parse();
      if (is_suffixed(r) || !is_prefixed(r)) {
        RegExExecutorBwd executor(s_core, r);
        executor.execute();
        executor.getResults(result);
      } else {
        RegExExecutorFwd executor(s_core, r);
        executor.execute();
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
                c = s_core->charAt(start_pos + len);
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
                c = s_core->charAt(start_pos + len);
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
              char c = s_core->charAt(cur_pos);
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
          RegEx *r = p.parse();
          RegExExecutorBlackBox executor(s_core, r);
          executor.execute();
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
                  char c = s_core->charAt(cur_pos);
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
                  char c = s_core->charAt(cur_pos);
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
                  char c = s_core->charAt(cur_pos);
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
      RegExExecutorBlackBox executor(s_core, p.parse());
      executor.execute();
      executor.getResults(result);
#endif
    }
  }

  size_t subcount(RegEx *r) {
    if (is_suffixed(r) || !is_prefixed(r)) {
      RegExExecutorBwd executor(s_core, r);
      return executor.count();
    } else {
      RegExExecutorFwd executor(s_core, r);
      return executor.count();
    }
  }

  void explain() {
    fprintf(stderr, "***");
    for (auto subexp : subexps) {
      RegExParser p((char *) subexp.c_str());
      explain_subexp(p.parse());
      fprintf(stderr, "***");
    }
  }

  void show_results(size_t limit) {
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

  void get_results(RegExResults &results) {
    results = r_results;
  }

 private:
  std::string exp;
  std::vector<std::string> subexps;
  SuccinctCore *s_core;
  bool opt;

  RegExResults r_results;
};

#endif
