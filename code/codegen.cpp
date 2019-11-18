#include "ASTnodes.hpp"

static llvm::LLVMContext TheContext;
static llvm::IRBuilder<> Builder(TheContext);
//static map<std::string, AllocaInst*> NamedValues;
static std::map<std::string, llvm::Value*> GlobalNamedValues;
//static std::map<std::string, Scope*> scopes;
static std::unique_ptr<llvm::Module> TheModule;
static Scope* currentScope;
static bool newFunc = false;

llvm::Value *LogErrorV(const char *Str) {
  fprintf(stderr, "Error: %s\n", Str);
  return nullptr;
}

llvm::Type* stringToType(std::string type){
  if (type == "void") return llvm::Type::getVoidTy(TheContext);
  else if (type == "int") return llvm::Type::getInt32Ty(TheContext);
  else if (type == "float") return llvm::Type::getFloatTy(TheContext);
  else if (type == "bool") return llvm::Type::getInt1Ty(TheContext);
  else {
    LogErrorV("no matching type");
    return nullptr;
  }
}

llvm::Type* checkNumType(llvm::Value *l, llvm::Value *r) {
  if (l->getType()->isIntegerTy(32) && r->getType()->isIntegerTy(32)) return llvm::Type::getInt32Ty(TheContext);
  else if (l->getType()->isFloatTy() && r->getType()->isFloatTy()) return llvm::Type::getFloatTy(TheContext);
  else if ((l->getType()->isFloatTy() && r->getType()->isIntegerTy()) || (l->getType()->isIntegerTy() && r->getType()->isFloatTy())) return llvm::Type::getFloatTy(TheContext);
  else if (l->getType()->isIntegerTy(1) && r->getType()->isIntegerTy(1)) return llvm::Type::getInt1Ty(TheContext);
  else if ((l->getType()->isIntegerTy(1) && r->getType()->isIntegerTy(32)) || (l->getType()->isIntegerTy(32) && r->getType()->isIntegerTy(1))) return llvm::Type::getInt32Ty(TheContext);
  else {
    LogErrorV("Expected int or float");
    return nullptr;
  }
}

llvm::Value *applyType(llvm::Value *v, llvm::Type* t){
  if (v->getType() == t) return v;
  else if (v->getType()->isIntegerTy() && t->isFloatTy()) return Builder.CreateSIToFP(v, llvm::Type::getFloatTy(TheContext));
  else if (v->getType()->isIntegerTy(1) && t->isIntegerTy(32)) return Builder.CreateIntCast(v, llvm::Type::getInt32Ty(TheContext), true);
  else if (v->getType()->isFloatTy() && t->isIntegerTy(32)) return Builder.CreateFPToSI(v, llvm::Type::getInt32Ty(TheContext));
  else if (v->getType()->isFloatTy() && t->isIntegerTy(1)) return Builder.CreateFPToSI(v, llvm::Type::getInt1Ty(TheContext));
  else return nullptr;
}

static llvm::AllocaInst *CreateEntryBlockAlloca(llvm::Function *TheFunction, const std::string &VarName, llvm::Type *type) {
  llvm::IRBuilder<> TmpB(&TheFunction->getEntryBlock(), TheFunction->getEntryBlock().begin());
  return TmpB.CreateAlloca(type, nullptr, VarName);
}

void Scope::createNewLocal(std::string name, llvm::Type *type) {
  auto *check = Values[name];
  if (!check) {
    llvm::AllocaInst* out = CreateEntryBlockAlloca(Builder.GetInsertBlock()->getParent(), name, type);
    llvm::Value *val;
    if (type == llvm::Type::getInt32Ty(TheContext)) val = llvm::ConstantInt::get(TheContext, llvm::APInt(32, 0, true));
    else if (type == llvm::Type::getFloatTy(TheContext)) val = llvm::ConstantFP::get(TheContext, llvm::APFloat(0.0));
    else if (type == llvm::Type::getInt1Ty(TheContext)) val = 0 ? llvm::ConstantInt::get(TheContext, llvm::APInt(1, 1)) : llvm::ConstantInt::get(TheContext, llvm::APInt());
    Builder.CreateStore(val, out);
    Values[name] = out;
  }
}

