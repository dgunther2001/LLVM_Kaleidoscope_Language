#include "../include/kaleidoscope/parser.h"

// increment to the next token...
static int getNextToken() {
    return CurTok = gettok(); // sets the current token to the next token...
}

// STANDARD ERROR LOGGING FUNCTION

std::unique_ptr<ExprAST> LogError(const char* Str) {
    fprintf(stderr, "Error: %s\n", Str); // writes the error message to the filestream
    return nullptr; // returns a null pointer
}

std::unique_ptr<PrototypeAST> LogErrorP(const char* Str) {
    LogError(Str); // calls the LogError function on the passes string
    return nullptr; // returns a null pointer
}

// PARSING STEPS FOR LOWEST LEVEL EXPRS (LITERALS) => recursive descent parsers
// 1. create a new expression node for the AST from whatever was passes as a token
// 2. go to the next token
// 3. transfer ownership of the resultant AST node back to the calling function

// parses numeric expressions only (LITERALS)
static std::unique_ptr<ExprAST> ParseNumberExpr() { // creates a unique pointer that 
    auto Result = std::make_unique<NumberExprAST>(NumVal); // takes the current number value and creates a new numeric expression node
    getNextToken(); // sets the current token to the next token
    return std::move(Result); // transfers ownership of the result back to where it was called from
}

// parses expressions within parenthesis, and eats the parenthesis as well because they are used for grouping, and don't need to be included in the final AST
static std::unique_ptr<ExprAST> ParseParenExpr() {
    getNextToken(); // consumes the '(' character and goes to the actual expression...
    // allows us to handle recursive grammars...
    auto V = ParseExpression(); // calls a generic ParseExpression function to evaluate the expression inside '('  and ')'
    if (!V) return nullptr; // if the parsed expression is null, then return a null pointer

    if (CurTok != ')') return LogError("Expected a ')' after expression."); // if the next token isn't a ')' after evaluating the inner expression, throw an error

    getNextToken(); // otherwise, consume the ')' character and move on
    return V; // return the pointer to the parsed expression within the parenthesis
}

// parses identifiers (VARIABLES AND FUNCTION CALLS!!!)
static std::unique_ptr<ExprAST> ParseIdentifierExpr() {
    std::string IdName = IdentifierStr; // gets the value stored in identifier string, which is a byproduct of the lexer (buffer for the identifier in the current token...)
    getNextToken(); // consume the identifier as we have now stored it in IdName

    // IF WE DON'T GET PARENTHESIS, ITS NOT A FUNCTION CALL
    if (CurTok != '(') return std::make_unique<VariableExprAST>(IdName); // create an identifier AST node with the name stored in IdName

    // if we do get a '(' it is a function call...
    getNextToken(); // consume the '('  as this doesn't need to be included in the AST
    std::vector<std::unique_ptr<ExprAST>> Args; // declares a collection of pointers to expressions, which is the argument list to the function call
    if (CurTok != ')') { // if the argument list is non-empty...
        while (true) { // until we break out of the loop...
            if (auto Arg = ParseExpression()) { // if the parsed expression (single argument) is not evaluated to null...
                Args.push_back(std::move(Arg)); // put it on the end of the vector, and also transfer ownership to Args
            } else {
                return nullptr; // otherwise the result of the parsed expression was a nullptr, so we return that back up, thus "reporting an errors"
            }

            if (CurTok == ')') break; // if we hit the end of the args list, break out of the loop, and stop parsing arguments...

            if (CurTok != ',') return LogError("Expected ')' or ',' in argument list, but neither were provided."); // if we get an unexpected token, log an error

            getNextToken(); // otherwise, go to the next token...
        }
    }

    getNextToken(); //consume the closing ')'

    return std::make_unique<CallExprAST>(IdName, std::move(Args)); // create and return a unique pointer to a Call Expression with the IdName and parsed collection of arguments
}


// Helper function that parses primary expressions (NUMERIC, IDENTIFIERS, PARENTHETICAL)
static std::unique_ptr<ExprAST> ParsePrimary() {
    switch (CurTok) { // based on the type of token we are parsing...
        default: // return our usual nullptr if there's an error, and log it
            return LogError("expected expression, unknown token...");
        case tok_identifier:
            return ParseIdentifierExpr(); // if it's an identifier, parse it as such
        case tok_number:
            return ParseNumberExpr(); // if it's a number, parse it that way
        case '(':
            return ParseParenExpr(); // if it's a parenthesis, deal with it that way...
        
    }
}


// INFIX BINARY EXPRESSIONS
static int GetTokPrecedence() { // get the precedence out of the precedence map
    if (!isascii(CurTok)) {
        return -1; // if the current token is not an ascii value, return a special value
    }

    // verify that it's a declared binary operation 
    int TokenPrecedence = BinOpPrecedence[CurTok]; // get the precedence value of the current token in the map
    
    // if the token precedece is lower than 0 (invalid operator...)
    if (TokenPrecedence <= 0) return -1;

    // otherwise, return the precedence vale
    return TokenPrecedence;
}

static std::unique_ptr<ExprAST> ParseBinOpRHS(int ExpressionPrecedence /* MINUMUM PRECEDENCE OF OPERATOR WE CAN CONSUME */, std::unique_ptr<ExprAST> LHS) {
    while (true) { // if the current token is a binary operator
        int TokPrec = GetTokPrecedence(); // get the precedence of the current operator

        if (TokPrec < ExpressionPrecedence) { // if the token precedence is too low to be consumed
            return LHS; // we don't consume the token and just return back the left hand side
        }

        // now we know we have a binary operator that we can consume...
        int BinOp = CurTok; // store the enum value of the binary operator token
        getNextToken(); // consume the operator and go the next token (increment CurTok)

        // parse the right hand side after the operator has been consumed
        auto RHS = ParsePrimary(); // parse the right side of the expression (can also be infinitely nested...)
        if (!RHS) { // if the right hand side is not a pointer to an expression...
            return nullptr; // return a null pointer that acts as our current error handling scheme
        }

        // NOTE => WE ARE ALREADY AT THE NEXT TOKEN 
        int NextPrec = GetTokPrecedence();
        if (TokPrec < NextPrec) { // if the previous operator precedence is greater than the next operator's precedencs...
            RHS = ParseBinOpRHS(TokPrec + 1, std::move(RHS)); // parse the right hand side of the subexpression again with the enxt order of precedence
            if (!RHS) {
                return nullptr; // if the righthand side is null, do our error handling process...
            }
        }

        // merging into one binary expression AST node
        LHS = std::make_unique<BinaryExprAST>(BinOp, std::move(LHS), std::move(RHS)); // transfers the operator as well as ownership into the AST node itself
    }    
}

// FULLY PARSING EXPRESSIONS
static std::unique_ptr<ExprAST> ParseExpression() { // the function we call to begin parsing expressions
    auto LHS = ParsePrimary(); // parses the left hand side of a potential expression as a primary expression (can be infinitely nested) => can contain non-primary expressions within them...
    if (!LHS) return nullptr; // if we get a nullptr back after parsing the left hand side of the expression, we pass back a null pointer (ERROR SCHEME)

    return ParseBinOpRHS(0 /* minimum operator precedence (non-negative numbers) */, std::move(LHS) /* pointer to preparsed expression */); // parse the right hand side WITH the operator itself include
}


