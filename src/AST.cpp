#include <string>

// EXPRESSIONS => combination of literals, identifiers, operators, etc...
class ExprAST { // BASE CLASS FOR ALL EXPRESSION TYPES
public: // TODO => ADD A TYPE PARAMATER TO THIS
    virtual ~ExprAST() = default; // destructor that should be overwritten by derived classes...
};

// Numeric only expressions (A LITERAL)
class NumberExprAST : public ExprAST { 
    // private value allows us to tell the compiler the literal value
    double Value; // the actual value held by the expression
public:
    NumberExprAST(double Value) : Value(Value) {} // construction of a NumberExpr in our AST that gets passed a double
};

// Identifier names (considered an expression)
class VariableExprAST : public ExprAST { 
    std::string Name; // stores the name of the identifier
public:
    VariableExprAST(const std::string &Name) : Name(Name) {} // constructor that takes a reference to an identifier name (String), and holds it
};
 
// binary expressions with an intermediate operator => NEST OTHER EXPRESSIONS!!!
class BinaryExprAST : public ExprAST { 
    char Op; // the operation itself, which is a character (presumably will define the possible operations later)
    std::unique_ptr<ExprAST> LHS, RHS; // unique pointers to 2 more expressions that are the left and right hand side (recursive and context free...)
public:
    BinaryExprAST(char Op, std::unique_ptr<ExprAST> LHS, std::unique_ptr<ExprAST> RHS) : // takes in an operator, and pointers to the expressions on either side
        Op(Op) /* passes in the operation character */, 
        LHS(std::move(LHS) /* transfers ownership of the LHS expression to the LHS attribute of BinaryExprAST */), 
        RHS(std::move(RHS)/* equivalent to the prior constructor, but for the expr to the right of the operator*/) 
        {}
};

// calling expressions (FUNCTION CALLS)
class CallExprAST : public ExprAST {
    std::string Callee; // the name of the function being called
    std::vector<std::unique_ptr<ExprAST>> Args; // a collection of pointers to expressions that represent the argument list for the function itself

public:
    CallExprAST(const std::string &Callee, std::vector<std::unique_ptr<ExprAST>> Args) : // takes a string with the function name being called, as well as a collection of pointers to arguments (other expressions)
        Callee(Callee), // passes a const reference to the name of the function being called
        Args(std::move(Args)) // transfers ownership of the arguments (expressions) to the Args attribute of CallExprAST
        {}
};


// PROTOTYPES => this is the class that holds what is passed where we declare but do not implement a function (declarations)
class PrototypeAST {
    std::string Name;
    std::vector<std::string> Args;

public:
    PrototypeAST(const std::string &Name, std::vector<std::string> Args) : // takes a string with the name of the function prototype being stored, as well as a collection of pointers to arguments (other expressions)
        Name(Name), // passes a const reference to the name of the function in the declaration
        Args(std::move(Args)) // transfers ownership of the argument parameter names
        {}

    const std::string &getName() const { return Name; } // returns a reference to the name of the prototype functon (function does not change what is stored at the passed reference)
};


// FUNCTIONS => this is where the actual function definition is stored
class FunctionAST {
    std::unique_ptr<PrototypeAST> Proto; // the function delcaration
    std::unique_ptr<ExprAST> Body; // the body of the function, which could feasibly be an infinitely nested series of expressions...

public:
    FunctionAST(std::unique_ptr<PrototypeAST> Proto, std::unique_ptr<ExprAST> Body) : // pass a pointer to a function prototype, and a function body (just a bunch of nested expressions)
        Proto(std::move(Proto)), // ownsership transferred to FunctionAST 
        Body(std::move(Body)) // ownership transferred to FunctionAST
        {}

};