bool Scope::assignLocal(std::string name, llvm::Value *val) {
  llvm::Value *check = Values[name];
  if (check) {
    val = applyType(val, check->getType());
    Builder.CreateStore(val, check);
    //Values[name] = val;
    return true;
  } else if (parentScope != nullptr) return parentScope->assignLocal(name, val);
  else return false;
}

llvm::Value *Scope::getLocal(std::string name) {
  llvm::Value *check = Values[name];
  if (check) {
    return Builder.CreateLoad(check, name.c_str());
  }
  else if (parentScope != nullptr) {
    return parentScope->getLocal(name);
  } else return nullptr;
}

llvm::Value *negValASTnode::codegen() {
  llvm::Value *v = Val->codegen();
  if (TokCount % 2 == 0) return v;
  else {
    llvm::Value *v = Val->codegen();
    if (v->getType()->isIntegerTy() && Tok.type == MINUS) return Builder.CreateNeg(v);
    else if (v->getType()->isFloatTy() && Tok.type == MINUS) return Builder.CreateFNeg(v);
    else if (v->getType()->isIntegerTy() && Tok.type == NOT) return Builder.CreateNot(v);
    else return LogErrorV("Expected int or float");
  }
}

llvm::Value *modValASTnode::codegen() {
  if(right.empty()) return left->codegen();
  else {
    llvm::Value *l = left->codegen();
    for (auto&& i : right) {
      llvm::Value *r = i->codegen();
      llvm::Type* t = checkNumType(l, r);
      l = applyType(l, t);
      r = applyType(l, t);
      if (t->isIntegerTy()) l = Builder.CreateSRem(l, r);
      else if (t->isFloatTy()) l = Builder.CreateFRem(l, r);
      else LogErrorV("Expected int or float");
    }
    return l;
  }
}

llvm::Value *divValASTnode::codegen() {
  if(right.empty()) return left->codegen();
  else {
    llvm::Value *l = left->codegen();
    for (auto&& i : right) {
      llvm::Value *r = i->codegen();
      llvm::Type* t = checkNumType(l, r);
      l = applyType(l, t);
      r = applyType(l, t);
      if (t->isIntegerTy()) l = Builder.CreateSDiv(l, r);
      else if (t->isFloatTy()) l = Builder.CreateFDiv(l, r);
      else LogErrorV("Expected int or float");
    }
    return l;
  }
}

llvm::Value *mulValASTnode::codegen() {
  if(right.empty()) return left->codegen();
  else {
    llvm::Value *l = left->codegen();
    for (auto&& i : right) {
      llvm::Value *r = i->codegen();
      llvm::Type* t = checkNumType(l, r);
      l = applyType(l, t);
      r = applyType(l, t);
      if (t->isIntegerTy()) l = Builder.CreateMul(l, r);
      else if (t->isFloatTy()) l = Builder.CreateFMul(l, r);
      else LogErrorV("Expected int or float");
    }
    return l;
  }
}

llvm::Value *addValASTnode::codegen() {
  if(right.empty()) return left->codegen();
  else {
    llvm::Value *l = left->codegen();
    for (auto&& i : right) {
      llvm::Value *r = i->codegen();
      llvm::Type* t = checkNumType(l, r);
      l = applyType(l, t);
      r = applyType(l, t);
      if (t->isIntegerTy()) l = Builder.CreateAdd(l, r);
      else if (t->isFloatTy()) l = Builder.CreateFAdd(l, r);
      else LogErrorV("Expected int or float");
    }
    return l;
  }
}

