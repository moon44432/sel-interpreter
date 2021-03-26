#include "parser.h"
#include "lexer.h"
#include <iostream>


int CurTok;
std::map<std::string, int> BinopPrecedence;
std::string OpChrList = "<>+-*/%^!~?&|:=";

void binopPrecInit()
{
	BinopPrecedence["="] = 18 - 17;
	BinopPrecedence["=="] = 18 - 10;
	BinopPrecedence["!="] = 18 - 10;
	BinopPrecedence["<"] = 18 - 9;
	BinopPrecedence[">"] = 18 - 9;
	BinopPrecedence["<="] = 18 - 9;
	BinopPrecedence[">="] = 18 - 9;
	BinopPrecedence["+"] = 18 - 7;
	BinopPrecedence["-"] = 18 - 7;
	BinopPrecedence["*"] = 18 - 3;
	BinopPrecedence["/"] = 18 - 3;
}

int getNextToken()
{
	return CurTok = gettok();
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
std::shared_ptr<ExprAST> ParseNumberExpr()
{
	auto Result = std::make_shared<NumberExprAST>(NumVal);
	getNextToken(); // consume the number
	return std::move(Result);
}

/// parenexpr ::= '(' expression ')'
std::shared_ptr<ExprAST> ParseParenExpr()
{
	getNextToken(); // eat (.
	auto V = ParseExpression();
	if (!V)
		return nullptr;

	if (CurTok != ')')
		return LogError("expected ')'");
	getNextToken(); // eat ).
	return V;
}

/// identifierexpr
///   ::= identifier
///   ::= identifier '(' expression* ')'
std::shared_ptr<ExprAST> ParseIdentifierExpr()
{
	std::string IdName = IdentifierStr;

	getNextToken(); // eat identifier.

	if (CurTok != '(') // simple variable or array element ref.
	{
		if (CurTok != '[') return std::make_shared<VariableExprAST>(IdName);
		getNextToken();

		std::cout << "array ref: " << IdName << std::endl;
		std::vector<std::shared_ptr<ExprAST>> Indices;
		if (CurTok != ']')
		{
			while (true)
			{
				if (auto Idx = ParseExpression())
					Indices.push_back(std::move(Idx));
				else return nullptr;

				if (CurTok == ']')
					break;

				if (CurTok != ',')
					return LogError("Expected ']' or ',' in index list");
				getNextToken();
			}
		}
		getNextToken();

		return std::make_shared<VariableExprAST>(IdName, Indices);
	}

	// Call.
	getNextToken(); // eat (
	std::vector<std::shared_ptr<ExprAST>> Args;
	if (CurTok != ')')
	{
		while (true)
		{
			if (auto Arg = ParseExpression())
				Args.push_back(std::move(Arg));
			else return nullptr;

			if (CurTok == ')')
				break;

			if (CurTok != ',')
				return LogError("Expected ')' or ',' in argument list");
			getNextToken();
		}
	}

	// Eat the ')'.
	getNextToken();

	return std::make_shared<CallExprAST>(IdName, std::move(Args));
}

/// arrdeclexpr
std::shared_ptr<ExprAST> ParseArrDeclExpr()
{
	// std::cout << "arr declaration" << std::endl;

	getNextToken(); // eat the arr.

	std::string IdName = IdentifierStr;
	getNextToken();

	if (CurTok != '[')
		return LogError("expected '[' after array name");
	getNextToken();

	std::vector<int> Indices;

	if (CurTok != ']')
	{
		while (true)
		{
			if (CurTok == tok_number)
			{
				std::cout << NumVal << std::endl;
				if ((unsigned)NumVal >= 1) Indices.push_back((unsigned)NumVal);
				else return LogError("length of each dimension must be 1 or higher");
			}
			else return LogError("length of each dimension must be an integer");
			getNextToken();

			if (CurTok == ']') break;

			if (CurTok != ',')
				return LogError("expected ']' or ','");
			getNextToken();
		}
	}
	getNextToken();

	std::cout << "ArrName: " << IdName << std::endl;

	return std::make_shared<ArrDeclExprAST>(IdName, Indices);
}

/// ifexpr ::= 'if' expression 'then' expression 'else' expression
std::shared_ptr<ExprAST> ParseIfExpr()
{
	getNextToken(); // eat the if.

	// condition.
	auto Cond = ParseExpression();
	if (!Cond)
		return nullptr;

	if (CurTok != tok_then)
		return LogError("expected then");
	getNextToken(); // eat the then

	auto Then = ParseBlockExpression();
	if (!Then)
		return nullptr;

	if (CurTok != tok_else)
		return LogError("expected else");

	getNextToken();

	auto Else = ParseBlockExpression();
	if (!Else)
		return nullptr;

	return std::make_shared<IfExprAST>(std::move(Cond), std::move(Then),
		std::move(Else));
}

/// forexpr ::= 'for' identifier '=' expr ',' expr (',' expr)? 'in' expression
std::shared_ptr<ExprAST> ParseForExpr()
{
	getNextToken(); // eat the for.

	if (CurTok != '(')
		return LogError("expected '(' after for");
	getNextToken();

	if (CurTok != tok_identifier)
		return LogError("expected identifier");

	std::string IdName = IdentifierStr;
	getNextToken(); // eat identifier.

	if (CurTok != '=')
		return LogError("expected '=' after for");
	getNextToken(); // eat '='.

	auto Start = ParseExpression();
	if (!Start)
		return nullptr;
	if (CurTok != ',')
		return LogError("expected ',' after for start value");
	getNextToken();

	auto End = ParseExpression();
	if (!End)
		return nullptr;

	// The step value is optional.
	std::shared_ptr<ExprAST> Step;
	if (CurTok == ',') {
		getNextToken();
		Step = ParseExpression();
		if (!Step)
			return nullptr;
	}

	if (CurTok != ')')
		return LogError("expected ')' after for");
	getNextToken(); // eat ')'.

	auto Body = ParseBlockExpression();
	if (!Body)
		return nullptr;

	return std::make_shared<ForExprAST>(IdName, std::move(Start), std::move(End),
		std::move(Step), std::move(Body));
}

std::shared_ptr<ExprAST> ParseRepeatExpr()
{
	getNextToken(); // eat the rept.

	if (CurTok != '(')
		return LogError("expected '(' after rept");
	getNextToken();

	auto IterNum = ParseExpression();
	if (!IterNum)
		return nullptr;

	if (CurTok != ')')
		return LogError("expected ')'");
	getNextToken(); // eat ')'.

	auto Body = ParseBlockExpression();
	if (!Body)
		return nullptr;

	return std::make_shared<RepeatExprAST>(std::move(IterNum), std::move(Body));
}


/// primary
///   ::= identifierexpr
///   ::= numberexpr
///   ::= parenexpr
///   ::= ifexpr
///   ::= forexpr
///   ::= arrexpr
std::shared_ptr<ExprAST> ParsePrimary()
{
	switch (CurTok) {
	default:
		return LogError("unknown token when expecting an expression");
	case tok_identifier:
		return ParseIdentifierExpr();
	case tok_number:
		return ParseNumberExpr();
	case tok_for:
		return ParseForExpr();
	case tok_if:
		return ParseIfExpr();
	case tok_repeat:
		return ParseRepeatExpr();
	case tok_arr:
		return ParseArrDeclExpr();
	case '(': // must be the last one (for, var, if, etc also use '(')
		return ParseParenExpr();
	}
}

/// unary
///   ::= primary
///   ::= '!' unary
std::shared_ptr<ExprAST> ParseUnary()
{
	// If the current token is not an operator, it must be a primary expr.
	if (!isascii(CurTok) || CurTok == '(' || CurTok == ',')
		return ParsePrimary();

	// If this is a unary operator, read it.
	int Opc = CurTok;
	getNextToken();
	std::cout << "UnaryOp" << (char)Opc << std::endl;
	if (auto Operand = ParseUnary())
		return std::make_shared<UnaryExprAST>(Opc, std::move(Operand));
	return nullptr;
}

/// binoprhs
///   ::= ('+' unary)*
std::shared_ptr<ExprAST> ParseBinOpRHS(int ExprPrec, std::shared_ptr<ExprAST> LHS)
{
	// If this is a binop, find its precedence.
	while (true)
	{
		std::string BinOp;
		BinOp += (char)CurTok;

		if (OpChrList.find(CurTok) != std::string::npos && 
			OpChrList.find(LastChar) != std::string::npos)
		{
			BinOp += (char)LastChar;
			getNextToken();
		}
		int TokPrec = GetTokPrecedence(BinOp);
		std::cout << BinOp << std::endl;

		// If this is a binop that binds at least as tightly as the current binop,
		// consume it, otherwise we are done.
		if (TokPrec < ExprPrec)
			return LHS;

		getNextToken();

		// Parse the unary expression after the binary operator.
		auto RHS = ParseUnary();
		if (!RHS)
			return nullptr;

		// If BinOp binds less tightly with RHS than the operator after RHS, let
		// the pending operator take RHS as its LHS.
		std::string NextOp;
		NextOp += (char)CurTok;

		if (OpChrList.find(CurTok) != std::string::npos &&
			OpChrList.find(LastChar) != std::string::npos)
		{
			NextOp += (char)LastChar;
		}

		int NextPrec = GetTokPrecedence(NextOp);
		if (TokPrec < NextPrec) {
			RHS = ParseBinOpRHS(TokPrec + 1, std::move(RHS));
			if (!RHS)
				return nullptr;
		}

		// Merge LHS/RHS.
		LHS =
			std::make_shared<BinaryExprAST>(BinOp, std::move(LHS), std::move(RHS));
	}
}

/// expression
///   ::= unary binoprhs
///
std::shared_ptr<ExprAST> ParseExpression()
{
	auto LHS = ParseUnary();
	if (!LHS)
		return nullptr;

	return ParseBinOpRHS(0, std::move(LHS));
}

/// blockexpr
std::shared_ptr<ExprAST> ParseBlockExpression()
{
	if (CurTok != tok_openblock)
	{
		printf("Parse Expression... ch : %c\n", LastChar);
		return ParseExpression();
	}
	std::vector<std::shared_ptr<ExprAST>> ExprSeq;

	getNextToken();
	printf("Parse Block Expression... ch : %c\n", LastChar);
	while (true)
	{
		auto Expr = ParseBlockExpression();
		ExprSeq.push_back(std::move(Expr));
		if (CurTok == ';')
			getNextToken();
		if (CurTok == tok_closeblock)
		{
			getNextToken();
			break;
		}
	}
	auto Block = std::make_shared<BlockExprAST>(std::move(ExprSeq));
	return std::move(Block);
}

/// prototype
///   ::= id '(' id* ')'
///   ::= binary LETTER number? (id, id)
///   ::= unary LETTER (id)
std::shared_ptr<PrototypeAST> ParsePrototype()
{
	std::string FnName;

	unsigned Kind = 0; // 0 = identifier, 1 = unary, 2 = binary.
	unsigned BinaryPrecedence = 30;

	switch (CurTok) {
	default:
		return LogErrorP("Expected function name in prototype");
	case tok_identifier:
		FnName = IdentifierStr;
		Kind = 0;
		getNextToken();
		break;
	case tok_unary:
		getNextToken();
		if (!isascii(CurTok))
			return LogErrorP("Expected unary operator");
		FnName = "unary";
		FnName += (char)CurTok;
		Kind = 1;
		getNextToken();
		break;
	case tok_binary:
		getNextToken();
		if (!isascii(CurTok))
			return LogErrorP("Expected binary operator");
		FnName = "binary";
		FnName += (char)CurTok;
		if (OpChrList.find(LastChar) != std::string::npos)
		{
			FnName += (char)LastChar;
			getNextToken();
		}
		Kind = 2;
		getNextToken();

		// Read the precedence if present.
		if (CurTok == tok_number) {
			if (NumVal < 1 || NumVal > 100)
				return LogErrorP("Invalid precedence: must be 1~100");
			BinaryPrecedence = (unsigned)NumVal;
			getNextToken();
		}
		break;
	}

	if (CurTok != '(')
		return LogErrorP("Expected '(' in prototype");

	std::vector<std::string> ArgNames;
	while (true)
	{
		if (getNextToken() == tok_identifier)
			ArgNames.push_back(IdentifierStr);

		getNextToken();
		if (CurTok == ')') break;
		if (CurTok != ',')
			return LogErrorP("Expected ',' or ')'");
	}

	std::cout << ArgNames.size() << std::endl;

	// success.
	getNextToken(); // eat ')'

	// Verify right number of names for operator.
	if (Kind && ArgNames.size() != Kind)
		return LogErrorP("Invalid number of operands for operator");

	return std::make_shared<PrototypeAST>(FnName, ArgNames, Kind != 0,
		BinaryPrecedence);
}

/// definition ::= 'func' prototype expression
std::shared_ptr<FunctionAST> ParseDefinition()
{
	getNextToken(); // eat func.
	auto Proto = ParsePrototype();
	if (!Proto)
		return nullptr;

	if (auto E = ParseBlockExpression())
		return std::make_shared<FunctionAST>(std::move(Proto), std::move(E));
	return nullptr;
}

/// toplevelexpr ::= expression
std::shared_ptr<FunctionAST> ParseTopLevelExpr()
{
	if (auto E = ParseBlockExpression()) {
		// Make an anonymous proto.
		auto Proto = std::make_shared<PrototypeAST>("__anon_expr",
			std::vector<std::string>());
		return std::make_shared<FunctionAST>(std::move(Proto), std::move(E));
	}
	return nullptr;
}