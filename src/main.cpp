#include <iostream>
#include "../include/kaleidoscope/ast.h"
#include "../include/kaleidoscope/parser.h"
#include "../include/kaleidoscope/lexer.h"


static void HandleDefinition() {
    if (ParseDefinition()) { // parse a function definition 
        fprintf(stderr, "Parsed a function definition.\n"); // say that it was parsed correctly as an error message
    } else {
        getNextToken(); // otherwise go to the next token
    }
}

static void HandleDecl() {
    if (ParseDecl()) { // parse a declaration
        fprintf(stderr, "Parsed a function declaration.\n"); // indicate that we have parsed a declaration as an error message
    } else {
        getNextToken();
    }
}

static void HandleTopLevelExpression() {
    if (ParseTopLevelExpr()) {
        fprintf(stderr, "Parsed a top level expression.\n"); // indicate that we have parsed a top-level expression
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

    MainLoop(); // run the maininterpreter loop

    return 0;
}