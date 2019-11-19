#ifndef ASTNODES_HPP
#define ASTNODES_HPP


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
#include <string.h>
#include <string>
#include <list>
#include <iostream>
#include<sstream>
#include <tuple>
#include <vector>

#include "token.hpp"

extern llvm::LLVMContext TheContext;
extern llvm::IRBuilder<> Builder;
extern std::unique_ptr<llvm::Module> TheModule;
extern std::map<std::string, llvm::Value*> GlobalNamedValues;
extern std::list<std::string> funcNames;

class Scope {
public:
  Scope *parentScope;
  std::map<std::string, llvm::AllocaInst*> Values;
  Scope(Scope *par) : parentScope(par) {}
  void createNewLocal(llvm::Function *TheFunction, const std::string &name, llvm::Type *type);
  bool assignLocal(std::string name, llvm::Value *val);
  llvm::Value *getLocal(std::string name);
};

//===----------------------------------------------------------------------===//
// AST nodes
//===----------------------------------------------------------------------===//

/// ASTnode - Base class for all AST nodes.
class ASTnode {
public:
  virtual ~ASTnode() {}
  //virtual llvm::Value *codegen() = 0;
  virtual std::string to_string() const { return ""; };
};







class stmtASTnode : public ASTnode {
  public : std::string stmtType;
public:
  stmtASTnode(std::string st) : stmtType(st) {}
  virtual llvm::Value *codegen() = 0;

};

class baseValASTnode : public ASTnode {
  std::string Type;
public:
  baseValASTnode(std::string type) : Type(type) {}
  virtual llvm::Value *codegen() = 0;
};

class negValASTnode : public ASTnode {
  TOKEN Tok;
  int TokCount;
  std::unique_ptr<baseValASTnode> Val;
public:
  negValASTnode(TOKEN tok, int tokCount, std::unique_ptr<baseValASTnode> val) : Tok(tok), TokCount(tokCount), Val(std::move(val)) {}
  virtual llvm::Value *codegen();
  virtual std::string to_string() const override {
    if (TokCount == 0) return "negVal : " + Val->to_string();
    else {
      std::stringstream ss;
      ss << TokCount;
      return "negVal : " + ss.str() + "x" + Tok.lexeme + " (" + Val->to_string() + ")";
    }
  }
};

class modValASTnode : public ASTnode {
  std::unique_ptr<negValASTnode> left;
  std::list<std::unique_ptr<negValASTnode>> right;
public:
  modValASTnode(std::unique_ptr<negValASTnode> l, std::list<std::unique_ptr<negValASTnode>> r) : left(std::move(l)), right(std::move(r)) {}
  virtual llvm::Value *codegen();
  virtual std::string to_string() const override {
    std::string ret = "modVal : (" + left->to_string() + ")";
    for (auto&& i : right)  ret += " (" + i->to_string() + ")";
    return ret;
  }
};

class divValASTnode : public ASTnode {
  std::unique_ptr<modValASTnode> left;
  std::list<std::unique_ptr<modValASTnode>> right;
public:
  divValASTnode(std::unique_ptr<modValASTnode> l, std::list<std::unique_ptr<modValASTnode>> r) : left(std::move(l)), right(std::move(r)) {}
  virtual llvm::Value *codegen();
  virtual std::string to_string() const override {
    std::string ret = "divVal : (" + left->to_string() + ")";
    for (auto&& i : right)  ret += " (" + i->to_string() + ")";
    return ret;
  }
};

class mulValASTnode : public ASTnode {
  std::unique_ptr<divValASTnode> left;
  std::list<std::unique_ptr<divValASTnode>> right;
public:
  mulValASTnode(std::unique_ptr<divValASTnode> l, std::list<std::unique_ptr<divValASTnode>> r) : left(std::move(l)), right(std::move(r)) {}
  virtual llvm::Value *codegen();
  virtual std::string to_string() const override {
    std::string ret = "mulVal : (" + left->to_string() + ")";
    for (auto&& i : right)  ret += " (" + i->to_string() + ")";
    return ret;
  }
};

class addValASTnode : public ASTnode {
  std::unique_ptr<mulValASTnode> left;
  std::list<std::unique_ptr<mulValASTnode>> right;
public:
  addValASTnode(std::unique_ptr<mulValASTnode> l, std::list<std::unique_ptr<mulValASTnode>> r) : left(std::move(l)), right(std::move(r)) {}
  virtual llvm::Value *codegen();
  virtual std::string to_string() const override {
    std::string ret = "addVal : (" + left->to_string() + ")";
    for (auto&& i : right)  ret += " (" + i->to_string() + ")";
    return ret;
  }
};

