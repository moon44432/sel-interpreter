#pragma once

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

extern int CurTok;
extern std::map<std::string, int> BinopPrecedence;
extern std::string OpChrList;

class Value
{
    bool Empty = false;
    double Val = 0.0;
public:
    Value(double Val) : Val(Val) {}
    Value(bool IsEmpty) : Empty(IsEmpty) {}
    Value() {}
    void updateVal(double dVal) { Val = dVal; }
    double getVal() { return Val; }
    bool isEmpty() { return Empty; }
};

/// ExprAST - Base class for all expression nodes.
class ExprAST {
public:
    virtual ~ExprAST() = default;
    virtual Value execute() = 0;
};

/// NumberExprAST - Expression class for numeric literals like "1.0".
class NumberExprAST : public ExprAST {
    Value Val;

public:
    NumberExprAST(double Val) : Val(Val) {}
    Value execute() override;
};

/// VariableExprAST - Expression class for referencing a variable, like "a".
class VariableExprAST : public ExprAST {
    std::string Name;

public:
    VariableExprAST(const std::string& Name) : Name(Name) {}
    const std::string& getName() const { return Name; }
    Value execute() override;
};

/// UnaryExprAST - Expression class for a unary operator.
class UnaryExprAST : public ExprAST {
    char Opcode;
    std::shared_ptr<ExprAST> Operand;

public:
    UnaryExprAST(char Opcode, std::shared_ptr<ExprAST> Operand)
        : Opcode(Opcode), Operand(std::move(Operand)) {}
    Value execute() override;
};

/// BinaryExprAST - Expression class for a binary operator.
class BinaryExprAST : public ExprAST {
    std::string Op;
    std::shared_ptr<ExprAST> LHS, RHS;

public:
    BinaryExprAST(std::string Op, std::shared_ptr<ExprAST> LHS,
        std::shared_ptr<ExprAST> RHS)
        : Op(Op), LHS(std::move(LHS)), RHS(std::move(RHS)) {}
    Value execute() override;
};

/// CallExprAST - Expression class for function calls.
class CallExprAST : public ExprAST {
    std::string Callee;
    std::vector<std::shared_ptr<ExprAST>> Args;

public:
    CallExprAST(const std::string& Callee,
        std::vector<std::shared_ptr<ExprAST>> Args)
        : Callee(Callee), Args(std::move(Args)) {}
    Value execute() override;
};

/// IfExprAST - Expression class for if/then/else.
class IfExprAST : public ExprAST {
    std::shared_ptr<ExprAST> Cond, Then, Else;

public:
    IfExprAST(std::shared_ptr<ExprAST> Cond, std::shared_ptr<ExprAST> Then,
        std::shared_ptr<ExprAST> Else)
        : Cond(std::move(Cond)), Then(std::move(Then)), Else(std::move(Else)) {}
    Value execute() override;
};

/// ForExprAST - Expression class for for/in.
class ForExprAST : public ExprAST {
    std::string VarName;
    std::shared_ptr<ExprAST> Start, End, Step, Body;

public:
    ForExprAST(const std::string& VarName, std::shared_ptr<ExprAST> Start,
        std::shared_ptr<ExprAST> End, std::shared_ptr<ExprAST> Step,
        std::shared_ptr<ExprAST> Body)
        : VarName(VarName), Start(std::move(Start)), End(std::move(End)),
        Step(std::move(Step)), Body(std::move(Body)) {}
    Value execute() override;
};

/// VarExprAST - Expression class for var
class VarExprAST : public ExprAST {
    std::vector<std::pair<std::string, std::shared_ptr<ExprAST>>> VarNames;
    std::shared_ptr<ExprAST> Body;

public:
    VarExprAST(
        std::vector<std::pair<std::string, std::shared_ptr<ExprAST>>> VarNames,
        std::shared_ptr<ExprAST> Body)
        : VarNames(std::move(VarNames)), Body(std::move(Body)) {}
    Value execute() override;
};

/// BlockExprAST - Sequence of expressions
class BlockExprAST : public ExprAST {
    std::vector<std::shared_ptr<ExprAST>> Expressions;

public:
    BlockExprAST(std::vector<std::shared_ptr<ExprAST>> Expressions)
        : Expressions(std::move(Expressions)) {}
    Value execute() override;
};

/// PrototypeAST - This class represents the "prototype" for a function,
/// which captures its name, and its argument names (thus implicitly the number
/// of arguments the function takes), as well as if it is an operator.
class PrototypeAST {
    std::string Name;
    std::vector<std::string> Args;
    bool IsOperator;
    unsigned Precedence; // Precedence if a binary op.

public:
    PrototypeAST(const std::string& Name, std::vector<std::string> Args,
        bool IsOperator = false, unsigned Prec = 0)
        : Name(Name), Args(std::move(Args)), IsOperator(IsOperator),
        Precedence(Prec) {}

    const std::string& getName() const { return Name; }

    bool isUnaryOp() const { return IsOperator && Args.size() == 1; }
    bool isBinaryOp() const { return IsOperator && Args.size() == 2; }

    std::string getOperatorName() const {
        assert(isUnaryOp() || isBinaryOp());

        if (isBinaryOp()) return Name.substr(0, Name.find("binary"));
        else return Name.substr(0, Name.find("unary"));
    }

    unsigned getBinaryPrecedence() const { return Precedence; }
    Value execute();
    const int getArgsSize() const { return Args.size(); }
    const std::vector<std::string>& getArgs() const { return Args; }
};

/// FunctionAST - This class represents a function definition itself.
class FunctionAST {
    std::shared_ptr<PrototypeAST> Proto;
    std::shared_ptr<ExprAST> Body;

public:
    FunctionAST(std::shared_ptr<PrototypeAST> Proto,
        std::shared_ptr<ExprAST> Body)
        : Proto(std::move(Proto)), Body(std::move(Body)) {}
    Value execute(std::vector<Value> Ops);
    int arg_size() const { return Proto->getArgsSize(); }
    const std::string getFuncName() const { return Proto->getName(); }
    const std::vector<std::string> getFuncArgs() const { return Proto->getArgs(); }
};

void binopPrecInit();

int getNextToken();

int GetTokPrecedence(std::string Op);

std::shared_ptr<ExprAST> LogError(const char* Str);

std::shared_ptr<PrototypeAST> LogErrorP(const char* Str);

std::shared_ptr<ExprAST> ParseNumberExpr();

std::shared_ptr<ExprAST> ParseParenExpr();

std::shared_ptr<ExprAST> ParseIdentifierExpr();

std::shared_ptr<ExprAST> ParseIfExpr();

std::shared_ptr<ExprAST> ParseForExpr();

std::shared_ptr<ExprAST> ParseVarExpr();

std::shared_ptr<ExprAST> ParsePrimary();

std::shared_ptr<ExprAST> ParseUnary();

std::shared_ptr<ExprAST> ParseBinOpRHS(int ExprPrec, std::shared_ptr<ExprAST> LHS);

std::shared_ptr<ExprAST> ParseExpression();

std::shared_ptr<ExprAST> ParseBlockExpression();

std::shared_ptr<PrototypeAST> ParsePrototype();

std::shared_ptr<FunctionAST> ParseDefinition();

std::shared_ptr<FunctionAST> ParseTopLevelExpr();