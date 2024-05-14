#include <iostream>
#include "../include/kaleidoscope/AST.h"
#include "../include/kaleidoscope/parser.h"
#include "../include/kaleidoscope/lexer.h"
#include "../include/kaleidoscope/codegen.h"
#include "../include/kaleidoscope/expression_handler.h"

int main() {
    llvm::InitializeNativeTarget(); // checks the target architecture on the local host
    llvm::InitializeNativeTargetAsmPrinter(); // initializes a native assembly printer
    llvm::InitializeNativeTargetAsmParser(); // initializes a native assembly parser

    // indicate our operator precedence
    BinOpPrecedence['<'] = 10;
    BinOpPrecedence['+'] = 20;
    BinOpPrecedence['-'] = 30;
    BinOpPrecedence['*'] = 40;
    BinOpPrecedence['/'] = 50;

    fprintf(stderr, ">> "); // prime the inital token
    getNextToken(); // go the the next one...

    TheJIT = ExitOnErr(llvm::orc::KaleidoscopeJIT::Create());

    InitializeModuleAndManagers();

    MainLoop(); // run the maininterpreter loop

    TheModule->print(llvm::errs(), nullptr);

    return 0;
}