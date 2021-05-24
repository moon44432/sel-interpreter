
// SEL Project
// execute.h

#pragma once

#include "value.h"
#include <vector>
#include <string>

extern int MainIdx;
extern bool IsInteractive;
extern std::string MainCode;

typedef enum class ArrAction
{
    getVal,
    getAddr,
    setVal,
} arrAction;

typedef struct NamedValue
{
    std::string Name;
    int Addr;

    bool IsArr = false;
    std::vector<int> DimInfo;
} namedValue;

class Memory
{
    std::vector<Value> Stack;
public:
    Value getValue(unsigned int Addr) { return Stack[Addr]; }
    void setValue(unsigned int Addr, Value Val) { Stack[Addr] = Val; }
    void deleteScope(unsigned int Addr) { unsigned int size = Stack.size(); for (unsigned int i = Addr; i < size; i++) Stack.pop_back(); }
    unsigned int push(Value Val) { Stack.push_back(Val); return Stack.size() - 1; }
    unsigned int getSize() { return Stack.size(); }
};

Value LogErrorV(const char* Str);

void HandleDefinition(std::string& Code, int& Idx);

void HandleTopLevelExpression(std::string& Code, int& Idx);

void HandleImport(std::string& Code, int& Idx, int tmpCurTok, int tmpLastChar, bool tmpFlag);

void MainLoop(std::string& Code, int& Idx);

void ExecuteScript(const char* FileName);