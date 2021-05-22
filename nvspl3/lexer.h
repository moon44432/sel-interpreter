
// SEL Project
// lexer.h

#pragma once

#include <string>
#include "value.h"

enum Token
{
    tok_eof = -1,

    // commands
    tok_def = -2,
    tok_extern = -3,
    tok_import = -4,

    // primary
    tok_identifier = -10,
    tok_number = -11,

    // control
    tok_if = -16,
    tok_then = -17,
    tok_else = -18,
    tok_for = -19,
    tok_while = -20,
    tok_repeat = -21,
    tok_loop = -22,
    tok_break = -30,
    tok_return = -31,

    // operators
    tok_binary = -40,
    tok_unary = -41,

    // var definition
    tok_var = -45,
    tok_arr = -46,
    tok_as = -47,

    // block
    tok_openblock = -90,
    tok_closeblock = -91,

    // data types
    tok_int = -101,
    tok_dbl = -102,

    // interactive mode commands
    cmd_help = -201,
};

extern std::string IdentifierStr;
extern std::string PathStr;
extern std::string MainCode;

extern type NumValType;
extern double NumVal;

extern int CurTok;
extern int LastChar;

int GetTok(std::string& Code, int* Idx);

std::string GetPath(std::string& Code, int* Idx);