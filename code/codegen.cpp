#include "ASTnodes.hpp"

llvm::LLVMContext TheContext;
llvm::IRBuilder<> Builder(TheContext);
std::unique_ptr<llvm::Module> TheModule = llvm::make_unique<llvm::Module>("mini-c", TheContext);
// static llvm::LLVMContext TheContext;
// static llvm::IRBuilder<> Builder(TheContext);
// static std::unique_ptr<llvm::Module> TheModule = llvm::make_unique<llvm::Module>("mini-c", TheContext);
std::map<std::string, llvm::Value*> GlobalNamedValues;
std::list<std::string> funcNames = {};

static Scope* currentScope;

static bool newFunc = false;

bool funcExsists(std::string name){
  for (std::string i : funcNames) if (name == i) return true;
  return false;
}

llvm::Value *LogErrorV(const char *Str) {
  fprintf(stderr, "Error: %s\n", Str);
  exit(1);
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

llvm::AllocaInst *CreateEntryBlockAlloca(llvm::Function *TheFunction, const std::string &VarName, llvm::Type* t) {
  if (TheFunction == nullptr) {
    return Builder.CreateAlloca(t, 0, VarName.c_str());
  }
  llvm::IRBuilder<> TmpB = llvm::IRBuilder<>(&TheFunction->getEntryBlock(), TheFunction->getEntryBlock().begin());
  return TmpB.CreateAlloca(t, 0, VarName.c_str());
}


llvm::Value *applyType(llvm::Value *v, llvm::Type* t){
  llvm::Type *vt = v->getType();
  if (vt == t) return v;
  else if (vt->isIntegerTy() && t->isFloatTy()) return Builder.CreateSIToFP(v, t);
  else if (vt->isIntegerTy(1) && t->isIntegerTy(32)) return Builder.CreateIntCast(v, t, true);
  else if (vt->isFloatTy() && t->isIntegerTy(32)) return Builder.CreateFPToSI(v, t);
  else if (vt->isIntegerTy(32) && t->isIntegerTy(1)) return Builder.CreateIntCast(v, t, true);
  else if (vt->isFloatTy() && t->isIntegerTy(1)) return Builder.CreateFPToSI(v, t);
  else return LogErrorV("type cast didn't work");
}

void Scope::createNewLocal(llvm::Function *TheFunction, const std::string &name, llvm::Type *type) {
  if (Values.find(name) == Values.end()) {
    //llvm::IRBuilder<> TmpB(&TheFunction->getEntryBlock(),TheFunction->getEntryBlock().begin());
    llvm::AllocaInst *out = CreateEntryBlockAlloca(TheFunction, name, type); //TmpB.CreateAlloca(type, nullptr, name);
    llvm::Value *val;
    //LogErrorV("test");
    std::cout<<name<<" storing new "<<out<<std::endl;
    if (type == llvm::Type::getInt32Ty(TheContext)) val = llvm::ConstantInt::get(TheContext, llvm::APInt(32, 0, true));
    else if (type == llvm::Type::getFloatTy(TheContext)) val = llvm::ConstantFP::get(TheContext, llvm::APFloat(0.0));
    else if (type == llvm::Type::getInt1Ty(TheContext)) val = 0 ? llvm::ConstantInt::get(TheContext, llvm::APInt(1, 1)) : llvm::ConstantInt::get(TheContext, llvm::APInt());
    Builder.CreateStore(val, out);
    Values[name] = out;
  } else
  {
    LogErrorV("Cannot redefine variables");
  }
}

bool Scope::assignLocal(std::string name, llvm::Value *val) {
  if (Values.find(name) != Values.end()) {
    //AllocaInst *Alloca = CreateEntryBlockAlloca(nullptr, VarName);
    llvm::Value *check = Values[name];
    val = applyType(val, check->getType()->getPointerElementType());
    std::cout<<name<<" store "<<val<<" in "<<check<<std::endl;
    Builder.CreateStore(val, check);
    //Values[name] = val;
    std::cout<<" store suc"<<std::endl;
    return true;
  } else if (parentScope != nullptr) return parentScope->assignLocal(name, val);
  else return false;
}

llvm::Value *Scope::getLocal(std::string name) {
  std::cout<<"values is size "<<Values.size()<<std::endl;
  std::cout<<name<<" "<<Values[name]<<std::endl;
  llvm::Value *check = Values[name];
  if (check) {
    llvm::Value *check = Values[name];
    std::cout<<name<<" get "<<check<<std::endl;
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
      r = applyType(r, t);
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
      r = applyType(r, t);
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
      r = applyType(r, t);
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
      r = applyType(r, t);
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
      r = applyType(r, t);
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
      r = applyType(r, t);
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
      r = applyType(r, t);
      if (std::get<0>(i).type == EQ) {
        if (t->isIntegerTy()) l = Builder.CreateICmpEQ(l, r);
        else if (t->isFloatTy()) l = Builder.CreateFCmpOEQ(l, r);
      } else if (std::get<0>(i).type == NE) {
        if (t->isIntegerTy()) l = Builder.CreateICmpNE(l, r);
        else if (t->isFloatTy()) l = Builder.CreateFCmpONE(l, r);
      }
      else LogErrorV("Expected int or float");
    }
    std::cout<<" returning "<<l<<std::endl;
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
    llvm::Constant *val;
    std::cout << "test";
    llvm::Type *t = stringToType(Type);
    if (t == llvm::Type::getInt32Ty(TheContext)) val = llvm::ConstantInt::get(TheContext, llvm::APInt(32, 0, true));
    else if (t == llvm::Type::getFloatTy(TheContext)) val = llvm::ConstantFP::get(TheContext, llvm::APFloat(0.0));
    else if (t == llvm::Type::getInt1Ty(TheContext)) val = 0 ? llvm::ConstantInt::get(TheContext, llvm::APInt(1, 1)) : llvm::ConstantInt::get(TheContext, llvm::APInt());
    TheModule->getOrInsertGlobal(Name, t);
    llvm::GlobalVariable *Var = TheModule->getGlobalVariable(Name);
    Var->setInitializer(val);
    GlobalNamedValues[Name] = Var;

    //llvm::AllocaInst* out = CreateEntryBlockAlloca(Builder.GetInsertBlock()->getParent(), Name, t);
    //Builder.CreateStore(val, out);
    //GlobalNamedValues[Name] = val;
  } else currentScope->createNewLocal(/*Builder.GetInsertBlock()->getParent()*/nullptr, Name, stringToType(Type));
  return nullptr;
}