llvm::Value *subValASTnode::codegen(){
  if(right.empty()) return left->codegen();
  else {
    llvm::Value *l = left->codegen();
    for (auto&& i : right) {
      llvm::Value *r = i->codegen();
      llvm::Type* t = checkNumType(l, r);
      l = applyType(l, t);
      r = applyType(l, t);
      if (t->isIntegerTy()) l = Builder.CreateSub(l, r);
      else if (t->isFloatTy()) l = Builder.CreateFSub(l, r);
      else LogErrorV("Expected int or float");
    }
    return l;
  }
}

llvm::Value *ineqValASTnode::codegen(){
  if(right.empty()) return left->codegen();
  else {
    llvm::Value *l = left->codegen();
    for (auto&& i : right) {
      llvm::Value *r = std::get<1>(i)->codegen();
      llvm::Type* t = checkNumType(l, r);
      l = applyType(l, t);
      r = applyType(l, t);
      if (std::get<0>(i).type == LT) {
        if (t->isIntegerTy()) l = Builder.CreateICmpSLT(l, r);
        else if (t->isFloatTy()) l = Builder.CreateFCmpULT(l, r);
      } else if (std::get<0>(i).type == GT) {
        if (t->isIntegerTy()) l = Builder.CreateICmpSGT(l, r);
        else if (t->isFloatTy()) l = Builder.CreateFCmpUGT(l, r);
      } else if (std::get<0>(i).type == LE) {
        if (t->isIntegerTy()) l = Builder.CreateICmpSLE(l, r);
        else if (t->isFloatTy()) l = Builder.CreateFCmpULE(l, r);
      } else if (std::get<0>(i).type == GE) {
        if (t->isIntegerTy()) l = Builder.CreateICmpSGE(l, r);
        else if (t->isFloatTy()) l = Builder.CreateFCmpUGE(l, r);
      }
      else LogErrorV("Expected int or float");
    }
    return l;
  }
}

llvm::Value *eqValASTnode::codegen(){
  if(right.empty()) return left->codegen();
  else {
    llvm::Value *l = left->codegen();
    for (auto&& i : right) {
      llvm::Value *r = std::get<1>(i)->codegen();
      llvm::Type* t = checkNumType(l, r);
      l = applyType(l, t);
      r = applyType(l, t);
      if (std::get<0>(i).type == EQ) {
        if (t->isIntegerTy()) l = Builder.CreateICmpEQ(l, r);
        else if (t->isFloatTy()) l = Builder.CreateFCmpOEQ(l, r);
      } else if (std::get<0>(i).type == NE) {
        if (t->isIntegerTy()) l = Builder.CreateICmpNE(l, r);
        else if (t->isFloatTy()) l = Builder.CreateFCmpONE(l, r);
      }
      else LogErrorV("Expected int or float");
    }
    return l;
  }
}

llvm::Value *andValASTnode::codegen(){
  if(right.empty()) return left->codegen();
  else {
    llvm::Value *l = left->codegen();
    for (auto&& i : right) {
      llvm::Value *r = i->codegen();
      if (l->getType()->isFloatTy()) l = applyType(l, llvm::Type::getInt32Ty(TheContext));
      if (r->getType()->isFloatTy()) r = applyType(r, llvm::Type::getInt32Ty(TheContext));
      return Builder.CreateAnd(l, r);
    }
    return l;
  }
}

llvm::Value *orValASTnode::codegen(){
  if(right.empty()) return left->codegen();
  else {
    llvm::Value *l = left->codegen();
    for (auto&& i : right) {
      llvm::Value *r = i->codegen();
      if (l->getType()->isFloatTy()) l = applyType(l, llvm::Type::getInt32Ty(TheContext));
      if (r->getType()->isFloatTy()) r = applyType(r, llvm::Type::getInt32Ty(TheContext));
      return Builder.CreateOr(l, r);
    }
    return l;
  }
}

