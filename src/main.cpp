#include <iostream>
#include "../include/kaleidoscope/AST.h"
#include "../include/kaleidoscope/parser.h"
#include "../include/kaleidoscope/lexer.h"
#include "../include/kaleidoscope/codegen.h"
#include "../include/kaleidoscope/expression_handler.h"

int main(int argc, char** argv) {
    llvm::InitializeNativeTarget(); // checks the target architecture on the local host
    llvm::InitializeNativeTargetAsmPrinter(); // initializes a native assembly printer
    llvm::InitializeNativeTargetAsmParser(); // initializes a native assembly parser

    // indicate our operator precedence
    BinOpPrecedence['='] = 2;
    BinOpPrecedence['<'] = 10;
    BinOpPrecedence['+'] = 20;
    BinOpPrecedence['-'] = 30;
    BinOpPrecedence['*'] = 40;
    BinOpPrecedence['/'] = 50;

    std::fstream file;
    if (argc > 1) {
        file.open(argv[1]);
        if (!file) {
            fprintf(stderr, "File not found.\n");
            return 0;
        }
        input = &file;
    } else {
        fprintf(stderr, ">> "); // prime the inital token
        input = &std::cin;
    }

    getNextToken(); // go the the next one...

    TheJIT = ExitOnErr(llvm::orc::KaleidoscopeJIT::Create());

    InitializeModuleAndManagers();

    MainLoop(); // run the maininterpreter loop

    TheModule->print(llvm::errs(), nullptr);

    return 0;
}