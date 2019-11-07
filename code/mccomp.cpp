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
#include <iostream>
#include <sstream>

#include "parser.hpp"

using namespace llvm;
using namespace llvm::sys;

FILE *pFile;

//===----------------------------------------------------------------------===//
// Lexer
//===----------------------------------------------------------------------===//

// The lexer returns one of these for known things.
enum TOKEN_TYPE {

  IDENT = -1,        // [a-zA-Z_][a-zA-Z_0-9]*
  ASSIGN = int('='), // '='

  // delimiters
  LBRA = int('{'),  // left brace
  RBRA = int('}'),  // right brace
  LPAR = int('('),  // left parenthesis
  RPAR = int(')'),  // right parenthesis
  SC = int(';'),    // semicolon
  COMMA = int(','), // comma

  // types
  INT_TOK = -2,   // "int"
  VOID_TOK = -3,  // "void"
  FLOAT_TOK = -4, // "float"
  BOOL_TOK = -5,  // "bool"

  // keywords
  EXTERN = -6,  // "extern"
  IF = -7,      // "if"
  ELSE = -8,    // "else"
  WHILE = -9,   // "while"
  RETURN = -10, // "return"
  // TRUE   = -12,     // "true"
  // FALSE   = -13,     // "false"

  // literals
  INT_LIT = -14,   // [0-9]+
  FLOAT_LIT = -15, // [0-9]+.[0-9]+
  BOOL_LIT = -16,  // "true" or "false" key words

  // logical operators
  AND = -17, // "&&"
  OR = -18,  // "||"

  // operators
  PLUS = int('+'),    // addition or unary plus
  MINUS = int('-'),   // substraction or unary negative
  ASTERIX = int('*'), // multiplication
  DIV = int('/'),     // division
  MOD = int('%'),     // modular
  NOT = int('!'),     // unary negation

  // comparison operators
  EQ = -19,      // equal
  NE = -20,      // not equal
  LE = -21,      // less than or equal to
  LT = int('<'), // less than
  GE = -23,      // greater than or equal to
  GT = int('>'), // greater than

  // special tokens
  EOF_TOK = 0, // signal end of file

  // invalid
  INVALID = -100 // signal invalid token
};

// TOKEN struct is used to keep track of information about a token
struct TOKEN {
  int type = -100;
  std::string lexeme;
  int lineNo;
  int columnNo;
};

static std::string IdentifierStr; // Filled in if IDENT
static int IntVal;                // Filled in if INT_LIT
static bool BoolVal;              // Filled in if BOOL_LIT
static float FloatVal;            // Filled in if FLOAT_LIT
static std::string StringVal;     // Filled in if String Literal
static int lineNo, columnNo;

static TOKEN returnTok(std::string lexVal, int tok_type) {
  TOKEN return_tok;
  return_tok.lexeme = lexVal;
  return_tok.type = tok_type;
  return_tok.lineNo = lineNo;
  return_tok.columnNo = columnNo - lexVal.length() - 1;
  return return_tok;
}

