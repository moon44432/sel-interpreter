
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

static std::map<std::string, std::shared_ptr<FunctionAST>> Functions;
static std::vector<namedValue> SymTbl;
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
    return Value(Err);
}

Value NumberExprAST::execute()
{
    return Val;
}

Value DeRefExprAST::execute()
{
    Value Address = AddrExpr->execute();
    if (Address.isErr())
        return Value(Err);
    if (Address.isUInt()) // address should be an uint
        return StackMemory.getValue(Address.getVal().i);

    return LogErrorV("Address must be an unsigned integer");
}

Value HandleArr(std::string ArrName, std::vector<std::shared_ptr<ExprAST>>& Indices, arrAction Action, Value Val)
{
    for (int i = SymTbl.size() - 1; i >= 0; i--)
    {
        if (SymTbl[i].Name == ArrName && SymTbl[i].IsArr)
        {
            std::vector<Value> IdxV;
            for (int k = 0, e = Indices.size(); k != e; ++k) {
                IdxV.push_back(Indices[k]->execute());
                if (IdxV.back().isErr()) return LogErrorV("Error while calculating indices");
                if (!IdxV.back().isInt()) return LogErrorV("Index must be an integer");
            }

            if (IdxV.size() != SymTbl[i].Dim.size()) return LogErrorV("Dimension mismatch");

            int AddVal = 0;
            for (int l = 0; l < IdxV.size(); l++)
            {
                int MulVal = 1;
                for (int m = l + 1; m < IdxV.size(); m++) MulVal *= SymTbl[i].Dim[m];
                AddVal += MulVal * IdxV[l].getVal().i;
            }
            switch (Action)
            {
            case arrAction::_GETVAL:
                return StackMemory.getValue(SymTbl[i].Addr + AddVal);
            case arrAction::_GETADDR:
                return Value((int)(SymTbl[i].Addr + AddVal));
            case arrAction::_SETVAL:
                StackMemory.setValue(SymTbl[i].Addr + AddVal, Val);
                return Val;
            }
        }
    }
    return LogErrorV((((std::string)("\"") + ArrName + (std::string)("\" is not an array"))).c_str());
}

Value HandleArr(std::string ArrName, std::vector<std::shared_ptr<ExprAST>>& Indices, arrAction Action) { return HandleArr(ArrName, Indices, Action, Value()); }

Value VariableExprAST::execute()
{
    if (!Indices.empty()) // array element
        return HandleArr(Name, Indices, arrAction::_GETVAL);

    // normal variable
    for (int i = SymTbl.size() - 1; i >= 0; i--)
    {
        if (SymTbl[i].Name == Name && !SymTbl[i].IsArr)
            return StackMemory.getValue(SymTbl[i].Addr);
    }
    return LogErrorV(std::string("Identifier \"" + Name + "\" not found").c_str());
}

Value ArrDeclExprAST::execute()
{
    namedValue Arr = { Name, StackMemory.push(Value(0)), true, Indices };
    SymTbl.push_back(Arr);

    int size = 1;
    for (int i = 0; i < Indices.size(); i++) size *= Indices[i];
    for (int i = 0; i < size - 1; i++) StackMemory.push(Value(0));
    
    return Value(size);
}

Value UnaryExprAST::execute()
{
    if (Opcode == '&') // reference operator
    {
        VariableExprAST* Op = static_cast<VariableExprAST*>(Operand.get());
        if (!Op) return LogErrorV("Operand of '&' must be a variable");

        std::vector<std::shared_ptr<ExprAST>> Indices = Op->getIndices();
        if (!Indices.empty()) // array element
        {
            return HandleArr(Op->getName(), Indices, arrAction::_GETADDR);
        }
        else // normal variable
        {
            for (int i = SymTbl.size() - 1; i >= 0; i--)
            {
                if (SymTbl[i].Name == Op->getName())
                {
                    return Value((int)SymTbl[i].Addr);
                }
            }
            return LogErrorV(std::string("Variable \"" + Op->getName() + "\" not found").c_str());
        }
    }

    Value OperandV = Operand->execute();
    if (OperandV.isErr())
        return Value(Err);

    switch (Opcode)
    {
    case '!':
        if (OperandV.getType().dType == dataType::_DOUBLE) return Value(!(bool)(OperandV.getVal().dbl));
        else if (OperandV.getType().dType == dataType::_INT) return Value(!(bool)(OperandV.getVal().i));
        break;
    case '+':
        if (OperandV.getType().dType == dataType::_DOUBLE) return Value(+(OperandV.getVal().dbl));
        else if (OperandV.getType().dType == dataType::_INT) return Value(+(OperandV.getVal().i));
        break;
    case '-':
        if (OperandV.getType().dType == dataType::_DOUBLE) return Value(-(OperandV.getVal().dbl));
        else if (OperandV.getType().dType == dataType::_INT) return Value(-(OperandV.getVal().i));
        break;
    }

    std::shared_ptr<FunctionAST> F = Functions[(std::string("unary") + Opcode)];
    if (!F) return LogErrorV("Unknown unary operator");

    std::vector<Value> Op;
    Op.push_back(OperandV);

    return F->execute(Op);
}

