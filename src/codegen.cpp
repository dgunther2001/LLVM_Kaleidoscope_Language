#include "../include/kaleidoscope/codegen.h"

std::unique_ptr<llvm::LLVMContext> TheContext;  // internally declares the llvm context (use this so that we can use other llvm apis)
std::unique_ptr<llvm::IRBuilder<>> Builder; // the actual llvm ir builder (codegenerator)
std::unique_ptr<llvm::Module> TheModule; // top level llvm structure that holds functions and global variables (owns all of the ir (memory-wise))
std::map<std::string, llvm::AllocaInst*> NamedValues; // keeps track of values defined in the current scope...

std::map<std::string, std::unique_ptr<PrototypeAST>> FunctionProtos;

llvm::Value *LogErrorV(const char* Str) { // codegen error logging function
    LogError(Str); // calls the LogError function on the passed string
    return nullptr; // passes a nullptr back up
}

llvm::Function* getFunction(std::string Name) {
    if (auto* F = TheModule->getFunction(Name)) { // if the function is already defined in the module symbol table, return a pointer to it
        return F;
    }

    // if the function is not in the symbol table, see if we can create it from an existing prototype
    auto FI = FunctionProtos.find(Name); // find the name in the protos table
    if (FI != FunctionProtos.end()) {
        return FI->second->codegen(); // generate code based on the function bofy
    }

    // if no prototype exists...
    return nullptr; // pass a nullptr back up
    
}

// helper function that ensures that allocas are generated in the entry block of a function (WHERE THEY ARE INTENDED TO BE PLACED!!!)
llvm::AllocaInst* CreateEntryBlockAllocation(llvm::Function* TheFunction, llvm::StringRef VarName) {
    // create an ir builder that creates an allocation with the associated name
    llvm::IRBuilder<> TmpBuiler(&TheFunction->getEntryBlock(), TheFunction->getEntryBlock().begin()); // creates an ir vbuilder that points to the first insturction in the function's entry block
    // returns a pointer to an allocated object (POINTER TO WHERE IT LIVES ON THE STACK)
    return TmpBuiler.CreateAlloca(llvm::Type::getDoubleTy(*TheContext), nullptr, VarName); // it then returns a memory allocation with the expected name and returns it
}

// numeric constants represented as ConstantFPs, which holds an APFloat (float with arbitrary precision)
llvm::Value *NumberExprAST::codegen() { // generating ir for numeric constants
    return llvm::ConstantFP::get(*TheContext, llvm::APFloat(Value)); // creates and returns a ConstantFP
}

llvm::Value *BinaryExprAST::codegen() { // RECURSIVELY EMIT IR FOR LHS AND RHS
    // evaluate the special case of an '=' token
    if (Op == '=') {
        VariableExprAST *LHSE = static_cast<VariableExprAST*>(LHS.get()); // casts from a basic expression to a varibale expression node
        if (!LHSE) {
            return LogErrorV("must be assigned to a variable..."); // if the variable is not there, throw back a nullptr
        }
        
        llvm::Value* val = RHS->codegen(); // evaluate the RHS of the assignment operator...
        if (!val) {
            return nullptr; // if it is not converted to llvm ir, return a nullptr back...
        }

        llvm::Value* Variable = NamedValues[LHSE->getName()]; // store a pointer to the variables location in the named values map
        if (!Variable) {
            return LogErrorV("Unknown var name"); // if the variable isn't in the map, pass back a nullptr
        }

        Builder->CreateStore(val, Variable); // creates a store instruction that puts the evaluated RHS expression into the location where the variable was allocated
        return val;
    }



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
            break; // means it is a user defined operator...
    }

    llvm::Function* F = getFunction(std::string("binary") + Op); // looks for the defined function in the module symbol table
    assert(F && "binary operator not found."); 

    llvm::Value* Operands[2] = { L, R }; // creates an llvm Value pointer array that contains the codegened LHS & RHS
    return Builder->CreateCall(F, Operands, "binop"); // creates a function call to the user defined operator
}

llvm::Value *VariableExprAST::codegen() {
    llvm::AllocaInst* A = NamedValues[Name]; // gets a pointer to the symbol table that holds where the variable is allocated
    if (!A) { // if the varibale isn't in the NamedValues table, throw an error
        LogErrorV("Undeclared variable name."); // pass a nullptr back 
    }
    return Builder->CreateLoad(A->getAllocatedType(), A, Name.c_str()); // generates a load instruction for the variable A
}

