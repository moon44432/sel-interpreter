
// SEL Project
// execute.cpp

#include "lexer.h"
#include "value.h"
#include "ast.h"
#include "execute.h"
#include "stdlibrary.h"
#include "interactiveMode.h"
#include <cmath>
#include <iostream>

#ifdef _WIN32
#include <Windows.h>
#endif

static std::vector<std::map<std::string, int>> AddrTable(1);
static std::vector<std::map<std::string, std::vector<int>>> ArrTable(1);
static std::map<std::string, std::shared_ptr<FunctionAST>> Functions;
static Memory StackMemory;

typedef enum class ArrAction
{
    _GETVAL,
    _GETADDR,
    _SETVAL,
} arrAction;

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

Value DeRefExprAST::execute(int lvl, int stackIdx)
{
    Value Address = AddrExpr->execute(lvl, stackIdx);
    if (Address.isErr())
        return Value(type::_ERR);

    return StackMemory.getValue((unsigned)Address.getVal());
}

Value HandleArr(std::string ArrName, std::vector<std::shared_ptr<ExprAST>>& Indices, arrAction Action, Value Val, int lvl, int stackIdx)
{
    for (int i = lvl; i >= 0; i--)
    {
        if (AddrTable[i].find(ArrName) != AddrTable[i].end())
        {
            for (int j = lvl; j >= 0; j--)
            {
                if (ArrTable[j].find(ArrName) != ArrTable[j].end())
                {
                    std::vector<Value> IdxV;
                    for (unsigned k = 0, e = Indices.size(); k != e; ++k) {
                        IdxV.push_back(Indices[k]->execute(lvl, stackIdx));
                        if (IdxV.back().isErr()) return LogErrorV("Error while calculating indices");
                    }

                    std::vector<int> LenDim = ArrTable[j][ArrName];
                    if (IdxV.size() != LenDim.size()) return LogErrorV("Dimension mismatch");

                    int AddVal = 0;
                    for (int l = 0; l < IdxV.size(); l++)
                    {
                        int MulVal = 1;
                        for (int m = l + 1; m < IdxV.size(); m++) MulVal *= LenDim[m];
                        AddVal += MulVal * (int)IdxV[l].getVal();
                    }
                    switch (Action)
                    {
                    case arrAction::_GETVAL:
                        return StackMemory.getValue(AddrTable[i][ArrName] + AddVal);
                    case arrAction::_GETADDR:
                        return Value(AddrTable[i][ArrName] + AddVal);
                    case arrAction::_SETVAL:
                        StackMemory.setValue(AddrTable[i][ArrName] + AddVal, Val);
                        return Val;
                    }
                }
            }
            return LogErrorV((((std::string)("\"") + ArrName + (std::string)("\" is not an array"))).c_str());
        }
    }
}

Value VariableExprAST::execute(int lvl, int stackIdx)
{
    if (!Indices.empty()) // array element
    {
        return HandleArr(Name, Indices, arrAction::_GETVAL, Value(type::_ERR), lvl, stackIdx);
    }
    else // normal variable
    {
        for (int i = lvl; i >= 0; i--)
        {
            if (AddrTable[i].find(Name) != AddrTable[i].end())
                return StackMemory.getValue(AddrTable[i][Name]);
        }
    }
    return LogErrorV(std::string("Identifier \"" + Name + "\" not found").c_str());
}

Value ArrDeclExprAST::execute(int lvl, int stackIdx)
{
    AddrTable[lvl][Name] = StackMemory.addValue(Value(0.0));
    ArrTable[lvl][Name] = Indices;

    int size = 1;
    for (int i = 0; i < Indices.size(); i++) size *= Indices[i];
    for (int i = 0; i < size - 1; i++) StackMemory.addValue(Value(0.0));

    return Value(size);
}

