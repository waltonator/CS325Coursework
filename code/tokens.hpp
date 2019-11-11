#ifndef TOKENS_HPP
#define TOKENS_HPP

#include <memory>
#include <queue>

#include "ASTnodes.hpp"

class tokens {
public:
FILE *pFile;

std::string IdentifierStr; // Filled in if IDENT
int IntVal;                // Filled in if INT_LIT
bool BoolVal;              // Filled in if BOOL_LIT
float FloatVal;            // Filled in if FLOAT_LIT
std::string StringVal;     // Filled in if String Literal
int lineNo, columnNo;


TOKEN CurTok;
std::deque<TOKEN> tok_buffer;

tokens() {}
TOKEN returnTok(std::string lexVal, int tok_type);
TOKEN gettok();

TOKEN getNextToken();

void putBackToken(TOKEN tok);
};

#endif
