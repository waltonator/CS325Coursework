#include "ASTnodes.hpp"

llvm::LLVMContext TheContext;
llvm::IRBuilder<> Builder(TheContext);
std::unique_ptr<llvm::Module> TheModule = llvm::make_unique<llvm::Module>("mini-c", TheContext);
std::list<std::string> funcNames = {};

static Scope* currentScope; //holds current scope

static bool newFunc = false;

//returns true if function exsists
bool funcExsists(std::string name){
  for (std::string i : funcNames) if (name == i) return true;
  return false;
}

//returns errors
llvm::Value *LogErrorV(const char *Str) {
  fprintf(stderr, "Error: %s\n", Str);
  exit(1);
  return nullptr;
}

//returns a type object when given a class
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

//checks for the least expressive type of 2 variables in which no info would be lost, e.g for int and float it would return a float
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

//casts values from one type to another
llvm::Value *applyType(llvm::Value *v, llvm::Type* t){
  v->getType()->print(llvm::outs());
  t->print(llvm::outs());
  llvm::Type *vt = v->getType();
  if (vt == t) return v;
  else if (vt->isIntegerTy() && t->isFloatTy()) return Builder.CreateSIToFP(v, t);
  else if (vt->isIntegerTy(1) && t->isIntegerTy(32)) return Builder.CreateIntCast(v, t, true);
  else if (vt->isFloatTy() && t->isIntegerTy(32)) return Builder.CreateFPToSI(v, t);
  else if (vt->isIntegerTy(32) && t->isIntegerTy(1)) return Builder.CreateIntCast(v, t, true);
  else if (vt->isFloatTy() && t->isIntegerTy(1)) return Builder.CreateFPToSI(v, t);
  else return LogErrorV("type cast didn't work");
}

//creates a new local variable
void Scope::createNewLocal(llvm::Function *TheFunction, const std::string &name, llvm::Type *type) {
  if (Values.find(name) == Values.end()) {
    llvm::AllocaInst *out = CreateEntryBlockAlloca(TheFunction, name, type);
    llvm::Value *val;
    if (type == llvm::Type::getInt32Ty(TheContext)) val = llvm::ConstantInt::get(TheContext, llvm::APInt(32, 0, true));
    else if (type == llvm::Type::getFloatTy(TheContext)) val = llvm::ConstantFP::get(TheContext, llvm::APFloat(0.0f));
    else if (type == llvm::Type::getInt1Ty(TheContext)) val = 0 ? llvm::ConstantInt::get(TheContext, llvm::APInt(1, 1)) : llvm::ConstantInt::get(TheContext, llvm::APInt());
    Builder.CreateStore(val, out);
    Values[name] = out;
  } else LogErrorV("Cannot redefine variables");
}

//assigns a value to a variable in scope
bool Scope::assignLocal(std::string name, llvm::Value *val) {
  llvm::Value *check = Values[name];
  if (check) {
    val = applyType(val, check->getType()->getPointerElementType());
    Builder.CreateStore(val, check);
    return true;
  } else if (parentScope != nullptr) return parentScope->assignLocal(name, val);
  else return false;
}

//gets the value of a variable in scope
llvm::Value *Scope::getLocal(std::string name) {
  llvm::Value *check = Values[name];
  if (check) return Builder.CreateLoad(check, name.c_str());
  else if (parentScope != nullptr) {
    return parentScope->getLocal(name);
  } else return nullptr;
}

//===----------------------------------------------------------------------===//
// Code Generation
//===----------------------------------------------------------------------===//


