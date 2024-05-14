#ifndef CODE_GEN_H
#define CODE_GEN_H

#include <memory>
#include <map>
#include <string>

#include "parser.h"

#include "../../external_libs/KaleidoscopeJIT.h"


//#include "llvm/ExecutionEngine/Orc/KaleidoscopeJIT.h"

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
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

extern std::unique_ptr<llvm::LLVMContext> TheContext; // contains lots of LLVM core structures such as the type and constant tables, etc...

// NOTE => THE BUILDER IS ASSUMED TO BE SETUP TO GENERATE CODE INTO SOMETHING => explore further builder configuration options...
extern std::unique_ptr<llvm::IRBuilder<> /* keeps track of the current place to insert new instructions, as well as the methods to do so */> Builder; // helper object that makes it more straightforward to generate LLVM instructions 
extern std::unique_ptr<llvm::Module> TheModule; // LLVM construct that holds functions and global variables => the top-level structure we use to generate IR (owns memory of all of the IR we generate, which is why codegen() returns a raw pointer
extern std::map<std::string, llvm::Value*> NamedValues; // keeps track of which identfiers are valid in the current scope (THE SYMBOL TABLE)

extern std::unique_ptr<llvm::FunctionPassManager> TheFPM;
extern std::unique_ptr<llvm::LoopAnalysisManager> TheLAM;
extern std::unique_ptr<llvm::FunctionAnalysisManager> TheFAM;
extern std::unique_ptr<llvm::CGSCCAnalysisManager> TheCGAM;
extern std::unique_ptr<llvm::ModuleAnalysisManager> TheMAM;
extern std::unique_ptr<llvm::PassInstrumentationCallbacks> ThePIC;
extern std::unique_ptr<llvm::StandardInstrumentations> TheSI;

extern std::map<std::string, std::unique_ptr<PrototypeAST>> FunctionProtos;

llvm::Value *LogErrorV(const char* Str); // error reporting during LLVM code generation

extern llvm::Function* getFunction(std::string Name); // pass back an llvm function pointer based on a name

#endif