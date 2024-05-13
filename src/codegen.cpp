#include "../include/kaleidoscope/codegen.h"
#include "../include/kaleidoscope/AST.h"


std::unique_ptr<llvm::LLVMContext> TheContext;  // internally declares the llvm context (use this so that we can use other llvm apis)
std::unique_ptr<llvm::IRBuilder<>> Builder; // the actual llvm ir builder (codegenerator)
std::unique_ptr<llvm::Module> TheModule; // top level llvm structure that holds functions and global variables (owns all of the ir (memory-wise))
std::map<std::string, llvm::Value*> NamedValues; // keeps track of values defined in the current scope...

llvm::Value *LogErrorV(const char* Str) { // codegen error logging function
    LogError(Str); // calls the LogError function on the passed string
    return nullptr; // passes a nullptr back up
}

// numeric constants represented as ConstantFPs, which holds an APFloat (float with arbitrary precision)
llvm::Value *NumberExprAST::codegen() { // generating ir for numeric constants
    return llvm::ConstantFP::get(*TheContext, llvm::APFloat(Value)); // creates and returns a ConstantFP
}

llvm::Value *BinaryExprAST::codegen() { // RECURSIVELY EMIT IR FOR LHS AND RHS
    llvm::Value *L = LHS->codegen(); // calls the codegen function on the lefthandside of the expression
    llvm::Value *R = RHS->codegen(); // calls the codegen function on the righthand side
    if (!L || !R) { // if the LHS or RHS evaluate to a nullptr, we have an error, so pass that back up as a nullptr
        return nullptr;
    }

    switch (Op) { // this is where the IRBuilder class starts to show its merit
    // IF WE EMIT MULTIPLE addtmp, subtmp, etc, the LLVM adds an incresing numeric suffix to differentiate them => optional but easir to read llvm ir dumps
        case '+':
            return Builder->CreateFAdd(L, R, "addtmp"); // call the create FAdd instruction on the builder and pass it the LHS and RHS of the expression, as well as a name (optional)
        case '-':
            return Builder->CreateFSub(L, R, "subtmp"); // call and create the FSub insturction 
        case '*':
            return Builder->CreateFMul(L, R, "multmp"); // call and create the FMul instruction
        case '/':
            return Builder->CreateFDiv(L, R, "divtmp"); // call and create the FDiv instruction (MAKE SURE THIS WORKS)
        case '<': // FCmp requires an I1 (a 1 or a 0, so we need to typecast to this)
            L = Builder->CreateFCmpULT(L, R, "cmptmp"); // evaluates the expression and puts it into L as a 1 or 0 (this is a problem) => because comparing integers, not FP values (unordered or less than condition (ULT))
            return Builder->CreateUIToFP(L, llvm::Type::getDoubleTy(*TheContext), "booltmp"); // creates another ir insturction that takes what was evaluated above and converts it to an FP (what the second argument does for us)
        default:
            return LogErrorV("Invalid binary operator.");
    }
}

llvm::Value *VariableExprAST::codegen() {
    llvm::Value* V = NamedValues[Name]; // looks up the name of the variable expression in the table of named values (VariableExpr has a field called Name)
    if (!V) { // if the varibale isn't in the NamedValues table, throw an error
        LogErrorV("Undeclared variable name."); // pass a nullptr back 
    }
    return V; // otherwise return the Value pointer to the named value
}

