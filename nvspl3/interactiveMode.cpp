
// SEL Project
// interactiveMode.cpp

#include "interactiveMode.h"

void RunInteractiveShell()
{
    std::string VerStr = "v1.3.0 ";
    #if defined(_WIN32)
        #if defined(_WIN64)
            #if defined(_M_ARM64)
                VerStr += ("(64-Bit Windows(ARM64))");
            #elif defined(_M_X64) || defined(_M_AMD64)
                VerStr += ("(64-Bit Windows(AMD64))");
            #else
                VerStr += ("(64-Bit Windows)");
            #endif
        #else
            #if defined(_M_IX86)
                VerStr += ("(32-Bit Windows(x86))");
            #elif defined(_M_ARM)
                VerStr += ("(32-Bit Windows(ARM))");
            #else
                VerStr += ("(32-Bit Windows)");
            #endif
        #endif
    #elif defined(__APPLE__)
        #if defined(__aarch64__)
            VerStr += ("(64-Bit macOS(ARM64))");
        #elif defined(__x86_64__)
            VerStr += ("(64-Bit macOS(AMD64))");
        #else
            VerStr += ("(macOS)");
        #endif
    #elif defined(__linux__)
        #if defined(__aarch64__)
            VerStr += ("(64-Bit Linux(ARM64))");
        #elif defined(__x86_64__)
            VerStr += ("(64-Bit Linux(AMD64))");
        #elif defined(__i386__)
            VerStr += ("(32-Bit Linux(x86))");
        #elif defined(__arm__)
            VerStr += ("(32-Bit Linux(ARM))");
        #else
            VerStr += ("(Linux)");
        #endif
    #else
        VerStr += ("(Other OS)");
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
    fprintf(stderr, "Welcome to the SEL help utility.\n");
    fprintf(stderr, "Here is a brief overview of the language syntax and features:\n\n");

    fprintf(stderr, "1. Variables & Data Types:\n");
    fprintf(stderr, "   Data Types: int, double\n");
    fprintf(stderr, "   <name> = <value>; (Variables are implicitly declared upon assignment)\n");
    fprintf(stderr, "   arr <name>[<size>]...; (Array declaration, can be multi-dimensional)\n");
    fprintf(stderr, "   @<expr>; (Pointer/Dereference expression)\n\n");

    fprintf(stderr, "2. Functions:\n");
    fprintf(stderr, "   func <name>(<args>) { ... }\n");
    fprintf(stderr, "   func binary<op> <precedence> (arg1, arg2) { ... } (Custom binary operator)\n");
    fprintf(stderr, "   func unary<op> (arg) { ... } (Custom unary operator)\n");
    fprintf(stderr, "   return <value>;\n\n");

    fprintf(stderr, "3. Control Flow:\n");
    fprintf(stderr, "   if <condition> then { ... } else { ... } (else block is optional)\n");
    fprintf(stderr, "   for <id> = <start>, <end>, <step> { ... } (step is optional)\n");
    fprintf(stderr, "   while <condition> { ... }\n");
    fprintf(stderr, "   loop { ... } (Infinite loop)\n");
    fprintf(stderr, "   rep <expression> { ... } (Repeats a specific number of times)\n");
    fprintf(stderr, "   break;\n\n");

    fprintf(stderr, "4. Operators:\n");
    fprintf(stderr, "   Available operators: +, -, *, /, %%, **, <, >, <=, >=, ==, !=, &&, ||, =\n\n");

    fprintf(stderr, "5. Standard Functions:\n");
    fprintf(stderr, "   print(<args...>);   (Prints arguments)\n");
    fprintf(stderr, "   println(<args...>); (Prints arguments with a newline)\n");
    fprintf(stderr, "   printch(<args...>); (Prints arguments as characters)\n");
    fprintf(stderr, "   input();            (Reads a number from input)\n");
    fprintf(stderr, "   inputch();          (Reads a character from input)\n\n");

    fprintf(stderr, "6. Other commands:\n");
    fprintf(stderr, "   import <path>; (Imports a script file w/o quotes. '.sel' is appended)\n");
    fprintf(stderr, "   help; (Displays this help message)\n\n");
}