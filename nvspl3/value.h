
// SEL Project
// value.h

#pragma once

typedef enum Type
{
    _RETURN = -3,
    _BREAK = -2,
    _ERR = -1,

    _INT = 1,
    _DOUBLE = 2,
} type;

class Value
{
    type Type = type::_DOUBLE;
    double NumericVal = 0.0;
public:
    Value() {}
    Value(double Val) : NumericVal(Val) {}
    Value(type Type) : Type(Type) {}
    Value(type Type, double Val) : Type(Type), NumericVal(Val) {}
    void updateVal(double dVal) { NumericVal = dVal; }
    double getVal() { return NumericVal; }
    bool isErr() { return (Type == type::_ERR); }
    type getType() { return Type; }
};