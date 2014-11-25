#ifndef REGEX_H
#define REGEX_H

enum RegExType {
    Or,
    Concat,
    Repeat,
    Primitive,
    Blank
};

class RegEx {
public:

    RegEx(RegExType re_type) {
        this->re_type = re_type;
    }

    RegExType getType() {
        return re_type;
    }

private:
    RegExType re_type;
};

class RegExOr: public RegEx {
private:
    RegEx *first;
    RegEx *second;

public:
    RegExOr(RegEx *first, RegEx *second): RegEx(RegExType::Or) {
        this->first = first;
        this->second = second;
    }

    RegEx *getFirst() {
        return first;
    }

    RegEx *getSecond() {
        return second;
    }
};

class RegExConcat: public RegEx {
private:
    RegEx *first;
    RegEx *second;

public:
    RegExConcat(RegEx *first, RegEx *second): RegEx(RegExType::Concat) {
        this->first = first;
        this->second = second;
    }

    RegEx *getFirst() {
        return first;
    }

    RegEx *getSecond() {
        return second;
    }
};

enum RepeatType {
    ZeroOrMore,
    OneOrMore
};

class RegExRepeat: public RegEx {
private:
    RegEx *internal;
    RepeatType r_type;

public:
    RegExRepeat(RegEx *internal, RepeatType r_type): RegEx(RegExType::Repeat) {
        this->internal = internal;
        this->r_type = r_type;
    }

    RegEx *getInternal() {
        return internal;
    }

    RepeatType getRepeatType() {
        return r_type;
    }
};

class RegExPrimitive: public RegEx {
private:
    char c;

public:
    RegExPrimitive(char c): RegEx(RegExType::Primitive) {
        this->c = c;
    }

    char getCharacter() {
        return c;
    }
};

class RegExBlank: public RegEx {
public:
    RegExBlank(): RegEx(RegExType::Blank) {}
};

#endif
