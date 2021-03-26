
// NVSPL3 Language Project
// main.cpp

#include "parser.h"
#include "execute.h"

int main()
{
    std::string VerStr = "NVSPL3 alpha ";
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
    binopPrecInit();

    // Prime the first token.
    fprintf(stderr, (VerStr + " Interactive Shell\n").c_str());
    fprintf(stderr, "Type \"help\" for help. Visit https://github.com/moon44432/nvspl3-interpreter for more information.\n\n");
    fprintf(stderr, ">>> ");
    getNextToken();

    // Run the main "interpreter loop" now.
    MainLoop();

    return 0;
}