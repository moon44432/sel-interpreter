
// NVSPL3 Language Project
// stdlibrary.h

#pragma once

#include "parser.h"

extern std::vector<std::string> StdFuncList;

Value CallStdFunc(const std::string& Name, const std::vector<Value>& Args);

Value print(const std::vector<Value>& Args);

Value println(const std::vector<Value>& Args);

Value printch(const std::vector<Value>& Args);

Value input(const std::vector<Value>& Args);