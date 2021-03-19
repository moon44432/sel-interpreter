#include "lexer.h"
#include "parser.h"
#include "execute.h"

static std::map<std::string, Value*> NamedValues;
static std::map<std::string, std::unique_ptr<PrototypeAST>> FunctionProtos;
static std::map<std::string, FunctionAST*> Functions;

Value* LogErrorV(const char* Str) {
    LogError(Str);
    return nullptr;
}

Value* NumberExprAST::execute() {
    return &Val;
}

Value* VariableExprAST::execute() {
    // Look this variable up in the function.
    Value* V = NamedValues[Name];
    if (!V)
        return LogErrorV("Unknown variable name");

    // Load the value.
    return V;
}

Value* UnaryExprAST::execute() {
    Value* OperandV = Operand->execute();
    if (!OperandV)
        return nullptr;

    FunctionAST* F = Functions[(std::string("unary") + Opcode)];
    if (!F)
        return LogErrorV("Unknown unary operator");

    switch (Opcode)
    {
    case '!':
        return std::make_unique<Value>(!(OperandV->getVal())).get();
        break;
    case '+':
        return std::make_unique<Value>(+(OperandV->getVal())).get();
        break;
    case '-':
        return std::make_unique<Value>(-(OperandV->getVal())).get();
        break;
    }
}

Value* BinaryExprAST::execute() {
    // Special case '=' because we don't want to emit the LHS as an expression.
    if (Op == "=") {
        // Assignment requires the LHS to be an identifier.
        VariableExprAST* LHSE = static_cast<VariableExprAST*>(LHS.get());
        if (!LHSE)
            return LogErrorV("destination of '=' must be a variable");
        // execute the RHS.
        Value* Val = RHS->execute();
        if (!Val)
            return nullptr;

        // Look up the name.
        Value* Variable = NamedValues[LHSE->getName()];
        if (!Variable)
            NamedValues[LHSE->getName()] = Val; // add new var
        else Variable->updateVal(Val->getVal());
        return Val;
    }

    Value* L = LHS->execute();
    Value* R = RHS->execute();
    if (!L || !R)
        return nullptr;

    if (Op == "==") return std::make_unique<Value>((double)(L->getVal() == R->getVal())).get();
    if (Op == "!=") return std::make_unique<Value>((double)(L->getVal() != R->getVal())).get();
    if (Op == "<") return std::make_unique<Value>((double)(L->getVal() < R->getVal())).get();
    if (Op == ">") return std::make_unique<Value>((double)(L->getVal() > R->getVal())).get();
    if (Op == "<=") return std::make_unique<Value>((double)(L->getVal() <= R->getVal())).get();
    if (Op == ">=") return std::make_unique<Value>((double)(L->getVal() >= R->getVal())).get();
    if (Op == "+") return std::make_unique<Value>(L->getVal() + R->getVal()).get();
    if (Op == "-") return std::make_unique<Value>(L->getVal() - R->getVal()).get();
    if (Op == "*") return std::make_unique<Value>(L->getVal() * R->getVal()).get();
    if (Op == "/") return std::make_unique<Value>(L->getVal() / R->getVal()).get();

    // If it wasn't a builtin binary operator, it must be a user defined one. Emit
    // a call to it.
    FunctionAST* F = Functions[(std::string("binary") + Op)];
    assert(F && "binary operator not found!");

    Value* Ops[] = { L, R };
    return F->execute(Ops);
}

Value* CallExprAST::execute() {
    // Look up the name in the global module table.
    FunctionAST* CalleeF = Functions[Callee];
    if (!CalleeF)
        return LogErrorV("Unknown function referenced");

    // If argument mismatch error.
    if (CalleeF->arg_size() != Args.size())
        return LogErrorV("Incorrect # arguments passed");

    std::vector<Value*> ArgsV;
    for (unsigned i = 0, e = Args.size(); i != e; ++i) {
        ArgsV.push_back(Args[i]->execute());
        if (!ArgsV.back())
            return nullptr;
    }

    return Builder.CreateCall(CalleeF, ArgsV, "calltmp");
}