llvm::Value *blockASTnode::codegen(){
  //llvm::BasicBlock *b = llvm::BasicBlock::Create(TheContext);
  Scope *s = currentScope;
  if (!newFunc) {
    Scope *n = new Scope(s);
    currentScope = n;
  } else newFunc = false;
  //Builder.SetInsertPoint(b);
  for (auto&& i : localDecls) {
    i->codegen();
  }

  for (auto&& j : statements) {
    if (j != nullptr) auto *cur = j->codegen();
    //b->getInstList().push_back(cur);
  }
  currentScope = s;
  //b = Builder.GetInsertBlock();
  //return b;
  //return Builder.CreateBr(b);
  return nullptr;
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
  std::cout<<Name<<" ident"<<std::endl;
  llvm::Value *V = currentScope->getLocal(Name);
  std::cout<<"getting "<<V<<std::endl;
  if (!V) {
    V = GlobalNamedValues[Name];
    if (V) return Builder.CreateLoad(V, Name.c_str());
    else return LogErrorV("Unknown variable name");
  }
  else return V;
}

llvm::Value *exprASTnode::codegen(){
  if (Vals != nullptr){
    llvm::Value *E = Vals->codegen();
    for (auto&& i : Names) {
      bool suc = currentScope->assignLocal(i, E);
      if (!suc) {
        std::cout<<i<<" get in expr"<<std::endl;
        llvm::Value *V = GlobalNamedValues[i];
        if (GlobalNamedValues.find(i) != GlobalNamedValues.end()) {
          llvm::Value *V = GlobalNamedValues[i];
          E = applyType(E, V->getType());
          Builder.CreateStore(E, V);
          GlobalNamedValues[i] = E;
        } else return LogErrorV("Unknown variable name in expression");
      }
    }
    return E;
  } else return nullptr;
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
  TheFunction->getBasicBlockList().push_back(ebb);

  Builder.SetInsertPoint(ebb);
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
  TheFunction->getBasicBlockList().push_back(code);

  llvm::Value *wl;
  if (Statement != nullptr) Statement->codegen();
  Builder.CreateBr(first);
  code = Builder.GetInsertBlock();

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
  if (funcExsists(Name)) return LogErrorV("function already exsists");
  std::vector<llvm::Type *> argTypes = {};
  std::vector<std::string> names = {};
  if (paramaters != nullptr) {
    if (paramaters->Type == "list") {
      for (auto &par : paramaters->getList()){
        argTypes.push_back(stringToType(par->Type));
        names.push_back(par->Name);
      }
    }
  }
  llvm::FunctionType *type = llvm::FunctionType::get(stringToType(Type), argTypes, false);
  llvm::Function *f = llvm::Function::Create(type, llvm::Function::ExternalLinkage, Name, TheModule.get());
  unsigned Idx = 0;
  for (auto &arg : f->args()) arg.setName(names[Idx++]);
  funcNames.push_back(Name);
  return f;
}

llvm::Value *funcDeclASTnode::codegen(){
  if (funcExsists(Name)) return LogErrorV("function already exsists");
  std::vector<llvm::Type *> argTypes = {};
  std::vector<std::string> names = {};
  if (Paramaters != nullptr) {
    if (Paramaters->Type == "list") {
      for (auto &par : Paramaters->getList()){
        argTypes.push_back(stringToType(par->Type));
        names.push_back(par->Name);
      }
    }
  }
  llvm::FunctionType *type = llvm::FunctionType::get(stringToType(Type), argTypes, false);
  llvm::Function *nf = llvm::Function::Create(type, llvm::Function::ExternalLinkage, Name, TheModule.get());
  //return LogErrorV("test");
  //nf = TheModule->getFunction(Name);
  unsigned Idx = 0;
  for (auto &arg : nf->args()) arg.setName(names[Idx++]);
  llvm::BasicBlock *b = llvm::BasicBlock::Create(TheContext, "funcBlock", nf);
  Builder.SetInsertPoint(b);
  currentScope = new Scope(nullptr);
  newFunc = true;
  for (auto &arg: nf->args())
  {
    std::string name = arg.getName();
    llvm::Type *type = arg.getType();
    llvm::AllocaInst *out = CreateEntryBlockAlloca(nf, name, type); //TmpB.CreateAlloca(type, nullptr, name);
    //LogErrorV("test");
    Builder.CreateStore(&arg, out);
    std::cout<<name<<" arg stored "<<out<<std::endl;
    currentScope->Values[name] = out;
    //currentScope->createNewLocal(nf, arg.getName().str(), arg.getType());
  }
  Block->codegen();
  std::cout<<"Finishing func"<<std::endl;
  llvm::verifyFunction(*nf);
  funcNames.push_back(Name);
  std::cout<<"Finished func"<<std::endl;
  return nf;
}

llvm::Value *ProgramASTnode::codegen(){
  for (auto&& e : externals) e->codegen();
  for (auto&& d : declerations) d->codegen();
  return nullptr;
}
