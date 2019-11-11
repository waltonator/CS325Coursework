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
#include <utility>

//#include "token.hpp"
//#include "tokens.cpp"
//#include "ASTnodes.hpp"
#include "parser.hpp"


  void parser::exceptionString (TOKEN tok, std::list<std::string> expected) {
    std::string ret = expected.back();
    expected.pop_back();
    while (expected.size() > 0){
      ret = expected.back() + " or " + ret;
      expected.pop_back();
    }
    std::stringstream ss;
    std::string l;
    ss << tok.lineNo;
    l = ss.str();
    std::string c;
    ss << tok.columnNo;
    c = ss.str();
    ret = "found " + tok.lexeme + " at line " + l + ", at column " + c + " when parser expected " + ret;
    std::cerr << ret;
    exit(1);
  }

  bool parser::isVarType(){
     return (t->CurTok.type == INT_TOK || t->CurTok.type == FLOAT_TOK || t->CurTok.type == BOOL_TOK);
   }


   std::unique_ptr<IntASTnode> parser::parseInt() {
    std::stringstream val(t->CurTok.lexeme);
    int ival = 0;
    val >> ival;
    return llvm::make_unique<IntASTnode>(t->CurTok, ival);
  }

   std::unique_ptr<FloatASTnode> parser::parseFloat() {
    std::stringstream val(t->CurTok.lexeme);
    float ival = 0.0;
    val >> ival;
    return llvm::make_unique<FloatASTnode>(t->CurTok, ival);
  }

  std::unique_ptr<BoolASTnode> parser::parseBool() {
    std::stringstream val(t->CurTok.lexeme);
    bool ival = false;
    val >> ival;
    return llvm::make_unique<BoolASTnode>(t->CurTok, ival);
  }




   std::unique_ptr<paramASTnode> parser::parseParam() {
    std::string type = t->CurTok.lexeme;
    TOKEN tok = t->getNextToken();
    return llvm::make_unique<paramASTnode>(tok, type, tok.lexeme);
  }

   std::unique_ptr<externASTnode> parser::parseExtern(){
    std::string type = t->getNextToken().lexeme;
    if (t->CurTok.type != VOID_TOK && !(isVarType())) exceptionString(t->CurTok, {"int","float","bool","void"});
    TOKEN tok = t->getNextToken();
    std::string name = tok.lexeme;
    if (tok.type != IDENT) exceptionString(t->CurTok, {"an identifier"});
    if (t->getNextToken().type != LPAR) exceptionString(t->CurTok, {"("});
    std::unique_ptr<paramsASTnode> p;
    if (t->getNextToken().type != RPAR){
      if (t->CurTok.type == VOID_TOK) p = llvm::make_unique<voidParamASTnode>(t->CurTok);
      else if (isVarType()) {
      //create param list
        std::list <std::unique_ptr<paramASTnode>> params = {};
        while (isVarType()) {
          params.push_back(parseParam());
          t->getNextToken();
          if (t->CurTok.type != COMMA && t->CurTok.type != RPAR) exceptionString(t->CurTok, {",", ")"});
          t->getNextToken();
        }
        p = llvm::make_unique<paramListASTnode>(std::move(params));
      } else exceptionString(t->CurTok, {"int","float","bool","void", ")"});
    }
    if (t->CurTok.type != SC) exceptionString(t->CurTok, {";"});
    return llvm::make_unique<externASTnode>(tok, type, name, std::move(p));
  }

  std::unique_ptr<declASTnode> parser::parseDecl(){

  }


   std::unique_ptr<ProgramASTnode> parser::prog(){
    if (t->CurTok.type != EXTERN && !(isVarType()) && t->CurTok.type != VOID_TOK) exceptionString(t->CurTok, {"extern","int","float","bool","void"});
    std::list <std::unique_ptr<externASTnode>> externs = {};
    while (t->CurTok.type == EXTERN) {
      externs.push_front(parseExtern());
      t->getNextToken();
    }
    if (!(isVarType()) && t->CurTok.type != VOID_TOK) exceptionString(t->CurTok, {"int","float","bool","void"});
    std::list <std::unique_ptr<declASTnode>> decls = {};
    //while (t->CurTok.type == VOID_TOK || isVarType()) {
      //decls.push_front(parseDecl());
      //t->getNextToken();
    //}

    return llvm::make_unique<ProgramASTnode>(std::move(externs), std::move(decls));
  }