Value* IfExprAST::execute() {
    Value* CondV = Cond->execute();
    if (!CondV)
        return nullptr;

    // Convert condition to a bool by comparing non-equal to 0.0.
    CondV = Builder.CreateFCmpONE(
        CondV, ConstantFP::get(TheContext, APFloat(0.0)), "ifcond");

    Function* TheFunction = Builder.GetInsertBlock()->getParent();

    // Create blocks for the then and else cases.  Insert the 'then' block at the
    // end of the function.
    BasicBlock* ThenBB = BasicBlock::Create(TheContext, "then", TheFunction);
    BasicBlock* ElseBB = BasicBlock::Create(TheContext, "else");
    BasicBlock* MergeBB = BasicBlock::Create(TheContext, "ifcont");

    Builder.CreateCondBr(CondV, ThenBB, ElseBB);

    // Emit then value.
    Builder.SetInsertPoint(ThenBB);

    Value* ThenV = Then->execute();
    if (!ThenV)
        return nullptr;

    Builder.CreateBr(MergeBB);
    // execute of 'Then' can change the current block, update ThenBB for the PHI.
    ThenBB = Builder.GetInsertBlock();

    // Emit else block.
    TheFunction->getBasicBlockList().push_back(ElseBB);
    Builder.SetInsertPoint(ElseBB);

    Value* ElseV = Else->execute();
    if (!ElseV)
        return nullptr;

    Builder.CreateBr(MergeBB);
    // execute of 'Else' can change the current block, update ElseBB for the PHI.
    ElseBB = Builder.GetInsertBlock();

    // Emit merge block.
    TheFunction->getBasicBlockList().push_back(MergeBB);
    Builder.SetInsertPoint(MergeBB);
    PHINode* PN = Builder.CreatePHI(Type::getDoubleTy(TheContext), 2, "iftmp");

    PN->addIncoming(ThenV, ThenBB);
    PN->addIncoming(ElseV, ElseBB);
    return PN;
}

Value* ForExprAST::execute() {
    Function* TheFunction = Builder.GetInsertBlock()->getParent();

    // Create an alloca for the variable in the entry block.
    AllocaInst* Alloca = CreateEntryBlockAlloca(TheFunction, VarName);

    // Emit the start code first, without 'variable' in scope.
    Value* StartVal = Start->execute();
    if (!StartVal)
        return nullptr;

    // Store the value into the alloca.
    Builder.CreateStore(StartVal, Alloca);

    // Make the new basic block for the loop header, inserting after current
    // block.
    BasicBlock* LoopBB = BasicBlock::Create(TheContext, "loop", TheFunction);

    // Insert an explicit fall through from the current block to the LoopBB.
    Builder.CreateBr(LoopBB);

    // Start insertion in LoopBB.
    Builder.SetInsertPoint(LoopBB);

    // Within the loop, the variable is defined equal to the PHI node.  If it
    // shadows an existing variable, we have to restore it, so save it now.
    AllocaInst* OldVal = NamedValues[VarName];
    NamedValues[VarName] = Alloca;

    // Emit the body of the loop.  This, like any other expr, can change the
    // current BB.  Note that we ignore the value computed by the body, but don't
    // allow an error.
    if (!Body->execute())
        return nullptr;

    // Emit the step value.
    Value* StepVal = nullptr;
    if (Step) {
        StepVal = Step->execute();
        if (!StepVal)
            return nullptr;
    }
    else {
        // If not specified, use 1.0.
        StepVal = std::make_unique<Value>(1.0).get();
    }

    // Compute the end condition.
    Value* EndCond = End->execute();
    if (!EndCond)
        return nullptr;

    // Reload, increment, and restore the alloca.  This handles the case where
    // the body of the loop mutates the variable.
    Value* CurVar = Builder.CreateLoad(Alloca, VarName.c_str());
    Value* NextVar = Builder.CreateFAdd(CurVar, StepVal, "nextvar");
    Builder.CreateStore(NextVar, Alloca);

    // Convert condition to a bool by comparing non-equal to 0.0.
    EndCond = Builder.CreateFCmpONE(
        EndCond, ConstantFP::get(TheContext, APFloat(0.0)), "loopcond");

    // Create the "after loop" block and insert it.
    BasicBlock* AfterBB =
        BasicBlock::Create(TheContext, "afterloop", TheFunction);

    // Insert the conditional branch into the end of LoopEndBB.
    Builder.CreateCondBr(EndCond, LoopBB, AfterBB);

    // Any new code will be inserted in AfterBB.
    Builder.SetInsertPoint(AfterBB);

    // Restore the unshadowed variable.
    if (OldVal)
        NamedValues[VarName] = OldVal;
    else
        NamedValues.erase(VarName);

    // for expr always returns 0.0.
    return Constant::getNullValue(Type::getDoubleTy(TheContext));
}

