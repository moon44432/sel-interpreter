#include "lexer.h"
#include "parser.h"
#include "execute.h"
#include <iostream>

static std::vector<std::map<std::string, Value>> NamedValues(65536);
static std::map<std::string, std::shared_ptr<FunctionAST>> Functions;

Value LogErrorV(const char* Str) {
    LogError(Str);
    Value RetVal(true);
    return RetVal;
}

Value NumberExprAST::execute(int lvl)
{
    return Val;
}

Value VariableExprAST::execute(int lvl)
{
    // Look this variable up in the function.
    for (int i = lvl; i >= 0; i--)
    {
        if (NamedValues[i].find(Name) != NamedValues[i].end())
            return NamedValues[i][Name];
    }

    Value RetVal(true);
    return RetVal;
}

Value UnaryExprAST::execute(int lvl)
{
    Value OperandV = Operand->execute(lvl);
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

Value BinaryExprAST::execute(int lvl) {
    // Special case '=' because we don't want to emit the LHS as an expression.
    if (Op == "=") {
        // Assignment requires the LHS to be an identifier.
        VariableExprAST* LHSE = static_cast<VariableExprAST*>(LHS.get());
        if (!LHSE)
            return LogErrorV("destination of '=' must be a variable");
        // execute the RHS.
        Value Val = RHS->execute(lvl);
        if (Val.isEmpty())
        {
            Value RetVal(true);
            return RetVal;
        }

        // Look up the name.
        bool found = false;
        for (int i = lvl; i >= 0; i--)
        {
            if (NamedValues[i].find(LHSE->getName()) != NamedValues[i].end())
            {
                NamedValues[i][LHSE->getName()] = Val;
                found = true;
                break;
            }
        }
        if (!found)
            NamedValues[lvl][LHSE->getName()] = Val;

        return Val;
    }

    Value L = LHS->execute(lvl);
    Value R = RHS->execute(lvl);
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

Value CallExprAST::execute(int lvl) {
    // Look up the name in the global module table.
    std::shared_ptr<FunctionAST> CalleeF = Functions[Callee];
    if (!CalleeF)
        return LogErrorV("Unknown function referenced");

    // If argument mismatch error.
    if (CalleeF->arg_size() != Args.size())
        return LogErrorV("Incorrect # arguments passed");

    std::vector<Value> ArgsV;
    for (unsigned i = 0, e = Args.size(); i != e; ++i) {
        ArgsV.push_back(Args[i]->execute(lvl));
        if (ArgsV.back().isEmpty())
        {
            Value RetVal(true);
            return RetVal;
        }
    }
    return CalleeF->execute(ArgsV, lvl);
}

Value IfExprAST::execute(int lvl)
{
    Value CondV = Cond->execute(lvl);
    if (CondV.isEmpty())
    {
        Value RetVal(true);
        return RetVal;
    }

    if ((bool)(CondV.getVal()))
    {
        // std::cout << "then" << std::endl;
        Value ThenV = Then->execute(lvl + 1);
        if (ThenV.isEmpty())
        {
            Value RetVal(true);
            return RetVal;
        }
        NamedValues[lvl + 1].clear();
        return ThenV;
    }
    else
    {
        // std::cout << "else" << std::endl;
        Value ElseV = Else->execute(lvl + 1);
        if (ElseV.isEmpty())
        {
            Value RetVal(true);
            return RetVal;
        }
        NamedValues[lvl + 1].clear();
        return ElseV;
    }
}

Value ForExprAST::execute(int lvl)
{
    Value StartVal = Start->execute(lvl);
    if (StartVal.isEmpty())
    {
        Value RetVal(true);
        return RetVal;
    }
    bool found = false;
    int StartValLvl;
    for (int i = lvl; i >= 0; i--)
    {
        if (NamedValues[i].find(VarName) != NamedValues[i].end())
        {
            NamedValues[i][VarName] = StartVal;
            StartValLvl = i;
            found = true;
            break;
        }
    }
    if (!found)
    {
        NamedValues[lvl][VarName] = StartVal;
        StartValLvl = lvl;
    }

    // Emit the step value.
    Value StepVal(1.0);
    if (Step) {
        StepVal = Step->execute(lvl);
        if (StepVal.isEmpty())
        {
            Value RetVal(true);
            return RetVal;
        }
    }

    while (true)
    {
        Value EndCond = End->execute(lvl);
        if (EndCond.isEmpty())
        {
            Value RetVal(true);
            return RetVal;
        }
        if (!(bool)EndCond.getVal()) break;

        Value BodyExpr = Body->execute(lvl + 1);
        if (BodyExpr.isEmpty())
        {
            Value RetVal(true);
            return RetVal;
        }

        NamedValues[StartValLvl][VarName].updateVal(NamedValues[StartValLvl][VarName].getVal() + StepVal.getVal());
    }
    NamedValues[lvl + 1].clear();

    Value RetVal(0.0);
    return RetVal;
}

Value RepeatExprAST::execute(int lvl)
{
    Value Iter = IterNum->execute(lvl);
    if (Iter.isEmpty())
    {
        Value RetVal(true);
        return RetVal;
    }
   
    for (int i = 0; i < (unsigned int)(Iter.getVal()); i++)
    {
        Value BodyExpr = Body->execute(lvl + 1);
        if (BodyExpr.isEmpty())
        {
            Value RetVal(true);
            return RetVal;
        }
    }
    Value RetVal(0.0);
    return RetVal;
}

Value BlockExprAST::execute(int lvl)
{
    Value RetVal(true);
    for (auto& Expr : Expressions)
        RetVal = Expr->execute(lvl + 1);

    NamedValues[lvl + 1].clear();
    return RetVal;
}

Value PrototypeAST::execute(int lvl)
{
    Value RetVal(0.0);
    return RetVal;
}

Value FunctionAST::execute(std::vector<Value> Ops, int lvl)
{
    // If this is an operator, install it.
    if (Proto->isBinaryOp())
        BinopPrecedence[Proto->getOperatorName()] = Proto->getBinaryPrecedence();

    auto& Arg = Proto->getArgs();
    for (int i = 0; i < Proto->getArgsSize(); i++)
        NamedValues[lvl + 1][Arg[i]] = Ops[i].getVal();

    Value RetVal = Body->execute(lvl + 1);
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
        Value RetVal = FnAST->execute(std::vector<Value>(), 0);
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
