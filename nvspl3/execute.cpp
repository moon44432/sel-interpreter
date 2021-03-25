#include "lexer.h"
#include "parser.h"
#include "execute.h"
#include <iostream>

static std::vector<std::map<std::string, int>> VarTable;
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
    // Look this variable up in the function.
    for (int i = lvl; i >= 0; i--)
    {
        if (VarTable[i].find(Name) != VarTable[i].end())
            return StackMemory.getValue(VarTable[i][Name]);
    }

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

    // Last character까지 보고 정의된 binop이면 건너뛰기
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
    if (Op == "=") {
        // Assignment requires the LHS to be an identifier.
        VariableExprAST* LHSE = static_cast<VariableExprAST*>(LHS.get());
        if (!LHSE)
            return LogErrorV("destination of '=' must be a variable");
        // execute the RHS.
        Value Val = RHS->execute(lvl, stackIdx);
        if (Val.isEmpty())
        {
            Value RetVal(true);
            return RetVal;
        }

        // Look up the name.
        bool found = false;
        for (int i = lvl; i >= 0; i--)
        {
            if (VarTable[i].find(LHSE->getName()) != VarTable[i].end())
            {
                StackMemory.setValue(VarTable[i][LHSE->getName()], Val);
                found = true;
                break;
            }
        }
        if (!found)
        {
            int idx = StackMemory.addValue(Val);
            VarTable[lvl][LHSE->getName()] = idx;
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
    std::map<std::string, int> Table;
    VarTable.push_back(Table);

    if ((bool)(CondV.getVal()))
    {
        // std::cout << "then" << std::endl;
        Value ThenV = Then->execute(lvl + 1, StartIdx);
        if (ThenV.isEmpty())
        {
            Value RetVal(true);
            return RetVal;
        }
        VarTable.pop_back();
        StackMemory.quickDelete(StartIdx);
        return ThenV;
    }
    else
    {
        // std::cout << "else" << std::endl;
        Value ElseV = Else->execute(lvl + 1, StartIdx);
        if (ElseV.isEmpty())
        {
            Value RetVal(true);
            return RetVal;
        }
        VarTable.pop_back();
        StackMemory.quickDelete(StartIdx);
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
        if (VarTable[i].find(VarName) != VarTable[i].end())
        {
            StackMemory.setValue(VarTable[i][VarName], StartVal);
            StartValLvl = i;
            found = true;
            break;
        }
    }
    if (!found)
    {
        int idx = StackMemory.addValue(StartVal);
        VarTable[lvl][VarName] = idx;
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
    std::map<std::string, int> Table;
    VarTable.push_back(Table);

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

        StackMemory.setValue(VarTable[StartValLvl][VarName],
            Value(StackMemory.getValue(VarTable[StartValLvl][VarName]).getVal() + StepVal.getVal()));
    }
    VarTable.pop_back();
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
   
    for (int i = 0; i < (unsigned int)(Iter.getVal()); i++)
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
    std::map<std::string, int> Table;
    VarTable.push_back(Table);

    for (auto& Expr : Expressions)
        RetVal = Expr->execute(lvl + 1, StartIdx);

    VarTable.pop_back();
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
    std::map<std::string, int> Table;
    VarTable.push_back(Table);

    auto& Arg = Proto->getArgs();
    for (int i = 0; i < Proto->getArgsSize(); i++)
    {
        int idx = StackMemory.addValue(Ops[i]);
        VarTable[lvl + 1][Arg[i]] = idx;
    }

    Value RetVal = Body->execute(lvl + 1, StackMemory.getSize());
    if (!RetVal.isEmpty())
        return RetVal;

    VarTable.pop_back();
    StackMemory.quickDelete(StartIdx);

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
        std::map<std::string, int> Table;
        VarTable.push_back(Table);

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
