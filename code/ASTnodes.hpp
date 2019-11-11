#ifndef ASTNODES_HPP
#define ASTNODES_HPP



#include <string.h>
#include <string>
#include <list>
#include <iostream>

#include "token.hpp"

//===----------------------------------------------------------------------===//
// AST nodes
//===----------------------------------------------------------------------===//

/// ASTnode - Base class for all AST nodes.
class ASTnode {
public:
  virtual ~ASTnode() {}
  //virtual Value *codegen() = 0;
  virtual std::string to_string() const {};
};







class stmtASTnode : public ASTnode {
public:
  stmtASTnode() {}
  //virtual Value *codegen() override;
};

class baseValASTnode : public ASTnode {
  std::string type;
public:
  baseValASTnode() {}
  //virtual Value *codegen() override;
};

class negValASTnode : public ASTnode {
  TOKEN tok;
  int tokCount;
  std::unique_ptr<baseValASTnode> val;
public:
  negValASTnode() {}
  //virtual Value *codegen() override;
};

class mulValASTnode : public ASTnode {
  std::unique_ptr<negValASTnode> left;
  std::list<std::tuple<TOKEN, std::unique_ptr<negValASTnode>>> right;
public:
  mulValASTnode() {}
  //virtual Value *codegen() override;
};

class addValASTnode : public ASTnode {
  std::unique_ptr<mulValASTnode> left;
  std::list<std::tuple<TOKEN, std::unique_ptr<mulValASTnode>>> right;
public:
  addValASTnode() {}
  //virtual Value *codegen() override;
};

class ineqValASTnode : public ASTnode {
  std::unique_ptr<addValASTnode> left;
  std::list<std::tuple<TOKEN, std::unique_ptr<addValASTnode>>> right;
public:
  ineqValASTnode() {}
  //virtual Value *codegen() override;
};

class eqValASTnode : public ASTnode {
  std::unique_ptr<ineqValASTnode> left;
  std::list<std::tuple<TOKEN, std::unique_ptr<ineqValASTnode>>> right;
public:
  eqValASTnode() {}
  //virtual Value *codegen() override;
};

class andValASTnode : public ASTnode {
  std::unique_ptr<eqValASTnode> left;
  std::list<std::unique_ptr<eqValASTnode>> right;
public:
  andValASTnode() {}
  //virtual Value *codegen() override;
};

class orValASTnode : public ASTnode {
  std::unique_ptr<andValASTnode> left;
  std::list<std::unique_ptr<andValASTnode>> right;
public:
  orValASTnode() {}
  //virtual Value *codegen() override;
};

class declASTnode : public ASTnode {
  TOKEN Tok;
  std::string Type;
  std::string Name;
public:
  declASTnode(TOKEN tok, std::string type, std::string name) : Tok(tok), Type(type), Name(name){}
  //virtual Value *codegen() override;
};

class varDeclASTnode : public declASTnode {
public:
  varDeclASTnode(TOKEN tok, std::string type, std::string name) : declASTnode(tok, type, name) {}
  //virtual Value *codegen() override;
};

class blockASTnode : public stmtASTnode {
  std::list<std::unique_ptr<varDeclASTnode>> localDecls;
  std::list<std::unique_ptr<stmtASTnode>> statments;
public:
  blockASTnode(std::list<std::unique_ptr<varDeclASTnode>> locals, std::list<std::unique_ptr<stmtASTnode>> stmts) : localDecls(std::move(locals)), statments(std::move(stmts)) {}
  //virtual Value *codegen() override;
};













/// IntASTnode - Class for integer literals like 1, 2, 10,
class IntASTnode : public baseValASTnode {
  int Val;
  TOKEN Tok;
  std::string Name;

public:
  IntASTnode(TOKEN tok, int val) : Val(val), Tok(tok) {}
  //virtual Value *codegen() override;
  // virtual std::string to_string() const override {
  // return a sting representation of this AST node
  //};
};

