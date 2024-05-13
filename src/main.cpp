#include <iostream>
#include "../include/kaleidoscope/AST.h"
#include "../include/kaleidoscope/parser.h"
#include "../include/kaleidoscope/lexer.h"
#include "../include/kaleidoscope/codegen.h"

static void InitializeModule() {
    TheContext = std::make_unique<llvm::LLVMContext>(); // initializes an llvm context object
    TheModule = std::make_unique<llvm::Module>("Just in Time (JIT) Compiler", *TheContext); // initializes an llvm module to hold functions and other global declarations

    Builder = std::make_unique<llvm::IRBuilder<>>(*TheContext); // declares the ir builder, which is passed a pointer to the context object
}

static void HandleDefinition() {
    if (auto FnAST = ParseDefinition()) { // parse the function definition
        if (auto* FnIR = FnAST->codegen()) { // generate llvm ir from the definition
            fprintf(stderr, "Read function definition: "); // print out the generated ir (next 2 lines as well)
            FnIR->print(llvm::errs());
            fprintf(stderr, "\n");
        } 
    } else { // error handling
        getNextToken();
    }
}

static void HandleDecl() {
    if (auto ProtoAST = ParseDecl()) { // parse the function delcaration into an AST node
        if (auto* FnIR = ProtoAST->codegen()) { // generate llvm ir for the function delcaration
            fprintf(stderr, "Read function declaration: "); // print out the ir
            FnIR->print(llvm::errs());
            fprintf(stderr, "\n");
        }
    } else {
        getNextToken();
    }
}

static void HandleTopLevelExpression() {
    if (auto FnAST = ParseTopLevelExpr()) {
        if (auto* FnIR = FnAST->codegen()) {
            fprintf(stderr, "Read top-level expression: "); // indicate that we have parsed a top-level expression 
            FnIR->print(llvm::errs());
            fprintf(stderr, "\n");

            FnIR->eraseFromParent(); // removes the delcaration from the module, cleaning up after it has been printed 
        }
    } else {
        getNextToken();
    }
}

static void MainLoop() {
    while (true) {
        fprintf(stderr, ">> ");
        switch(CurTok) {
            case tok_eof: // if its the end of the file, exit the loop
                return;
            case ';':
                getNextToken(); // ignore semicolons and get the next token...
                break; // then break out of the switch statement
            case tok_def:
                HandleDefinition(); // handle function definitions
                break; 
            case tok_decl:
                HandleDecl(); // handle function declarations
                break;
            default:
                HandleTopLevelExpression(); // otherwise, it's a top level expression, so deal with that...
                break;
        }
    }
}


int main() {
    // indicate our operator precedence
    BinOpPrecedence['<'] = 10;
    BinOpPrecedence['+'] = 20;
    BinOpPrecedence['-'] = 30;
    BinOpPrecedence['*'] = 40;
    BinOpPrecedence['/'] = 50;

    fprintf(stderr, ">> "); // prime the inital token
    getNextToken(); // go the the next one...

    InitializeModule();

    MainLoop(); // run the maininterpreter loop

    TheModule->print(llvm::errs(), nullptr);

    return 0;
}