Value* VarExprAST::execute() {
    std::vector<AllocaInst*> OldBindings;

    Function* TheFunction = Builder.GetInsertBlock()->getParent();

    // Register all variables and emit their initializer.
    for (unsigned i = 0, e = VarNames.size(); i != e; ++i) {
        const std::string& VarName = VarNames[i].first;
        ExprAST* Init = VarNames[i].second.get();

        // Emit the initializer before adding the variable to scope, this prevents
        // the initializer from referencing the variable itself, and permits stuff
        // like this:
        //  var a = 1 in
        //    var a = a in ...   # refers to outer 'a'.
        Value* InitVal;
        if (Init) {
            InitVal = Init->execute();
            if (!InitVal)
                return nullptr;
        }
        else { // If not specified, use 0.0.
            InitVal = ConstantFP::get(TheContext, APFloat(0.0));
        }

        AllocaInst* Alloca = CreateEntryBlockAlloca(TheFunction, VarName);
        Builder.CreateStore(InitVal, Alloca);

        // Remember the old variable binding so that we can restore the binding when
        // we unrecurse.
        OldBindings.push_back(NamedValues[VarName]);

        // Remember this binding.
        NamedValues[VarName] = Alloca;
    }

    // execute the body, now that all vars are in scope.
    Value* BodyVal = Body->execute();
    if (!BodyVal)
        return nullptr;

    // Pop all our variables from scope.
    for (unsigned i = 0, e = VarNames.size(); i != e; ++i)
        NamedValues[VarNames[i].first] = OldBindings[i];

    // Return the body computation.
    return BodyVal;
}

void PrototypeAST::execute() {
    // Make the function type:  double(double,double) etc.
    std::vector<Type*> Doubles(Args.size(), Type::getDoubleTy(TheContext));
    FunctionType* FT =
        FunctionType::get(Type::getDoubleTy(TheContext), Doubles, false);

    Function* F =
        Function::Create(FT, Function::ExternalLinkage, Name, TheModule.get());

    // Set names for all arguments.
    unsigned Idx = 0;
    for (auto& Arg : F->args())
        Arg.setName(Args[Idx++]);

    return F;
}

Value* FunctionAST::execute(Value *Ops[]) {
    // Transfer ownership of the prototype to the FunctionProtos map, but keep a
    // reference to it for use below.
    auto& P = *Proto;
    FunctionProtos[Proto->getName()] = std::move(Proto);
    Function* TheFunction = getFunction(P.getName());
    if (!TheFunction)
        return nullptr;

    // If this is an operator, install it.
    if (P.isBinaryOp())
        BinopPrecedence[P.getOperatorName()] = P.getBinaryPrecedence();

    // Create a new basic block to start insertion into.
    BasicBlock* BB = BasicBlock::Create(TheContext, "entry", TheFunction);
    Builder.SetInsertPoint(BB);

    // Record the function arguments in the NamedValues map.
    NamedValues.clear();
    for (auto& Arg : TheFunction->args()) {
        // Create an alloca for this variable.
        AllocaInst* Alloca = CreateEntryBlockAlloca(TheFunction, Arg.getName());

        // Store the initial value into the alloca.
        Builder.CreateStore(&Arg, Alloca);

        // Add arguments to variable symbol table.
        NamedValues[Arg.getName()] = Alloca;
    }

    if (Value* RetVal = Body->execute()) {
        // Finish off the function.
        Builder.CreateRet(RetVal);

        // Validate the generated code, checking for consistency.
        verifyFunction(*TheFunction);

        // Run the optimizer on the function.
        TheFPM->run(*TheFunction);

        return TheFunction;
    }

    // Error reading body, remove function.
    TheFunction->eraseFromParent();

    if (P.isBinaryOp())
        BinopPrecedence.erase(P.getOperatorName());
    return nullptr;
}

static void HandleDefinition() {
    if (auto FnAST = ParseDefinition()) {
        if (auto* FnIR = FnAST->execute()) {
            fprintf(stderr, "Read function definition:");
            FnIR->print(errs());
            fprintf(stderr, "\n");
            TheJIT->addModule(std::move(TheModule));
            InitializeModuleAndPassManager();
        }
    }
    else {
        // Skip token for error recovery.
        getNextToken();
    }
}

static void HandleTopLevelExpression() {
    // Evaluate a top-level expression into an anonymous function.
    if (auto FnAST = ParseTopLevelExpr()) {
        if (FnAST->execute()) {
            // JIT the module containing the anonymous expression, keeping a handle so
            // we can free it later.
            auto H = TheJIT->addModule(std::move(TheModule));
            InitializeModuleAndPassManager();

            // Search the JIT for the __anon_expr symbol.
            auto ExprSymbol = TheJIT->findSymbol("__anon_expr");
            assert(ExprSymbol && "Function not found");

            // Get the symbol's address and cast it to the right type (takes no
            // arguments, returns a double) so we can call it as a native function.
            double (*FP)() =
                (double (*)())(intptr_t)cantFail(ExprSymbol.getAddress());
            fprintf(stderr, "Evaluated to %f\n", FP());

            // Delete the anonymous expression module from the JIT.
            TheJIT->removeModule(H);
        }
    }
    else {
        // Skip token for error recovery.
        getNextToken();
    }
}

/// top ::= definition | external | expression | ';'
static void MainLoop() {
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
