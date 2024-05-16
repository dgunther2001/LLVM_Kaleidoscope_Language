#ifndef AST_H
#define AST_H

#include <string>
#include <memory>

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/StandardInstrumentations.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/Transforms/Scalar/Reassociate.h"
#include "llvm/Transforms/Scalar/SimplifyCFG.h"
#include "llvm/Transforms/Utils.h"

// EXPRESSIONS => combination of literals, identifiers, operators, etc...
class ExprAST { // BASE CLASS FOR ALL EXPRESSION TYPES
public: // TODO => ADD A TYPE PARAMATER TO THIS
    virtual ~ExprAST() = default; // destructor that should be overwritten by derived classes...
   
    // returns an LLVM value object => represents a Static Single Assignment (SSA) => no way to change SSA values (immutable)
    // EACH VARIBALE ASSIGNED EXACTLY ONCE
    virtual llvm::Value *codegen() = 0; // llvm ir generation functions (GENERATE IR FOR THE AST NODE AND EVERYTHING IT DEPENDS ON!!!)
};

// Numeric only expressions (A LITERAL)
class NumberExprAST : public ExprAST { 
    // private value allows us to tell the compiler the literal value
    double Value; // the actual value held by the expression
public:
    NumberExprAST(double Value) : Value(Value) {} // construction of a NumberExpr in our AST that gets passed a double
    llvm::Value *codegen() override; // overrides the generic llvm ir codegen function
};

// Identifier names (considered an expression)
class VariableExprAST : public ExprAST { 
    std::string Name; // stores the name of the identifier
public:
    VariableExprAST(const std::string &Name) : Name(Name) {} // constructor that takes a reference to an identifier name (String), and holds it
    llvm::Value *codegen() override;
    const std::string &getName() const { return Name; }
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
    llvm::Value *codegen() override;
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
    llvm::Value *codegen() override;
};


// PROTOTYPES => this is the class that holds what is passed where we declare but do not implement a function (declarations)
class PrototypeAST {
    std::string Name;
    std::vector<std::string> Args;
    bool IsOperator; // if the porototype is a user defined operator...
    unsigned Precedence; // precedence if it is a binary operator

public:
    PrototypeAST(const std::string &Name, std::vector<std::string> Args, bool IsOperator = false, unsigned Prec = 0) : // takes a string with the name of the function prototype being stored, as well as a collection of pointers to arguments (other expressions)
        Name(Name), // passes a const reference to the name of the function in the declaration
        Args(std::move(Args)), // transfers ownership of the argument parameter names
        IsOperator(IsOperator), // sets the default of IsOperator to false...
        Precedence(Prec) // sets the defaul precedence value to 0
        {}
    
    llvm::Function *codegen();
    const std::string &getName() const { return Name; } // returns a reference to the name of the prototype functon (function does not change what is stored at the passed reference)

    bool isUnaryOp() const { return IsOperator && Args.size() == 1; } // unary ops have 1 argument
    bool isBinaryOp() const { return IsOperator && Args.size() == 2; } // binary ops have 2 arguments...

    char getOperatorName() const { // get the operator as an ascii character
        assert(isUnaryOp() || isBinaryOp());
        return Name[Name.size() - 1];
    }

    unsigned getBinaryPrecedence() const { return Precedence; } // returns the operator precedence (ONLY USE IF BINARY EXPR)
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
    llvm::Function *codegen();

};

class IfExprAST : public ExprAST {
    std::unique_ptr<ExprAST> Condition; // a pointer to the expression in the condition
    std::unique_ptr<ExprAST> Then; // a pointer to a then subexpression
    std::unique_ptr<ExprAST> Else; // a pointer to the else subexpression

public: // constructor transfers ownership of the pointers to the subexpressions into the IfExprAST node
    IfExprAST( 
        std::unique_ptr<ExprAST> Condition,
        std::unique_ptr<ExprAST> Then,
        std::unique_ptr<ExprAST> Else
    ) :
    Condition(std::move(Condition)),
    Then(std::move(Then)),
    Else(std::move(Else))
    {}

    llvm::Value* codegen() override; // defines a codegen function that we implement elsewhere
};

class ForExprAST : public ExprAST {
    std::string VarName; // the name of the iterator
    std::unique_ptr<ExprAST> Start; // pointer to the intial value of th iterator
    std::unique_ptr<ExprAST> End; // end condition of the for loop
    std::unique_ptr<ExprAST> Step; // the for loop step
    std::unique_ptr<ExprAST> Body; // the body of the for loop itself

public:
    ForExprAST( // basic constructor that transfers ownership of all of the pointers to important loop constituents to the AST Node
        const std::string &VarName,
        std::unique_ptr<ExprAST> Start,
        std::unique_ptr<ExprAST> End,
        std::unique_ptr<ExprAST> Step,
        std::unique_ptr<ExprAST> Body
    ) :
    VarName(VarName),
    Start(std::move(Start)),
    End(std::move(End)),
    Step(std::move(Step)),
    Body(std::move(Body))
    {}

    llvm::Value* codegen() override; 
};

class UnaryExprAST : public ExprAST {
    char Operator; // the operator itself
    std::unique_ptr<ExprAST> Operand; // a pointer to the expression in which the operator will act on

public:
    UnaryExprAST(char Operator, std::unique_ptr<ExprAST> Operand) :
    Operator(Operator),
    Operand(std::move(Operand))
    {}

    llvm::Value* codegen() override;
};


#endif