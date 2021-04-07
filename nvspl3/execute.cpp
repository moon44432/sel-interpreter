
// SEL Project
// execute.cpp

#include "lexer.h"
#include "ast.h"
#include "execute.h"
#include "stdlibrary.h"
#include "interactiveMode.h"
#include <Windows.h>
#include <cmath>
#include <iostream>

static std::vector<std::map<std::string, int>> AddrTable(1);
static std::vector<std::map<std::string, std::vector<int>>> ArrTable(1);
static std::map<std::string, std::shared_ptr<FunctionAST>> Functions;
static Memory StackMemory;

bool IsInteractive = true; // true for default

Value LogErrorV(const char* Str)
{
    LogError(Str);
    return Value(type::_ERR);
}

Value NumberExprAST::execute(int lvl, int stackIdx)
{
    return Val;
}

Value VariableExprAST::execute(int lvl, int stackIdx)
{
    if (!Indices.empty()) // array element
    {
        for (int i = lvl; i >= 0; i--)
        {
            if (AddrTable[i].find(Name) != AddrTable[i].end())
            {
                for (int j = lvl; j >= 0; j--)
                {
                    if (ArrTable[j].find(Name) != ArrTable[j].end())
                    {
                        std::vector<Value> IdxV;
                        for (unsigned k = 0, e = Indices.size(); k != e; ++k) {
                            IdxV.push_back(Indices[k]->execute(lvl, stackIdx));
                            if (IdxV.back().isErr()) return LogErrorV("error while calculating indices");
                        }

                        std::vector<int> LenDim = ArrTable[j][Name];
                        if (IdxV.size() != LenDim.size()) return LogErrorV("dimension mismatch");

                        int AddVal = 0;
                        for (int l = 0; l < IdxV.size(); l++)
                        {
                            int MulVal = 1;
                            for (int m = l + 1; m < IdxV.size(); m++) MulVal *= LenDim[m];
                            AddVal += MulVal * (int)IdxV[l].getVal();
                        }
                        return StackMemory.getValue(AddrTable[i][Name] + AddVal);
                    }
                }
                return LogErrorV(((Name + (std::string)(" is not an array"))).c_str());
            }
        }
    }
    else // normal variable
    {
        for (int i = lvl; i >= 0; i--)
        {
            if (AddrTable[i].find(Name) != AddrTable[i].end())
                return StackMemory.getValue(AddrTable[i][Name]);
        }
    }
    return LogErrorV("identifier not found");
}

Value ArrDeclExprAST::execute(int lvl, int stackIdx)
{
    AddrTable[lvl][Name] = StackMemory.addValue(Value(0.0));
    ArrTable[lvl][Name] = Indices;

    int size = 1;
    for (int i = 0; i < Indices.size(); i++) size *= Indices[i];
    for (int i = 0; i < size - 1; i++) StackMemory.addValue(Value(0.0));

    return Value(0.0);
}

Value UnaryExprAST::execute(int lvl, int stackIdx)
{
    Value OperandV = Operand->execute(lvl, stackIdx);
    if (OperandV.isErr())
        return Value(type::_ERR);

    switch (Opcode)
    {
    case '!':
        return Value((double)(!(bool)(OperandV.getVal())));
        break;
    case '+':
        return Value(+(OperandV.getVal()));
        break;
    case '-':
        return Value(-(OperandV.getVal()));
        break;
    }

    std::shared_ptr<FunctionAST> F = Functions[(std::string("unary") + Opcode)];
    if (!F) return LogErrorV("Unknown unary operator");

    std::vector<Value> Op;
    Op.push_back(OperandV);

    return F->execute(Op, lvl, stackIdx);
}