// Read file line by line -- or look for \n and if found add 1 to line number
// and reset column number to 0
/// gettok - Return the next token from standard input.
static TOKEN gettok() {

  static int LastChar = ' ';
  static int NextChar = ' ';

  // Skip any whitespace.
  while (isspace(LastChar)) {
    if (LastChar == '\n' || LastChar == '\r') {
      lineNo++;
      columnNo = 1;
    }
    LastChar = getc(pFile);
    columnNo++;
  }

  if (isalpha(LastChar) ||
      (LastChar == '_')) { // identifier: [a-zA-Z_][a-zA-Z_0-9]*
    IdentifierStr = LastChar;
    columnNo++;

    while (isalnum((LastChar = getc(pFile))) || (LastChar == '_')) {
      IdentifierStr += LastChar;
      columnNo++;
    }

    if (IdentifierStr == "int")
      return returnTok("int", INT_TOK);
    if (IdentifierStr == "bool")
      return returnTok("bool", BOOL_TOK);
    if (IdentifierStr == "float")
      return returnTok("float", FLOAT_TOK);
    if (IdentifierStr == "void")
      return returnTok("void", VOID_TOK);
    if (IdentifierStr == "bool")
      return returnTok("bool", BOOL_TOK);
    if (IdentifierStr == "extern")
      return returnTok("extern", EXTERN);
    if (IdentifierStr == "if")
      return returnTok("if", IF);
    if (IdentifierStr == "else")
      return returnTok("else", ELSE);
    if (IdentifierStr == "while")
      return returnTok("while", WHILE);
    if (IdentifierStr == "return")
      return returnTok("return", RETURN);
    if (IdentifierStr == "true") {
      BoolVal = true;
      return returnTok("true", BOOL_LIT);
    }
    if (IdentifierStr == "false") {
      BoolVal = false;
      return returnTok("false", BOOL_LIT);
    }

    return returnTok(IdentifierStr.c_str(), IDENT);
  }

  if (LastChar == '=') {
    NextChar = getc(pFile);
    if (NextChar == '=') { // EQ: ==
      LastChar = getc(pFile);
      columnNo += 2;
      return returnTok("==", EQ);
    } else {
      LastChar = NextChar;
      columnNo++;
      return returnTok("=", ASSIGN);
    }
  }

  if (LastChar == '{') {
    LastChar = getc(pFile);
    columnNo++;
    return returnTok("{", LBRA);
  }
  if (LastChar == '}') {
    LastChar = getc(pFile);
    columnNo++;
    return returnTok("}", RBRA);
  }
  if (LastChar == '(') {
    LastChar = getc(pFile);
    columnNo++;
    return returnTok("(", LPAR);
  }
  if (LastChar == ')') {
    LastChar = getc(pFile);
    columnNo++;
    return returnTok(")", RPAR);
  }
  if (LastChar == ';') {
    LastChar = getc(pFile);
    columnNo++;
    return returnTok(";", SC);
  }
  if (LastChar == ',') {
    LastChar = getc(pFile);
    columnNo++;
    return returnTok(",", COMMA);
  }

  if (isdigit(LastChar) || LastChar == '.') { // Number: [0-9]+.
    std::string NumStr;

    if (LastChar == '.') { // Floatingpoint Number: .[0-9]+
      do {
        NumStr += LastChar;
        LastChar = getc(pFile);
        columnNo++;
      } while (isdigit(LastChar));

      FloatVal = strtof(NumStr.c_str(), nullptr);
      return returnTok(NumStr, FLOAT_LIT);
    } else {
      do { // Start of Number: [0-9]+
        NumStr += LastChar;
        LastChar = getc(pFile);
        columnNo++;
      } while (isdigit(LastChar));

      if (LastChar == '.') { // Floatingpoint Number: [0-9]+.[0-9]+)
        do {
          NumStr += LastChar;
          LastChar = getc(pFile);
          columnNo++;
        } while (isdigit(LastChar));

        FloatVal = strtof(NumStr.c_str(), nullptr);
        return returnTok(NumStr, FLOAT_LIT);
      } else { // Integer : [0-9]+
        IntVal = strtod(NumStr.c_str(), nullptr);
        return returnTok(NumStr, INT_LIT);
      }
    }
  }

  if (LastChar == '&') {
    NextChar = getc(pFile);
    if (NextChar == '&') { // AND: &&
      LastChar = getc(pFile);
      columnNo += 2;
      return returnTok("&&", AND);
    } else {
      LastChar = NextChar;
      columnNo++;
      return returnTok("&", int('&'));
    }
  }

  if (LastChar == '|') {
    NextChar = getc(pFile);
    if (NextChar == '|') { // OR: ||
      LastChar = getc(pFile);
      columnNo += 2;
      return returnTok("||", OR);
    } else {
      LastChar = NextChar;
      columnNo++;
      return returnTok("|", int('|'));
    }
  }

  if (LastChar == '!') {
    NextChar = getc(pFile);
    if (NextChar == '=') { // NE: !=
      LastChar = getc(pFile);
      columnNo += 2;
      return returnTok("!=", NE);
    } else {
      LastChar = NextChar;
      columnNo++;
      return returnTok("!", NOT);
      ;
    }
  }

  if (LastChar == '<') {
    NextChar = getc(pFile);
    if (NextChar == '=') { // LE: <=
      LastChar = getc(pFile);
      columnNo += 2;
      return returnTok("<=", LE);
    } else {
      LastChar = NextChar;
      columnNo++;
      return returnTok("<", LT);
    }
  }

  if (LastChar == '>') {
    NextChar = getc(pFile);
    if (NextChar == '=') { // GE: >=
      LastChar = getc(pFile);
      columnNo += 2;
      return returnTok(">=", GE);
    } else {
      LastChar = NextChar;
      columnNo++;
      return returnTok(">", GT);
    }
  }

  if (LastChar == '/') { // could be division or could be the start of a comment
    LastChar = getc(pFile);
    columnNo++;
    if (LastChar == '/') { // definitely a comment
      do {
        LastChar = getc(pFile);
        columnNo++;
      } while (LastChar != EOF && LastChar != '\n' && LastChar != '\r');

      if (LastChar != EOF)
        return gettok();
    } else
      return returnTok("/", DIV);
  }

  // Check for end of file.  Don't eat the EOF.
  if (LastChar == EOF) {
    columnNo++;
    return returnTok("0", EOF_TOK);
  }

  // Otherwise, just return the character as its ascii value.
  int ThisChar = LastChar;
  std::string s(1, ThisChar);
  LastChar = getc(pFile);
  columnNo++;
  return returnTok(s, int(ThisChar));
}

