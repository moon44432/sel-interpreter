
// SEL Project
// ast.h

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
#include "value.h"

extern std::map<std::string, int> BinopPrecedence;
extern std::string OpChrList;

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
    NumberExprAST(Value Val) : Val(Val) {}
    Value execute() override;
};

/// VariableExprAST - Expression class for referencing a variable or an array element, like "i" or "ar[2][3]".
class VariableExprAST : public ExprAST {
    std::string Name;
    std::vector<std::shared_ptr<ExprAST>> Indices;

public:
    VariableExprAST(const std::string& Name, std::vector<std::shared_ptr<ExprAST>>& Indices)
        : Name(Name), Indices(Indices) {}
    VariableExprAST(const std::string& Name) : Name(Name) {}
    const std::string& getName() const { return Name; }
    const std::vector<std::shared_ptr<ExprAST>> getIndices() const { return Indices; }
    Value execute() override;
};

/// DeRefExprAST - Expression class for dereferencing a memory address, like "@a" or "@(ptr + 10)".
class DeRefExprAST : public ExprAST {
    std::shared_ptr<ExprAST> AddrExpr;

public:
    DeRefExprAST(std::shared_ptr<ExprAST> Addr) : AddrExpr(std::move(Addr)) {}
    const std::shared_ptr<ExprAST> getExpr() { return AddrExpr; }
    Value execute() override;
};

/// ArrDeclExprAST - Expression class for declaring an array, like "arr ar[2][2][2]".
class ArrDeclExprAST : public ExprAST {
    std::string Name;
    std::vector<int> Indices;

public:
    ArrDeclExprAST(const std::string& Name, std::vector<int>& Indices) : Name(Name), Indices(Indices) {}
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
    BinaryExprAST(const std::string& Op, std::shared_ptr<ExprAST> LHS,
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
    IfExprAST(std::shared_ptr<ExprAST> Cond, std::shared_ptr<ExprAST> Then)
        : Cond(std::move(Cond)), Then(std::move(Then)) { Else = nullptr; }
    Value execute() override;
};

/// ForExprAST - Expression class for for.
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

/// WhileExprAST - Expression class for while.
class WhileExprAST : public ExprAST {
    std::shared_ptr<ExprAST> Cond, Body;

public:
    WhileExprAST(std::shared_ptr<ExprAST> Cond, std::shared_ptr<ExprAST> Body)
        : Cond(std::move(Cond)), Body(std::move(Body)) {}
    Value execute() override;
};

/// RepeatExprAST - Expression class for rept.
class RepeatExprAST : public ExprAST {
    std::shared_ptr<ExprAST> IterNum;
    std::shared_ptr<ExprAST> Body;

public:
    RepeatExprAST(std::shared_ptr<ExprAST> IterNum, std::shared_ptr<ExprAST> Body)
        : IterNum(std::move(IterNum)), Body(std::move(Body)) {}
    Value execute() override;
};

/// LoopExprAST - Expression class for loop.
class LoopExprAST : public ExprAST {
    std::shared_ptr<ExprAST> Body;

public:
    LoopExprAST(std::shared_ptr<ExprAST> Body) : Body(std::move(Body)) {}
    Value execute() override;
};

/// BlockExprAST - Sequence of expressions.
class BlockExprAST : public ExprAST {
    std::vector<std::shared_ptr<ExprAST>> Expressions;

public:
    BlockExprAST(std::vector<std::shared_ptr<ExprAST>> Expressions)
        : Expressions(std::move(Expressions)) {}
    Value execute() override;
};

/// BreakExprAST - Expression class for break.
class BreakExprAST : public ExprAST {
    std::shared_ptr<ExprAST> Expr;

public:
    BreakExprAST(std::shared_ptr<ExprAST> Expr) : Expr(std::move(Expr)) {}
    Value execute() override;
};

/// ReturnExprAST - Expression class for return.
class ReturnExprAST : public ExprAST {
    std::shared_ptr<ExprAST> Expr;

public:
    ReturnExprAST(std::shared_ptr<ExprAST> Expr) : Expr(std::move(Expr)) {}
    Value execute() override;
};

/// PrototypeAST - This class represents the "prototype" for a function,
/// which captures its name, and its argument names (thus implicitly the number
/// of arguments the function takes), as well as if it is an operator.
class PrototypeAST {
    std::string Name;
    std::vector<std::string> Args;
    std::vector<bool> ArgsType; // 0 - variable, 1 - array
    bool IsOperator;
    unsigned int Precedence; // Precedence if a binary op.

public:
    PrototypeAST(const std::string& Name, const std::vector<std::string>& Args,
        bool IsOperator = false, unsigned int Prec = 0)
        : Name(Name), Args(Args), IsOperator(IsOperator),
        Precedence(Prec) {}

