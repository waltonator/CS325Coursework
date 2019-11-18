#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/Optional.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <map>
#include <memory>
#include <queue>
#include <string.h>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#include "parser.hpp"

using namespace llvm;
using namespace llvm::sys;

parser* par;
std::unique_ptr<ProgramASTnode> program;
//===----------------------------------------------------------------------===//
// Recursive Descent Parser - Function call for each production
//===----------------------------------------------------------------------===//

/* Add function calls for each production */


static void parse() {
  program = par->prog();
  llvm::outs() << program->to_string() <<"\n";
  // add body
}


//===----------------------------------------------------------------------===//
// Code Generation
//===----------------------------------------------------------------------===//

static LLVMContext TheContext;
static IRBuilder<> Builder(TheContext);
static std::unique_ptr<Module> TheModule;

//===----------------------------------------------------------------------===//
// AST Printer
//===----------------------------------------------------------------------===//

inline llvm::raw_ostream &operator<<(llvm::raw_ostream &os,
                                     const ASTnode &ast) {
  os << ast.to_string();
  return os;
}

//===----------------------------------------------------------------------===//
// Main driver code.
//===----------------------------------------------------------------------===//

int main(int argc, char **argv) {
  tokens* lexer = new tokens();
  if (argc == 2) {
    lexer->pFile = fopen(argv[1], "r");
    if (lexer->pFile == NULL)
      perror("Error opening file");
  } else {
    std::cout << "Usage: ./code InputFile\n";
    return 1;
  }

  lexer->lineNo = 1;
  lexer->columnNo = 1;
  par = new parser(lexer);
  // initialize line number and column numbers to zero
  par->t->lineNo = 1;
  par->t->columnNo = 1;

  // get the first token
  par->t->getNextToken();
  //while (CurTok.type != EOF_TOK) {
    //fprintf(stderr, "Token: %s with type %d\n", CurTok.lexeme.c_str(),
      //      CurTok.type);
    //getNextToken();
  //}
  //fprintf(stderr, "Lexer Finished\n");

  // Make the module, which holds all the code.
  TheModule = llvm::make_unique<Module>("mini-c", TheContext);

  // Run the parser now.
  parse();
  fprintf(stderr, "Parsing Finished\n");


  //********************* Start printing final IR **************************
  // Print out all of the generated code into a file called output.ll
  auto Filename = "output.ll";
  std::error_code EC;
  raw_fd_ostream dest(Filename, EC, sys::fs::F_None);

  if (EC) {
    errs() << "Could not open file: " << EC.message();
    return 1;
  }
  // TheModule->print(errs(), nullptr); // print IR to terminal
  TheModule->print(dest, nullptr);
  //********************* End printing final IR ****************************

  fclose(par->t->pFile); // close the file that contains the code that was parsed
  return 0;
}
