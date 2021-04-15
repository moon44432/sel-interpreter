
// SEL Project
// parser.cpp

#include "ast.h"
#include "lexer.h"
#include <iostream>
#include <cmath>

int CurTok;
std::map<std::string, int> BinopPrecedence;
std::string OpChr = "<>+-*/%!&|=";

void InitBinopPrec()
{
    BinopPrecedence["**"] = 18 - 4; // highest
    BinopPrecedence["*"] = 18 - 5;
    BinopPrecedence["/"] = 18 - 5;
    BinopPrecedence["%"] = 18 - 5;
    BinopPrecedence["+"] = 18 - 6;
    BinopPrecedence["-"] = 18 - 6;
    BinopPrecedence["<"] = 18 - 8;
    BinopPrecedence[">"] = 18 - 8;
    BinopPrecedence["<="] = 18 - 8;
    BinopPrecedence[">="] = 18 - 8;
    BinopPrecedence["=="] = 18 - 9;
    BinopPrecedence["!="] = 18 - 9;
    BinopPrecedence["&&"] = 18 - 13;
    BinopPrecedence["||"] = 18 - 14;
    BinopPrecedence["="] = 18 - 15; // lowest
}

int GetNextToken(std::string& Code, int* Idx)
{
    return CurTok = gettok(Code, Idx);
}

/// GetTokPrecedence - Get the precedence of the pending binary operator token.
int GetTokPrecedence(std::string Op)
{
    // Make sure it's a declared binop.
    int TokPrec = BinopPrecedence[Op];
    if (TokPrec <= 0)
        return -1;
    return TokPrec;
}

/// LogError* - These are little helper functions for error handling.
std::shared_ptr<ExprAST> LogError(const char* Str)
{
    fprintf(stderr, "Error: %s\n", Str);
    return nullptr;
}

std::shared_ptr<PrototypeAST> LogErrorP(const char* Str)
{
    LogError(Str);
    return nullptr;
}

/// numberexpr ::= number
std::shared_ptr<ExprAST> ParseNumberExpr(std::string& Code, int* Idx)
{
    auto Expr = std::make_shared<NumberExprAST>(NumVal);
    GetNextToken(Code, Idx); // consume the number
    return std::move(Expr);
}

/// parenexpr ::= '(' expression ')'
std::shared_ptr<ExprAST> ParseParenExpr(std::string& Code, int* Idx)
{
    GetNextToken(Code, Idx); // eat (.
    auto Expr = ParseExpression(Code, Idx);
    if (!Expr)
        return nullptr;

    if (CurTok != ')')
        return LogError("Expected ')'");
    GetNextToken(Code, Idx); // eat ).
    return Expr;
}

/// identifierexpr
///   ::= identifier
///   ::= identifier ('[' expression ']')+
///   ::= identifier '(' expression* ')'
std::shared_ptr<ExprAST> ParseIdentifierExpr(std::string& Code, int* Idx)
{
    std::string IdName = IdentifierStr;

    GetNextToken(Code, Idx); // eat identifier.

    if (CurTok != '(') // simple variable or array element ref.
    {
        if (CurTok != '[') return std::make_shared<VariableExprAST>(IdName);
        GetNextToken(Code, Idx);

        std::vector<std::shared_ptr<ExprAST>> Indices;
        if (CurTok != ']')
        {
            while (true)
            {
                if (auto ArrIdx = ParseExpression(Code, Idx))
                    Indices.push_back(std::move(ArrIdx));
                else return nullptr;

                if (CurTok == ']')
                {
                    GetNextToken(Code, Idx);

                    if (CurTok != '[') break;
                    else GetNextToken(Code, Idx);
                }
            }
        }
        else return LogError("Array index missing");

        return std::make_shared<VariableExprAST>(IdName, Indices);
    }

    // Call.
    GetNextToken(Code, Idx); // eat (
    std::vector<std::shared_ptr<ExprAST>> Args;
    if (CurTok != ')')
    {
        while (true)
        {
            if (auto Arg = ParseExpression(Code, Idx))
                Args.push_back(std::move(Arg));
            else return nullptr;

            if (CurTok == ')')
                break;

            if (CurTok != ',')
                return LogError("Expected ')' or ',' in argument list");
            GetNextToken(Code, Idx);
        }
    }

    // Eat the ')'.
    GetNextToken(Code, Idx);

    return std::make_shared<CallExprAST>(IdName, std::move(Args));
}

