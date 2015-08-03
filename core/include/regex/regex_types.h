#ifndef REGEX_TYPES_H
#define REGEX_TYPES_H

#include <iostream>

enum RegExType {
  UNION,
  CONCAT,
  REPEAT,
  PRIMITIVE,
  BLANK
};

class RegEx {
 public:
  RegEx(RegExType regex_type) {
    regex_type_ = regex_type;
  }

  RegExType GetType() {
    return regex_type_;
  }

 private:
  RegExType regex_type_;
};

class RegExUnion : public RegEx {
 public:
  RegExUnion(RegEx *first, RegEx *second)
      : RegEx(RegExType::UNION) {
    first_ = first;
    second_ = second;
  }

  RegEx *GetFirst() {
    return first_;
  }

  RegEx *GetSecond() {
    return second_;
  }

 private:
  RegEx *first_;
  RegEx *second_;
};

class RegExConcat : public RegEx {
 public:
  RegExConcat(RegEx *left, RegEx *right)
      : RegEx(RegExType::CONCAT) {
    this->left_ = left;
    this->right_ = right;
  }

  RegEx *getLeft() {
    return left_;
  }

  RegEx *getRight() {
    return right_;
  }

 private:
  RegEx *left_;
  RegEx *right_;
};

enum RegExRepeatType {
  ZeroOrMore,
  OneOrMore,
  MinToMax
};

class RegExRepeat : public RegEx {
 public:
  RegExRepeat(RegEx *internal, RegExRepeatType repeat_type, int min = -1,
              int max = -1)
      : RegEx(RegExType::REPEAT) {
    this->internal_ = internal;
    this->repeat_type_ = repeat_type;
    this->min_ = min;
    this->max_ = max;
  }

  RegEx *GetInternal() {
    return internal_;
  }

  RegExRepeatType GetRepeatType() {
    return repeat_type_;
  }

  int GetMin() {
    return min_;
  }

  int GetMax() {
    return max_;
  }

 private:
  RegEx *internal_;
  RegExRepeatType repeat_type_;
  int min_;
  int max_;
};

enum RegExPrimitiveType {
  MGRAM,
  DOT,
  RANGE
};

class RegExPrimitive : public RegEx {
 public:
  RegExPrimitive(std::string primitive, RegExPrimitiveType primitive_type =
                     RegExPrimitiveType::MGRAM)
      : RegEx(RegExType::PRIMITIVE) {
    this->primitive_type_ = primitive_type;
    if (primitive_type == RegExPrimitiveType::RANGE) {
      // Expand all ranges
      std::string buf = "";
      for (size_t i = 0; i < primitive.length(); i++) {
        if (primitive[i] == '-')
          for (char c = primitive[i - 1] + 1; c < primitive[i + 1]; c++)
            buf += c;
        else
          buf += primitive[i];
      }
      primitive = buf;
    }
    this->primitive_ = primitive;
  }

  std::string GetPrimitive() {
    return primitive_;
  }

  RegExPrimitiveType GetPrimitiveType() {
    return primitive_type_;
  }

 private:
  std::string primitive_;
  RegExPrimitiveType primitive_type_;
};

class RegExBlank : public RegEx {
 public:
  RegExBlank()
      : RegEx(RegExType::BLANK) {
  }
};

#endif
