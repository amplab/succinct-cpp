#ifndef PARSER_H
#define PARSER_H

class RegExExecutor {
private:
    SuccinctCore *s_core;
    RegEx *re;
    std::vector<long> results;

public:
    RegExExecutor(SuccinctCore *s_core, RegEx *re) {
        this->s_core = s_core;
        this->re = re;
    }

    void execute() {
        compute(re);
    }

    std::vector<long> getResults() {
        return results;
    }

private:
    void compute(RegEx *r);

};
#endif