/// derefexpr
///   ::= '@' expression
std::shared_ptr<ExprAST> ParseDeRefExpr(std::string& Code, int* Idx)
{
    GetNextToken(Code, Idx); // eat the '@'.

    auto Primary = ParsePrimary(Code, Idx);
    if (!Primary)
        return nullptr;

    return std::make_shared<DeRefExprAST>(std::move(Primary));
}

/// arrdeclexpr ::= 'arr' identifier ('[' number ']')+
std::shared_ptr<ExprAST> ParseArrDeclExpr(std::string& Code, int* Idx)
{
    GetNextToken(Code, Idx); // eat the arr.

    std::string IdName = IdentifierStr;
    GetNextToken(Code, Idx);

    if (CurTok != '[') return LogError("Expected '[' after array name");

    GetNextToken(Code, Idx);

    std::vector<int> Indices;

    if (CurTok != ']')
    {
        while (true)
        {
            if (CurTok == tok_number && fmod(NumVal, 1.0) == 0)
            {
                if ((unsigned)NumVal >= 1) Indices.push_back((unsigned)NumVal);
                else return LogError("Length of each dimension must be 1 or higher");
            }
            else return LogError("Length of each dimension must be an integer");
            GetNextToken(Code, Idx);

            if (CurTok == ']')
            {
                if (LastChar != '[')
                    break;

                GetNextToken(Code, Idx);
                GetNextToken(Code, Idx);
            }
        }
    }
    else return LogError("Array dimension missing");

    GetNextToken(Code, Idx);
    return std::make_shared<ArrDeclExprAST>(IdName, Indices);
}

/// ifexpr ::= 'if' expression 'then' blockexpr 'else' blockexpr
std::shared_ptr<ExprAST> ParseIfExpr(std::string& Code, int* Idx)
{
    GetNextToken(Code, Idx); // eat the if.

    // condition.
    auto Cond = ParseExpression(Code, Idx);
    if (!Cond)
        return nullptr;

    if (CurTok != tok_then)
        return LogError("Expected then");
    GetNextToken(Code, Idx); // eat the then

    auto Then = ParseBlockExpression(Code, Idx);
    if (!Then)
        return nullptr;

    if (CurTok == tok_else)
    {
        GetNextToken(Code, Idx);

        auto Else = ParseBlockExpression(Code, Idx);
        if (!Else)
            return nullptr;

        return std::make_shared<IfExprAST>(std::move(Cond), std::move(Then),
            std::move(Else));
    }
    else return std::make_shared<IfExprAST>(std::move(Cond), std::move(Then));
}

/// forexpr ::= 'for' identifier '=' expr ',' expr (',' expr)? blockexpr
std::shared_ptr<ExprAST> ParseForExpr(std::string& Code, int* Idx)
{
    GetNextToken(Code, Idx); // eat the for.

    if (CurTok != tok_identifier)
        return LogError("Expected identifier");

    std::string IdName = IdentifierStr;
    GetNextToken(Code, Idx); // eat identifier.

    if (CurTok != '=')
        return LogError("Expected '=' after identifier");
    GetNextToken(Code, Idx); // eat '='.

    auto Start = ParseExpression(Code, Idx);
    if (!Start)
        return nullptr;
    if (CurTok != ',')
        return LogError("Expected ','");
    GetNextToken(Code, Idx);

    auto End = ParseExpression(Code, Idx);
    if (!End)
        return nullptr;

    // The step value is optional.
    std::shared_ptr<ExprAST> Step;
    if (CurTok == ',') {
        GetNextToken(Code, Idx);
        Step = ParseExpression(Code, Idx);
        if (!Step)
            return nullptr;
    }

    auto Body = ParseBlockExpression(Code, Idx);
    if (!Body)
        return nullptr;

    return std::make_shared<ForExprAST>(IdName, std::move(Start), std::move(End),
        std::move(Step), std::move(Body));
}

