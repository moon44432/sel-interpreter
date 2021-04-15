
// SEL Project
// execute.h

#pragma once

class Memory
{
    std::vector<Value> Stack;
public:
    Value* getValuePtr(int idx) { return &Stack[idx]; }
    Value getValue(int idx) { return Stack[idx]; }
    void setValue(int idx, Value Val) { Stack[idx] = Val; }
    void quickDelete(int idx) { for (int i = idx; i < Stack.size(); i++) Stack.pop_back(); }
    int addValue(Value Val) { Stack.push_back(Val); return Stack.size() - 1; }
    int getSize() { return Stack.size(); }
};

Value LogErrorV(const char* Str);

void HandleDefinition(std::string& Code, int* Idx);

void HandleTopLevelExpression(std::string& Code, int* Idx);

void HandleImport(std::string& Code, int* Idx, int tmpCurTok, int tmpLastChar, bool tmpFlag);

void MainLoop(std::string& Code, int* Idx);

void Execute(const char* FileName);