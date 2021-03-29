
// NVSPL3 Language Project
// lexer.h

#pragma once

#include <string>

enum Token
{
    tok_eof = -1,

    // commands
    tok_def = -2,
    tok_extern = -3,

    // primary
    tok_identifier = -4,
    tok_number = -5,

    // control
    tok_if = -6,
    tok_then = -7,
    tok_else = -8,
    tok_for = -9,
    tok_while = -10,
    tok_repeat = -11,

    // operators
    tok_binary = -30,
    tok_unary = -31,

    // var definition
    tok_var = -35,
    tok_arr = -36,

    // block
    tok_openblock = -90,
    tok_closeblock = -91,

    // interactive mode commands
    cmd_help = -201,
};

extern std::string IdentifierStr;
extern std::string MainCode;
extern double NumVal;
extern int LastChar;
extern bool IsInteractive;

int gettok();