llvm::Value *VarExprAST::codegen() {
    std::vector<llvm::AllocaInst*> OldBindings; // remeber the previous value of a variable that we replace

    llvm::Function* TheFunction = Builder->GetInsertBlock()->getParent(); // gets the functiton in which the block exists

    for (unsigned i = 0, e = VarNames.size(); i != e; ++i) { // iterate over the table of variable names...
        const std::string &VarName = VarNames[i].first; // extracts the variable name from the vector pair entry
        ExprAST *InitExpr = VarNames[i].second.get(); // gets the value of the expression corresponding to the name in the vector
    
        llvm::Value* InitVal; // declare an initial value variable
        if (InitExpr) { // if there was an initial expressiond eclared in the declaration...
            InitVal = InitExpr->codegen(); // generate ir for that expression
            if (!InitVal) {
                return nullptr; // if code generation failed, pass back a nullptr
            }
        } else {
            InitVal = llvm::ConstantFP::get(*TheContext, llvm::APFloat(0.0)); // if the value is unspecified, just set it to 0
        }

        llvm::AllocaInst* Allocation = CreateEntryBlockAllocation(TheFunction, VarName); // create a memory allocation in the function with the corresponding variable name
        Builder->CreateStore(InitVal, Allocation); // create a store instruction that stores the initial value at the allocation

        OldBindings.push_back(NamedValues[VarName]); // store the old variable binding for when we jump down the recursive call stack later and want to restore old variables...

        NamedValues[VarName] = Allocation; // put the new allocation into the named values map for active use
    }

    llvm::Value* BodyValue = Body->codegen(); // generate ir for the body
    if (!BodyValue) { // if the body failed to evaluate, pass back a nullptr
        return nullptr;
    }

    for (unsigned i = 0, e = VarNames.size(); i != e; ++i) {
        NamedValues[VarNames[i].first] = OldBindings[i]; // reset the old bindings after we go out of scope
    }

    return BodyValue; // return the actual value of the computation
}