Value BinaryExprAST::execute(int lvl, int stackIdx) {
    // Special case '=' because we don't want to emit the LHS as an expression.
    if (Op == "=")
    {
        // Assignment requires the LHS to be an identifier.
        VariableExprAST* LHSE = static_cast<VariableExprAST*>(LHS.get());
        if (!LHSE)
            return LogErrorV("destination of '=' must be a variable");
        // execute the RHS.
        Value Val = RHS->execute(lvl, stackIdx);

        if (Val.isErr())
            return Value(type::_ERR);

        // Look up the name.
        std::vector<std::shared_ptr<ExprAST>> Indices = LHSE->getIndices();
        if (!Indices.empty()) // array element
        {
            for (int i = lvl; i >= 0; i--)
            {
                if (AddrTable[i].find(LHSE->getName()) != AddrTable[i].end())
                {
                    int j;
                    for (j = lvl; j >= 0; j--)
                    {
                        if (ArrTable[j].find(LHSE->getName()) != ArrTable[j].end())
                        {
                            std::vector<Value> IdxV;
                            for (unsigned k = 0, e = Indices.size(); k != e; ++k) {
                                IdxV.push_back(Indices[k]->execute(lvl, stackIdx));
                                if (IdxV.back().isErr()) return LogErrorV("error while calculating indices");
                            }

                            std::vector<int> LenDim = ArrTable[j][LHSE->getName()];
                            if (IdxV.size() != LenDim.size()) return LogErrorV("dimension mismatch");

                            int AddVal = 0;
                            for (int l = 0; l < IdxV.size(); l++)
                            {
                                int MulVal = 1;
                                for (int m = l + 1; m < IdxV.size(); m++) MulVal *= LenDim[m];
                                AddVal += MulVal * (int)IdxV[l].getVal();
                            }
                            StackMemory.setValue(AddrTable[i][LHSE->getName()] + AddVal, Val);
                            break;
                        }
                    }
                    if (j < 0) return LogErrorV(((LHSE->getName() + (std::string)(" is not an array"))).c_str());
                }
            }
        }
        else // normal variable
        {
            bool found = false;
            for (int i = lvl; i >= 0; i--)
            {
                if (AddrTable[i].find(LHSE->getName()) != AddrTable[i].end())
                {
                    StackMemory.setValue(AddrTable[i][LHSE->getName()], Val);
                    found = true;
                    break;
                }
            }
            if (!found)
                AddrTable[lvl][LHSE->getName()] = StackMemory.addValue(Val);
        }
        return Val;
    }

    Value L = LHS->execute(lvl, stackIdx);
    Value R = RHS->execute(lvl, stackIdx);

    if (L.isErr() || R.isErr())
        return Value(type::_ERR);

    if (Op == "==")
        return Value((double)(L.getVal() == R.getVal()));

    if (Op == "!=")
        return Value((double)(L.getVal() != R.getVal()));

    if (Op == "&&")
        return Value((double)(L.getVal() && R.getVal()));

    if (Op == "||")
        return Value((double)(L.getVal() || R.getVal()));

    if (Op == "<")
        return Value((double)(L.getVal() < R.getVal()));

    if (Op == ">")
        return Value((double)(L.getVal() > R.getVal()));

    if (Op == "<=") 
        return Value((double)(L.getVal() <= R.getVal()));

    if (Op == ">=")
        return Value((double)(L.getVal() >= R.getVal()));

    if (Op == "+")
        return Value((double)(L.getVal() + R.getVal()));

    if (Op == "-") 
        return Value((double)(L.getVal() - R.getVal()));

    if (Op == "*")
        return Value((double)(L.getVal() * R.getVal()));

    if (Op == "/")
        return Value((double)(L.getVal() / R.getVal()));

    if (Op == "%")
        return Value(fmod(L.getVal(), R.getVal()));

    if (Op == "**")
        return Value(pow(L.getVal(), R.getVal()));

    // If it wasn't a builtin binary operator, it must be a user defined one. Emit
    // a call to it.
    
    std::shared_ptr<FunctionAST> F = Functions[(std::string("binary") + Op)];
    if (!F) return LogErrorV("binary operator not found!");

    std::vector<Value> Ops;
    Ops.push_back(L);
    Ops.push_back(R);

    return F->execute(Ops, lvl, stackIdx);
}

Value CallExprAST::execute(int lvl, int stackIdx)
{
    std::vector<Value> ArgsV;
    for (unsigned i = 0, e = Args.size(); i != e; ++i) {
        ArgsV.push_back(Args[i]->execute(lvl, stackIdx));
        if (ArgsV.back().isErr())
            return Value(type::_ERR);
    }

    if (std::find(StdFuncList.begin(), StdFuncList.end(), Callee) != StdFuncList.end())
    {
        return Value(CallStdFunc(Callee, ArgsV));
    }

    // Look up the name in the global module table.
    std::shared_ptr<FunctionAST> CalleeF = Functions[Callee];
    if (!CalleeF)
        return LogErrorV("Unknown function referenced");

    // If argument mismatch error.
    if (CalleeF->arg_size() != Args.size())
        return LogErrorV("Incorrect # arguments passed");
    
    return CalleeF->execute(ArgsV, lvl, stackIdx);
}

