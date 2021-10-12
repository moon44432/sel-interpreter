
// SEL Project
// interactiveMode.cpp

#include "interactiveMode.h"

void RunInteractiveShell()
{
    std::string VerStr = "v1.2.2 ";
    #ifdef _WIN32
    #ifdef _WIN64
    #ifdef _M_X64
        VerStr += ("(64-Bit Windows(AMD64))");
    #endif
    #elif defined(_M_IX86)
        VerStr += ("(32-Bit Windows(x86))");
    #endif
    #else
        VerStr += ("(Non-Windows System)");
    #endif

    // Install standard binary operators.
    // 1 is lowest precedence.
    // highest.
    InitBinopPrec();

    // Prime the first token.
    fprintf(stderr, ("SEL " + VerStr + " Interactive Shell\n").c_str());
    fprintf(stderr, "Type \"help;\" for help. Visit https://github.com/moon44432/sel-interpreter for more information.\n\n");
    fprintf(stderr, ">>> ");
    GetNextToken(MainCode, MainIdx);

    // Run the main "interpreter loop" now.
    MainLoop(MainCode, MainIdx);
}

void RunHelp()
{
    fprintf(stderr, "Welcome to SEL help utility.\n");
}