llvm::Value *CallExprAST::codegen() { // WE CAN CALL NATIVE C FUNCTIONS BY DEFAULT!!!
    llvm::Function *CalleeF = TheModule->getFunction(Callee); // creates a pointer to an LLVM Function object by looking up the function name in TheModule symbol table (THE CONTAINER OF FUNCTIONS!!!)
    if (!CalleeF) { // if the function is not found...
        return LogErrorV("Function not found in module symbol table"); // throw an error and pass back a nullptr
    }

    if(CalleeF->arg_size() != Args.size()) { // if the llvm function object doesn't contain the same number of arguments as those specified in the CallExpr node, then throw an error
        return LogErrorV("Incorrect number of arguments to function.");
    }

    std::vector<llvm::Value*> ArgsV; // declare a vector of llvm Value pointers which will contain the arguments themselves
    for (unsigned i = 0, e = Args.size(); i != e; i++) { // iterate until we generate ir for all arguments passed (e)
        ArgsV.push_back(Args[i]->codegen()); /// generate ir for the particular argument (an expression) which returns an llvm Value, and add it to the ArgsV vector
        if (!ArgsV.back()) { // if the element hasn't been added, then the end of the vector is a nullptr because the ir hasn't been evauluated properly..
            return nullptr; // pass nullptr back (error-handling)
        }
    }

    return Builder->CreateCall(CalleeF, ArgsV, "calltmp"); // build a function call with pointer to the function name in the symbol table, and the vector of evaluated arguments
}

llvm::Function *PrototypeAST::codegen() {
    std::vector<llvm::Type*> Doubles(Args.size(), llvm::Type::getDoubleTy(*TheContext)); // creates a vector called Doubles that is passed the size of arguments for the prototype, and creates sets the ir to floating point types (ALL ARGS ARE DOUBLES)
    llvm::FunctionType *FT = llvm::FunctionType::get(llvm::Type::getDoubleTy(*TheContext), Doubles, false);  // creates an LLVM function type that sets the return type to a Double in terms of the context, and indicates that the function args list does not vary (false)
    llvm::Function *F = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, Name, TheModule.get()); // creates the llvm ir for the prototype, which indicates the type, name, which symbol table to define it in (TheModule), and the external linkage (MUST IT BE DEFINED IN THE SAME MODULE)

    unsigned Index = 0; // set an iterator
    for (auto &Arg : F->args()) { // iterate over the arguments list 
        Arg.setName(Args[Index++]); // set the name of each function argument to that passed in the prototype (MAKES IR MORE CONSISTENT)
    }

    return F;
}

llvm::Function *FunctionAST::codegen() {
    llvm::Function *TheFunction = TheModule->getFunction(Proto->getName()); // see if the function has already been declared with a decl statement

    if (!TheFunction) { // if the function hasn't been declared...
        TheFunction = Proto->codegen(); // generate codegen for the function header
    }

    if (!TheFunction) { // if the function still is a nullptr...
        return nullptr;  // pass the nullptr back up as an error
    }

    if (!TheFunction->empty()) { // make sure the function does not yet have a body
        return  (llvm::Function*)LogErrorV("Function already defined."); // pass back an error and unwind...
    }

    llvm::BasicBlock *BasicBlock = llvm::BasicBlock::Create(*TheContext, "entry", TheFunction); // creates a basic block => fundamental to ir control flow in that it basically has explicit entry and exit points for control flow
    Builder->SetInsertPoint(BasicBlock); // setting the insertion point for llvm it within the function

    NamedValues.clear(); // clears named values in case they are defined globally, etc so that we don't get an error
    for (auto &Arg : TheFunction->args()) { // add arguments defined in the already ir-ified prototype, and put them into the NamedValues table
        NamedValues[std::string(Arg.getName())] = &Arg; // populate the NamedValues map
    }

    if (llvm::Value* ReturnVal = Body->codegen()) { // if we properly turn the body into llvm ir... => call codegen on the root expression of the function
        Builder->CreateRet(ReturnVal); // create a return value in the builder that corresponds to the Return Value computed above => "completes the function"
        llvm::verifyFunction(*TheFunction); // validate generated ir => VERY VERY VERY IMPORTANT
        return TheFunction; // return the fully ir-ified function
    } 

    TheFunction->eraseFromParent(); // delete the function itself, allowing the user tor edefine the function correctly
    return nullptr; // pass a nullptr back up
}
