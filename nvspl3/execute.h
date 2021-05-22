
// SEL Project
// execute.h

#pragma once

#include "value.h"
#include <vector>

extern int MainIdx;
extern bool IsInteractive;

typedef struct NamedValue
{
    std::string Name;
    int Addr;

    bool IsArr = false;
    std::vector<int> Dim;
} namedValue;

class Memory
{
    std::vector<Value> Stack;
public:
    Value getValue(int Addr) { return Stack[Addr]; }
    void setValue(int Addr, Value Val) { Stack[Addr] = Val; }
    void deleteScope(int Addr) { int size = Stack.size(); for (int i = Addr; i < size; i++) Stack.pop_back(); }
    int push(Value Val) { Stack.push_back(Val); return Stack.size() - 1; }
    int getSize() { return Stack.size(); }
};

Value LogErrorV(const char* Str);

void HandleDefinition(std::string& Code, int* Idx);

void HandleTopLevelExpression(std::string& Code, int* Idx);

void HandleImport(std::string& Code, int* Idx, int tmpCurTok, int tmpLastChar, bool tmpFlag);

void MainLoop(std::string& Code, int* Idx);

void Execute(const char* FileName);