llvm::Value *varDeclASTnode::codegen(){
  if (Global) {
    llvm::Value *val;
    llvm::Type *t = stringToType(Type);
    if (t == llvm::Type::getInt32Ty(TheContext)) val = llvm::ConstantInt::get(TheContext, llvm::APInt(32, 0, true));
    else if (t == llvm::Type::getFloatTy(TheContext)) val = llvm::ConstantFP::get(TheContext, llvm::APFloat(0.0));
    else if (t == llvm::Type::getInt1Ty(TheContext)) val = 0 ? llvm::ConstantInt::get(TheContext, llvm::APInt(1, 1)) : llvm::ConstantInt::get(TheContext, llvm::APInt());
    GlobalNamedValues[Name] = val;
  }
  else currentScope->createNewLocal(Name, stringToType(Type));
  return nullptr;
}

llvm::Value *blockASTnode::codegen(){
  llvm::BasicBlock *b = llvm::BasicBlock::Create(TheContext);
  Scope *s = currentScope;
  if (!newFunc) {
    Scope *n = new Scope(s);
    currentScope = n;
  }
  Builder.SetInsertPoint(b);
  for (auto&& i : localDecls) {
    i->codegen();
  }

  for (auto&& j : statements) {
    auto *cur = j->codegen();
    //b->getInstList().push_back(cur);
  }
  currentScope = s;
  b = Builder.GetInsertBlock();
  return b;
  //return Builder.CreateBr(b);
}

llvm::Value *IntASTnode::codegen(){
  return llvm::ConstantInt::get(TheContext, llvm::APInt(32, Val, true));
}

llvm::Value *FloatASTnode::codegen(){
  return llvm::ConstantFP::get(TheContext, llvm::APFloat(Val));
}

llvm::Value *BoolASTnode::codegen(){
  return Val ? llvm::ConstantInt::get(TheContext, llvm::APInt(1, 1)) : llvm::ConstantInt::get(TheContext, llvm::APInt());
}

llvm::Value *IdentASTnode::codegen(){
  llvm::Value *V = currentScope->getLocal(Name);
  if (!V) V = GlobalNamedValues[Name];
  if (!V) return LogErrorV("Unknown variable name");
  else return V;
}

llvm::Value *exprASTnode::codegen(){
  llvm::Value *E = Vals->codegen();
  for (auto&& i : Names) {
    bool suc = currentScope->assignLocal(i, E);
    if (!suc) {
      llvm::Value *V = GlobalNamedValues[i];
      if (V) GlobalNamedValues[i] = E;
      else LogErrorV("Unknown variable name");
    }
  }
  return E;
}

llvm::Value *subExprASTnode::codegen(){
  return expression->codegen();
}

llvm::Value *funcCallASTnode::codegen(){
  llvm::Function *Called = TheModule->getFunction(Name);
  if (!Called) return LogErrorV("Unknown function name");
  if (Called->arg_size() != arguments.size()) return LogErrorV("wrong number of arguments");
  std::vector<llvm::Value *> args;
  for (auto&& i : arguments) {
    args.push_back(i->codegen());
  }
  auto *ret = Builder.CreateCall(Called, args);
  return ret;
}

llvm::Value *ifStmtASTnode::codegen(){
  llvm::Value *expr = Expression->codegen();
  expr = Builder.CreateICmpNE(expr, llvm::ConstantInt::get(TheContext, llvm::APInt(1, 0)), "ifcond");
  llvm::Function *TheFunction = Builder.GetInsertBlock()->getParent();
  llvm::BasicBlock *ibb = llvm::BasicBlock::Create(TheContext, "if", TheFunction);
  llvm::BasicBlock *ebb = llvm::BasicBlock::Create(TheContext, "else");
  llvm::BasicBlock *cbb = llvm::BasicBlock::Create(TheContext, "cont");
  Builder.CreateCondBr(expr, ibb, ebb);
  Builder.SetInsertPoint(ibb);
  llvm::Value *b = Block->codegen();
  Builder.CreateBr(cbb);
  ibb = Builder.GetInsertBlock();
  llvm::Value *ElV;
  if (Els != nullptr) ElV = Els->codegen();
  Builder.CreateBr(cbb);
  ebb = Builder.GetInsertBlock();

  TheFunction->getBasicBlockList().push_back(cbb);
  Builder.SetInsertPoint(cbb);
  //PHINode *PN = Builder.CreatePHI(Type::getDoubleTy(TheContext), 2, "iftmp");

  //PN->addIncoming(ThenV, ThenBB);
  //PN->addIncoming(ElseV, ElseBB);
  //return PN;
  return nullptr;
}