class subValASTnode : public ASTnode {
  std::unique_ptr<addValASTnode> left;
  std::list<std::unique_ptr<addValASTnode>> right;
public:
  subValASTnode(std::unique_ptr<addValASTnode> l, std::list<std::unique_ptr<addValASTnode>> r) : left(std::move(l)), right(std::move(r)) {}
  virtual llvm::Value *codegen();
  virtual std::string to_string() const override {
    std::string ret = "subVal : (" + left->to_string() + ")";
    for (auto&& i : right)  ret += " (" + i->to_string() + ")";
    return ret;
  }
};

class ineqValASTnode : public ASTnode {
  std::unique_ptr<subValASTnode> left;
  std::list<std::tuple<TOKEN, std::unique_ptr<subValASTnode>>> right;
public:
  ineqValASTnode(std::unique_ptr<subValASTnode> l, std::list<std::tuple<TOKEN, std::unique_ptr<subValASTnode>>> r) : left(std::move(l)), right(std::move(r)) {}
  virtual llvm::Value *codegen();
  virtual std::string to_string() const override {
    std::string ret = "ineqVal : (" + left->to_string() + ")";
    for (auto& i : right)  ret += std::get<0>(i).lexeme + " (" + std::get<1>(i)->to_string() + ")";
    return ret;
  }
};

class eqValASTnode : public ASTnode {
  std::unique_ptr<ineqValASTnode> left;
  std::list<std::tuple<TOKEN, std::unique_ptr<ineqValASTnode>>> right;
public:
  eqValASTnode(std::unique_ptr<ineqValASTnode> l, std::list<std::tuple<TOKEN, std::unique_ptr<ineqValASTnode>>> r) : left(std::move(l)), right(std::move(r)) {}
  virtual llvm::Value *codegen();
  virtual std::string to_string() const override {
    std::string ret = "eqVal : (" + left->to_string() + ")";
    for (auto& i : right)  ret += std::get<0>(i).lexeme + " (" + std::get<1>(i)->to_string() + ")";
    return ret;
  }
};

class andValASTnode : public ASTnode {
  std::unique_ptr<eqValASTnode> left;
  std::list<std::unique_ptr<eqValASTnode>> right;
public:
  andValASTnode(std::unique_ptr<eqValASTnode> l, std::list<std::unique_ptr<eqValASTnode>> r) : left(std::move(l)), right(std::move(r)) {}
  virtual llvm::Value *codegen();
  virtual std::string to_string() const override {
    std::string ret = "andVal : (" + left->to_string() + ")";
    for (auto&& i : right)  ret += " (" + i->to_string() + ")";
    return ret;
  }
};

class orValASTnode : public ASTnode {
  std::unique_ptr<andValASTnode> left;
  std::list<std::unique_ptr<andValASTnode>> right;
public:
  orValASTnode(std::unique_ptr<andValASTnode> l, std::list<std::unique_ptr<andValASTnode>> r) : left(std::move(l)), right(std::move(r)) {}
  virtual llvm::Value *codegen();
  virtual std::string to_string() const override {
    std::string ret = "orVal : (" + left->to_string() + ")";
    for (auto&& i : right)  ret += " (" + i->to_string() + ")";
    return ret;
  }
};

class declASTnode : public ASTnode {
  TOKEN Tok;
  public : std::string Type;
  public : std::string Name;
public:
  declASTnode(TOKEN tok, std::string type, std::string name) : Tok(tok), Type(type), Name(name){}
  virtual llvm::Value *codegen() = 0;
  virtual std::string to_string() const override {
    return "decleration : " + Type + Name;
  }
};

class varDeclASTnode : public declASTnode {
  bool Global;
public:
  varDeclASTnode(TOKEN tok, std::string type, std::string name, bool global) : Global(global), declASTnode(tok, type, name) {}
  virtual llvm::Value *codegen() override;
  virtual std::string to_string() const override {
    if (Global) return "global variable decleration : " + Type + Name;
    else return "variable decleration : " + Type + " " + Name;
  }
};

class blockASTnode : public stmtASTnode {
  std::list<std::unique_ptr<varDeclASTnode>> localDecls;
  std::list<std::unique_ptr<stmtASTnode>> statements;
public:
  blockASTnode(std::list<std::unique_ptr<varDeclASTnode>> locals, std::list<std::unique_ptr<stmtASTnode>> stmts) : localDecls(std::move(locals)), statements(std::move(stmts)), stmtASTnode("block") {}
  virtual llvm::Value *codegen() override;
  virtual std::string to_string() const override {
    std::string ret = "block : ";
    for (auto&& i : localDecls)  ret += " (" + i->to_string() + ")";
    for (auto&& j : statements) if (j != nullptr) ret += " (" + j->to_string() + ")";
    return ret;
  }
};