/// whileexpr ::= 'while' expr blockexpr
std::shared_ptr<ExprAST> ParseWhileExpr(std::string& Code, int* Idx)
{
    GetNextToken(Code, Idx); // eat the while.

    auto Cond = ParseExpression(Code, Idx);
    if (!Cond)
        return nullptr;

    auto Body = ParseBlockExpression(Code, Idx);
    if (!Body)
        return nullptr;

    return std::make_shared<WhileExprAST>(std::move(Cond), std::move(Body));
}

/// reptexpr ::= 'rept' expr blockexpr
std::shared_ptr<ExprAST> ParseRepeatExpr(std::string& Code, int* Idx)
{
    GetNextToken(Code, Idx); // eat the rept.

    auto IterNum = ParseExpression(Code, Idx);
    if (!IterNum)
        return nullptr;

    auto Body = ParseBlockExpression(Code, Idx);
    if (!Body)
        return nullptr;

    return std::make_shared<RepeatExprAST>(std::move(IterNum), std::move(Body));
}

/// loopexpr ::= 'loop' blockexpr
std::shared_ptr<ExprAST> ParseLoopExpr(std::string& Code, int* Idx)
{
    GetNextToken(Code, Idx); // eat the loop.

    auto Body = ParseBlockExpression(Code, Idx);
    if (!Body)
        return nullptr;

    return std::make_shared<LoopExprAST>(std::move(Body));
}

/// breakexpr
///   ::= 'break' expr
std::shared_ptr<ExprAST> ParseBreakExpr(std::string& Code, int* Idx)
{
    GetNextToken(Code, Idx); // eat the break.

    auto Expr = ParseExpression(Code, Idx);
    if (!Expr)
        return nullptr;

    return std::make_shared<BreakExprAST>(std::move(Expr));
}

/// returnexpr
///   ::= 'return' expr
std::shared_ptr<ExprAST> ParseReturnExpr(std::string& Code, int* Idx)
{
    GetNextToken(Code, Idx); // eat the return.

    auto Expr = ParseExpression(Code, Idx);
    if (!Expr)
        return nullptr;

    return std::make_shared<ReturnExprAST>(std::move(Expr));
}

/// primary
///   ::= identifierexpr
///   ::= derefexpr
///   ::= numberexpr
///   ::= parenexpr
///   ::= ifexpr
///   ::= forexpr
///   ::= whileexpr
///   ::= reptexpr
///   ::= loopexpr
std::shared_ptr<ExprAST> ParsePrimary(std::string& Code, int* Idx)
{
    switch (CurTok) {
    default:
        return LogError("Unknown token when expecting an expression");
    case tok_identifier:
        return ParseIdentifierExpr(Code, Idx);
    case '@':
        return ParseDeRefExpr(Code, Idx);
    case tok_number:
        return ParseNumberExpr(Code, Idx);
    case tok_for:
        return ParseForExpr(Code, Idx);
    case tok_while:
        return ParseWhileExpr(Code, Idx);
    case tok_if:
        return ParseIfExpr(Code, Idx);
    case tok_repeat:
        return ParseRepeatExpr(Code, Idx);
    case tok_loop:
        return ParseLoopExpr(Code, Idx);
    case '(':
        return ParseParenExpr(Code, Idx);
    }
}

