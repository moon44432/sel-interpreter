
// SEL Project
// lexer.cpp

#include "lexer.h"
#include "execute.h"
#include <iostream>
#include <cctype>
#include <cmath>

std::string IdStr;

dataType NumType;
double NumVal;

int LastChar = ' ';

int GetTok(std::string& Code, int& Idx)
{
    // Skip any whitespace.
    while (isspace(LastChar))
    {
        if (IsInteractive) LastChar = getchar();
        else LastChar = Code[Idx++];
    }

    if (isalpha(LastChar)) // identifier: [a-zA-Z][a-zA-Z0-9_]*
    { 
        IdStr = LastChar;
        
        while (true)
        {
            if (IsInteractive) LastChar = getchar();
            else LastChar = Code[Idx++];

            if (isalnum(LastChar) || LastChar == '_') IdStr += LastChar;
            else break;
        }

        // keywords
        if (IdStr == "func")
            return tok_def;
        if (IdStr == "extern")
            return tok_extern;
        if (IdStr == "import")
            return tok_import;
        if (IdStr == "arr")
            return tok_arr;
        if (IdStr == "if")
            return tok_if;
        if (IdStr == "then")
            return tok_then;
        if (IdStr == "else")
            return tok_else;
        if (IdStr == "for")
            return tok_for;
        if (IdStr == "while")
            return tok_while;
        if (IdStr == "rept")
            return tok_repeat;
        if (IdStr == "loop")
            return tok_loop;
        if (IdStr == "binary")
            return tok_binary;
        if (IdStr == "unary")
            return tok_unary;
        if (IdStr == "break")
            return tok_break;
        if (IdStr == "return")
            return tok_return;
        if (IdStr == "var")
            return tok_var;
        if (IdStr == "as")
            return tok_as;
        if (IdStr == "int")
            return tok_int;
        if (IdStr == "double")
            return tok_dbl;

        // interactive mode commands
        if (IdStr == "help")
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
            else LastChar = Code[Idx++];
        } while (isdigit(LastChar) || LastChar == '.');

        NumVal = strtod(NumStr.c_str(), nullptr);

        if (trunc(NumVal) == NumVal) NumType = dataType::t_int;
        else NumType = dataType::t_double;

        return tok_number;
    }

    if (LastChar == '#')
    {
        // Comment until end of line.
        do {
            if (IsInteractive) LastChar = getchar();
            else LastChar = Code[Idx++];
        } while (LastChar != EOF && LastChar != '\n' && LastChar != '\r');

        if (LastChar != EOF)
            return GetTok(Code, Idx);
    }

    if (LastChar == '{')
    {
        if (IsInteractive) LastChar = getchar();
        else LastChar = Code[Idx++];
        return tok_openblock;
    }
    if (LastChar == '}')
    {
        if (IsInteractive) LastChar = getchar();
        else LastChar = Code[Idx++];
        return tok_closeblock;
    }

    // Check for end of file.  Don't eat the EOF.
    if (LastChar == EOF)
        return tok_eof;

    // Otherwise, just return the character as its ascii value.
    int ThisChar = LastChar;
    if (IsInteractive) LastChar = getchar();
    else LastChar = Code[Idx++];
    return ThisChar;
}

std::string GetPath(std::string& Code, int& Idx)
{
    std::string PathStr;
    // Skip any whitespace.
    while (isspace(LastChar))
    {
        if (IsInteractive) LastChar = getchar();
        else LastChar = Code[Idx++];
    }

    PathStr = LastChar;
    while (true)
    {
        if (IsInteractive) LastChar = getchar();
        else LastChar = Code[Idx++];

        if (LastChar != '\n' && LastChar != ';' && LastChar != EOF) PathStr += LastChar;
        else break;
    }
    return PathStr;
}