/// IntASTnode - Class for integer literals like 1, 2, 10,
class IntASTnode : public baseValASTnode {
  int Val;
  TOKEN Tok;
  std::string Name;

public:
  IntASTnode(TOKEN tok, int val) : Val(val), Tok(tok), baseValASTnode("int") {}
  virtual llvm::Value *codegen() override;
  virtual std::string to_string() const override {
    return "int : " + Tok.lexeme;
  }
};

class FloatASTnode : public baseValASTnode {
  float Val;
  TOKEN Tok;
  std::string Name;

public:
  FloatASTnode(TOKEN tok, float val) : Val(val), Tok(tok), baseValASTnode("float") {}
  virtual llvm::Value *codegen() override;
  virtual std::string to_string() const override {
    return "float : " + Tok.lexeme;
  }
};

class BoolASTnode : public baseValASTnode {
  bool Val;
  TOKEN Tok;
  std::string Name;
public:
  BoolASTnode(TOKEN tok, bool val) : Val(val), Tok(tok), baseValASTnode("bool") {}
  virtual llvm::Value *codegen() override;
  virtual std::string to_string() const override {
    return "bool : " + Tok.lexeme;
  }
};

class IdentASTnode : public baseValASTnode {
  TOKEN Tok;
  std::string Name;
public:
  IdentASTnode(TOKEN tok, std::string name) : Tok(tok), Name(name), baseValASTnode("ident") {}
  virtual llvm::Value *codegen() override;
  virtual std::string to_string() const override {
    return "variable identifier : " + Name;
  }
};



class exprASTnode : public stmtASTnode {
  std::list<std::string> Names;
  std::unique_ptr<orValASTnode> Vals;
public:
  exprASTnode(std::list<std::string> names, std::unique_ptr<orValASTnode> vals) : Names(std::move(names)), Vals(std::move(vals)), stmtASTnode("expr") {}
  virtual llvm::Value *codegen() override;
  virtual std::string to_string() const override {
    std::string ret = "expression : ";
    for (auto&& i : Names)  ret += i + " = ";
    if (Vals != nullptr) ret += " (" + Vals->to_string() + ")";
    return ret;
  }
};

class subExprASTnode : public baseValASTnode {
  std::unique_ptr<exprASTnode> expression;
public:
  subExprASTnode(std::unique_ptr<exprASTnode> expr) : expression(std::move(expr)), baseValASTnode("subExpr") {}
  virtual llvm::Value *codegen() override;
  virtual std::string to_string() const override {
    return "sub expression : (" + expression->to_string() + ")";
  }
};

class funcCallASTnode : public baseValASTnode {
  TOKEN Tok;
  std::string Name;
  std::list<std::unique_ptr<exprASTnode>> arguments;
public:
  funcCallASTnode(TOKEN tok, std::string name, std::list<std::unique_ptr<exprASTnode>> args) : Tok(tok), Name(name), arguments(std::move(args)), baseValASTnode("func") {}
  virtual llvm::Value *codegen() override;
  virtual std::string to_string() const override {
    std::string ret = "function call : " + Name + " (";
    for (auto&& i : arguments)  ret += i->to_string() + ", ";
    ret += ")";
    return ret;
  }
};

class ifStmtASTnode : public stmtASTnode {
  std::unique_ptr<exprASTnode> Expression;
  std::unique_ptr<blockASTnode> Block;
  std::unique_ptr<blockASTnode> Els;
public:
  ifStmtASTnode(std::unique_ptr<exprASTnode> expr, std::unique_ptr<blockASTnode> block, std::unique_ptr<blockASTnode> els) : Expression(std::move(expr)), Block(std::move(block)), Els(std::move(els)), stmtASTnode("if") {}
  virtual llvm::Value *codegen() override;
  virtual std::string to_string() const override {
    if (Els == nullptr) return "if statment : (" + Expression->to_string() + ") (" + Block->to_string() + ")";
    else return "if statment : (" + Expression->to_string() + ") (" + Block->to_string() + ") (" + Els->to_string() + ")";
  }
};

