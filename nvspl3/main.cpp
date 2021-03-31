
// SEL Project
// main.cpp

#include "parser.h"
#include "execute.h"
#include "interactiveMode.h"

int main(int argc, char* argv[])
{
    if (argc == 1) runInteractiveShell();
    else if (argc == 2) execute(argv[1]);
    else fprintf(stderr, "You can run only one file at once.\nusage: %s \"filename.nvs\"\n", argv[0]);

    return 0;
}