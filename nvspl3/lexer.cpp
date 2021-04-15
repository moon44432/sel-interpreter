
// SEL Project
// lexer.cpp

#include "lexer.h"
#include <iostream>
#include <cctype>

std::string IdentifierStr;
std::string MainCode;
double NumVal;
int LastChar = ' ';
int MainIdx = 0;

int GetTok(std::string& Code, int *Idx)
{
    // Skip any whitespace.
    while (isspace(LastChar))
    {
        if (IsInteractive) LastChar = getchar();
        else LastChar = Code[(*Idx)++];
    }

    if (isalpha(LastChar)) // identifier: [a-zA-Z][a-zA-Z0-9_]*
    { 
        IdentifierStr = LastChar;
        
        while (true)
        {
            if (IsInteractive) LastChar = getchar();
            else LastChar = Code[(*Idx)++];

            if (isalnum(LastChar) || LastChar == '_') IdentifierStr += LastChar;
            else break;
        }

        // keywords
        if (IdentifierStr == "func")
            return tok_def;
        if (IdentifierStr == "extern")
            return tok_extern;
        if (IdentifierStr == "import")
            return tok_import;
        if (IdentifierStr == "arr")
            return tok_arr;
        if (IdentifierStr == "if")
            return tok_if;
        if (IdentifierStr == "then")
            return tok_then;
        if (IdentifierStr == "else")
            return tok_else;
        if (IdentifierStr == "for")
            return tok_for;
        if (IdentifierStr == "while")
            return tok_while;
        if (IdentifierStr == "rept")
            return tok_repeat;
        if (IdentifierStr == "loop")
            return tok_loop;
        if (IdentifierStr == "binary")
            return tok_binary;
        if (IdentifierStr == "unary")
            return tok_unary;
        if (IdentifierStr == "break")
            return tok_break;
        if (IdentifierStr == "return")
            return tok_return;

        // interactive mode commands
        if (IdentifierStr == "help")
            return cmd_help;

        return tok_identifier;
    }

    if (isdigit(LastChar) || LastChar == '.')
    { // Number: [0-9.]+
        std::string NumStr;
        do
        {
            NumStr += LastChar;

            if (IsInteractive) LastChar = getchar();
            else LastChar = Code[(*Idx)++];
        } while (isdigit(LastChar) || LastChar == '.');

        NumVal = strtod(NumStr.c_str(), nullptr);
        return tok_number;
    }

    if (LastChar == '#')
    {
        // Comment until end of line.
        do {
            if (IsInteractive) LastChar = getchar();
            else LastChar = Code[(*Idx)++];
        } while (LastChar != EOF && LastChar != '\n' && LastChar != '\r');

        if (LastChar != EOF)
            return GetTok(Code, Idx);
    }

    if (LastChar == '{')
    {
        if (IsInteractive) LastChar = getchar();
        else LastChar = Code[(*Idx)++];
        return tok_openblock;
    }
    if (LastChar == '}')
    {
        if (IsInteractive) LastChar = getchar();
        else LastChar = Code[(*Idx)++];
        return tok_closeblock;
    }

    // Check for end of file.  Don't eat the EOF.
    if (LastChar == EOF)
        return tok_eof;

    // Otherwise, just return the character as its ascii value.
    int ThisChar = LastChar;
    if (IsInteractive) LastChar = getchar();
    else LastChar = Code[(*Idx)++];
    return ThisChar;
}

std::string GetPath(std::string& Code, int* Idx)
{
    std::string PathStr;
    // Skip any whitespace.
    while (isspace(LastChar))
    {
        if (IsInteractive) LastChar = getchar();
        else LastChar = Code[(*Idx)++];
    }

    PathStr = LastChar;
    while (true)
    {
        if (IsInteractive) LastChar = getchar();
        else LastChar = Code[(*Idx)++];

        if (LastChar != '\n' && LastChar != ';' && LastChar != EOF) PathStr += LastChar;
        else break;
    }
    return PathStr;
}