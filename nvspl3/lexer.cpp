#include "lexer.h"
#include <iostream>

std::string IdentifierStr;
double NumVal;
int LastChar = ' ';


int gettok()
{
    // Skip any whitespace.
    while (isspace(LastChar))
        LastChar = getchar();

    if (isalpha(LastChar) || LastChar == '_')
    { // identifier: [a-zA-Z_][a-zA-Z0-9_]*
        std::cout << IdentifierStr << std::endl;
        IdentifierStr = LastChar;
        
        while (true)
        {
            LastChar = getchar();
            if (isalnum(LastChar) || LastChar == '_') IdentifierStr += LastChar;
            else break;
        }

        std::cout << IdentifierStr << std::endl;
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
        if (IdentifierStr == "binary")
            return tok_binary;
        if (IdentifierStr == "unary")
            return tok_unary;
        if (IdentifierStr == "var")
            return tok_var;
        if (IdentifierStr == "rept")
            return tok_repeat;
        return tok_identifier;
    }

    if (isdigit(LastChar) || LastChar == '.')
    { // Number: [0-9.]+
        std::string NumStr;
        do
        {
            NumStr += LastChar;
            LastChar = getchar();
        } while (isdigit(LastChar) || LastChar == '.');

        NumVal = strtod(NumStr.c_str(), nullptr);
        return tok_number;
    }

    if (LastChar == '#')
    {
        // Comment until end of line.
        do
            LastChar = getchar();
        while (LastChar != EOF && LastChar != '\n' && LastChar != '\r');

        if (LastChar != EOF)
            return gettok();
    }

    if (LastChar == '{')
    {
        LastChar = getchar();
        return tok_openblock;
    }
    if (LastChar == '}')
    {
        LastChar = getchar();
        return tok_closeblock;
    }

    // Check for end of file.  Don't eat the EOF.
    if (LastChar == EOF)
        return tok_eof;

    // Otherwise, just return the character as its ascii value.
    int ThisChar = LastChar;
    LastChar = getchar();
    return ThisChar;
}