#include "lexer.h"
#include "parser.h"
#include "execute.h"
#include <iostream>

static std::map<std::string, Value> NamedValues;
static std::map<std::string, FunctionAST*> Functions;

Value LogErrorV(const char* Str) {
    LogError(Str);
    Value RetVal(true);
    return RetVal;
}

Value NumberExprAST::execute()
{
    return Val;
}

Value VariableExprAST::execute()
{
    // Look this variable up in the function.
    if (NamedValues.find(Name) != NamedValues.end())
    {
        Value RetVal(NamedValues[Name].getVal());
        return RetVal;
    }
    else
    {
        Value RetVal(true);
        return RetVal;
    }
}

Value UnaryExprAST::execute()
{
    Value OperandV = Operand->execute();
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

    FunctionAST* F = Functions[(std::string("unary") + Opcode)];
    if (!F)
        return LogErrorV("Unknown unary operator");

    std::vector<Value> Op;
    Op.push_back(OperandV);

    return F->execute(Op);
}

Value BinaryExprAST::execute() {
    // Special case '=' because we don't want to emit the LHS as an expression.
    if (Op == "=") {
        // Assignment requires the LHS to be an identifier.
        VariableExprAST* LHSE = static_cast<VariableExprAST*>(LHS.get());
        if (!LHSE)
            return LogErrorV("destination of '=' must be a variable");
        // execute the RHS.
        Value Val = RHS->execute();
        if (Val.isEmpty())
        {
            Value RetVal(true);
            return RetVal;
        }

        // Look up the name.
        
        if (NamedValues.find(LHSE->getName()) != NamedValues.end())
        {
            Value * Variable = &NamedValues[LHSE->getName()];
            Variable->updateVal(Val.getVal());
        }
        else NamedValues[LHSE->getName()] = Val; // add new var

        return Val;
    }

    Value L = LHS->execute();
    Value R = RHS->execute();
    std::cout << L.getVal() << ' ' << R.getVal() << std::endl;
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
    FunctionAST* F = Functions[(std::string("binary") + Op)];
    assert(F && "binary operator not found!");

    std::vector<Value> Ops;
    Ops.push_back(L);
    Ops.push_back(R);

    return F->execute(Ops);
}

Value CallExprAST::execute() {
    // Look up the name in the global module table.
    FunctionAST* CalleeF = Functions[Callee];
    if (!CalleeF)
        return LogErrorV("Unknown function referenced");

    // If argument mismatch error.
    if (CalleeF->arg_size() != Args.size())
        return LogErrorV("Incorrect # arguments passed");

    std::vector<Value> ArgsV;
    for (unsigned i = 0, e = Args.size(); i != e; ++i) {
        ArgsV.push_back(Args[i]->execute());
        if (ArgsV.back().isEmpty())
        {
            Value RetVal(true);
            return RetVal;
        }
    }
    return CalleeF->execute(ArgsV);
}

Value IfExprAST::execute() {
    Value CondV = Cond->execute();
    if (CondV.isEmpty())
    {
        Value RetVal(true);
        return RetVal;
    }
    if ((bool)(CondV.getVal()))
    {
        Value ThenV = Then->execute();
        if (ThenV.isEmpty())
        {
            Value RetVal(true);
            return RetVal;
        }
        return ThenV;
    }
    else
    {
        Value ElseV = Else->execute();
        if (ElseV.isEmpty())
        {
            Value RetVal(true);
            return RetVal;
        }
        return ElseV;
    }
}

Value ForExprAST::execute()
{
    Value StartVal = Start->execute();
    if (StartVal.isEmpty())
    {
        Value RetVal(true);
        return RetVal;
    }
    NamedValues[VarName] = StartVal;

    // Emit the step value.
    Value StepVal(1.0);
    if (Step) {
        StepVal = Step->execute();
        if (StepVal.isEmpty())
        {
            Value RetVal(true);
            return RetVal;
        }
    }

    while (true)
    {
        Value EndCond = End->execute();
        if (EndCond.isEmpty())
        {
            Value RetVal(true);
            return RetVal;
        }

        Value BodyExpr = Body->execute();
        if (BodyExpr.isEmpty())
        {
            Value RetVal(true);
            return RetVal;
        }

        NamedValues[VarName].updateVal(NamedValues[VarName].getVal() + StepVal.getVal());
    }
    Value RetVal(0.0);
    return RetVal;
}

Value BlockExprAST::execute()
{
    Value RetVal(true);
    for (auto& Expr : Expressions)
        RetVal = Expr->execute();

    return RetVal;
}

Value PrototypeAST::execute()
{
    Value RetVal(0.0);
    return RetVal;
}

Value FunctionAST::execute(std::vector<Value> Ops)
{
    Functions[Proto->getName()] = this;

    // If this is an operator, install it.
    if (Proto->isBinaryOp())
        BinopPrecedence[Proto->getOperatorName()] = Proto->getBinaryPrecedence();

    auto& Arg = Proto->getArgs();
    for (int i = 0; i < Proto->getArgsSize(); i++)
        NamedValues[Arg[i]].updateVal(Ops[i].getVal());

    Value RetVal = Body->execute();
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

        unsigned Idx = 0;
        for (auto& Arg : FnAST->getFuncArgs())
            NamedValues[Arg] = std::move(std::make_unique<Value>(0.0).get());

        Functions[FnAST->getFuncName()] = FnAST.get();
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
        Value RetVal = FnAST->execute(std::vector<Value>());
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