    const std::string& getName() const { return Name; }

    bool isUnaryOp() const { return IsOperator && Args.size() == 1; }
    bool isBinaryOp() const { return IsOperator && Args.size() == 2; }

    std::string getOperatorName() const {
        assert(isUnaryOp() || isBinaryOp());

        if (isBinaryOp()) return Name.substr(0, Name.find("binary"));
        else return Name.substr(0, Name.find("unary"));
    }

    unsigned int getBinaryPrecedence() const { return Precedence; }
    const int getArgsSize() const { return Args.size(); }
    const std::vector<std::string>& getArgs() const { return Args; }
    const std::vector<bool>& getArgsType() const { return ArgsType; }
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
    int argsSize() const { return Proto->getArgsSize(); }
    const std::string getFuncName() const { return Proto->getName(); }
    const std::vector<std::string> getFuncArgs() const { return Proto->getArgs(); }
};

/// ImportAST - This class represents a module import.
class ImportAST {
    std::string ModuleName;

public:
    ImportAST(const std::string& Name) : ModuleName(Name) {}
    const std::string getModuleName() const { return ModuleName; }
};

void InitBinopPrec();

int GetNextToken(std::string& Code, int* Idx);

int GetTokPrecedence(std::string Op);

std::shared_ptr<ExprAST> LogError(const char* Str);

std::shared_ptr<PrototypeAST> LogErrorP(const char* Str);

std::shared_ptr<ExprAST> ParseNumberExpr(std::string& Code, int* Idx);

std::shared_ptr<ExprAST> ParseParenExpr(std::string& Code, int* Idx);

std::shared_ptr<ExprAST> ParseIdentifierExpr(std::string& Code, int* Idx);

std::shared_ptr<ExprAST> ParseDeRefExpr(std::string& Code, int* Idx);

std::shared_ptr<ExprAST> ParseArrDeclExpr(std::string& Code, int* Idx);

std::shared_ptr<ExprAST> ParseIfExpr(std::string& Code, int* Idx);

std::shared_ptr<ExprAST> ParseForExpr(std::string& Code, int* Idx);

std::shared_ptr<ExprAST> ParseWhileExpr(std::string& Code, int* Idx);

std::shared_ptr<ExprAST> ParseRepeatExpr(std::string& Code, int* Idx);

std::shared_ptr<ExprAST> ParseLoopExpr(std::string& Code, int* Idx);

std::shared_ptr<ExprAST> ParseBreakExpr(std::string& Code, int* Idx);

std::shared_ptr<ExprAST> ParseReturnExpr(std::string& Code, int* Idx);

std::shared_ptr<ExprAST> ParsePrimary(std::string& Code, int* Idx);

std::shared_ptr<ExprAST> ParseUnary(std::string& Code, int* Idx);

std::shared_ptr<ExprAST> ParseBinOpRHS(std::string& Code, int* Idx, int ExprPrec, std::shared_ptr<ExprAST> LHS);

std::shared_ptr<ExprAST> ParseExpression(std::string& Code, int* Idx);

std::shared_ptr<ExprAST> ParseBlockExpression(std::string& Code, int* Idx);

std::shared_ptr<PrototypeAST> ParsePrototype(std::string& Code, int* Idx);

std::shared_ptr<FunctionAST> ParseDefinition(std::string& Code, int* Idx);

std::shared_ptr<FunctionAST> ParseTopLevelExpr(std::string& Code, int* Idx);

std::shared_ptr<ImportAST> ParseImport(std::string& Code, int* Idx);