//===----------------------------------------------------------------------===//
// Parser
//===----------------------------------------------------------------------===//

/// CurTok/getNextToken - Provide a simple token buffer.  CurTok is the current
/// token the parser is looking at.  getNextToken reads another token from the
/// lexer and updates CurTok with its results.
static TOKEN CurTok;
static std::deque<TOKEN> tok_buffer;

static TOKEN getNextToken() {

  if (tok_buffer.size() == 0)
    tok_buffer.push_back(gettok());

  TOKEN temp = tok_buffer.front();
  tok_buffer.pop_front();

  return CurTok = temp;
}

static void putBackToken(TOKEN tok) { tok_buffer.push_front(tok); }

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
public:
  declASTnode() {}
  //virtual Value *codegen() override;
};

class varDeclASTnode : public declASTnode {
  TOKEN Tok;
  std::string type;
  std::string Name;
public:
  varDeclASTnode() {}
  //virtual Value *codegen() override;
};

class blockASTnode : public stmtASTnode {
  std::list<std::unique_ptr<varDeclASTnode>> localDecls;
  std::list<std::unique_ptr<stmtASTnode>> statments;
public:
  blockASTnode() {}
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
  TOKEN Tok;
  std::string type;
  std::string name;
  std::unique_ptr<paramsASTnode> paramaters;
  std::unique_ptr<blockASTnode> block;
public:
  funcDeclASTnode() {}
  //virtual Value *codegen() override;
};

class ProgramASTnode : public ASTnode {
  std::list <std::unique_ptr<externASTnode>> externals;
  std::list <std::unique_ptr<declASTnode>> declerations;
public:
  ProgramASTnode(std::list <std::unique_ptr<externASTnode>> externs, std::list <std::unique_ptr<declASTnode>> decls) : externals(std::move(externs)), declerations(std::move(decls)) {}
  //virtual Value *codegen() override;
};
//===----------------------------------------------------------------------===//
// Recursive Descent Parser - Function call for each production
//===----------------------------------------------------------------------===//

/* Add function calls for each production */

static void exceptionString (TOKEN tok, std::list<std::string> expected) {
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
  std::string c;std::list <std::unique_ptr<externASTnode>> externs = {};
  ss << tok.columnNo;
  c = ss.str();
  ret = "found " + tok.lexeme + " at line " + l + ", at column " + c + " when parser expected " + ret;
  std::cerr << ret;
  exit(1);
}



