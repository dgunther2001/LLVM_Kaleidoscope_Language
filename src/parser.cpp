#include "../include/kaleidoscope/parser.h"

int CurTok;

std::map<char, int> BinOpPrecedence;

// increment to the next token...
int getNextToken() {
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
std::unique_ptr<ExprAST> ParseNumberExpr() { // creates a unique pointer that 
    auto Result = std::make_unique<NumberExprAST>(NumVal); // takes the current number value and creates a new numeric expression node
    getNextToken(); // sets the current token to the next token
    return std::move(Result); // transfers ownership of the result back to where it was called from
}

// parses expressions within parenthesis, and eats the parenthesis as well because they are used for grouping, and don't need to be included in the final AST
std::unique_ptr<ExprAST> ParseParenExpr() {
    getNextToken(); // consumes the '(' character and goes to the actual expression...
    // allows us to handle recursive grammars...
    auto V = ParseExpression(); // calls a generic ParseExpression function to evaluate the expression inside '('  and ')'
    if (!V) return nullptr; // if the parsed expression is null, then return a null pointer

    if (CurTok != ')') return LogError("Expected a ')' after expression."); // if the next token isn't a ')' after evaluating the inner expression, throw an error

    getNextToken(); // otherwise, consume the ')' character and move on
    return V; // return the pointer to the parsed expression within the parenthesis
}

// parses identifiers (VARIABLES AND FUNCTION CALLS!!!)
std::unique_ptr<ExprAST> ParseIdentifierExpr() {
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

std::unique_ptr<ExprAST> ParseVarExpr() {
    getNextToken(); // consume the "spawn" keyword

    std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>> VarNames; // a vector of pairs of variable names, as well as their evaluation before assignment

    if (CurTok != tok_identifier) { // if there is not at least one identifier after the var keyword, pass back a nullptr
        LogErrorP("Expected at least one identifier after 'spawn'.");
        return nullptr;
    }

    while(true) {
        std::string Name = IdentifierStr; // hold the name of the current identifier
        getNextToken(); // consume the identifier name
        std::unique_ptr<ExprAST> InitialVal; // declares a pointer which may or may not hold an initial value
        if (CurTok == '=') { // if we are declaring an initial value...
            getNextToken(); // consume the '='
            InitialVal = ParseExpression(); // parse the initial value
            if (!InitialVal) { // return a nullptr if we didn't parse the expression properly
                return nullptr;
            }
        }

        VarNames.push_back(std::make_pair(Name, std::move(InitialVal))); // push the newly declared variable into the vector of pairs defined earlier

        if (CurTok != ',') break; // if we're not going to list more variables, break out of the loop;
        getNextToken(); // otherwise consume the ','

        if (CurTok != tok_identifier) {
            LogError("expected list of identifiers after 'spawn' keyword.");
            return nullptr;
        }
    }

    if (CurTok != tok_endspawn) {
        LogErrorP("Expected the 'endspawn' keyword."); // return a  nullptr if we don't see the 'enspawn' keyword after vaiable declaration and initialization
        return nullptr;
    }

    getNextToken(); // consume the "endspawn" token

    auto Body = ParseExpression(); // now parse the "body" of the variable declarations
    if (!Body) {
        return nullptr; // if the body isn't parsed, throw back a nullptr
    }

    return std::make_unique<VarExprAST>(std::move(VarNames), std::move(Body));
}


// Helper function that parses primary expressions (NUMERIC, IDENTIFIERS, PARENTHETICAL)
std::unique_ptr<ExprAST> ParsePrimary() {
    switch (CurTok) { // based on the type of token we are parsing...
        default: // return our usual nullptr if there's an error, and log it
            getNextToken();
            return LogError("expected expression, unknown token...");
        case tok_identifier:
            return ParseIdentifierExpr(); // if it's an identifier, parse it as such
        case tok_number:
            return ParseNumberExpr(); // if it's a number, parse it that way
        case '(':
            return ParseParenExpr(); // if it's a parenthesis, deal with it that way...
        case tok_if:
            return ParseIfExpr(); // parse a conditional expression
        case tok_for:
            return ParseForExpr(); // parses for loop expressions in their totality
        case tok_var:
            return ParseVarExpr(); // parse local variable declaration expressions
    }
}


// INFIX BINARY EXPRESSIONS
int GetTokPrecedence() { // get the precedence out of the precedence map
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

std::unique_ptr<ExprAST> ParseBinOpRHS(int ExpressionPrecedence /* MINUMUM PRECEDENCE OF OPERATOR WE CAN CONSUME */, std::unique_ptr<ExprAST> LHS) {
    while (true) { // if the current token is a binary operator
        int TokPrec = GetTokPrecedence(); // get the precedence of the current operator

        if (TokPrec < ExpressionPrecedence) { // if the token precedence is too low to be consumed
            return LHS; // we don't consume the token and just return back the left hand side
        }

        // now we know we have a binary operator that we can consume...
        int BinOp = CurTok; // store the enum value of token of the LHS
        getNextToken(); // consume the operator and go the next token (increment CurTok)

        // parse the right hand side after the operator has been consumed
        auto RHS = ParseUnaryExpr(); // parse the right side of the expression (can also be infinitely nested...)
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


// PARSING FUNCTION DECLARATIONS

// parse function prototypes (where the function and its arguments are listed)
std::unique_ptr<PrototypeAST> ParsePrototype() {
    std::string FunctionName; // the name of the function
    unsigned KindOfProto = 0; // 0 => identifier; 1 => unary op; 2 => binary op
    unsigned BinaryPrecedence = 30;

    switch (CurTok) { // based on the current token type, parse differently...
        default:
            return LogErrorP("Expected function name.");
        case tok_identifier: // if it's an identifier...
            FunctionName = IdentifierStr; // set the function name to the identifier
            KindOfProto = 0; // set the KindOfProto to a function prototype (basic)
            getNextToken(); // consume the function name
            break;
        case tok_unary: // if it's a unary operator...
            getNextToken(); // consume the "unary" token
            if (!isascii(CurTok)) {
                return LogErrorP("Expected an operator.");
            }
            FunctionName = "unary"; // set the function name to unary for codegen purposes
            FunctionName += (char)CurTok; // append the defined operator to "unary"
            KindOfProto = 1; // set the type of prototype to a unary declaration
            getNextToken(); // consume the operator token
            break; // break out of the switch statement
        case tok_binary:
            getNextToken();  // consume the "binary" keyword
            if (!isascii(CurTok)) { // if the current token is not an ascii character (a new operator...)
                return LogErrorP("Expected a binary operator."); // pass back a nulptr and throw an error
            }
            FunctionName = "binary"; // set the function name to binary
            FunctionName += (char)CurTok; // add the operator to the function name itself
            KindOfProto = 2; // set the prototype to a user defined binary expression
            getNextToken(); // consume the operator token

            if (CurTok == tok_number) { // if the next token is a number...
                if (NumVal < 1 || NumVal > 100) { // and the number is between this specified range...
                    return LogErrorP("Invalid precedence value...");
                }
                BinaryPrecedence = (unsigned)NumVal; // set the precedence to the current token
                getNextToken(); // consume the passed precedence value
            }
            break; // break out of the switch statement
    }

    if (CurTok != '(') { // if the next token after an identifier is not a '('...
        return LogErrorP("Expected '(' in the function declaration."); // throw an error and return a nullptr back up the parse tree
    }

    std::vector<std::string> ArgNames; // initialize a vectore that will hold the name of the arguments
    while (true) {
        getNextToken(); // consumes the '(' or ','
        if (CurTok == ')') { // if we immediately get a closing brace...
            break; // break out of the loop
        }

        if (CurTok != tok_identifier) { // if the current token is not an identier...
            return LogErrorP("Expected identfier in arg list."); // throw a nullptr back up
        }

        ArgNames.push_back(IdentifierStr); // push the name of the identifier name into the args list

        getNextToken(); // go to the next token

        if (CurTok == ',') { // if it's a comma, we expect another argument, so we proceed with the loop
            continue;
        } else if (CurTok == ')') { // if it's a closing bracket, we break out of the loop
            break;
        } else { // if it's something else, we throw a nullptr back up
            return LogErrorP("Expected ',' or ')' in arg list.");
        }

    }

    getNextToken(); // consume the ')'

    if (KindOfProto && ArgNames.size() != KindOfProto) { // if KindOfProto is non-zero (NORMAL PROTOTYPE), and the number of args doesn't match a unary or binary expression...
        return LogErrorP("Invalid number of operands for desired operator type...");
    }

    return std::make_unique<PrototypeAST>(FunctionName, std::move(ArgNames), KindOfProto != 0 /* gives a boolean => NORMAL PROTOS ARE 0 */, BinaryPrecedence); // return a pointer to a PrototypeAST node with the name and arguments defined
}

// parse function definitions
std::unique_ptr<FunctionAST> ParseDefinition() {
    getNextToken(); // eat the keyword 'def'
    auto Proto = ParsePrototype(); // parse the function prototype
    if (!Proto) {
        return nullptr; // if the prototype is not parsed correctly, pass a nullptr back up
    }

    if (auto Expression = ParseExpression()) { // parse the function body as an expression
        return std::make_unique<FunctionAST>(std::move(Proto), std::move(Expression)); // create a new FunctionAST node and transfer ownership of both the prototype and expression
    }

    return nullptr; 
}

// parse function declarations with no definitions
std::unique_ptr<PrototypeAST> ParseDecl() {
    getNextToken(); // eat the 'decl' keyword
    return ParsePrototype(); // return the parsed function prototype
}

// parse conditional expressions
std::unique_ptr<ExprAST> ParseIfExpr() {
    getNextToken(); // consume the "if" token

    auto Condition  = ParseExpression();  // parse the expression conditional corresponding to the if statement
    if (!Condition) { // if the condition isn't parsed correctly...
        return nullptr; // pass a nullptr back up
    }

    if (CurTok != tok_then) { // if we don't have a then token...
        return LogError("Expected a 'then' statement."); // throw an error back up w/ a nullptr
    }

    getNextToken(); // consume the "then" token

    auto Then = ParseExpression(); // parse the body within the conditional statement
    if (!Then) { // if the condition body isn't parsed correctly...
        return nullptr; // pass a nullptr back up
    }

    if (CurTok != tok_else) {
        return LogError("Exprected an 'else;' statement."); // if we don't get an else, pass an error back up
    }

    getNextToken(); // consume the "else" token

    auto Else = ParseExpression(); // parse the expression in the else body
    if (!Else) { // if the expression wasn't parsed correctly...
        return nullptr; // pass a nullptr back up
    }

    return std::make_unique<IfExprAST>(std::move(Condition), std::move(Then), std::move(Else)); // transfer ownership of the parsed expression nodes into a new IfExprAST node
}   
// parsing for loop expressions
std::unique_ptr<ExprAST> ParseForExpr() {
    getNextToken(); // consume the "for" token

    if (CurTok != tok_identifier) {
        return LogError("Expected an identifier after the for statement.");
    }

    std::string IdName = IdentifierStr; // store the variable name
    getNextToken(); // consume the identifier

    if (CurTok != '=') {
        return LogError("Expected iterator intitialization with an '='.");
    }
    getNextToken(); // consume the '='

    auto Start = ParseExpression(); // parse the initialization of the iterator
    if (!Start) { // if an invalid initialization state is provided, pass a nullptr back up
        return nullptr;
    }

    if (CurTok != ',') {
        return LogError("Expected a ',' after initialization of iterator");
    }
    getNextToken(); // consume the ','

    auto End = ParseExpression(); // parse the end condition
    if(!End) { // if the end conditon fails to parse, pass a nullptr back up and unwind
        return nullptr;
    }

    // the option of providing how much to step the iterator by (NON-COMPULSORY)
    std::unique_ptr<ExprAST> Step;
    if (CurTok == ',') {
        getNextToken(); // consume the ','
        Step = ParseExpression(); // parse the step expression
        if (!Step) { // if the step didn't evaluate, passa nullptr back up...
            return nullptr;
        }
    }

    if (CurTok != tok_in) {
        return LogError("Expected 'in' token to close for-loop initialization");
    }
    getNextToken(); // consume the 'in' token

    auto Body = ParseExpression(); // parse the body as an expression
    if (!Body) { // if the body evaluated to a nullptr, pass that back up
        return nullptr;
    }

    return std::make_unique<ForExprAST>(IdName, std::move(Start), std::move(End), std::move(Step), std::move(Body)); // transfer ownership of parsed components and initialize a for-loop AST node
}

// parsing of unary expressions
std::unique_ptr<ExprAST> ParseUnaryExpr() {
    if (!isascii(CurTok) || CurTok == '(' || CurTok == ',') { // if its not an operator, it must just be a primary expression so parse it as such
        return ParsePrimary();
    }

    int Operator = CurTok; // ascii value of the current user defined unary operator
    getNextToken(); // consume the operater token
    if (auto Operand = ParseUnaryExpr()) { // THIS RECURSIVE NATURE ALLOWS US TO PARSE CONSECUTIVE UNARY OPERATORS!!!
        return std::make_unique<UnaryExprAST>(Operator, std::move(Operand));
    }
    return nullptr;
}

// parsing top level expressions
std::unique_ptr<FunctionAST> ParseTopLevelExpr() {
    if (auto Expression = ParseExpression()) { // if we are able to parse the expression (non nullptr return...)
        auto Proto = std::make_unique<PrototypeAST>("__anon_expr", /* FUNCTION NAME IS EMPTY */ std::vector<std::string>() /* pass an empty arguments list */);
        return std::make_unique<FunctionAST>(std::move(Proto), std::move(Expression)); // transfer ownership of the expression and prototype (delcaration) into a FunctionAST node
    }

    return nullptr; // if we could not parse the expression, pass a nullptr back
}


// FULLY PARSING EXPRESSIONS
std::unique_ptr<ExprAST> ParseExpression() { // the function we call to begin parsing expressions
    auto LHS = ParseUnaryExpr(); // parses the left hand side of a potential expression as a primary expression (can be infinitely nested) => can contain non-primary expressions within them...
    if (!LHS) return nullptr; // if we get a nullptr back after parsing the left hand side of the expression, we pass back a null pointer (ERROR SCHEME)

    return ParseBinOpRHS(0 /* minimum operator precedence (non-negative numbers) */, std::move(LHS) /* pointer to preparsed expression */); // parse the right hand side WITH the operator itself include
}