Value UnaryExprAST::execute(int lvl, int stackIdx)
{
    if (Opcode == '&') // reference operator
    {
        VariableExprAST* Op = static_cast<VariableExprAST*>(Operand.get());
        if (!Op) return LogErrorV("Operand of '&' must be a variable");

        std::vector<std::shared_ptr<ExprAST>> Indices = Op->getIndices();
        if (!Indices.empty()) // array element
        {
            return HandleArr(Op->getName(), Indices, arrAction::_GETADDR, Value(type::_ERR), lvl, stackIdx);
        }
        else // normal variable
        {
            for (int i = lvl; i >= 0; i--)
            {
                if (AddrTable[i].find(Op->getName()) != AddrTable[i].end())
                {
                    return Value(AddrTable[i][Op->getName()]);
                }
            }
            return LogErrorV(std::string("Variable \"" + Op->getName() + "\" not found").c_str());
        }
    }

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
        // execute the RHS.
        Value Val = RHS->execute(lvl, stackIdx);

        if (Val.isErr())
            return Value(type::_ERR);

        // Assignment requires the LHS to be an identifier.
        VariableExprAST* LHSE = dynamic_cast<VariableExprAST*>(LHS.get());
        if (!LHSE)
        {
            DeRefExprAST* LHSE = dynamic_cast<DeRefExprAST*>(LHS.get());
            if (!LHSE)
                return LogErrorV("Destination of '=' must be a variable");
            
            // update value at the memory address
            StackMemory.setValue((unsigned)LHSE->getExpr()->execute(lvl, stackIdx).getVal(), Val);
            return Val;
        }

        // Look up the name.
        std::vector<std::shared_ptr<ExprAST>> Indices = LHSE->getIndices();
        if (!Indices.empty()) // array element
        {
            return HandleArr(LHSE->getName(), Indices, arrAction::_SETVAL, Val, lvl, stackIdx);
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
    if (!F) return LogErrorV("Binary operator not found");

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
        return Value(CallStdFunc(Callee, ArgsV));

    // Look up the name in the global module table.
    std::shared_ptr<FunctionAST> CalleeF = Functions[Callee];
    if (!CalleeF)
        return LogErrorV("Unknown function referenced");

    // If argument mismatch error.
    if (CalleeF->argsSize() != Args.size())
        return LogErrorV("Incorrect number of arguments passed");
    
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

    Value BodyExpr;
    while (true)
    {
        Value EndCond = End->execute(lvl, stackIdx);
        if (EndCond.isErr())
            return Value(type::_ERR);

        if (!(bool)EndCond.getVal()) break;

        BodyExpr = Body->execute(lvl + 1, StartIdx);
        if (BodyExpr.isErr() || BodyExpr.getType() == type::_BREAK) break;

        StackMemory.setValue(AddrTable[StartValLvl][VarName],
            Value(StackMemory.getValue(AddrTable[StartValLvl][VarName]).getVal() + StepVal.getVal()));
    }
    AddrTable.pop_back();
    ArrTable.pop_back();
    StackMemory.quickDelete(StartIdx);

    if (BodyExpr.isErr())
        return Value(type::_ERR);

    if (BodyExpr.getType() == type::_BREAK) return Value(type::_DOUBLE, BodyExpr.getVal());
    else return BodyExpr;
}

Value WhileExprAST::execute(int lvl, int stackIdx)
{
    int StartIdx = StackMemory.getSize();
    AddrTable.push_back(std::map<std::string, int>());
    ArrTable.push_back(std::map<std::string, std::vector<int>>());

    Value BodyExpr;
    while (true)
    {
        Value EndCond = Cond->execute(lvl, stackIdx);
        if (EndCond.isErr())
            return Value(type::_ERR);

        if (!(bool)EndCond.getVal()) break;

        BodyExpr = Body->execute(lvl + 1, StartIdx);
        if (BodyExpr.isErr() || BodyExpr.getType() == type::_BREAK) break;
    }
    AddrTable.pop_back();
    ArrTable.pop_back();
    StackMemory.quickDelete(StartIdx);

    if (BodyExpr.isErr())
        return Value(type::_ERR);

    if (BodyExpr.getType() == type::_BREAK) return Value(type::_DOUBLE, BodyExpr.getVal());
    else return BodyExpr;
}

Value RepeatExprAST::execute(int lvl, int stackIdx)
{
    Value Iter = IterNum->execute(lvl, stackIdx);
    if (Iter.isErr())
        return Value(type::_ERR);

    int StartIdx = StackMemory.getSize();
    AddrTable.push_back(std::map<std::string, int>());
    ArrTable.push_back(std::map<std::string, std::vector<int>>());

    Value BodyExpr;
    for (unsigned i = 0; i < (unsigned)(Iter.getVal()); i++)
    {
        BodyExpr = Body->execute(lvl + 1, StackMemory.getSize());
        if (BodyExpr.isErr() || BodyExpr.getType() == type::_BREAK) break;
    }

    AddrTable.pop_back();
    ArrTable.pop_back();
    StackMemory.quickDelete(StartIdx);

    if (BodyExpr.isErr())
        return Value(type::_ERR);

    if (BodyExpr.getType() == type::_BREAK) return Value(type::_DOUBLE, BodyExpr.getVal());
    else return BodyExpr;
}

Value LoopExprAST::execute(int lvl, int stackIdx)
{
    int StartIdx = StackMemory.getSize();
    AddrTable.push_back(std::map<std::string, int>());
    ArrTable.push_back(std::map<std::string, std::vector<int>>());

    Value BodyExpr;
    while (true)
    {
        BodyExpr = Body->execute(lvl + 1, StackMemory.getSize());
        if (BodyExpr.isErr() || BodyExpr.getType() == type::_BREAK) break;
    }

    AddrTable.pop_back();
    ArrTable.pop_back();
    StackMemory.quickDelete(StartIdx);

    if (BodyExpr.isErr())
        return Value(type::_ERR);

    if (BodyExpr.getType() == type::_BREAK) return Value(type::_DOUBLE, BodyExpr.getVal());
    else return BodyExpr;
}

Value BreakExprAST::execute(int lvl, int stackIdx)
{
    Value RetVal = Expr->execute(lvl, stackIdx);
    if (RetVal.isErr())
        return LogErrorV("Failed to return a value");

    return Value(type::_BREAK, RetVal.getVal());
}

Value ReturnExprAST::execute(int lvl, int stackIdx)
{
    Value RetVal = Expr->execute(lvl, stackIdx);
    if (RetVal.isErr())
        return LogErrorV("Failed to return a value");

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

    int level = lvl;

    if (Proto->getName() != "__anon_expr") // user defined function, not top-level expr
    {
        AddrTable.push_back(std::map<std::string, int>());
        ArrTable.push_back(std::map<std::string, std::vector<int>>());
        level++;
    }

    auto& Arg = Proto->getArgs();
    for (int i = 0; i < Proto->getArgsSize(); i++)
    {
        AddrTable[level][Arg[i]] = StackMemory.addValue(Ops[i]);
    }

    Value RetVal(type::_ERR);
    RetVal = Body->execute(level, StackMemory.getSize());

    if (Proto->getName() != "__anon_expr")
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

    return Value(type::_ERR);
}

void HandleDefinition(std::string& Code, int* Idx)
{
    if (auto FnAST = ParseDefinition(Code, Idx))
    {
        if (IsInteractive) fprintf(stderr, "Read function definition\n");
        Functions[FnAST->getFuncName()] = FnAST;
    }
    else GetNextToken(Code, Idx); // Skip token for error recovery.
}

void HandleTopLevelExpression(std::string& Code, int* Idx)
{
    // Evaluate a top-level expression into an anonymous function.
    if (auto FnAST = ParseTopLevelExpr(Code, Idx))
    {
        Value RetVal = FnAST->execute(std::vector<Value>(), 0, 0);
        if (!RetVal.isErr())
        {
            double FP = RetVal.getVal();
            if (IsInteractive) fprintf(stderr, "Evaluated to %f\n", FP);
        }
    }
    else GetNextToken(Code, Idx); // Skip token for error recovery.
}

void HandleImport(std::string& Code, int* Idx, int tmpCurTok, int tmpLastChar, bool tmpFlag)
{
    if (auto ImAST = ParseImport(Code, Idx))
    {
        IsInteractive = false;
        int moduleIdx = 0;
        std::string ModuleCode;

        FILE* fp = fopen(ImAST->getModuleName().c_str(), "r");
        if (fp == NULL)
        {
            fprintf(stderr, "Error: Cannot find module\n");
            return;
        }
        while (!feof(fp)) ModuleCode += (char)fgetc(fp);
        ModuleCode += EOF;
        fclose(fp);

        GetNextToken(ModuleCode, &moduleIdx);

        while (true)
        {
            if (CurTok == tok_eof) break;

            switch (CurTok)
            {
            case tok_import:
                HandleImport(ModuleCode, &moduleIdx, CurTok, LastChar, tmpFlag);
                GetNextToken(ModuleCode, &moduleIdx);
                break;
            case tok_def:
                HandleDefinition(ModuleCode, &moduleIdx);
                break;
            default:
                GetNextToken(ModuleCode, &moduleIdx);
                break;
            }
        }
        CurTok = tmpCurTok, LastChar = tmpLastChar;

        if (tmpFlag) fprintf(stderr, "Successfully installed module \"%s\".\n",
            ImAST->getModuleName().c_str());
    }
    else GetNextToken(Code, Idx); // Skip token for error recovery.
}

/// top ::= definition | import | external | expression | ';'
void MainLoop(std::string& Code, int* Idx)
{
    bool tmpFlag = false;
    while (true)
    {
        if (IsInteractive) fprintf(stderr, ">>> ");
        switch (CurTok)
        {
        case tok_eof:
            return;
        case ';': // ignore top-level semicolons.
            GetNextToken(Code, Idx);
            break;
        case tok_import:
            if (IsInteractive) tmpFlag = true;
            HandleImport(Code, Idx, CurTok, LastChar, tmpFlag);
            IsInteractive = tmpFlag;
            if (IsInteractive) fprintf(stderr, ">>> ");
            GetNextToken(Code, Idx);
            break;
        case tok_def:
            HandleDefinition(Code, Idx);
            break;
        case cmd_help:
            if (IsInteractive)
            {
                GetNextToken(Code, Idx);
                RunHelp();
            }
            break;
        default:
            HandleTopLevelExpression(Code, Idx);
            break;
        }
    }
}

void Execute(const char* FileName)
{
    IsInteractive = false;

    FILE* fp = fopen(FileName, "r");
    if (fp == NULL)
    {
        fprintf(stderr, "Error: Unknown file name\n");
        return;
    }
    while (!feof(fp)) MainCode += (char)fgetc(fp);
    MainCode += EOF;
    fclose(fp);

#ifdef _WIN32
    DWORD t = GetTickCount64();
#endif

    InitBinopPrec();
    GetNextToken(MainCode, &MainIdx);
    MainLoop(MainCode, &MainIdx);

#ifdef _WIN32
    DWORD diff = (GetTickCount64() - t);
    fprintf(stderr, "\nExecution finished (%.3lfs).\n", (double)diff / 1000);
#else
    fprintf(stderr, "\nExecution finished.\n");
#endif
}