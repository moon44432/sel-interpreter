
// SEL Project
// stdlibrary.h

#pragma once
#pragma warning (disable:4996)

#include "parser.h"

extern std::vector<std::string> StdFuncList;

Value CallStdFunc(const std::string& Name, const std::vector<Value>& Args);

Value print(const std::vector<Value>& Args);

Value println(const std::vector<Value>& Args);

Value printch(const std::vector<Value>& Args);

Value input(const std::vector<Value>& Args);