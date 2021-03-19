
// NVSPL3 Language Project
// main.cpp

#include "parser.h"
#include "execute.h"

int main() {

    // Install standard binary operators.
    // 1 is lowest precedence.
    // highest.
    binopPrecInit();

    // Prime the first token.
    fprintf(stderr, "ready> ");
    getNextToken();

    // Run the main "interpreter loop" now.
    MainLoop();

    return 0;
}
