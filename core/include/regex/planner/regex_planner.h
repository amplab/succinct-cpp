#ifndef REGEX_PLANNER_H
#define REGEX_PLANNER_H

#include "succinct_core.h"

class RegExPlanner {
 public:
  RegExPlanner(SuccinctCore *s_file, RegEx *re) {
    this->succinct_core_ = s_file;
    this->regex_ = re;
  }

  virtual ~RegExPlanner() {
    // Do nothing
  }

  virtual RegEx* plan() = 0;

 protected:
  SuccinctCore *succinct_core_;
  RegEx *regex_;
};

class NaiveRegExPlanner : public RegExPlanner {
 public:
  NaiveRegExPlanner(SuccinctCore *succinct_core, RegEx *regex)
      : RegExPlanner(succinct_core, regex) {
  }

  // Does no cost estimation
  virtual RegEx* plan() {
    return regex_;
  }
};

#endif
