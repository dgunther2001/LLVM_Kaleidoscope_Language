#ifndef PARSER_H
#define PARSER_H

#include <string>
#include <map>
#include "AST.h"
#include "lexer.h"


// NOTE => NULLPTR RETURNED ON ERRORS IN THE PARSER!!!

extern int CurTok; // a buffer to hold the current toen being evaluated
int getNextToken(); // get the next token in the stream

// ERROR HELPER FUNCTIONS
extern std::unique_ptr<ExprAST> LogError(const char* Str); // logs an error for expressions
extern std::unique_ptr<PrototypeAST> LogErrorP(const char* Str); // logs an error for function declarations

// numeric expression parser declaration
extern std::unique_ptr<ExprAST> ParseNumberExpr();

// evaluation of expressions within parenthesis...
extern std::unique_ptr<ExprAST> ParseParenExpr();

// evaluation of identifiers and function calles
extern std::unique_ptr<ExprAST> ParseIdentifierExpr();

// helper function that parses the above three types of expressions (primary expressions)
extern std::unique_ptr<ExprAST> ParsePrimary();


// PARSING BINARY EXPRESSIONS (INFIX)

// Operator Precedence Parsing of Binary Operators 
extern std::map<char, int> BinOpPrecedence; // dictionary with key value pairs of operators and their preceence

int GetTokPrecedence(); // get a binary operator's precedence

// parsing the right hand side of an expression 
extern std::unique_ptr<ExprAST> ParseBinOpRHS(int ExpressionPrecedence /* minumum operator precedence */ , std::unique_ptr<ExprAST> LHS /* pointer to the left hand side of the expression (already parsed) */);


// PARSING FUNCTION DECLARATIONS
extern std::unique_ptr<PrototypeAST> ParsePrototype(); // parse a function header

extern std::unique_ptr<FunctionAST> ParseDefinition(); // parse a function defintion

extern std::unique_ptr<PrototypeAST> ParseDecl(); // parse exculsive function declarations

extern std::unique_ptr<ExprAST> ParseIfExpr(); // allows us to parse conditional expressions

extern std::unique_ptr<FunctionAST> ParseTopLevelExpr(); // allows us to create functions without declaring them (lambdas??)

// FULLY PARSING EXPRESSIONS
extern std::unique_ptr<ExprAST> ParseExpression(); // the function where we start to parse an expression (can be infinitely recursive)



#endif