class FloatASTnode : public baseValASTnode {
  float Val;
  TOKEN Tok;
  std::string Name;

public:
  FloatASTnode(TOKEN tok, float val) : Val(val), Tok(tok) {}
  //virtual Value *codegen() override;
};

class BoolASTnode : public baseValASTnode {
  bool Val;
  TOKEN Tok;
  std::string Name;
public:
  BoolASTnode(TOKEN tok, bool val) : Val(val), Tok(tok) {}
  //virtual Value *codegen() override;
};



class exprASTnode : public stmtASTnode {
  std::list<std::string> names;
  std::unique_ptr<orValASTnode> val;
public:
  exprASTnode() {}
  //virtual Value *codegen() override;
};

class subExprASTnode : baseValASTnode {
  std::unique_ptr<exprASTnode> expresion;
public:
  subExprASTnode() {}
  //virtual Value *codegen() override;
};

class funcCallASTnode : baseValASTnode {
  std::string name;
  std::list<std::unique_ptr<exprASTnode>> arguments;
public:
  funcCallASTnode() {}
  //virtual Value *codegen() override;
};

class ifStmtASTnode : public stmtASTnode {
  std::unique_ptr<exprASTnode> expresion;
  std::unique_ptr<blockASTnode> block;
  std::unique_ptr<blockASTnode> els;
public:
  ifStmtASTnode() {}
  //virtual Value *codegen() override;
};

class whileStmtASTnode : public stmtASTnode {
  std::unique_ptr<exprASTnode> expresion;
  std::unique_ptr<stmtASTnode> statment;
public:
  whileStmtASTnode() {}
  //virtual Value *codegen() override;
};

class returnStmtASTnode : public stmtASTnode {
  std::unique_ptr<exprASTnode> expresion;
public:
  returnStmtASTnode() {}
  //virtual Value *codegen() override;
};


class paramASTnode : public ASTnode {
  TOKEN Tok;
  std::string Type;
  std::string Name;
public:
  paramASTnode(TOKEN tok, std::string type, std::string name) : Tok(tok), Type(type), Name(name) {}
  //virtual Value *codegen() override;
};

class paramsASTnode : public ASTnode {
  std::string type;
public:
  paramsASTnode() {}
  //virtual Value *codegen() override;
};

class paramListASTnode : public paramsASTnode {
  std::list <std::unique_ptr<paramASTnode>> paramaters;
public:
  paramListASTnode(std::list <std::unique_ptr<paramASTnode>> l) : paramaters(std::move(l)) {}
  //virtual Value *codegen() override;
};

class voidParamASTnode : public paramsASTnode {
  TOKEN Tok;
public:
  voidParamASTnode(TOKEN tok) : Tok(tok) {}
  //virtual Value *codegen() override;
};

class externASTnode : public ASTnode {
  TOKEN Tok;
  std::string Type;
  std::string Name;
  std::unique_ptr<paramsASTnode> paramaters;
public:
  externASTnode(TOKEN tok, std::string type, std::string name, std::unique_ptr<paramsASTnode> param) : Tok(tok), Type(type), Name(name), paramaters(std::move(param)) {}
  //virtual Value *codegen() override;
};

class funcDeclASTnode : public declASTnode {
  std::unique_ptr<paramsASTnode> Paramaters;
  std::unique_ptr<blockASTnode> Block;
public:
  funcDeclASTnode(TOKEN tok, std::string type, std::string name, std::unique_ptr<paramsASTnode> param, std::unique_ptr<blockASTnode> block) : Paramaters(std::move(param)), Block(std::move(block)), declASTnode(tok, type, name) {}
  //virtual Value *codegen() override;
};

class ProgramASTnode : public ASTnode {
  std::list <std::unique_ptr<externASTnode>> externals;
  std::list <std::unique_ptr<declASTnode>> declerations;
public:
  ProgramASTnode(std::list <std::unique_ptr<externASTnode>> externs, std::list <std::unique_ptr<declASTnode>> decls) : externals(std::move(externs)), declerations(std::move(decls)) {}
  //virtual Value *codegen() override;
};

#endif
