#include <iostream>
#include "../include/kaleidoscope/AST.h"
#include "../include/kaleidoscope/parser.h"
#include "../include/kaleidoscope/lexer.h"
#include "../include/kaleidoscope/codegen.h"

static std::unique_ptr<llvm::orc::KaleidoscopeJIT> TheJIT;
static llvm::ExitOnError ExitOnErr;

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

    llvm::PassBuilder PB; // pass builder -> object to control optimization passes
    PB.registerModuleAnalyses(*TheMAM); // registers module level passes
    PB.registerFunctionAnalyses(*TheFAM); // registers function level passes
    PB.crossRegisterProxies(*TheLAM, *TheFAM, *TheCGAM, *TheMAM); // allows analyes from one manager to be accessed by others
}

static void HandleDefinition() {
    if (auto FnAST = ParseDefinition()) { // parse the function definition
        if (auto* FnIR = FnAST->codegen()) { // generate llvm ir from the definition
            fprintf(stderr, "Read function definition: "); // print out the generated ir (next 2 lines as well)
            FnIR->print(llvm::errs());
            fprintf(stderr, "\n");
        } 
    } else { // error handling
        getNextToken();
    }
}

static void HandleDecl() {
    if (auto ProtoAST = ParseDecl()) { // parse the function delcaration into an AST node
        if (auto* FnIR = ProtoAST->codegen()) { // generate llvm ir for the function delcaration
            fprintf(stderr, "Read function declaration: "); // print out the ir
            FnIR->print(llvm::errs());
            fprintf(stderr, "\n");
        }
    } else {
        getNextToken();
    }
}

static void HandleTopLevelExpression() {
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
    llvm::InitializeNativeTarget(); // checks the target architecture on the local host
    llvm::InitializeNativeTargetAsmPrinter(); // initializes a native assembly printer
    llvm::InitializeNativeTargetAsmParser(); // initializes a native assembly parser

    // indicate our operator precedence
    BinOpPrecedence['<'] = 10;
    BinOpPrecedence['+'] = 20;
    BinOpPrecedence['-'] = 30;
    BinOpPrecedence['*'] = 40;
    BinOpPrecedence['/'] = 50;

    fprintf(stderr, ">> "); // prime the inital token
    getNextToken(); // go the the next one...

    TheJIT = ExitOnErr(llvm::orc::KaleidoscopeJIT::Create());

    InitializeModuleAndManagers();

    MainLoop(); // run the maininterpreter loop

    TheModule->print(llvm::errs(), nullptr);

    return 0;
}