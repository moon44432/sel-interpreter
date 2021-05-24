
// SEL Project
// value.h

#pragma once

typedef enum class DataType
{
    t_int = 1,
    t_double = 2,
} dataType;

typedef enum class ValueType
{
    val_data = 0,
    val_return = 1,
    val_break = 2,
    val_err = 3,
} valueType;

typedef union
{
    int i;
    double dbl;
} valueData;

class Value
{
    valueType vType = valueType::val_data;
    dataType dType = dataType::t_int;
    valueData Data;
public:
    Value() {}
    Value(valueType vType) : vType(vType) {}
    Value(dataType dType) : dType(dType) {}
    Value(valueType vType, dataType dType, valueData Val)
        : vType(vType), dType(dType), Data(Val) {}
    Value(dataType dType, valueData Val)
        : dType(dType), Data(Val) {}
    Value(double dVal) { dType = dataType::t_double; Data.dbl = dVal; }
    Value(int iVal) { dType = dataType::t_int; Data.i = iVal; }

    void setvType(valueType vTy) { vType = vTy; }
    void setdType(dataType dTy) { dType = dTy; }
    void updateVal(valueData Val) { Data = Val; }

    bool isErr() { return vType == valueType::val_err; }
    bool isInt() { return dType == dataType::t_int; }
    bool isUInt() { return isInt() && Data.i >= 0; }

    valueType getvType() { return vType; }
    dataType getdType() { return dType; }
    valueData getVal() { return Data; }

    int getiVal() { if (dType == dataType::t_double) return (int)Data.dbl;
                    else if (dType == dataType::t_int) return Data.i; }
    double getdVal() { if (dType == dataType::t_double) return Data.dbl;
                       else if (dType == dataType::t_int) return (double)Data.i; }
};