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
  ret = "found " + tok.lexeme + " " + t->getNextToken().lexeme + " at line " + l + ", at column " + c + " when parser expected " + ret + "\n";
  std::cerr << ret;
  exit(1);
}

bool parser::isVarType(){
  return (t->CurTok.type == INT_TOK || t->CurTok.type == FLOAT_TOK || t->CurTok.type == BOOL_TOK);
}
bool parser::isVal(){
  return (t->CurTok.type == INT_LIT || t->CurTok.type == FLOAT_LIT || t->CurTok.type == BOOL_LIT);
}

std::unique_ptr<IntASTnode> parser::parseInt() {
  std::stringstream val(t->CurTok.lexeme);
  int ival = 0;
  val >> ival;
  TOKEN tok = t->CurTok;
  t->getNextToken();
  return llvm::make_unique<IntASTnode>(tok, ival);
}

std::unique_ptr<FloatASTnode> parser::parseFloat() {
  std::stringstream val(t->CurTok.lexeme);
  float ival = 0.0;
  val >> ival;
  TOKEN tok = t->CurTok;
  t->getNextToken();
  return llvm::make_unique<FloatASTnode>(tok, ival);
}

std::unique_ptr<BoolASTnode> parser::parseBool() {
  std::stringstream val(t->CurTok.lexeme);
  bool ival = false;
  val >> ival;
  TOKEN tok = t->CurTok;
  t->getNextToken();
  return llvm::make_unique<BoolASTnode>(tok, ival);
}

   std::unique_ptr<paramASTnode> parser::parseParam() {
    std::string type = t->CurTok.lexeme;
    TOKEN tok = t->getNextToken();
    return llvm::make_unique<paramASTnode>(tok, type, tok.lexeme);
  }

  std::unique_ptr<paramsASTnode> parser::parseParams() {
    if (t->CurTok.type != LPAR) exceptionString(t->CurTok, {"("});
    std::unique_ptr<paramsASTnode> p;
    if (t->getNextToken().type != RPAR){
      if (t->CurTok.type == VOID_TOK) {
        p = llvm::make_unique<voidParamASTnode>(t->CurTok);
        if (t->getNextToken().type != RPAR) exceptionString(t->CurTok, {")"});
        t->getNextToken();
      } else if (isVarType()) {
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
    } else {
      p = std::unique_ptr<paramsASTnode>(nullptr);
      t->getNextToken();
    }
    return std::move(p);
  }

   std::unique_ptr<externASTnode> parser::parseExtern(){
    std::string type = t->getNextToken().lexeme;
    if (t->CurTok.type != VOID_TOK && !(isVarType())) exceptionString(t->CurTok, {"int","float","bool","void"});
    TOKEN tok = t->getNextToken();
    std::string name = tok.lexeme;
    if (tok.type != IDENT) exceptionString(t->CurTok, {"an identifier"});
    t->getNextToken();
    std::unique_ptr<paramsASTnode> p = parseParams();
    if (t->CurTok.type != SC) exceptionString(t->CurTok, {";"});
    return llvm::make_unique<externASTnode>(tok, type, name, std::move(p));
  }

  std::unique_ptr<varDeclASTnode> parser::parseLocal() {
    std::string type = t->CurTok.lexeme;
    TOKEN tok = t->getNextToken();
    std::string name = tok.lexeme;
    if (tok.type != IDENT) exceptionString(t->CurTok, {"an identifier"});
    if (t->getNextToken().type != SC) exceptionString(t->CurTok, {";"});
    return llvm::make_unique<varDeclASTnode>(tok, type, name, false);
  }

  std::unique_ptr<subExprASTnode> parser::parseSubExpr() {
    t->getNextToken();
    std::unique_ptr<exprASTnode> e = parseExpr();
    if (t->CurTok.type != RPAR) exceptionString(t->CurTok, {")","some operator"});
    t->getNextToken();
    return llvm::make_unique<subExprASTnode>(std::move(e));
  }

  std::unique_ptr<baseValASTnode> parser::parseBaseVal() {
    if (t->CurTok.type == INT_LIT) return parseInt();
    else if (t->CurTok.type == FLOAT_LIT) return parseFloat();
    else if (t->CurTok.type == BOOL_LIT) return parseBool();
    else if (t->CurTok.type == LPAR) return parseSubExpr();
    else {
      if (t->CurTok.type != IDENT) exceptionString(t->CurTok, {"an identifier"});
      TOKEN n = t->CurTok;
      if (t->getNextToken().type == LPAR) {
        std::list<std::unique_ptr<exprASTnode>> args = {};
        t->getNextToken();
        while (t->CurTok.type != RPAR) {
          args.push_back(parseExpr());
          if ((t->CurTok.type != RPAR) && (t->CurTok.type != COMMA)) exceptionString(t->CurTok, {")",","});
        }
        t->getNextToken();
        return llvm::make_unique<funcCallASTnode>(n, n.lexeme, std::move(args));
      } else return llvm::make_unique<IdentASTnode>(n, n.lexeme);
    }
  }

  std::unique_ptr<negValASTnode> parser::parseNeg() {
    TOKEN tok = t->CurTok;
    int count = 0;
    if (t->CurTok.type == NOT) {
      count = 1;
      while (t->getNextToken().type == NOT) {
        count++;
      }
      if ((t->CurTok.type != IDENT) && (t->CurTok.type != LPAR) && !(isVal())) exceptionString(t->CurTok, {"(","!","an identifier","a literal value"});
    } else if (t->CurTok.type == MINUS) {
      count = 1;
      while (t->getNextToken().type == MINUS) {
        count++;
      }
      if ((t->CurTok.type != IDENT) && (t->CurTok.type != LPAR) && !(isVal())) exceptionString(t->CurTok, {"(","-","an identifier","a literal value"});
    }
    if ((t->CurTok.type != IDENT) && (t->CurTok.type != LPAR) && !(isVal())) exceptionString(t->CurTok, {"(","an identifier","a literal value"});
    std::unique_ptr<baseValASTnode> base = parseBaseVal();
    return llvm::make_unique<negValASTnode>(tok, count, std::move(base));
  }

  std::unique_ptr<modValASTnode> parser::parseMod() {
    if ((t->CurTok.type != IDENT) && (t->CurTok.type != LPAR) && !(isVal()) && (t->CurTok.type != MINUS) && (t->CurTok.type != NOT)) exceptionString(t->CurTok, {"(","!","-","an identifier","a literal value"});
    std::unique_ptr<negValASTnode> l = parseNeg();
    std::list<std::unique_ptr<negValASTnode>> r = {};
    //if (t->lineNo > 16) exceptionString(t->CurTok, {"test"});
    while (t->CurTok.type == MOD) {
      t->getNextToken();
      r.push_back(parseNeg());
    }
    return llvm::make_unique<modValASTnode>(std::move(l), std::move(r));
  }

  std::unique_ptr<divValASTnode> parser::parseDiv() {
    std::unique_ptr<modValASTnode> l = parseMod();
    std::list<std::unique_ptr<modValASTnode>> r = {};
    while (t->CurTok.type == DIV) {
      t->getNextToken();
      r.push_back(parseMod());
    }
    return llvm::make_unique<divValASTnode>(std::move(l), std::move(r));
  }

  std::unique_ptr<mulValASTnode> parser::parseMul() {
    std::unique_ptr<divValASTnode> l = parseDiv();
    std::list<std::unique_ptr<divValASTnode>> r = {};
    while (t->CurTok.type == ASTERIX) {
      t->getNextToken();
      r.push_back(parseDiv());
    }
    return llvm::make_unique<mulValASTnode>(std::move(l), std::move(r));
  }

  std::unique_ptr<addValASTnode> parser::parseAdd() {
    std::unique_ptr<mulValASTnode> l = parseMul();
    std::list<std::unique_ptr<mulValASTnode>> r = {};
    while (t->CurTok.type == PLUS) {
      t->getNextToken();
      r.push_back(parseMul());
    }
    return llvm::make_unique<addValASTnode>(std::move(l), std::move(r));
  }

  std::unique_ptr<subValASTnode> parser::parseSub() {
    std::unique_ptr<addValASTnode> l = parseAdd();
    std::list<std::unique_ptr<addValASTnode>> r = {};
    while (t->CurTok.type == MINUS) {
      t->getNextToken();
      r.push_back(parseAdd());
    }
    return llvm::make_unique<subValASTnode>(std::move(l), std::move(r));
  }

  std::unique_ptr<ineqValASTnode> parser::parseIneq() {
    std::unique_ptr<subValASTnode> l = parseSub();
    std::list<std::tuple<TOKEN, std::unique_ptr<subValASTnode>>> r = {};
    while ((t->CurTok.type == LE) || (t->CurTok.type == LT) ||(t->CurTok.type == GE) || (t->CurTok.type == GT)) {
      TOKEN tok = t->CurTok;
      t->getNextToken();
      std::tuple<TOKEN, std::unique_ptr<subValASTnode>> tpl = std::make_tuple(tok, parseSub());
      r.push_back(std::move(tpl));
    }
    return llvm::make_unique<ineqValASTnode>(std::move(l), std::move(r));
  }

  std::unique_ptr<eqValASTnode> parser::parseEq() {
    std::unique_ptr<ineqValASTnode> l = parseIneq();
    std::list<std::tuple<TOKEN, std::unique_ptr<ineqValASTnode>>> r = {};
    while ((t->CurTok.type == EQ) || (t->CurTok.type == NE)) {
      TOKEN tok = t->CurTok;
      t->getNextToken();
      std::tuple<TOKEN, std::unique_ptr<ineqValASTnode>> tpl = std::make_tuple(tok, parseIneq());
      r.push_back(std::move(tpl));
    }
    return llvm::make_unique<eqValASTnode>(std::move(l), std::move(r));
  }

  std::unique_ptr<andValASTnode> parser::parseAnd() {
    std::unique_ptr<eqValASTnode> l = parseEq();
    std::list<std::unique_ptr<eqValASTnode>> r = {};
    while (t->CurTok.type == AND) {
      t->getNextToken();
      r.push_back(parseEq());
    }
    return llvm::make_unique<andValASTnode>(std::move(l), std::move(r));
  }

  std::unique_ptr<orValASTnode> parser::parseOr() {
    std::unique_ptr<andValASTnode> l = parseAnd();
    std::list<std::unique_ptr<andValASTnode>> r = {};
    while (t->CurTok.type == OR) {
      t->getNextToken();
      r.push_back(parseAnd());
    }
    return llvm::make_unique<orValASTnode>(std::move(l), std::move(r));
  }

  std::unique_ptr<exprASTnode> parser::parseExpr() {
    if ((t->CurTok.type != IDENT) && (t->CurTok.type != LPAR) && !(isVal()) && (t->CurTok.type != MINUS) && (t->CurTok.type != NOT)) exceptionString(t->CurTok, {"(","!","-","an identifier","a literal value"});
    std::list<std::string> n = {};
    n.push_back(t->CurTok.lexeme);
    TOKEN c = t->CurTok;
    bool back = true;
    while (t->getNextToken().type == ASSIGN){
      c = t->getNextToken();
      if (c.type != IDENT) {
        back = false;
        break;
      }
      n.push_back(c.lexeme);
    }
    if (back) {
      t->putBackToken(t->CurTok);
      t->CurTok = c;
      n.pop_back();
    }
    if ((t->CurTok.type == LPAR) || isVal() || (t->CurTok.type == MINUS) || (t->CurTok.type == NOT) || (t->CurTok.type == IDENT)) {
      std::unique_ptr<orValASTnode> v = parseOr();
      return llvm::make_unique<exprASTnode>(std::move(n), std::move(v));
    } else return llvm::make_unique<exprASTnode>(std::move(n), std::unique_ptr<orValASTnode>(nullptr));
  }

  std::unique_ptr<ifStmtASTnode> parser::parseIf() {
    if (t->getNextToken().type != LPAR) exceptionString(t->CurTok, {"("});
    t->getNextToken();
    std::unique_ptr<exprASTnode> expr = parseExpr();
    if (t->CurTok.type != RPAR) exceptionString(t->CurTok, {")"});
    t->getNextToken();
    std::unique_ptr<blockASTnode> b = parseBlock();
    if ((t->CurTok.type != ELSE) && (t->CurTok.type != RBRA) && (t->CurTok.type != RETURN) && (t->CurTok.type != LBRA) && (t->CurTok.type != WHILE) && (t->CurTok.type != IF) && (t->CurTok.type != SC) && (t->CurTok.type != IDENT) && (t->CurTok.type != LPAR) && !(isVal()) && (t->CurTok.type != MINUS) && (t->CurTok.type != NOT)) exceptionString(t->CurTok, {"else",";","{","}","return","(","while","if","!","-","an identifier","a literal value"});
    if (t->CurTok.type == ELSE) {
      t->getNextToken();
      std::unique_ptr<blockASTnode> e = parseBlock();
      return llvm::make_unique<ifStmtASTnode>(std::move(expr), std::move(b), std::move(e));
    } else return llvm::make_unique<ifStmtASTnode>(std::move(expr), std::move(b), std::unique_ptr<blockASTnode>(nullptr));
  }

  std::unique_ptr<whileStmtASTnode> parser::parseWhile() {
    if (t->getNextToken().type != LPAR) exceptionString(t->CurTok, {"("});
    t->getNextToken();
    std::unique_ptr<exprASTnode> expr = parseExpr();
    if (t->CurTok.type != RPAR) exceptionString(t->CurTok, {")"});
    t->getNextToken();
    std::unique_ptr<stmtASTnode> stmt = parseStatment();
    return llvm::make_unique<whileStmtASTnode>(std::move(expr), std::move(stmt));
  }

  std::unique_ptr<returnStmtASTnode> parser::parseReturn() {
    t->getNextToken();
    if ((t->CurTok.type != LBRA) && (t->CurTok.type != SC) && (t->CurTok.type != IDENT) && (t->CurTok.type != MINUS) && (t->CurTok.type != NOT) && !(isVal())) exceptionString(t->CurTok, {";","(","!","-","an identifier","a literal value"});
    if (t->CurTok.type == SC) {
      t->getNextToken();
      return std::unique_ptr<returnStmtASTnode>(nullptr);
    }
    else {
      std::unique_ptr<exprASTnode> e = parseExpr();
      if (t->CurTok.type != SC) exceptionString(t->CurTok, {";"});
      t->getNextToken();
      return llvm::make_unique<returnStmtASTnode>(std::move(e));
    }
  }

  std::unique_ptr<stmtASTnode> parser::parseStatment() {
    if (t->CurTok.type == LBRA) return parseBlock();
    else if (t->CurTok.type == IF) return parseIf();
    else if (t->CurTok.type == WHILE) return parseWhile();
    else if (t->CurTok.type == RETURN) return parseReturn();
    else if (t->CurTok.type == SC) return nullptr;
    else {
      std::unique_ptr<exprASTnode> e = parseExpr();
      if (t->CurTok.type != SC) exceptionString(t->CurTok, {";"});
      t->getNextToken();
      return std::move(e);
    }
  }

  std::unique_ptr<blockASTnode> parser::parseBlock() {
    if (t->CurTok.type != LBRA) exceptionString(t->CurTok, {"{"});
    std::list <std::unique_ptr<varDeclASTnode>> locals = {};
    t->getNextToken();
    if ((t->CurTok.type != RBRA) && !(isVarType()) && (t->CurTok.type != RETURN) && (t->CurTok.type != LBRA) && (t->CurTok.type != WHILE) && (t->CurTok.type != IF) && (t->CurTok.type != SC) && (t->CurTok.type != IDENT) && (t->CurTok.type != LPAR) && !(isVal()) && (t->CurTok.type != MINUS) && (t->CurTok.type != NOT)) exceptionString(t->CurTok, {";","{","}","int","float","bool","return","(","while","if","!","-","an identifier","a literal value"});
    while (isVarType()){
      locals.push_front(parseLocal());
      t->getNextToken();
    }
    if ((t->CurTok.type != RBRA) && (t->CurTok.type != RETURN) && (t->CurTok.type != LBRA) && (t->CurTok.type != WHILE) && (t->CurTok.type != IF) && (t->CurTok.type != SC) && (t->CurTok.type != IDENT) && (t->CurTok.type != LPAR) && !(isVal()) && (t->CurTok.type != MINUS) && (t->CurTok.type != NOT)) exceptionString(t->CurTok, {";","{","}","return","(","while","if","!","-","an identifier","a literal value"});
    std::list <std::unique_ptr<stmtASTnode>> stmts = {};
    while (((t->CurTok.type == RETURN) || (t->CurTok.type == LBRA) || (t->CurTok.type == WHILE) || (t->CurTok.type == IF) || (t->CurTok.type == SC) || (t->CurTok.type == IDENT) || (t->CurTok.type == LPAR) || !(isVal()) || (t->CurTok.type == MINUS) || (t->CurTok.type == NOT)) && (t->CurTok.type != RBRA)){
      stmts.push_front(parseStatment());
    }
    if (t->CurTok.type != RBRA) exceptionString(t->CurTok, {";","{","}","return","(","while","if","!","-","an identifier","a literal value"});
    t->getNextToken();
    return llvm::make_unique<blockASTnode>(std::move(locals), std::move(stmts));
  }

  std::unique_ptr<funcDeclASTnode> parser::parseFuncDecl(TOKEN tok, std::string type, std::string name){
    std::unique_ptr<paramsASTnode> p = parseParams();
    std::unique_ptr<blockASTnode> b = parseBlock();
    return llvm::make_unique<funcDeclASTnode>(tok, type, name, std::move(p), std::move(b));
  }

  std::unique_ptr<funcDeclASTnode> parser::parseVoidFuncDecl(){
    std::string type = t->CurTok.lexeme;
    TOKEN tok = t->getNextToken();
    std::string name = tok.lexeme;
    if (tok.type != IDENT) exceptionString(t->CurTok, {"an identifier"});
    t->getNextToken();
    return parseFuncDecl(tok, type, name);
  }


  std::unique_ptr<declASTnode> parser::parseDecl(){
    if (t->CurTok.type == VOID_TOK) return parseVoidFuncDecl();
    else {
      std::string type = t->CurTok.lexeme;
      TOKEN tok = t->getNextToken();
      std::string name = tok.lexeme;
      if (tok.type != IDENT) exceptionString(t->CurTok, {"an identifier"});
      t->getNextToken();
      if ((t->CurTok.type != SC) && (t->CurTok.type != LPAR)) exceptionString(t->CurTok, {";","("});
      if (t->CurTok.type == SC) return llvm::make_unique<varDeclASTnode>(tok, type, name, true);
      else return parseFuncDecl(tok, type, name);
    }
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
    while (t->CurTok.type == VOID_TOK || isVarType()) {
      decls.push_front(parseDecl());
      t->getNextToken();
    }
    return llvm::make_unique<ProgramASTnode>(std::move(externs), std::move(decls));
  }