Value IfExprAST::execute(int lvl, int stackIdx)
{
    Value CondV = Cond->execute(lvl, stackIdx);
    if (CondV.isErr())
        return Value(type::_ERR);

    int StartIdx = StackMemory.getSize();
    AddrTable.push_back(std::map<std::string, int>());
    ArrTable.push_back(std::map<std::string, std::vector<int>>());

    if ((bool)(CondV.getVal()))
    {
        Value ThenV = Then->execute(lvl + 1, StartIdx);
        AddrTable.pop_back();
        ArrTable.pop_back();
        StackMemory.quickDelete(StartIdx);

        if (ThenV.isErr())
            return Value(type::_ERR);
        return ThenV;
    }
    else if (Else != nullptr)
    {
        Value ElseV = Else->execute(lvl + 1, StartIdx);
        AddrTable.pop_back();
        ArrTable.pop_back();
        StackMemory.quickDelete(StartIdx);

        if (ElseV.isErr())
            return Value(type::_ERR);
        return ElseV;
    }
    else return Value(0.0);
}

Value ForExprAST::execute(int lvl, int stackIdx)
{
    Value StartVal = Start->execute(lvl, stackIdx);
    if (StartVal.isErr())
        return Value(type::_ERR);

    bool found = false;
    int StartValLvl;
    for (int i = lvl; i >= 0; i--)
    {
        if (AddrTable[i].find(VarName) != AddrTable[i].end())
        {
            StackMemory.setValue(AddrTable[i][VarName], StartVal);
            StartValLvl = i;
            found = true;
            break;
        }
    }
    if (!found)
    {
        AddrTable[lvl][VarName] = StackMemory.addValue(StartVal);
        StartValLvl = lvl;
    }

    // Emit the step value.
    Value StepVal(1.0);
    if (Step) {
        StepVal = Step->execute(lvl, stackIdx);
        if (StepVal.isErr())
            return Value(type::_ERR);
    }

    int StartIdx = StackMemory.getSize();
    AddrTable.push_back(std::map<std::string, int>());
    ArrTable.push_back(std::map<std::string, std::vector<int>>());

    while (true)
    {
        Value EndCond = End->execute(lvl, stackIdx);
        if (EndCond.isErr())
            return Value(type::_ERR);

        if (!(bool)EndCond.getVal()) break;

        Value BodyExpr = Body->execute(lvl + 1, StartIdx);
        if (BodyExpr.isErr())
            return Value(type::_ERR);
        else if (BodyExpr.getType() == type::_BREAK) break;

        StackMemory.setValue(AddrTable[StartValLvl][VarName],
            Value(StackMemory.getValue(AddrTable[StartValLvl][VarName]).getVal() + StepVal.getVal()));
    }
    AddrTable.pop_back();
    ArrTable.pop_back();
    StackMemory.quickDelete(StartIdx);

    return Value(0.0);
}

Value WhileExprAST::execute(int lvl, int stackIdx)
{
    int StartIdx = StackMemory.getSize();
    AddrTable.push_back(std::map<std::string, int>());
    ArrTable.push_back(std::map<std::string, std::vector<int>>());

    while (true)
    {
        Value EndCond = Cond->execute(lvl, stackIdx);
        if (EndCond.isErr())
            return Value(type::_ERR);

        if (!(bool)EndCond.getVal()) break;

        Value BodyExpr = Body->execute(lvl + 1, StartIdx);
        if (BodyExpr.isErr())
            return Value(type::_ERR);
        else if (BodyExpr.getType() == type::_BREAK) break;
    }
    AddrTable.pop_back();
    ArrTable.pop_back();
    StackMemory.quickDelete(StartIdx);

    return Value(0.0);
}

Value RepeatExprAST::execute(int lvl, int stackIdx)
{
    Value Iter = IterNum->execute(lvl, stackIdx);
    if (Iter.isErr())
        return Value(type::_ERR);

    int StartIdx = StackMemory.getSize();
    AddrTable.push_back(std::map<std::string, int>());
    ArrTable.push_back(std::map<std::string, std::vector<int>>());
   
    for (unsigned i = 0; i < (unsigned)(Iter.getVal()); i++)
    {
        Value BodyExpr = Body->execute(lvl + 1, StackMemory.getSize());
        if (BodyExpr.isErr())
            return Value(type::_ERR);
        else if (BodyExpr.getType() == type::_BREAK) break;
    }

    AddrTable.pop_back();
    ArrTable.pop_back();
    StackMemory.quickDelete(StartIdx);

    return Value(0.0);
}