static bool isVarType(){
  return (CurTok.type == INT_TOK || CurTok.type == FLOAT_TOK || CurTok.type == BOOL_TOK);
}

static std::unique_ptr<IntASTnode> parseInt() {
  std::stringstream val(CurTok.lexeme);
  int ival = 0;
  val >> ival;
  return make_unique<IntASTnode>(CurTok, ival);
}

static std::unique_ptr<FloatASTnode> parseFloat() {
  std::stringstream val(CurTok.lexeme);
  float ival = 0.0;
  val >> ival;
  return make_unique<FloatASTnode>(CurTok, ival);
}

static std::unique_ptr<BoolASTnode> parseBool() {
  std::stringstream val(CurTok.lexeme);
  bool ival = false;
  val >> ival;
  return make_unique<BoolASTnode>(CurTok, ival);
}




static std::unique_ptr<paramASTnode> parseParam() {
  std::string type = CurTok.lexeme;
  TOKEN tok = getNextToken();
  return make_unique<paramASTnode>(tok, type, tok.lexeme);
}

static std::unique_ptr<externASTnode> parseExtern(){
  std::string type = getNextToken().lexeme;
  if (CurTok.type != VOID_TOK && !(isVarType())){
    exceptionString(CurTok, {"int","float","bool","void"});
  }
  TOKEN tok = getNextToken();
  std::string name = tok.lexeme;
  if (tok.type != IDENT){
    exceptionString(CurTok, {"an identifier"});
  }
  if (getNextToken().type != LPAR) {
    exceptionString(CurTok, {"("});
  }
  std::unique_ptr<paramsASTnode> p;
  if (getNextToken().type != RPAR){
    if (CurTok.type == VOID_TOK) {
      p = make_unique<voidParamASTnode>(CurTok);
    } else if (isVarType()) {
      //create param list
      std::list <std::unique_ptr<paramASTnode>> params = {};
      while (isVarType()) {
        params.push_back(parseParam());
        getNextToken();
        if (CurTok.type != COMMA && CurTok.type != RPAR){
          exceptionString(CurTok, {",", ")"});
        }
        getNextToken();
      }
      p = make_unique<paramListASTnode>(std::move(params));
    } else {
      exceptionString(CurTok, {"int","float","bool","void", ")"});
    }
  }
  if (CurTok.type != SC) {
    exceptionString(CurTok, {";"});
  }
  return make_unique<externASTnode>(tok, type, name, std::move(p));
}


static std::unique_ptr<ProgramASTnode> prog(){
  if (CurTok.type != EXTERN && !(isVarType()) && CurTok.type != VOID_TOK){
    exceptionString(CurTok, {"extern","int","float","bool","void"});
  }
  std::list <std::unique_ptr<externASTnode>> externs = {};
  while (CurTok.type == EXTERN) {
    externs.push_front(parseExtern());
    getNextToken();
  }
  std::list <std::unique_ptr<declASTnode>> decls = {};

  return make_unique<ProgramASTnode>(std::move(externs), std::move(decls));
}

// program ::= extern_list decl_list
static void parser() {
  //fseek(pFile, 0, SEEK_SET);
  std::unique_ptr<ProgramASTnode> program = prog();
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
  if (argc == 2) {
    pFile = fopen(argv[1], "r");
    if (pFile == NULL)
      perror("Error opening file");
  } else {
    std::cout << "Usage: ./code InputFile\n";
    return 1;
  }

  // initialize line number and column numbers to zero
  lineNo = 1;
  columnNo = 1;

  // get the first token
  getNextToken();
  //while (CurTok.type != EOF_TOK) {
    //fprintf(stderr, "Token: %s with type %d\n", CurTok.lexeme.c_str(),
      //      CurTok.type);
    //getNextToken();
  //}
  //fprintf(stderr, "Lexer Finished\n");

  // Make the module, which holds all the code.
  TheModule = llvm::make_unique<Module>("mini-c", TheContext);

  // Run the parser now.
  parser();
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

  fclose(pFile); // close the file that contains the code that was parsed
  return 0;
}
