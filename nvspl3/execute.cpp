#include "lexer.h"
#include "parser.h"
#include "execute.h"
#include <iostream>

static std::vector<std::map<std::string, int>> AddrTable(1);
static std::vector<std::map<std::string, std::vector<int>>> ArrTable(1);
static std::map<std::string, std::shared_ptr<FunctionAST>> Functions;
static Memory StackMemory;

Value LogErrorV(const char* Str) {
    LogError(Str);
    Value RetVal(true);
    return RetVal;
}

Value NumberExprAST::execute(int lvl, int stackIdx)
{
    return Val;
}

Value VariableExprAST::execute(int lvl, int stackIdx)
{
    if (!Indices.empty()) // array element
    {
        std::cout << "array element" << std::endl;
        for (int i = lvl; i >= 0; i--)
        {
            std::cout << "checking" << std::endl;
            if (AddrTable[i].find(Name) != AddrTable[i].end())
            {
                std::cout << "array found" << std::endl;
                for (int j = lvl; j >= 0; j--)
                {
                    if (ArrTable[j].find(Name) != ArrTable[j].end())
                    {
                        std::vector<Value> IdxV;
                        for (unsigned k = 0, e = Indices.size(); k != e; ++k) {
                            IdxV.push_back(Indices[k]->execute(lvl, stackIdx));
                            if (IdxV.back().isEmpty()) return LogErrorV("error while calculating indices");
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

                        std::cout << AddrTable[i][Name] + AddVal << std::endl;

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
    std::cout << "allocate array" << std::endl;

    AddrTable[lvl][Name] = StackMemory.addValue(Value(0.0));
    std::cout << AddrTable[lvl][Name] << std::endl;
    ArrTable[lvl][Name] = Indices;

    int size = 1;
    for (int i = 0; i < Indices.size(); i++) size *= Indices[i];
    for (int i = 0; i < size - 1; i++) StackMemory.addValue(Value(0.0));

    std::cout << "size: " << size << std::endl;
    std::cout << StackMemory.getSize() << std::endl;

    Value RetVal(true);
    return RetVal;
}

Value UnaryExprAST::execute(int lvl, int stackIdx)
{
    Value OperandV = Operand->execute(lvl, stackIdx);
    if (OperandV.isEmpty())
    {
        Value RetVal(true);
        return RetVal;
    }

    Value RetVal;
    switch (Opcode)
    {
    case '!':
        RetVal = Value(!(bool)(OperandV.getVal()));
        return RetVal;
        break;
    case '+':
        RetVal = Value(+(OperandV.getVal()));
        return RetVal;
        break;
    case '-':
        RetVal = Value(-(OperandV.getVal()));
        return RetVal;
        break;
    }
    /*
    std::shared_ptr<FunctionAST> F = Functions[(std::string("unary") + Opcode)];
    if (!F)
        return LogErrorV("Unknown unary operator");

    std::vector<Value> Op;
    Op.push_back(OperandV);

    return F->execute(Op);
    */
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
        std::cout << "Val: " << Val.getVal() << std::endl;

        if (Val.isEmpty())
        {
            Value RetVal(true);
            return RetVal;
        }

        // Look up the name.
        std::vector<std::shared_ptr<ExprAST>> Indices = LHSE->getIndices();
        if (!Indices.empty()) // array element
        {
            std::cout << "array element" << std::endl;
            for (int i = lvl; i >= 0; i--)
            {
                std::cout << "checking" << std::endl;
                if (AddrTable[i].find(LHSE->getName()) != AddrTable[i].end())
                {
                    std::cout << "array found" << std::endl;
                    int j;
                    for (j = lvl; j >= 0; j--)
                    {
                        if (ArrTable[j].find(LHSE->getName()) != ArrTable[j].end())
                        {
                            std::vector<Value> IdxV;
                            for (unsigned k = 0, e = Indices.size(); k != e; ++k) {
                                IdxV.push_back(Indices[k]->execute(lvl, stackIdx));
                                if (IdxV.back().isEmpty()) return LogErrorV("error while calculating indices");
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
                            std::cout << AddrTable[i][LHSE->getName()] + AddVal << std::endl;
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
    // std::cout << L.getVal() << ' ' << R.getVal() << std::endl;
    if (L.isEmpty() || R.isEmpty())
    {
        Value RetVal(true);
        return RetVal;
    }
    if (Op == "==")
    {
        Value RetVal((double)(L.getVal() == R.getVal()));
        return RetVal;
    }
    if (Op == "!=")
    {
        Value RetVal((double)(L.getVal() != R.getVal()));
        return RetVal;
    }
    if (Op == "<")
    {
        Value RetVal((double)(L.getVal() < R.getVal()));
        return RetVal;
    }
    if (Op == ">")
    {
        Value RetVal((double)(L.getVal() > R.getVal()));
        return RetVal;
    }
    if (Op == "<=") 
    {
        Value RetVal((double)(L.getVal() <= R.getVal()));
        return RetVal;
    }
    if (Op == ">=")
    {
        Value RetVal((double)(L.getVal() >= R.getVal()));
        return RetVal;
    }
    if (Op == "+")
    {
        Value RetVal((double)(L.getVal() + R.getVal()));
        return RetVal;
    }
    if (Op == "-") 
    {
        Value RetVal((double)(L.getVal() - R.getVal()));
        return RetVal;
    }
    if (Op == "*")
    {
        Value RetVal((double)(L.getVal() * R.getVal()));
        return RetVal;
    }
    if (Op == "/")
    {
        Value RetVal((double)(L.getVal() / R.getVal()));
        return RetVal;
    }

    // If it wasn't a builtin binary operator, it must be a user defined one. Emit
    // a call to it.
    /*
    std::shared_ptr<FunctionAST> F = Functions[(std::string("binary") + Op)];
    assert(F && "binary operator not found!");

    std::vector<Value> Ops;
    Ops.push_back(L);
    Ops.push_back(R);

    return F->execute(Ops);
    */
}

Value CallExprAST::execute(int lvl, int stackIdx) {
    // Look up the name in the global module table.
    std::shared_ptr<FunctionAST> CalleeF = Functions[Callee];
    if (!CalleeF)
        return LogErrorV("Unknown function referenced");

    // If argument mismatch error.
    if (CalleeF->arg_size() != Args.size())
        return LogErrorV("Incorrect # arguments passed");

    std::vector<Value> ArgsV;
    for (unsigned i = 0, e = Args.size(); i != e; ++i) {
        ArgsV.push_back(Args[i]->execute(lvl, stackIdx));
        if (ArgsV.back().isEmpty())
        {
            Value RetVal(true);
            return RetVal;
        }
    }
    return CalleeF->execute(ArgsV, lvl, stackIdx);
}

Value IfExprAST::execute(int lvl, int stackIdx)
{
    Value CondV = Cond->execute(lvl, stackIdx);
    if (CondV.isEmpty())
    {
        Value RetVal(true);
        return RetVal;
    }

    int StartIdx = StackMemory.getSize();
    AddrTable.push_back(std::map<std::string, int>());
    ArrTable.push_back(std::map<std::string, std::vector<int>>());

    if ((bool)(CondV.getVal()))
    {
        // std::cout << "then" << std::endl;
        Value ThenV = Then->execute(lvl + 1, StartIdx);
        AddrTable.pop_back();
        ArrTable.pop_back();
        StackMemory.quickDelete(StartIdx);

        if (ThenV.isEmpty())
        {
            Value RetVal(true);
            return RetVal;
        }
        return ThenV;
    }
    else
    {
        // std::cout << "else" << std::endl;
        Value ElseV = Else->execute(lvl + 1, StartIdx);
        AddrTable.pop_back();
        ArrTable.pop_back();
        StackMemory.quickDelete(StartIdx);

        if (ElseV.isEmpty())
        {
            Value RetVal(true);
            return RetVal;
        }
        return ElseV;
    }
}

Value ForExprAST::execute(int lvl, int stackIdx)
{
    Value StartVal = Start->execute(lvl, stackIdx);
    if (StartVal.isEmpty())
    {
        Value RetVal(true);
        return RetVal;
    }
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
        if (StepVal.isEmpty())
        {
            Value RetVal(true);
            return RetVal;
        }
    }

    int StartIdx = StackMemory.getSize();
    AddrTable.push_back(std::map<std::string, int>());
    ArrTable.push_back(std::map<std::string, std::vector<int>>());

    while (true)
    {
        Value EndCond = End->execute(lvl, stackIdx);
        if (EndCond.isEmpty())
        {
            Value RetVal(true);
            return RetVal;
        }
        if (!(bool)EndCond.getVal()) break;

        Value BodyExpr = Body->execute(lvl + 1, StartIdx);
        if (BodyExpr.isEmpty())
        {
            Value RetVal(true);
            return RetVal;
        }

        StackMemory.setValue(AddrTable[StartValLvl][VarName],
            Value(StackMemory.getValue(AddrTable[StartValLvl][VarName]).getVal() + StepVal.getVal()));
    }
    AddrTable.pop_back();
    ArrTable.pop_back();
    StackMemory.quickDelete(StartIdx);

    Value RetVal(0.0);
    return RetVal;
}

Value RepeatExprAST::execute(int lvl, int stackIdx)
{
    Value Iter = IterNum->execute(lvl, stackIdx);
    if (Iter.isEmpty())
    {
        Value RetVal(true);
        return RetVal;
    }
   
    for (unsigned i = 0; i < (unsigned)(Iter.getVal()); i++)
    {
        Value BodyExpr = Body->execute(lvl + 1, StackMemory.getSize());
        if (BodyExpr.isEmpty())
        {
            Value RetVal(true);
            return RetVal;
        }
    }
    Value RetVal(0.0);
    return RetVal;
}

Value BlockExprAST::execute(int lvl, int stackIdx)
{
    Value RetVal(true);
    int StartIdx = StackMemory.getSize();
    AddrTable.push_back(std::map<std::string, int>());
    ArrTable.push_back(std::map<std::string, std::vector<int>>());

    for (auto& Expr : Expressions)
        RetVal = Expr->execute(lvl + 1, StartIdx);

    AddrTable.pop_back();
    ArrTable.pop_back();
    StackMemory.quickDelete(StartIdx);
    return RetVal;
}

Value PrototypeAST::execute(int lvl, int stackIdx)
{
    Value RetVal(0.0);
    return RetVal;
}

Value FunctionAST::execute(std::vector<Value> Ops, int lvl, int stackIdx)
{
    // If this is an operator, install it.
    if (Proto->isBinaryOp())
        BinopPrecedence[Proto->getOperatorName()] = Proto->getBinaryPrecedence();

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

    Value RetVal;
    if (lvl > 0) RetVal = Body->execute(lvl + 1, StackMemory.getSize());
    else RetVal = Body->execute(lvl, StackMemory.getSize());

    if (lvl > 0)
    {
        AddrTable.pop_back();
        ArrTable.pop_back();
        StackMemory.quickDelete(StartIdx);
    }

    if (!RetVal.isEmpty())
        return RetVal;

    if (Proto->isBinaryOp())
        BinopPrecedence.erase(Proto->getOperatorName());

    RetVal = Value(true);
    return RetVal;
}

void HandleDefinition()
{
    if (auto FnAST = ParseDefinition())
    {
        fprintf(stderr, "Read function definition:");
        fprintf(stderr, "\n");

        Functions[FnAST->getFuncName()] = FnAST;
    }
    else
    {
        // Skip token for error recovery.
        getNextToken();
    }
}

void HandleTopLevelExpression()
{
    // Evaluate a top-level expression into an anonymous function.
    if (auto FnAST = ParseTopLevelExpr())
    {
        Value RetVal = FnAST->execute(std::vector<Value>(), 0, 0);
        if (!RetVal.isEmpty())
        {
            double FP = RetVal.getVal();
            fprintf(stderr, "Evaluated to %f\n", FP);
        }
    }
    else
    {
        // Skip token for error recovery.
        getNextToken();
    }
}

/// top ::= definition | external | expression | ';'
void MainLoop() {
    while (true) {
        fprintf(stderr, "ready> ");
        switch (CurTok) {
        case tok_eof:
            return;
        case ';': // ignore top-level semicolons.
            getNextToken();
            break;
        case tok_def:
            HandleDefinition();
            break;
        default:
            HandleTopLevelExpression();
            break;
        }
    }
}
