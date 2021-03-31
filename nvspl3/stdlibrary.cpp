
// SEL Project
// stdlibrary.cpp

#include "stdlibrary.h"
#include "execute.h"

std::vector<std::string> StdFuncList = {
	"print",
	"println",
	"printch",
	"input",
};

Value CallStdFunc(const std::string& Name, const std::vector<Value>& Args)
{
	if (Name == "print") return print(Args);
	else if (Name == "println") return println(Args);
	else if (Name == "printch") return printch(Args);
	else if (Name == "input") return input(Args);
}

Value print(const std::vector<Value>& Args)
{
	for (auto Arg : Args) fprintf(stderr, "%f ", Arg.getVal());
	return Value(0.0);
}

Value println(const std::vector<Value>& Args)
{
	for (auto Arg : Args) fprintf(stderr, "%f ", Arg.getVal());
	fprintf(stderr, "\n");
	return Value(0.0);
}

Value printch(const std::vector<Value>& Args)
{
	for (auto Arg : Args) fprintf(stderr, "%c", (char)(Arg.getVal()));
	return Value(0.0);
}

Value input(const std::vector<Value>& Args)
{
	if (Args.size() != 0) return LogErrorV("input() requires no arguments");

	double Val;
	fscanf(stdin, "%lf", &Val);
	return Value(Val);
}