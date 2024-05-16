#include "../include/kaleidoscope/expression_handler.h"


#ifdef _WIN32 // if we're on windows
#define DLLEXPORY __declspec(dllexport) // allow us to export from the windows dynamic link library
#else
#define DLLEXPORT // otherwise, define it as nothing
#endif

// treat it as a C function
extern "C" DLLEXPORT double putchard(double X) {
    fputc((char)X, stderr); // takes some double X, and prints it as a char to the error stream using fputc(...)
    //fprintf(stderr, "\n"); // OPTIONAL
    return 0;
}

// treat it as a C function
extern "C" DLLEXPORT double printd(double X) {
    fprintf(stderr, "%f\n", X); // takes some double X, and prints it as a double to the error stream using fprintf(...)
    return 0;
}

std::unique_ptr<llvm::orc::KaleidoscopeJIT> TheJIT;
llvm::ExitOnError ExitOnErr;

std::unique_ptr<llvm::FunctionPassManager> TheFPM;
std::unique_ptr<llvm::LoopAnalysisManager> TheLAM;
std::unique_ptr<llvm::FunctionAnalysisManager> TheFAM;
std::unique_ptr<llvm::CGSCCAnalysisManager> TheCGAM;
std::unique_ptr<llvm::ModuleAnalysisManager> TheMAM;
std::unique_ptr<llvm::PassInstrumentationCallbacks> ThePIC;
std::unique_ptr<llvm::StandardInstrumentations> TheSI;

void InitializeModuleAndManagers(void) {
    TheContext = std::make_unique<llvm::LLVMContext>(); // initializes an llvm context object
    TheModule = std::make_unique<llvm::Module>("Just in Time (JIT) Compiler", *TheContext); // initializes an llvm module to hold functions and other global declarations
    TheModule->setDataLayout(TheJIT->getDataLayout()); // sets the data layout to that of the just in time compiler
    
    Builder = std::make_unique<llvm::IRBuilder<>>(*TheContext); // declares the ir builder, which is passed a pointer to the context object

    TheFPM = std::make_unique<llvm::FunctionPassManager>(); // this holds and organizes the LLVM optimizations we want to run
    TheLAM = std::make_unique<llvm::LoopAnalysisManager>(); // optimizes loops in our program (for, while, etc)
    TheFAM = std::make_unique<llvm::FunctionAnalysisManager>(); // optimizes functions, and their bodies, etc
    TheCGAM = std::make_unique<llvm::CGSCCAnalysisManager>(); // managing optimizations at the CallGraph level (strongly connected components (SCC)) => groups of functions that call eachother
    TheMAM = std::make_unique<llvm::ModuleAnalysisManager>(); // module level optimizations
    ThePIC = std::make_unique<llvm::PassInstrumentationCallbacks>(); // define what happens in between passes
    TheSI = std::make_unique<llvm::StandardInstrumentations>(*TheContext, true); // define what happens between passes

    TheSI->registerCallbacks(*ThePIC, TheMAM.get()); // sets up callbacks for standard instrumentation passes

    TheFPM->addPass(llvm::InstCombinePass()); // wholly simplifies the ir by using algebraic identities, etc to simplify
    TheFPM->addPass(llvm::ReassociatePass()); // identifies associateive expressions, and uses for further constant folding optimizations (LLVM already has basic implemented)
    TheFPM->addPass(llvm::GVNPass()); // eliminates redundant subexpressions so that we don't compute the same things twice...
    TheFPM->addPass(llvm::SimplifyCFGPass()); // simplifies the control flow graph (merges blocks (currently just function bodies...))
   
    //TheFPM->addPass(llvm::createPromoteMemoryToRegisterPass()); // promote memory references to register references if possible
    //TheFPM->addPass(llvm::createInstructionCombiningPass()); //combine instructions to simplify and optimize # of exectuded instructions
    //TheFPM->addPass(llvm::createReassociatePass()); // reassociates expressions to execute the fewest instructions possible

    llvm::PassBuilder PB; // pass builder -> object to control optimization passes
    PB.registerModuleAnalyses(*TheMAM); // registers module level passes
    PB.registerFunctionAnalyses(*TheFAM); // registers function level passes
    PB.crossRegisterProxies(*TheLAM, *TheFAM, *TheCGAM, *TheMAM); // allows analyes from one manager to be accessed by others
}

void HandleDefinition() {
    if (auto FnAST = ParseDefinition()) { // parse the function definition
        if (auto* FnIR = FnAST->codegen()) { // generate llvm ir from the definition
            fprintf(stderr, "Read function definition: "); // print out the generated ir (next 2 lines as well)
            FnIR->print(llvm::errs());
            fprintf(stderr, "\n");
            ExitOnErr(TheJIT->addModule(llvm::orc::ThreadSafeModule(std::move(TheModule), std::move(TheContext)))); // transfer the new function to the JIT
            InitializeModuleAndManagers(); // open a new module to clean up the environment for further function defintiions,etc
        } 
    } else { // error handling
        getNextToken();
    }
}

void HandleDecl() {
    if (auto ProtoAST = ParseDecl()) { // parse the function delcaration into an AST node
        if (auto* FnIR = ProtoAST->codegen()) { // generate llvm ir for the function delcaration
            fprintf(stderr, "Read function declaration: "); // print out the ir
            FnIR->print(llvm::errs());
            fprintf(stderr, "\n");
            FunctionProtos[ProtoAST->getName()] = std::move(ProtoAST); // transfers ownership of the parsed function prototype into the ProtosMap for use later
        }
    } else {
        getNextToken();
    }
}

void HandleTopLevelExpression() {
    if (auto FnAST = ParseTopLevelExpr()) {
        if (FnAST->codegen()) {
            auto RT = TheJIT->getMainJITDylib().createResourceTracker(); // create a resource tracker to track JIT memory allocation

            auto TSM = llvm::orc::ThreadSafeModule(std::move(TheModule), std::move(TheContext)); // moves the context and the module itself into a thread safe module, which allows us to "safely" work with an llvm module
            ExitOnErr(TheJIT->addModule(std::move(TSM), RT)); // triggers code generation for all functions in the module
            InitializeModuleAndManagers(); // open up a new module

            auto ExprSymbol = ExitOnErr(TheJIT->lookup("__anon_expr")); // look for anonymous top level expressions in the JIT (GET A POINTER TO THE GENERATED CODE)
            //assert(ExprSymbol && "Function not found"); // assert that the lookup returned something

            // FUNCTIONALLY NO DIFFERENCE BETWEEN JIT COMPILED CODE AND NATIVE MACHINE CODE STATICALLY LINKED
            double (*FP)() = ExprSymbol.getAddress().toPtr<double (*)()>(); // gets the address of the anonymous symbol and returns a double so we can call it natively
            fprintf(stderr, "Evaluated to %f\n", FP());

            ExitOnErr(RT->remove()); // delete the anonymous expression module from the just in time compiler (b/c we don't support re-evaluation of top-level expressions)
        }
    }
}

void MainLoop() {
    while (true) {
        if (input == &std::cin) {
            fprintf(stderr, ">> \n");
        }
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