Value BinaryExprAST::execute() {
    // Special case '=' because we don't want to emit the LHS as an expression.
    if (Op == "=")
    {
        // execute the RHS.
        Value Val = RHS->execute();

        if (Val.isErr())
            return Value(Err);

        // Assignment requires the LHS to be an identifier.
        VariableExprAST* LHSE = dynamic_cast<VariableExprAST*>(LHS.get());
        if (!LHSE)
        {
            DeRefExprAST* LHSE = dynamic_cast<DeRefExprAST*>(LHS.get());
            if (!LHSE)
                return LogErrorV("Destination of '=' must be a variable");
            
            // update value at the memory address
            Value Addr = LHSE->getExpr()->execute();
            if (!Addr.isUInt()) return LogErrorV("Address must be an unsigned integer");
            
            StackMemory.setValue(Addr.getVal().i, Val);
            return Val;
        }

        // Look up the name.
        std::vector<std::shared_ptr<ExprAST>> Indices = LHSE->getIndices();
        if (!Indices.empty()) // array element
        {
            return HandleArr(LHSE->getName(), Indices, arrAction::_SETVAL, Val);
        }
        else // normal variable
        {
            bool found = false;
            for (int i = SymTbl.size() - 1; i >= 0; i--)
            {
                if (SymTbl[i].Name == LHSE->getName())
                {
                    StackMemory.setValue(SymTbl[i].Addr, Val);
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                namedValue Var = { LHSE->getName(), StackMemory.push(Val), false };
                SymTbl.push_back(Var);
            }
        }
        return Val;
    }

    Value L = LHS->execute();
    Value R = RHS->execute();

    if (L.isErr() || R.isErr())
        return Value(Err);

    dataType ResultType = (L.getType().dType >= R.getType().dType) ? L.getType().dType : R.getType().dType;

    if (Op == "==")
    {
        if (ResultType == dataType::_DOUBLE) return Value((L.getdVal() == R.getdVal()));
        else if (ResultType == dataType::_INT) return Value((L.getiVal() == R.getiVal()));
    }

    if (Op == "!=")
    {
        if (ResultType == dataType::_DOUBLE) return Value((L.getdVal() != R.getdVal()));
        else if (ResultType == dataType::_INT) return Value((L.getiVal() != R.getiVal()));
    }

    if (Op == "&&")
    {
        if (ResultType == dataType::_DOUBLE) return Value((L.getdVal() && R.getdVal()));
        else if (ResultType == dataType::_INT) return Value((L.getiVal() && R.getiVal()));
    }

    if (Op == "||")
    {
        if (ResultType == dataType::_DOUBLE) return Value((L.getdVal() || R.getdVal()));
        else if (ResultType == dataType::_INT) return Value((L.getiVal() || R.getiVal()));
    }

    if (Op == "<")
    {
        if (ResultType == dataType::_DOUBLE) return Value((L.getdVal() < R.getdVal()));
        else if (ResultType == dataType::_INT) return Value((L.getiVal() < R.getiVal()));
    }

    if (Op == ">")
    {
        if (ResultType == dataType::_DOUBLE) return Value((L.getdVal() > R.getdVal()));
        else if (ResultType == dataType::_INT) return Value((L.getiVal() > R.getiVal()));
    }

    if (Op == "<=") 
    {
        if (ResultType == dataType::_DOUBLE) return Value((L.getdVal() <= R.getdVal()));
        else if (ResultType == dataType::_INT) return Value((L.getiVal() <= R.getiVal()));
    }

    if (Op == ">=")
    {
        if (ResultType == dataType::_DOUBLE) return Value((L.getdVal() >= R.getdVal()));
        else if (ResultType == dataType::_INT) return Value((L.getiVal() >= R.getiVal()));
    }

    if (Op == "+")
    {
        if (ResultType == dataType::_DOUBLE) return Value((double)(L.getdVal() + R.getdVal()));
        else if (ResultType == dataType::_INT) return Value((int)(L.getiVal() + R.getiVal()));
    }

    if (Op == "-") 
    {
        if (ResultType == dataType::_DOUBLE) return Value((double)(L.getdVal() - R.getdVal()));
        else if (ResultType == dataType::_INT) return Value((int)(L.getiVal() - R.getiVal()));
    }

    if (Op == "*")
    {
        if (ResultType == dataType::_DOUBLE) return Value((double)(L.getdVal() * R.getdVal()));
        else if (ResultType == dataType::_INT) return Value((int)(L.getiVal() * R.getiVal()));
    }

    if (Op == "/")
    {
        if (ResultType == dataType::_DOUBLE) return Value((double)(L.getdVal() / R.getdVal()));
        else if (ResultType == dataType::_INT) return Value((int)(L.getiVal() / R.getiVal()));
    }

    if (Op == "%")
    {
        if (ResultType == dataType::_DOUBLE) return Value((double)fmod(L.getdVal(), R.getdVal()));
        else if (ResultType == dataType::_INT) return Value((int)(L.getiVal() % R.getiVal()));
    }

    if (Op == "**")
    {
        if (ResultType == dataType::_DOUBLE) return Value((double)pow(L.getdVal(), R.getdVal()));
        else if (ResultType == dataType::_INT) return Value((int)pow(L.getiVal(), R.getiVal()));
    }

    // If it wasn't a builtin binary operator, it must be a user defined one. Emit
    // a call to it.
    
    std::shared_ptr<FunctionAST> F = Functions[(std::string("binary") + Op)];
    if (!F) return LogErrorV("Binary operator not found");

    std::vector<Value> Ops;
    Ops.push_back(L);
    Ops.push_back(R);

    return F->execute(Ops);
}

Value CallExprAST::execute()
{
    std::vector<Value> ArgsV;
    for (int i = 0, e = Args.size(); i != e; ++i) {
        ArgsV.push_back(Args[i]->execute());
        if (ArgsV.back().isErr())
            return Value(Err);
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
    
    return CalleeF->execute(ArgsV);
}

Value IfExprAST::execute()
{
    Value CondV = Cond->execute();
    if (CondV.isErr())
        return Value(Err);

    if (CondV.getdVal())
    {
        int StackIdx = StackMemory.getSize(), TblIdx = SymTbl.size();
        Value ThenV = Then->execute();

        StackMemory.deleteScope(StackIdx);
        for (int i = SymTbl.size(); i > TblIdx; i--) SymTbl.pop_back();

        if (ThenV.isErr())
            return Value(Err);
        return ThenV;
    }
    else if (Else != nullptr)
    {
        int StackIdx = StackMemory.getSize(), TblIdx = SymTbl.size();
        Value ElseV = Else->execute();
        
        StackMemory.deleteScope(StackIdx);
        for (int i = SymTbl.size(); i > TblIdx; i--) SymTbl.pop_back();

        if (ElseV.isErr())
            return Value(Err);
        return ElseV;
    }
    return Value(0);
}

Value ForExprAST::execute()
{
    Value StartVal = Start->execute();
    if (StartVal.isErr())
        return Value(Err);

    int StackIdx = StackMemory.getSize(), TblIdx = SymTbl.size();
    int StartVarAddr;

    bool found = false;
    for (int i = SymTbl.size() - 1; i >= 0; i--)
    {
        if (SymTbl[i].Name == VarName)
        {
            StackMemory.setValue(i, StartVal);
            StartVarAddr = i;
            found = true;
            break;
        }
    }
    if (!found)
    {
        namedValue Var = { VarName, StackMemory.push(StartVal), false };
        SymTbl.push_back(Var);
        StartVarAddr = SymTbl.size() - 1;
    }

    // Emit the step value.
    Value StepVal(1);
    if (Step)
    {
        StepVal = Step->execute();
        if (StepVal.isErr())
            return Value(Err);
    }

    Value BodyExpr, EndCond;
    while (true)
    {
        EndCond = End->execute();
        if (EndCond.isErr() || !EndCond.getdVal()) break;

        BodyExpr = Body->execute();
        if (BodyExpr.isErr()) break;
        if (BodyExpr.getType().vType == valueType::_BREAK)
        {
            BodyExpr.setValueType(valueType::_DATA);
            break;
        }
        StackMemory.setValue(StartVarAddr,
            Value(StackMemory.getValue(StartVarAddr).getdVal() + StepVal.getdVal()));
    }

    StackMemory.deleteScope(StackIdx);
    for (int i = SymTbl.size(); i > TblIdx; i--) SymTbl.pop_back();

    if (BodyExpr.isErr() || EndCond.isErr())
        return Value(Err);

    return BodyExpr;
}

Value WhileExprAST::execute()
{
    int StackIdx = StackMemory.getSize(), TblIdx = SymTbl.size();

    Value BodyExpr, EndCond;
    while (true)
    {
        EndCond = Cond->execute();
        if (EndCond.isErr() || !EndCond.getdVal()) break;

        BodyExpr = Body->execute();
        if (BodyExpr.isErr()) break;
        if (BodyExpr.getType().vType == valueType::_BREAK)
        {
            BodyExpr.setValueType(valueType::_DATA);
            break;
        }
    }
    StackMemory.deleteScope(StackIdx);
    for (int i = SymTbl.size(); i > TblIdx; i--) SymTbl.pop_back();

    if (EndCond.isErr() || BodyExpr.isErr())
        return Value(Err);

    return BodyExpr;
}

Value RepeatExprAST::execute()
{
    Value Iter = IterNum->execute();
    if (Iter.isErr())
        return Value(Err);
    if (!Iter.isUInt()) return LogErrorV("Number of iterations should be an unsigned integer");

    int StackIdx = StackMemory.getSize(), TblIdx = SymTbl.size();

    Value BodyExpr;
    for (int i = 0; i < Iter.getiVal(); i++)
    {
        BodyExpr = Body->execute();
        
        if (BodyExpr.isErr()) break;
        if (BodyExpr.getType().vType == valueType::_BREAK)
        {
            BodyExpr.setValueType(valueType::_DATA);
            break;
        }
    }
    StackMemory.deleteScope(StackIdx);
    for (int i = SymTbl.size(); i > TblIdx; i--) SymTbl.pop_back();

    if (BodyExpr.isErr())
        return Value(Err);
    
    return BodyExpr;
}

Value LoopExprAST::execute()
{
    int StackIdx = StackMemory.getSize(), TblIdx = SymTbl.size();

    Value BodyExpr;
    while (true)
    {
        BodyExpr = Body->execute();

        if (BodyExpr.isErr()) break;
        if (BodyExpr.getType().vType == valueType::_BREAK)
        {
            BodyExpr.setValueType(valueType::_DATA);
            break;
        }
    }
    StackMemory.deleteScope(StackIdx);
    for (int i = SymTbl.size(); i > TblIdx; i--) SymTbl.pop_back();

    if (BodyExpr.isErr())
        return Value(Err);
    
    return BodyExpr;
}

Value BreakExprAST::execute()
{
    Value RetVal = Expr->execute();
    if (RetVal.isErr())
        return LogErrorV("Failed to return a value");

    RetVal.setValueType(valueType::_BREAK);
    return RetVal;
}

Value ReturnExprAST::execute()
{
    Value RetVal = Expr->execute();
    if (RetVal.isErr())
        return LogErrorV("Failed to return a value");

    RetVal.setValueType(valueType::_RETURN);
    return RetVal;
}

Value BlockExprAST::execute()
{
    Value RetVal(0);
    int StackIdx = StackMemory.getSize(), TblIdx = SymTbl.size();

    for (auto& Expr : Expressions)
    {
        RetVal = Expr->execute();
        if (RetVal.getType().vType == valueType::_BREAK || RetVal.getType().vType == valueType::_RETURN) break;
    }
    StackMemory.deleteScope(StackIdx);
    for (int i = SymTbl.size(); i > TblIdx; i--) SymTbl.pop_back();

    return RetVal;
}

Value FunctionAST::execute(std::vector<Value> Ops)
{
    int StackIdx = StackMemory.getSize(), TblIdx = SymTbl.size();

    auto& Arg = Proto->getArgs();
    for (int i = 0; i < Proto->getArgsSize(); i++)
    {
        namedValue ArgVar = { Arg[i], StackMemory.push(Ops[i]), false };
        SymTbl.push_back(ArgVar);
    }

    Value RetVal = Body->execute();

    if (Proto->getName() != "__anon_expr")
    {
        StackMemory.deleteScope(StackIdx);
        for (int i = SymTbl.size(); i > TblIdx; i--) SymTbl.pop_back();
    }

    if (RetVal.isErr()) return Value(Err);
    if (RetVal.getType().vType == valueType::_RETURN) RetVal.setValueType(valueType::_DATA);
    
    return RetVal;
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
        Value RetVal = FnAST->execute(std::vector<Value>());
        if (!RetVal.isErr() && IsInteractive)
        {
            if (RetVal.getType().dType == dataType::_DOUBLE) fprintf(stderr, ">>> Evaluated to %f\n", RetVal.getdVal());
            else if (RetVal.getType().dType == dataType::_INT) fprintf(stderr, ">>> Evaluated to %d\n", RetVal.getiVal());
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