/// unary
///   ::= primary
///   ::= unaryop unary
std::shared_ptr<ExprAST> ParseUnary(std::string& Code, int* Idx)
{
    // If the current token is not an operator, it must be a primary expr.
    if (!isascii(CurTok) || CurTok == '(' || CurTok == ',' || CurTok == '@')
        return ParsePrimary(Code, Idx);

    // If this is a unary operator, read it.
    int Opc;
    if (OpChr.find(CurTok) != std::string::npos)
    {
        Opc = CurTok;
        GetNextToken(Code, Idx);
    }
    else return LogError(((std::string)"Unknown token '" + (char)CurTok + (std::string)"'").c_str());

    if (auto Operand = ParseUnary(Code, Idx))
        return std::make_shared<UnaryExprAST>(Opc, std::move(Operand));
    return nullptr;
}

/// binoprhs
///   ::= (binop unary)*
std::shared_ptr<ExprAST> ParseBinOpRHS(std::string& Code, int* Idx, int ExprPrec, std::shared_ptr<ExprAST> LHS)
{
    // If this is a binop, find its precedence.
    while (true)
    {
        bool DoubleCh = false;
        std::string BinOp;
        BinOp += (char)CurTok;

        if (OpChr.find(CurTok) != std::string::npos && 
            OpChr.find(LastChar) != std::string::npos)
        {
            BinOp += (char)LastChar;
            DoubleCh = true;
        }
        int TokPrec = GetTokPrecedence(BinOp);

        // If this is a binop that binds at least as tightly as the current binop,
        // consume it, otherwise we are done.
        if (TokPrec < ExprPrec)
            return LHS;

        GetNextToken(Code, Idx);
        if (DoubleCh) GetNextToken(Code, Idx);

        // Parse the unary expression after the binary operator.
        auto RHS = ParseUnary(Code, Idx);
        if (!RHS)
            return nullptr;

        // If BinOp binds less tightly with RHS than the operator after RHS, let
        // the pending operator take RHS as its LHS.
        std::string NextOp;
        NextOp += (char)CurTok;

        if (OpChr.find(CurTok) != std::string::npos &&
            OpChr.find(LastChar) != std::string::npos)
        {
            NextOp += (char)LastChar;
        }

        int NextPrec = GetTokPrecedence(NextOp);
        if (TokPrec < NextPrec)
        {
            RHS = ParseBinOpRHS(Code, Idx, TokPrec + 1, std::move(RHS));
            if (!RHS) return nullptr;
        }

        // Merge LHS/RHS.
        LHS = std::make_shared<BinaryExprAST>(BinOp, std::move(LHS), std::move(RHS));
    }
}

/// expression
///   ::= unary binoprhs
///   ::= arrdeclexpr
///   ::= breakexpr
///   ::= returnexpr
std::shared_ptr<ExprAST> ParseExpression(std::string& Code, int* Idx)
{
    switch (CurTok)
    {
    case tok_arr:
        return ParseArrDeclExpr(Code, Idx);
    case tok_break:
        return ParseBreakExpr(Code, Idx);
    case tok_return:
        return ParseReturnExpr(Code, Idx);
    default:
        auto LHS = ParseUnary(Code, Idx);
        if (!LHS)
            return nullptr;
        return ParseBinOpRHS(Code, Idx, 0, std::move(LHS));
    }
}

/// blockexpr
///   ::= expression
///   ::= '{' expression+ '}'
std::shared_ptr<ExprAST> ParseBlockExpression(std::string& Code, int* Idx)
{
    if (CurTok != tok_openblock)
        return ParseExpression(Code, Idx);
    GetNextToken(Code, Idx);

    std::vector<std::shared_ptr<ExprAST>> ExprSeq;

    while (true)
    {
        auto Expr = ParseBlockExpression(Code, Idx);
        ExprSeq.push_back(std::move(Expr));
        if (CurTok == ';')
            GetNextToken(Code, Idx);
        if (CurTok == tok_closeblock)
        {
            GetNextToken(Code, Idx);
            break;
        }
    }
    auto Block = std::make_shared<BlockExprAST>(std::move(ExprSeq));
    return std::move(Block);
}