Value BreakExprAST::execute(int lvl, int stackIdx)
{
    return Value(type::_BREAK);
}

Value ReturnExprAST::execute(int lvl, int stackIdx)
{
    Value RetVal = Expr->execute(lvl, stackIdx);
    if (RetVal.isErr())
        return LogErrorV("failed to return a value");

    return Value(type::_RETURN, RetVal.getVal());
}

Value BlockExprAST::execute(int lvl, int stackIdx)
{
    Value RetVal(type::_ERR);
    int StartIdx = StackMemory.getSize();
    AddrTable.push_back(std::map<std::string, int>());
    ArrTable.push_back(std::map<std::string, std::vector<int>>());

    for (auto& Expr : Expressions)
    {
        RetVal = Expr->execute(lvl + 1, StartIdx);
        if (RetVal.getType() == type::_BREAK || RetVal.getType() == type::_RETURN) break;
    }

    AddrTable.pop_back();
    ArrTable.pop_back();
    StackMemory.quickDelete(StartIdx);

    return RetVal;
}

Value FunctionAST::execute(std::vector<Value> Ops, int lvl, int stackIdx)
{
    int StartIdx = StackMemory.getSize();
    if (lvl > 0)
    {
        AddrTable.push_back(std::map<std::string, int>());
        ArrTable.push_back(std::map<std::string, std::vector<int>>());
    }

    auto& Arg = Proto->getArgs();
    for (int i = 0; i < Proto->getArgsSize(); i++)
    {
        if (lvl > 0) AddrTable[lvl + 1][Arg[i]] = StackMemory.addValue(Ops[i]);
        else AddrTable[lvl][Arg[i]] = StackMemory.addValue(Ops[i]);
    }

    Value RetVal(type::_ERR);
    if (lvl > 0) RetVal = Body->execute(lvl + 1, StackMemory.getSize());
    else RetVal = Body->execute(lvl, StackMemory.getSize());

    if (lvl > 0)
    {
        AddrTable.pop_back();
        ArrTable.pop_back();
        StackMemory.quickDelete(StartIdx);
    }

    if (!RetVal.isErr())
    {
        if (RetVal.getType() == type::_RETURN) return Value(type::_DOUBLE, RetVal.getVal());
        else return RetVal;
    }

    if (Proto->isBinaryOp())
        BinopPrecedence.erase(Proto->getOperatorName());
    return Value(type::_ERR);
}

void HandleDefinition()
{
    if (auto FnAST = ParseDefinition())
    {
        if (IsInteractive) fprintf(stderr, "Read function definition\n");
        Functions[FnAST->getFuncName()] = FnAST;
    }
    else getNextToken(); // Skip token for error recovery.
}

void HandleTopLevelExpression()
{
    // Evaluate a top-level expression into an anonymous function.
    if (auto FnAST = ParseTopLevelExpr())
    {
        Value RetVal = FnAST->execute(std::vector<Value>(), 0, 0);
        if (!RetVal.isErr())
        {
            double FP = RetVal.getVal();
            if (IsInteractive) fprintf(stderr, "Evaluated to %f\n", FP);
        }
    }
    else getNextToken(); // Skip token for error recovery.
}

/// top ::= definition | external | expression | ';'
void MainLoop()
{
    while (true)
    {
        if (IsInteractive) fprintf(stderr, ">>> ");
        switch (CurTok)
        {
        case tok_eof:
            return;
        case ';': // ignore top-level semicolons.
            getNextToken();
            break;
        case tok_def:
            HandleDefinition();
            break;
        case cmd_help:
            if (IsInteractive)
            {
                getNextToken();
                runHelp();
            }
            break;
        default:
            HandleTopLevelExpression();
            break;
        }
    }
}

void execute(const char* filename)
{
    IsInteractive = false;

    FILE* fp = fopen(filename, "r");
    if (fp == NULL)
    {
        fprintf(stderr, "Error: Unknown file name\n");
        return;
    }
    while (!feof(fp)) MainCode += (char)fgetc(fp);
    MainCode += EOF;
    fclose(fp);

    DWORD t = GetTickCount();

    binopPrecInit();
    getNextToken();
    MainLoop();

    DWORD diff = (GetTickCount() - t);
    fprintf(stderr, "\nExecution finished (%.3lfs).\n", (double)diff / 1000);
}