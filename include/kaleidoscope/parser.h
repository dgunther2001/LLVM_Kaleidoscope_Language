#ifndef PARSER_H
#define PARSER_H

#include <string>
#include "ast.h"
#include "lexer.h"

// NOTE => NULLPTR RETURNED ON ERRORS IN THE PARSER!!!

static int CurTok; // a buffer to hold the current toen being evaluated
static int getNextToken(); // get the next token in the stream

// ERROR HELPER FUNCTIONS
std::unique_ptr<ExprAST> LogError(const char* Str); // logs an error for expressions
std::unique_ptr<PrototypeAST> LogErrorP(const char* Str); // logs an error for function declarations

// numeric expression parser declaration
static std::unique_ptr<ExprAST> ParseNumberExpr();

// evaluation of expressions within parenthesis...
static std::unique_ptr<ExprAST> ParseParenExpr();

// evaluation of identifiers and function calles
static std::unique_ptr<ExprAST> ParseIdentifierExpr();

// helper function that parses the above three types of expressions (primary expressions)
static std::unique_ptr<ExprAST> ParsePrimary();

#endif