class whileStmtASTnode : public stmtASTnode {
  std::unique_ptr<exprASTnode> Expression;
  std::unique_ptr<stmtASTnode> Statement;
public:
  whileStmtASTnode(std::unique_ptr<exprASTnode> expr, std::unique_ptr<stmtASTnode> stmt) : Expression(std::move(expr)), Statement(std::move(stmt)), stmtASTnode("while") {}
  virtual llvm::Value *codegen() override;
  virtual std::string to_string() const override {
    if (Statement != nullptr) return "while statement : (" + Expression->to_string() + ") (" + Statement->to_string() + ")";
    else return "while statement : (" + Expression->to_string() + ")";
  }
};

class returnStmtASTnode : public stmtASTnode {
  std::unique_ptr<exprASTnode> Expression;
public:
  returnStmtASTnode(std::unique_ptr<exprASTnode> expr) : Expression(std::move(expr)), stmtASTnode("return") {}
  virtual llvm::Value *codegen() override;
  virtual std::string to_string() const override {
    if (Expression == nullptr) return "return statment";
    else return "return statment : (" + Expression->to_string() + ")";
  }
};


class paramASTnode : public ASTnode {
  TOKEN Tok;
  public : std::string Type;
  std::string Name;
public:
  paramASTnode(TOKEN tok, std::string type, std::string name) : Tok(tok), Type(type), Name(name) {}
  //virtual llvm::Value *codegen() override = 0;
  virtual std::string to_string() const override {
    return "param : " + Type + Name;
  }
};

class paramsASTnode : public ASTnode {
  public : std::string Type;
public:
  paramsASTnode(std::string type) : Type(type) {}
  //virtual llvm::Value *codegen() override = 0;
  virtual std::string to_string() const override {
    return "params : " + Type;
  }
  virtual std::list <std::unique_ptr<paramASTnode>> getList() {
    return {};
  }
};

class paramListASTnode : public paramsASTnode {
  std::list <std::unique_ptr<paramASTnode>> paramaters;
public:
  paramListASTnode(std::list <std::unique_ptr<paramASTnode>> l) : paramaters(std::move(l)), paramsASTnode("list") {}
  //virtual llvm::Value *codegen() override = 0;
  virtual std::string to_string() const override {
    std::string ret = "paramList : (";
    for (auto&& i : paramaters)  ret += i->to_string() + ", ";
    ret += ")";
    return ret;
  }
  virtual std::list <std::unique_ptr<paramASTnode>> getList() override {
    return std::move(paramaters);
  }
};

class voidParamASTnode : public paramsASTnode {
  TOKEN Tok;
public:
  voidParamASTnode(TOKEN tok) : Tok(tok), paramsASTnode("void"){}
  //virtual llvm::Value *codegen() override = 0;
  virtual std::string to_string() const override {
    return "void param";
  }
};

class externASTnode : public ASTnode {
  TOKEN Tok;
  std::string Type;
  std::string Name;
  std::unique_ptr<paramsASTnode> paramaters;
public:
  externASTnode(TOKEN tok, std::string type, std::string name, std::unique_ptr<paramsASTnode> param) : Tok(tok), Type(type), Name(name), paramaters(std::move(param)) {}
  virtual llvm::Value *codegen();
  virtual std::string to_string() const override {
    if (paramaters == nullptr) "extern : " + Type + " " + Name;
    return "extern : " + Type + " " + Name + " " + paramaters->to_string();
  }
};

class funcDeclASTnode : public declASTnode {
  std::unique_ptr<paramsASTnode> Paramaters;
  std::unique_ptr<blockASTnode> Block;
public:
  funcDeclASTnode(TOKEN tok, std::string type, std::string name, std::unique_ptr<paramsASTnode> param, std::unique_ptr<blockASTnode> block) : Paramaters(std::move(param)), Block(std::move(block)), declASTnode(tok, type, name) {}
  virtual llvm::Value *codegen() override;
  virtual std::string to_string() const override {
    if (Paramaters == nullptr) return "function decleration : (" + Block->to_string() + ")";
    else return "function decleration : (" + Paramaters->to_string() + ") (" + Block->to_string() + ")";
  }
};

class ProgramASTnode : public ASTnode {
  std::list <std::unique_ptr<externASTnode>> externals;
  std::list <std::unique_ptr<declASTnode>> declerations;
public:
  ProgramASTnode(std::list <std::unique_ptr<externASTnode>> externs, std::list <std::unique_ptr<declASTnode>> decls) : externals(std::move(externs)), declerations(std::move(decls)) {}
  virtual llvm::Value *codegen();
  virtual std::string to_string() const override {
    std::string ret = "Program : ";
    for (auto&& e : externals)  ret += "(" + e->to_string() + ")";
    for (auto&& d : declerations)  ret += "(" + d->to_string() + ")";
    return ret;
  }
};

#endif
