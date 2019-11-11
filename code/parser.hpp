#ifndef PARSER_HPP
#define PARSER_HPP

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
#include <memory>
#include <iostream>
#include <sstream>
#include <string.h>
#include <string>

//#include "token.hpp"
#include "tokens.hpp"
//#include "ASTnodes.hpp"

class parser {
public:
  tokens* t;
  parser(tokens *lexer) : t(lexer) {}
  void exceptionString (TOKEN tok, std::list<std::string> expected);
  bool isVarType();
  std::unique_ptr<IntASTnode> parseInt();
  std::unique_ptr<FloatASTnode> parseFloat();
  std::unique_ptr<BoolASTnode> parseBool();
  std::unique_ptr<paramASTnode> parseParam();
  std::unique_ptr<paramsASTnode> parseParams();
  std::unique_ptr<externASTnode> parseExtern();
  std::unique_ptr<blockASTnode> parseBlock();
  std::unique_ptr<funcDeclASTnode> parseFuncDecl(TOKEN tok, std::string type, std::string name);
  std::unique_ptr<funcDeclASTnode> parseVoidFuncDecl();
  std::unique_ptr<declASTnode> parseDecl();
  std::unique_ptr<ProgramASTnode> prog();
};


#endif
