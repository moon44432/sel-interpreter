
// SEL Project
// value.h

#pragma once

typedef enum
{
    _INT = 1,
    _DOUBLE = 2,
} dataType;

typedef enum
{
    _DATA = 0,
    _RETURN = 1,
    _BREAK = 2,
    _ERR = 3,
} valueType;

typedef struct
{
    valueType vType;
    dataType dType;
} type;

typedef union
{
    char ch;
    int i;
    double dbl;
} valueData;

extern type Err;
extern type Int;
extern type Dbl;

class Value
{
    type Type = Int;
    valueData Data;
public:
    Value() {}
    Value(type Ty) : Type(Ty) {}
    Value(type Ty, valueData Val) : Type(Ty), Data(Val) {}
    Value(double dVal) { Type = Dbl; Data.dbl = dVal; }
    Value(int iVal) { Type = Int; Data.i = iVal; }

    void setType(type Ty) { Type = Ty; }
    void setValueType(valueType VTy) { Type.vType = VTy; }
    void updateVal(valueData Val) { Data = Val; }
    void cast(dataType DestTy);

    bool isErr() { return Type.vType == valueType::_ERR; }
    bool isInt() { return Type.dType == dataType::_INT; }
    bool isUInt() { return isInt() && Data.i >= 0; }

    type getType() { return Type; }
    valueData getVal() { return Data; }

    int getiVal() { if (Type.dType == dataType::_DOUBLE) return (int)Data.dbl;
                    else if (Type.dType == dataType::_INT) return Data.i; }
    double getdVal() { if (Type.dType == dataType::_DOUBLE) return Data.dbl;
                       else if (Type.dType == dataType::_INT) return (double)Data.i; }
};