llvm::Value *whileStmtASTnode::codegen(){
  llvm::Function *TheFunction = Builder.GetInsertBlock()->getParent();
  llvm::Value *e = Expression->codegen();
  e = Builder.CreateICmpNE(e, llvm::ConstantInt::get(TheContext, llvm::APInt(1, 0)), "whilecond");
  llvm::BasicBlock *first = llvm::BasicBlock::Create(TheContext, "whileFirst", TheFunction);
  llvm::BasicBlock *code = llvm::BasicBlock::Create(TheContext, "whileLoop");
  llvm::BasicBlock *end = llvm::BasicBlock::Create(TheContext, "whileEnd");
  Builder.SetInsertPoint(first);
  auto *l = Builder.CreateCondBr(e, code, end);
  first = Builder.GetInsertBlock();

  code = Builder.GetInsertBlock();
  llvm::Value *wl = Statement->codegen();
  Builder.CreateBr(first);

  TheFunction->getBasicBlockList().push_back(end);
  Builder.SetInsertPoint(end);


  return nullptr;
}

llvm::Value *returnStmtASTnode::codegen(){
  if (Expression == nullptr) return Builder.CreateRetVoid();
  else {
    llvm::Value *r = Expression->codegen();
    return Builder.CreateRet(r);
  }
}

llvm::Value *externASTnode::codegen(){
  std::vector<llvm::Type *> argTypes = {};
  std::vector<std::string> names = {};
  llvm::FunctionType *type;
  if (paramaters->Type == "list") {
    for (auto &par : paramaters->getList()){
      argTypes.push_back(stringToType(par->Type));
      names.push_back(par->Name);
    }
  }
  llvm::FunctionType::get(stringToType(Type), argTypes, false);
  llvm::Function *f = llvm::Function::Create(type, llvm::Function::ExternalLinkage, Name, TheModule.get());
  unsigned Idx = 0;
  for (auto &arg : f->args()) arg.setName(names[Idx++]);
  return f;
}

llvm::Value *funcDeclASTnode::codegen(){
  llvm::Function *f = TheModule->getFunction(Name);
  if (f) return LogErrorV("function already exsists");

  std::vector<llvm::Type *> argTypes = {};
  std::vector<std::string> names = {};
  llvm::FunctionType *type;
  if (Paramaters->Type == "list") {
    for (auto &par : Paramaters->getList()){
      argTypes.push_back(stringToType(par->Type));
      names.push_back(par->Name);
    }
  }
  llvm::FunctionType::get(stringToType(Type), argTypes, false);
  llvm::Function *nf = llvm::Function::Create(type, llvm::Function::ExternalLinkage, Name, TheModule.get());
  nf = TheModule->getFunction(Name);
  unsigned Idx = 0;
  for (auto &arg : nf->args()) arg.setName(names[Idx++]);

  llvm::BasicBlock *b = llvm::BasicBlock::Create(TheContext, "funcBlock", nf);
  Builder.SetInsertPoint(b);
  Scope *n = new Scope(nullptr);
  currentScope = n;
  newFunc = true;
  for (auto &arg: nf->args()){
    currentScope->createNewLocal(arg.getName(), arg.getType());
  }
  if (llvm::Value *ret = Block->codegen()) {
    Builder.CreateRet(ret);
    llvm::verifyFunction(*nf);
    return nf;
  } else {
    nf->eraseFromParent();
    return nullptr;
  }

}

llvm::Value *ProgramASTnode::codegen(){
  for (auto&& e : externals) e->codegen();
  for (auto&& d : declerations) d->codegen();
  return nullptr;
}