llvm::Value *negValASTnode::codegen() {
  llvm::Value *v = Val->codegen();
  if (TokCount % 2 == 0) return v;
  else {
    llvm::Value *v = Val->codegen();
    if (v->getType()->isIntegerTy() && Tok.type == MINUS) return Builder.CreateNeg(v);
    else if (v->getType()->isFloatTy() && Tok.type == MINUS) return Builder.CreateFNeg(v);
    else if (v->getType()->isIntegerTy() && Tok.type == NOT) {
      return Builder.CreateICmpEQ(v, llvm::ConstantInt::get(TheContext, llvm::APInt(1, 0)), "negcmp");
    } else return LogErrorV("Expected int or float");
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
    llvm::Type *t = stringToType(Type);
    if (t == llvm::Type::getInt32Ty(TheContext)) val = llvm::ConstantInt::get(TheContext, llvm::APInt(32, 0, true));
    else if (t == llvm::Type::getFloatTy(TheContext)) val = llvm::ConstantFP::get(TheContext, llvm::APFloat(0.0));
    else if (t == llvm::Type::getInt1Ty(TheContext)) val = 0 ? llvm::ConstantInt::get(TheContext, llvm::APInt(1, 1)) : llvm::ConstantInt::get(TheContext, llvm::APInt());
    TheModule->getOrInsertGlobal(Name, t);
    llvm::GlobalVariable *Var = TheModule->getGlobalVariable(Name);
    Var->setInitializer(val);
    llvm::Value *add = Var;
    Var->getType()->print(llvm::outs());
    add->getType()->print(llvm::outs());

  } else currentScope->createNewLocal(nullptr, Name, stringToType(Type));
  return nullptr;
}

llvm::Value *blockASTnode::codegen(){

  Scope *s = currentScope;
  if (!newFunc) {
    Scope *n = new Scope(s);
    currentScope = n;
  } else newFunc = false;

  for (auto&& i : localDecls) i->codegen();

  for (auto&& j : statements) if (j != nullptr) auto *cur = j->codegen();

  currentScope = s;

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
  llvm::Value *V = currentScope->getLocal(Name);
  if (!V) {
    llvm::GlobalVariable *glb = TheModule->getGlobalVariable(Name);
    if (glb) {
      V = glb->getInitializer();
      return V;
    }else return LogErrorV("Unknown variable name");
  }
  else return V;
}

llvm::Value *exprASTnode::codegen(){
  if (Vals != nullptr){
    llvm::Value *E = Vals->codegen();
    for (auto&& i : Names) {
      bool suc = currentScope->assignLocal(i, E);
      if (!suc) {
        llvm::GlobalVariable *glb = TheModule->getGlobalVariable(i);
        if (glb) {
          E = applyType(E, glb->getType()->getPointerElementType());
          llvm::Constant *NE = llvm::cast<llvm::Constant>(E);
          glb->setInitializer(NE);
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

  return nullptr;
}

llvm::Value *whileStmtASTnode::codegen(){
  llvm::Function *TheFunction = Builder.GetInsertBlock()->getParent();

  llvm::BasicBlock *first = llvm::BasicBlock::Create(TheContext, "whileFirst", TheFunction);
  llvm::BasicBlock *code = llvm::BasicBlock::Create(TheContext, "whileLoop");
  llvm::BasicBlock *end = llvm::BasicBlock::Create(TheContext, "whileEnd");

  Builder.CreateBr(first);
  Builder.SetInsertPoint(first);

  llvm::Value *e = Expression->codegen();
  e = Builder.CreateICmpNE(e, llvm::ConstantInt::get(TheContext, llvm::APInt(1, 0)), "whilecond");
  auto *l = Builder.CreateCondBr(e, code, end);
  first = Builder.GetInsertBlock();
  TheFunction->getBasicBlockList().push_back(code);

  Builder.SetInsertPoint(code);
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
    llvm::AllocaInst *out = CreateEntryBlockAlloca(nf, name, type);
    Builder.CreateStore(&arg, out);
    currentScope->Values[name] = out;
  }
  Block->codegen();
  llvm::verifyFunction(*nf);
  funcNames.push_back(Name);
  return nf;
}

llvm::Value *ProgramASTnode::codegen(){
  for (auto&& e : externals) e->codegen();
  for (auto&& d : declerations) d->codegen();
  return nullptr;
}
