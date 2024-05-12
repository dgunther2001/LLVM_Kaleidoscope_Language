#ifndef CODE_GEN_H
#define CODE_GEN_H

#include <memory>
#include <map>
#include <string>

#include "parser.h"

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"

extern std::unique_ptr<llvm::LLVMContext> TheContext; // contains lots of LLVM core structures such as the type and constant tables, etc...

// NOTE => THE BUILDER IS ASSUMED TO BE SETUP TO GENERATE CODE INTO SOMETHING => explore further builder configuration options...
extern std::unique_ptr<llvm::IRBuilder<> /* keeps track of the current place to insert new instructions, as well as the methods to do so */> Builder; // helper object that makes it more straightforward to generate LLVM instructions 
extern std::unique_ptr<llvm::Module> TheModule; // LLVM construct that holds functions and global variables => the top-level structure we use to generate IR (owns memory of all of the IR we generate, which is why codegen() returns a raw pointer
extern std::map<std::string, llvm::Value*> NamedValues; // keeps track of which identfiers are valid in the current scope (THE SYMBOL TABLE)

llvm::Value *LogErrorV(const char* Str); // error reporting during LLVM code generation

#endif