/// prototype
///   ::= id '(' id* ')'
///   ::= binary LETTER(LETTER)? number? (id, id)
///   ::= unary LETTER (id)
std::shared_ptr<PrototypeAST> ParsePrototype(std::string& Code, int* Idx)
{
    std::string FnName;

    unsigned Kind = 0; // 0 = identifier, 1 = unary, 2 = binary.
    unsigned BinaryPrecedence = 18;

    switch (CurTok) {
    default:
        return LogErrorP("Expected function name in prototype");
    case tok_identifier:
        FnName = IdentifierStr;
        Kind = 0;
        GetNextToken(Code, Idx);
        break;
    case tok_unary:
        GetNextToken(Code, Idx);
        if (!isascii(CurTok))
            return LogErrorP("Expected unary operator");
        FnName = "unary";
        FnName += (char)CurTok;
        Kind = 1;
        GetNextToken(Code, Idx);
        break;
    case tok_binary:
        GetNextToken(Code, Idx);
        if (!isascii(CurTok))
            return LogErrorP("Expected binary operator");

        std::string OpName;
        OpName += (char)CurTok;
        GetNextToken(Code, Idx);
        if (OpChr.find(CurTok) != std::string::npos)
        {
            OpName += (char)CurTok;
            GetNextToken(Code, Idx);
        }
        Kind = 2;

        // Read the precedence if present.
        if (CurTok == tok_number) {
            if (NumVal < 1 || NumVal > 18)
                return LogErrorP("Invalid precedence: must be 1~18");
            BinaryPrecedence = (unsigned)NumVal;
            GetNextToken(Code, Idx);
        }

        // install binary operator.
        BinopPrecedence[OpName] = BinaryPrecedence;

        FnName = "binary" + OpName;
        break;
    }

    if (CurTok != '(')
        return LogErrorP("Expected '(' in prototype");

    std::vector<std::string> ArgNames;

    if (GetNextToken(Code, Idx) != ')')
    {
        while (true)
        {
            if (CurTok == tok_identifier)
                ArgNames.push_back(IdentifierStr);

            GetNextToken(Code, Idx);
            if (CurTok == ')') break;
            if (CurTok != ',')
                return LogErrorP("Expected ',' or ')'");

            GetNextToken(Code, Idx);
        }
    }
    // success.
    GetNextToken(Code, Idx); // eat ')'

    // Verify right number of names for operator.
    if (Kind && ArgNames.size() != Kind)
        return LogErrorP("Invalid number of operands for operator");

    return std::make_shared<PrototypeAST>(FnName, ArgNames, Kind != 0,
        BinaryPrecedence);
}

/// definition ::= 'func' prototype expression
std::shared_ptr<FunctionAST> ParseDefinition(std::string& Code, int* Idx)
{
    GetNextToken(Code, Idx); // eat func.
    auto Proto = ParsePrototype(Code, Idx);
    if (!Proto)
        return nullptr;

    if (auto BlockExpr = ParseBlockExpression(Code, Idx))
        return std::make_shared<FunctionAST>(std::move(Proto), std::move(BlockExpr));
    return nullptr;
}

/// toplevelexpr ::= expression
std::shared_ptr<FunctionAST> ParseTopLevelExpr(std::string& Code, int* Idx)
{
    if (auto BlockExpr = ParseBlockExpression(Code, Idx)) {
        // Make an anonymous proto.
        auto Proto = std::make_shared<PrototypeAST>("__anon_expr",
            std::vector<std::string>());
        return std::make_shared<FunctionAST>(std::move(Proto), std::move(BlockExpr));
    }
    return nullptr;
}

/// importexpr ::= 'import' path
std::shared_ptr<ImportAST> ParseImport(std::string& Code, int* Idx)
{
    return std::make_shared<ImportAST>(getPath(Code, Idx) + ".sel");
}