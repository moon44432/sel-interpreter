
// SEL Project
// lexer.cpp

#include "lexer.h"
#include <iostream>
#include <cctype>

std::string IdentifierStr;
std::string MainCode;
double NumVal;
int LastChar = ' ';

static int idx = 0;

int gettok()
{
    // Skip any whitespace.
    while (isspace(LastChar))
    {
        if (IsInteractive) LastChar = getchar();
        else LastChar = MainCode[idx++];
    }

    if (isalpha(LastChar) || LastChar == '_') // identifier: [a-zA-Z_][a-zA-Z0-9_]*
    { 
        IdentifierStr = LastChar;
        
        while (true)
        {
            if (IsInteractive) LastChar = getchar();
            else LastChar = MainCode[idx++];

            if (isalnum(LastChar) || LastChar == '_') IdentifierStr += LastChar;
            else break;
        }

        // keywords
        if (IdentifierStr == "func")
            return tok_def;
        if (IdentifierStr == "extern")
            return tok_extern;
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
        if (IdentifierStr == "binary")
            return tok_binary;
        if (IdentifierStr == "unary")
            return tok_unary;
        if (IdentifierStr == "arr")
            return tok_arr;
        if (IdentifierStr == "rept")
            return tok_repeat;

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
            else LastChar = MainCode[idx++];
        } while (isdigit(LastChar) || LastChar == '.');

        NumVal = strtod(NumStr.c_str(), nullptr);
        return tok_number;
    }

    if (LastChar == '#')
    {
        // Comment until end of line.
        do
            if (IsInteractive) LastChar = getchar();
            else LastChar = MainCode[idx++];
        while (LastChar != EOF && LastChar != '\n' && LastChar != '\r');

        if (LastChar != EOF)
            return gettok();
    }

    if (LastChar == '{')
    {
        if (IsInteractive) LastChar = getchar();
        else LastChar = MainCode[idx++];
        return tok_openblock;
    }
    if (LastChar == '}')
    {
        if (IsInteractive) LastChar = getchar();
        else LastChar = MainCode[idx++];
        return tok_closeblock;
    }

    // Check for end of file.  Don't eat the EOF.
    if (LastChar == EOF)
        return tok_eof;

    // Otherwise, just return the character as its ascii value.
    int ThisChar = LastChar;
    if (IsInteractive) LastChar = getchar();
    else LastChar = MainCode[idx++];
    return ThisChar;
}