llvm::Value *CallExprAST::codegen() { // WE CAN CALL NATIVE C FUNCTIONS BY DEFAULT!!!
    llvm::Function *CalleeF = getFunction(Callee); // grabs a function pointer to the Callee from the FunctionProtos table
    if (!CalleeF) { // if the function is not found...
        return LogErrorV("Function not found in module symbol table"); // throw an error and pass back a nullptr
    }

    if(CalleeF->arg_size() != Args.size()) { // if the llvm function object doesn't contain the same number of arguments as those specified in the CallExpr node, then throw an error
        return LogErrorV("Incorrect number of arguments to function.");
    }

    std::vector<llvm::Value*> ArgsV; // declare a vector of llvm Value pointers which will contain the arguments themselves
    for (unsigned i = 0, e = Args.size(); i != e; ++i) { // iterate until we generate ir for all arguments passed (e)
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
    auto &P = *Proto;
    FunctionProtos[Proto->getName()] = std::move(Proto); // move ownership of the prototype into the prototype map
    llvm::Function *TheFunction = getFunction(P.getName()); // TheFunction points to the function retrieved from the FunctionProtos map
 
    if (!TheFunction) { // if the function evaluates to a nullptr, pass it back up
        return nullptr;
    }

    if (P.isBinaryOp()) { // if the prototype is a user defined binary operator...
        BinOpPrecedence[P.getOperatorName()] = P.getBinaryPrecedence(); // register the operator into the precedence table
    }

    llvm::BasicBlock *BasicBlock = llvm::BasicBlock::Create(*TheContext, "entry", TheFunction); // creates a basic block => fundamental to ir control flow in that it basically has explicit entry and exit points for control flow
    Builder->SetInsertPoint(BasicBlock); // setting the insertion point for llvm it within the function

    NamedValues.clear(); // clears named values in case they are defined globally, etc so that we don't get an error
    for (auto &Arg : TheFunction->args()) { // add arguments defined in the already ir-ified prototype, and put them into the NamedValues table
        llvm::AllocaInst* Allocation = CreateEntryBlockAllocation(TheFunction, Arg.getName()); //  creates a stack allocation for an argument to a function (OCCURS IN FUNCTION ENTRY BLOCK!!!!)
        Builder->CreateStore(&Arg, Allocation); // create a store instruction that puts the argument's initial value into the stack allocation
        NamedValues[std::string(Arg.getName())] = Allocation; // sets the the value of the argument name in the NamedValues map to the address of the allocation for that argument

    }

    if (llvm::Value* ReturnVal = Body->codegen()) { // if we properly turn the body into llvm ir... => call codegen on the root expression of the function
        Builder->CreateRet(ReturnVal); // create a return value in the builder that corresponds to the Return Value computed above => "completes the function"
        llvm::verifyFunction(*TheFunction); // validate generated ir => VERY VERY VERY IMPORTANT
        TheFPM->run(*TheFunction, *TheFAM); // run optimization passes
        return TheFunction; // return the fully ir-ified function
    } 

    TheFunction->eraseFromParent(); // delete the function itself, allowing the user tor edefine the function correctly
    if (P.isBinaryOp()) {
        BinOpPrecedence.erase(P.getOperatorName()); // removes the binary operator from the table of precedence values
    }
    
    return nullptr; // pass a nullptr back up
}

llvm::Value *IfExprAST::codegen(){
    llvm::Value* CondV = Condition->codegen(); // generates llvm ir for the if condition expression
    if (!CondV) { // if the condition didn't evaluate correclty, pass back a nullptr
        return nullptr;
    }

    CondV = Builder->CreateFCmpONE(CondV, llvm::ConstantFP::get(*TheContext, llvm::APFloat(0.0)), "ifcond"); // functionally, we emit the expression, and compare it to 0 to het a proper bool value

    llvm::Function *TheFunction = Builder->GetInsertBlock()->getParent(); // gets the current function block being built by getting the parent of that block (THE FUNCTION)
    
    // BASIC BLOCKS ARE INSTRUCTIONS WITH NO CONDITIONAL BRANCHES
    llvm::BasicBlock* ThenBasicBlock = llvm::BasicBlock::Create(*TheContext, "then", TheFunction); // codeblock of then statement
    llvm::BasicBlock* ElseBasicBlock = llvm::BasicBlock::Create(*TheContext, "else"); // codeblock of else statement
    llvm::BasicBlock* MergeBasicBlock = llvm::BasicBlock::Create(*TheContext, "ifcont"); // codeblock after if statement => POST CONDTIONAL

    // *** THE THEN PART!!!

    // NOTE THAT CONDV HOLDS A BOOLEAN => if we get a 1, go the ThenBasicBlock, otherwise go to the Else Basic Block
    Builder->CreateCondBr(CondV, ThenBasicBlock, ElseBasicBlock); // creates a conditional branch in the llvm ir that goes to the then block if the conditon is true, and the else block if it is false

    Builder->SetInsertPoint(ThenBasicBlock); // moves the builder's insertion point to the Then Block to generate ir for that block
    llvm::Value* ThenV = Then->codegen(); // generates the llvm ir for the Then Block
    if(!ThenV) { // if it hasn't been evaluated properly, return a nullptr back up
        return nullptr;
    }

    Builder->CreateBr(MergeBasicBlock); // unconditionally branches over the else block and past the entire if expression and goes back to the old control flow

    // DEALS WITH NESTED CONDITIONAL ISSUES AND PHI NODE GENERATION
    // NEED TO KNOW THE ENDPOINT OF EACH POSSIBLE PATH
    ThenBasicBlock = Builder->GetInsertBlock(); // has the Then basicblock to point where the builder is, so that if we use the if statment, we can properly find any phi nodes


    // *** THE ELSE PART

    TheFunction->insert(TheFunction->end(), ElseBasicBlock); // adds the else block to the function itself
    Builder->SetInsertPoint(ElseBasicBlock); // moves the builder's insertion point to the else block for it generation

    llvm::Value* ElseV = Else->codegen(); // generate llvm ir for the Else statement
    if (!ElseV) { // if the else expression block evaluated to a nullptr, pass the nullptr back up and unwind...
        return nullptr; 
    }

    Builder->CreateBr(MergeBasicBlock); // unconditonally branches out of the conditional
   
    // verify we are in the right spot at the end of the else block
    ElseBasicBlock = Builder->GetInsertBlock();

    TheFunction->insert(TheFunction->end(), MergeBasicBlock); // adding the merge block at the end of the function
    Builder->SetInsertPoint(MergeBasicBlock); // set the new insertion point of the builder to where the MergeBasicBlock begins

    llvm::PHINode* PN = Builder->CreatePHI(llvm::Type::getDoubleTy(*TheContext), 2, "iftmp"); // specifies that the phinode will choose from 2 possible values

    // IT CHOOSES BASED ON WHETHER CONTROL FLOW CAME FROM THE THEN OR ELSE BLOCK AND PICKS THE CORRECT ONE!!!
    PN->addIncoming(ThenV, ThenBasicBlock); // adds the llvm ir and the end of the Then block to the phi node (functionally adding a single possibility)
    PN->addIncoming(ElseV, ElseBasicBlock); // adds the else ir and block to the PN
    return PN;
}

llvm::Value* ForExprAST::codegen() {
    llvm::Function* TheFunction = Builder->GetInsertBlock()->getParent(); // gets the current function, which holds the for loop itse;f

    llvm::AllocaInst* Allocation = CreateEntryBlockAllocation(TheFunction, VarName); // creates a memory allocation for the iterator variable

    llvm::Value* StartValue = Start->codegen(); // generate ir for the initialization of the iterator
    if (!StartValue) { // if we failed to generate ir for the startvalue, pass an error back up
        return nullptr;
    }

    Builder->CreateStore(StartValue, Allocation); // creates a store instruction that stores the start value of the iterator at the location in memory it is allocated
    llvm::BasicBlock* LoopBasicBlock = llvm::BasicBlock::Create(*TheContext, "loop", TheFunction); // creatin a new basic block in the current function which corresponds to the function

    // for execution
    Builder->CreateBr(LoopBasicBlock); // jumps straight into the loop block

    // for further code insertion (comppiler use...)
    Builder->SetInsertPoint(LoopBasicBlock); // sets where new instructions will be inserted

    // this allows variable shadowing, so we hold the old value of the variable, and temporarily insert the iterator into the named values map
    /* Consider...
        {
            i = 10;

            for i = 1, i < 20, in
                ...loop_body
        }

        we can temporarily store the i = 10, thus "shadowing it", and allowing our iterator to be the variable i in the NamedValues map during the execution of the loop
    */
   
    llvm::AllocaInst* OldValue = NamedValues[VarName]; // temporarily pulls the old allocated value
    NamedValues[VarName] = Allocation; // sets the iterator allocating temporarily in the named values table

    if(!Body->codegen()) { // emit the body of the loop as ir, and if this doesn't execute, pass back a nullptr
        return nullptr;
    }

    llvm::Value* StepValue = nullptr; // initialize the StepValue to a nullptr to start
    if (Step) { // if we have defined a step in the for loop header...
        StepValue = Step->codegen(); // generate ir for the declared step value
        if (!StepValue) { // if we unsuccessfully create ir for the declared step value, pass back a nullptr
            return nullptr;
        }
    } else {
        StepValue = llvm::ConstantFP::get(*TheContext, llvm::APFloat(1.0)); // just set the step value to 1 if it isn't declared
    }

    // *** EVALUATING THE END CONDITION
    llvm::Value* EndCondition = End->codegen(); // generate ir for the end condition of the loop
    if (!EndCondition) { // if the end condition isn't evalutated to llvm ir properly, pass back a nullptr
        return nullptr;
    }

    llvm::Value* CurrentValue = Builder->CreateLoad(Allocation->getAllocatedType(), Allocation, VarName.c_str()); // creates a load insturction for the iterator
    llvm::Value* NextValue = Builder->CreateFAdd(CurrentValue, StepValue, "increment"); // adds the loaded allocation and increments it by the step value
    Builder->CreateStore(NextValue, Allocation); // creates a store instuction that puts the new iterator value at the location in memory of the allocation


    // functionally converting the end consdition to a boolean value...
    EndCondition = Builder->CreateFCmpONE(EndCondition, llvm::ConstantFP::get(*TheContext, llvm::APFloat(0.0)), "forendcond");

    llvm::BasicBlock* AfterLoopBasicBlock = llvm::BasicBlock::Create(*TheContext, "afterloop", TheFunction); // creates a block where control flow will go to after the loop is over

    /*
        if (EndCondition) => branch to the basic loop (slightly inverted logic because of the comparison of floats above)
        if (!EndCondition) => branch to the AfterLoop block, which is where control flow should go when we want to exit the loop
    */
    Builder->CreateCondBr(EndCondition, LoopBasicBlock, AfterLoopBasicBlock);
    
    Builder->SetInsertPoint(AfterLoopBasicBlock); // set the instruction insertion point to the spot after the loop, thus allowing us to continue building ir in the correct spot where contol flow is passed...

    if (OldValue) {
        NamedValues[VarName] = OldValue; // if the iterator in fact shadowed another variable, put it back in the named values table, and overwrite the iterator
    } else {
        NamedValues.erase(VarName); // otherwise, just erase the iterator from the named values table...
    }

    return llvm::Constant::getNullValue(llvm::Type::getDoubleTy(*TheContext)); // returns a default 0 value back becuase that is what codegen for the for loop always returns this...
}

llvm::Value* UnaryExprAST::codegen() {
    llvm::Value* OperandV = Operand->codegen(); // generate llvm ir for the expression that the unary operator is acting on...
    if (!OperandV) { // if the code generation of the operand failed, pass back a nullptr
        return nullptr;
    }

    llvm::Function* F = getFunction(std::string("unary") + Operator); // checks if the function has been defined, and get a pointer to it from the module
    if (!F) { // the function is undefined, throw a nullptr back up
        return LogErrorV("Undefined unary operator.");
    }

    return Builder->CreateCall(F, OperandV, "unop"); // generates a function call with the function name, and the operand evaluated to llvm ir
}