#ifndef CODE_GEN_H
#define CODE_GEN_H

#include <memory>
#include <map>
#include <string>

#include "parser.h"
#include "AST.h"
#include "expression_handler.h"

#include "../../external_libs/KaleidoscopeJIT.h"


//#include "llvm/ExecutionEngine/Orc/KaleidoscopeJIT.h"

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

extern std::unique_ptr<llvm::LLVMContext> TheContext; // contains lots of LLVM core structures such as the type and constant tables, etc...
extern std::unique_ptr<llvm::IRBuilder<>> Builder; // the actual llvm ir builder (codegenerator)
extern std::unique_ptr<llvm::Module> TheModule; // top level llvm structure that holds functions and global variables (owns all of the ir (memory-wise))
extern std::map<std::string, llvm::AllocaInst*> NamedValues; // keeps track of values defined in the current scope...
// NOTE => THE BUILDER IS ASSUMED TO BE SETUP TO GENERATE CODE INTO SOMETHING => explore further builder configuration options...

extern std::map<std::string, std::unique_ptr<PrototypeAST>> FunctionProtos;

llvm::Value *LogErrorV(const char* Str); // error reporting during LLVM code generation

extern llvm::Function* getFunction(std::string Name); // pass back an llvm function pointer based on a name

extern llvm::AllocaInst* CreateEntryBlockAllocation(llvm::Function* TheFunction, llvm::StringRef VarName);

#endif