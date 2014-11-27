#ifndef REGEX_PLANNER_H
#define REGEX_PLANNER_H

#include "succinct/SuccinctFile.hpp"

class RegExPlanner {
protected:
    SuccinctFile *s_file;
    RegEx *re;

public:
    RegExPlanner(SuccinctFile *s_file, RegEx *re) {
        this->s_file = s_file;
        this->re = re;
    }

    virtual ~RegExPlanner() {
        // Do nothing
    }

    virtual RegEx* plan() = 0;
};

class NaiveRegExPlanner: public RegExPlanner {
public:
    NaiveRegExPlanner(SuccinctFile *s_file, RegEx *re): RegExPlanner(s_file, re) {}

    // Does no cost estimation
    virtual RegEx* plan() {
        return re;
    }
};

#endif
