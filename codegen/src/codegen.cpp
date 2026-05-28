#include "../include/codegen.h"

#include <llvm/IR/Verifier.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/Support/raw_ostream.h>

#include <functional>

using namespace llvm;

namespace olang {

CodeGen::CodeGen(Program* program,
                 const std::unordered_map<std::string, ClassInfoPtr>& classTable,
                 const std::string& moduleName)
    : program_(program), classTable_(classTable) {
    ctx_ = std::make_unique<LLVMContext>();
    module_ = std::make_unique<Module>(moduleName, *ctx_);
    b_ = std::make_unique<IRBuilder<>>(*ctx_);
}

bool CodeGen::isInteger(const ClassInfoPtr& c) const { return c && c->name == "Integer"; }
bool CodeGen::isReal(const ClassInfoPtr& c) const { return c && c->name == "Real"; }
bool CodeGen::isBoolean(const ClassInfoPtr& c) const { return c && c->name == "Boolean"; }
bool CodeGen::isString(const ClassInfoPtr& c) const { return c && c->name == "String"; }
bool CodeGen::isIO(const ClassInfoPtr& c) const { return c && c->name == "IO"; }
bool CodeGen::isVoid(const ClassInfoPtr& c) const { return !c || c->name == "Void"; }

Type* CodeGen::llTy(const ClassInfoPtr& c) {
    if (!c || c->name == "Void") return Type::getVoidTy(*ctx_);
    if (c->name == "Integer") return Type::getInt64Ty(*ctx_);
    if (c->name == "Real")    return Type::getDoubleTy(*ctx_);
    if (c->name == "Boolean") return Type::getInt1Ty(*ctx_);

    return PointerType::getUnqual(*ctx_);
}

StructType* CodeGen::structTy(const ClassInfoPtr& c) {
    auto it = structTypes_.find(c->name);
    if (it != structTypes_.end()) return it->second;
    auto* st = StructType::create(*ctx_, "class." + c->name);
    structTypes_[c->name] = st;
    return st;
}

Constant* CodeGen::zeroOf(const ClassInfoPtr& c) {
    if (isInteger(c)) return ConstantInt::get(Type::getInt64Ty(*ctx_), 0);
    if (isReal(c))    return ConstantFP::get(Type::getDoubleTy(*ctx_), 0.0);
    if (isBoolean(c)) return ConstantInt::getFalse(*ctx_);
    return ConstantPointerNull::get(PointerType::getUnqual(*ctx_));
}

void CodeGen::collectFields(const ClassInfoPtr& c, std::vector<FieldInfo>& out) const {
    if (!c) return;
    collectFields(c->baseClass, out);
    for (auto& f : c->fields) out.push_back(f);
}

int CodeGen::fieldIndex(const ClassInfoPtr& c, const std::string& name) const {
    std::vector<FieldInfo> all;
    collectFields(c, all);
    for (size_t i = 0; i < all.size(); ++i)
        if (all[i].name == name) return (int)i + 1;
    return -1;
}

ClassInfoPtr CodeGen::fieldType(const ClassInfoPtr& c, const std::string& name) const {
    std::vector<FieldInfo> all;
    collectFields(c, all);
    for (auto& f : all) if (f.name == name) return f.type;
    return nullptr;
}

std::string CodeGen::mangleMethod(const ClassInfoPtr& cls, const std::string& name, int idx) const {
    return cls->name + "_" + name + "_" + std::to_string(idx);
}
std::string CodeGen::mangleCtor(const ClassInfoPtr& cls, int idx) const {
    return cls->name + "_ctor_" + std::to_string(idx);
}

CodeGen::VarInfo* CodeGen::findVar(const std::string& name) {
    for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
        auto f = it->find(name);
        if (f != it->end()) return &f->second;
    }
    return nullptr;
}

FunctionCallee CodeGen::rt(const std::string& name) {
    return module_->getOrInsertFunction(name, module_->getFunction(name)->getFunctionType());
}

void CodeGen::declareRuntime() {
    auto* i64 = Type::getInt64Ty(*ctx_);
    auto* i1  = Type::getInt1Ty(*ctx_);
    auto* dbl = Type::getDoubleTy(*ctx_);
    auto* ptr = PointerType::getUnqual(*ctx_);
    auto* vd  = Type::getVoidTy(*ctx_);

    auto add = [&](const char* n, FunctionType* ft) {
        module_->getOrInsertFunction(n, ft);
    };
    add("ol_int_to_string",  FunctionType::get(ptr, {i64}, false));
    add("ol_real_to_string", FunctionType::get(ptr, {dbl}, false));
    add("ol_bool_to_string", FunctionType::get(ptr, {i1},  false));
    add("ol_str_concat",     FunctionType::get(ptr, {ptr, ptr}, false));
    add("ol_str_at",         FunctionType::get(ptr, {ptr, i64}, false));
    add("ol_str_length",     FunctionType::get(i64, {ptr}, false));
    add("ol_str_equal",      FunctionType::get(i1,  {ptr, ptr}, false));
    add("ol_str_substring",  FunctionType::get(ptr, {ptr, i64, i64}, false));
    add("ol_str_new",        FunctionType::get(ptr, {ptr}, false));
    add("ol_str_empty",      FunctionType::get(ptr, {}, false));
    add("ol_io_write",       FunctionType::get(vd,  {ptr}, false));
    add("ol_io_writeln",     FunctionType::get(vd,  {ptr}, false));
    add("ol_io_read",        FunctionType::get(ptr, {}, false));
    add("ol_io_readline",    FunctionType::get(ptr, {}, false));
    add("ol_alloc",          FunctionType::get(ptr, {i64}, false));
    add("atoll",             FunctionType::get(i64, {ptr}, false));
    add("atof",              FunctionType::get(dbl, {ptr}, false));
}

void CodeGen::declareStructs() {

    for (auto& [name, cls] : classTable_) {
        if (cls->isBuiltin) continue;
        structTy(cls);
    }
    auto* ptr = PointerType::getUnqual(*ctx_);
    for (auto& [name, cls] : classTable_) {
        if (cls->isBuiltin) continue;
        std::vector<FieldInfo> all;
        collectFields(cls, all);
        std::vector<Type*> body;
        body.push_back(ptr);
        for (auto& f : all) body.push_back(llTy(f.type));
        structTy(cls)->setBody(body);
    }
}

void CodeGen::declareUserFunctions() {
    auto* ptr = PointerType::getUnqual(*ctx_);
    for (auto& cd : program_->classes) {
        auto it = classTable_.find(cd->name->name);
        if (it == classTable_.end()) continue;
        auto cls = it->second;
        std::unordered_map<std::string, int> methodCounter;

        for (auto& mem : cd->members) {
            if (auto* md = dynamic_cast<MethodDeclaration*>(mem.get())) {
                if (md->isForward) continue;
                int idx = methodCounter[md->name->name]++;
                ClassInfoPtr retT;
                if (md->returnType) {
                    auto rit = classTable_.find(md->returnType->name);
                    if (rit != classTable_.end()) retT = rit->second;
                }
                std::vector<Type*> params{ ptr  };
                for (auto& p : md->parameters) {
                    auto pit = classTable_.find(p->type->name);
                    params.push_back(llTy(pit != classTable_.end() ? pit->second : nullptr));
                }
                auto* ft = FunctionType::get(llTy(retT), params, false);
                std::string name = mangleMethod(cls, md->name->name, idx);
                Function::Create(ft, Function::ExternalLinkage, name, module_.get());
            }
        }
    }

    for (auto& cd : program_->classes) {
        auto it = classTable_.find(cd->name->name);
        if (it == classTable_.end()) continue;
        auto cls = it->second;
        int ctorIdx = 0;
        for (auto& mem : cd->members) {
            if (auto* ctor = dynamic_cast<ConstructorDeclaration*>(mem.get())) {
                std::vector<Type*> params{ ptr };
                for (auto& p : ctor->parameters) {
                    auto pit = classTable_.find(p->type->name);
                    params.push_back(llTy(pit != classTable_.end() ? pit->second : nullptr));
                }
                auto* ft = FunctionType::get(Type::getVoidTy(*ctx_), params, false);
                Function::Create(ft, Function::ExternalLinkage,
                                 mangleCtor(cls, ctorIdx++), module_.get());
            }
        }
    }
}

bool CodeGen::sameSignature(const std::vector<ClassInfoPtr>& a,
                            const std::vector<ClassInfoPtr>& b) const {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i) {
        if (!a[i] || !b[i]) return false;
        if (a[i]->name != b[i]->name) return false;
    }
    return true;
}

void CodeGen::buildVTables() {
    std::vector<ClassInfoPtr> ordered;
    std::unordered_map<std::string, bool> visited;
    std::function<void(const ClassInfoPtr&)> visit = [&](const ClassInfoPtr& c) {
        if (!c || c->isBuiltin || visited[c->name]) return;
        visited[c->name] = true;
        if (c->baseClass && !c->baseClass->isBuiltin) visit(c->baseClass);
        ordered.push_back(c);
    };
    for (auto& [name, cls] : classTable_) visit(cls);

    for (auto& cls : ordered) {
        std::vector<VTableEntry> entries;
        if (cls->baseClass && !cls->baseClass->isBuiltin) {
            entries = vtables_[cls->baseClass->name];
        }

        std::unordered_map<std::string, int> ownMethodCounter;
        for (auto& [mname, mvec] : cls->methods) {
            for (auto& mi : mvec) {
                if (mi.isForward) continue;
                auto owner = mi.ownerClass.lock();
                if (!owner || owner->name != cls->name) continue;

                std::vector<ClassInfoPtr> paramTypes;
                for (auto& p : mi.parameters) paramTypes.push_back(p.type);

                int idx = ownMethodCounter[mname]++;
                Function* fn = module_->getFunction(mangleMethod(cls, mname, idx));

                int slot = -1;
                for (size_t i = 0; i < entries.size(); ++i) {
                    if (entries[i].methodName == mname &&
                        sameSignature(entries[i].paramTypes, paramTypes)) {
                        slot = (int)i; break;
                    }
                }
                if (slot >= 0) {
                    entries[slot].impl = fn;
                    entries[slot].declaringClass = cls;
                } else {
                    entries.push_back({mname, paramTypes, mi.returnType, fn, cls});
                }
            }
        }
        vtables_[cls->name] = std::move(entries);
    }
}

void CodeGen::emitVTableGlobals() {
    auto* ptr = PointerType::getUnqual(*ctx_);
    for (auto& [cname, entries] : vtables_) {
        std::vector<Constant*> fns;
        for (auto& e : entries) fns.push_back(e.impl);
        auto* arrTy = ArrayType::get(ptr, fns.size());
        auto* init = ConstantArray::get(arrTy, fns);
        auto* gv = new GlobalVariable(*module_, arrTy, true,
                                      GlobalValue::PrivateLinkage,
                                      init, "vtable." + cname);
        vtableGlobals_[cname] = gv;
    }
}

int CodeGen::findVTableSlot(const ClassInfoPtr& cls,
                            const std::string& name,
                            const std::vector<ClassInfoPtr>& paramTypes) const {
    auto it = vtables_.find(cls->name);
    if (it == vtables_.end()) return -1;
    for (size_t i = 0; i < it->second.size(); ++i) {
        if (it->second[i].methodName == name &&
            sameSignature(it->second[i].paramTypes, paramTypes)) return (int)i;
    }
    return -1;
}

std::unique_ptr<Module> CodeGen::generate() {
    declareRuntime();
    declareStructs();
    declareUserFunctions();
    buildVTables();
    emitVTableGlobals();

    for (auto& cd : program_->classes) emitClass(cd.get());

    emitEntry();

    std::string err;
    raw_string_ostream os(err);
    if (verifyModule(*module_, &os)) {
        errs() << "LLVM module verification failed:\n" << os.str() << "\n";
    }
    return std::move(module_);
}

void CodeGen::emitClass(ClassDeclaration* cd) {
    auto it = classTable_.find(cd->name->name);
    if (it == classTable_.end()) return;
    auto cls = it->second;
    currentClass_ = cls;

    std::unordered_map<std::string, int> methodCounter;
    int ctorIdx = 0;
    for (auto& mem : cd->members) {
        if (auto* md = dynamic_cast<MethodDeclaration*>(mem.get())) {
            if (md->isForward) continue;
            int idx = methodCounter[md->name->name]++;
            emitMethod(md, cls, idx);
        } else if (auto* ctor = dynamic_cast<ConstructorDeclaration*>(mem.get())) {
            emitConstructor(ctor, cls, ctorIdx++);
        }
    }
    currentClass_ = nullptr;
}

void CodeGen::emitFunctionPrologue(Function* fn,
                                   const std::vector<std::pair<std::string, ClassInfoPtr>>& params,
                                   const ClassInfoPtr& selfCls) {
    auto* entry = BasicBlock::Create(*ctx_, "entry", fn);
    b_->SetInsertPoint(entry);

    scopes_.clear();
    scopes_.push_back({});

    thisSlot_ = b_->CreateAlloca(PointerType::getUnqual(*ctx_), nullptr, "this.addr");
    auto argIt = fn->arg_begin();
    argIt->setName("this");
    b_->CreateStore(&*argIt, thisSlot_);
    scopes_.back()["this"] = {thisSlot_, selfCls};
    ++argIt;

    for (size_t i = 0; i < params.size(); ++i, ++argIt) {
        auto& [pname, ptype] = params[i];
        argIt->setName("arg." + pname);
        auto* slot = b_->CreateAlloca(llTy(ptype), nullptr, "p." + pname);
        b_->CreateStore(&*argIt, slot);
        scopes_.back()[pname] = {slot, ptype};
    }
}

void CodeGen::emitMethod(MethodDeclaration* md, const ClassInfoPtr& cls, int overloadIdx) {
    ClassInfoPtr retT;
    if (md->returnType) {
        auto rit = classTable_.find(md->returnType->name);
        if (rit != classTable_.end()) retT = rit->second;
    }
    currentReturnType_ = retT;

    Function* fn = module_->getFunction(mangleMethod(cls, md->name->name, overloadIdx));

    std::vector<std::pair<std::string, ClassInfoPtr>> params;
    for (auto& p : md->parameters) {
        auto pit = classTable_.find(p->type->name);
        params.emplace_back(p->name->name, pit != classTable_.end() ? pit->second : nullptr);
    }
    emitFunctionPrologue(fn, params, cls);

    emitBody(md->body);

    if (!b_->GetInsertBlock()->getTerminator()) {
        if (isVoid(retT)) b_->CreateRetVoid();
        else b_->CreateRet(zeroOf(retT));
    }
}

void CodeGen::emitConstructor(ConstructorDeclaration* cd, const ClassInfoPtr& cls, int idx) {
    currentReturnType_ = nullptr;
    Function* fn = module_->getFunction(mangleCtor(cls, idx));

    std::vector<std::pair<std::string, ClassInfoPtr>> params;
    for (auto& p : cd->parameters) {
        auto pit = classTable_.find(p->type->name);
        params.emplace_back(p->name->name, pit != classTable_.end() ? pit->second : nullptr);
    }
    emitFunctionPrologue(fn, params, cls);

    auto gvIt = vtableGlobals_.find(cls->name);
    if (gvIt != vtableGlobals_.end()) {
        Value* thisV = b_->CreateLoad(PointerType::getUnqual(*ctx_), thisSlot_);
        Value* vtSlot = b_->CreateStructGEP(structTy(cls), thisV, 0);
        b_->CreateStore(gvIt->second, vtSlot);
    }

    emitBody(cd->body);

    if (!b_->GetInsertBlock()->getTerminator()) b_->CreateRetVoid();
}

void CodeGen::emitBody(const std::vector<std::unique_ptr<ASTNode>>& body) {
    scopes_.push_back({});
    for (auto& n : body) {
        if (b_->GetInsertBlock()->getTerminator()) break;
        emitStatement(n.get());
    }
    scopes_.pop_back();
}

void CodeGen::emitStatement(ASTNode* node) {
    if (auto* vd = dynamic_cast<VariableDeclaration*>(node)) { emitVarDecl(vd); return; }
    if (auto* a  = dynamic_cast<Assignment*>(node))          { emitAssign(a);  return; }
    if (auto* s  = dynamic_cast<IfStatement*>(node))         { emitIf(s);      return; }
    if (auto* s  = dynamic_cast<WhileLoop*>(node))           { emitWhile(s);   return; }
    if (auto* s  = dynamic_cast<ReturnStatement*>(node))     { emitReturn(s);  return; }
    if (auto* e  = dynamic_cast<Expression*>(node))          { emitExpr(e);    return; }
}

void CodeGen::emitVarDecl(VariableDeclaration* vd) {
    auto t = vd->initializer->resolvedType;
    Value* v = emitExpr(vd->initializer.get());
    auto* slot = b_->CreateAlloca(llTy(t), nullptr, "v." + vd->name->name);
    b_->CreateStore(v, slot);
    scopes_.back()[vd->name->name] = {slot, t};
}

void CodeGen::emitAssign(Assignment* a) {
    Value* v = emitExpr(a->value.get());
    auto valT = a->value->resolvedType;

    if (auto* id = dynamic_cast<Identifier*>(a->target.get())) {
        if (auto* var = findVar(id->name)) {
            b_->CreateStore(coerce(v, valT, var->type), var->slot);
            return;
        }
        if (currentClass_) {
            int idx = fieldIndex(currentClass_, id->name);
            if (idx >= 0) {
                Value* thisV = b_->CreateLoad(PointerType::getUnqual(*ctx_), thisSlot_);
                Value* gep = b_->CreateStructGEP(structTy(currentClass_), thisV, idx);
                b_->CreateStore(coerce(v, valT, fieldType(currentClass_, id->name)), gep);
                return;
            }
        }
    } else if (auto* ma = dynamic_cast<MemberAccess*>(a->target.get())) {
        Value* obj = emitExpr(ma->object.get());
        auto objT = ma->object->resolvedType;
        int idx = fieldIndex(objT, ma->member->name);
        if (idx >= 0) {
            Value* gep = b_->CreateStructGEP(structTy(objT), obj, idx);
            b_->CreateStore(coerce(v, valT, fieldType(objT, ma->member->name)), gep);
        }
    }
}

void CodeGen::emitIf(IfStatement* s) {
    Value* c = emitExpr(s->condition.get());
    Function* fn = b_->GetInsertBlock()->getParent();
    bool hasElse = !s->elseBody.empty();
    auto* thenBB = BasicBlock::Create(*ctx_, "if.then", fn);
    auto* elseBB = hasElse ? BasicBlock::Create(*ctx_, "if.else", fn) : nullptr;
    auto* endBB  = BasicBlock::Create(*ctx_, "if.end",  fn);

    b_->CreateCondBr(c, thenBB, hasElse ? elseBB : endBB);

    b_->SetInsertPoint(thenBB);
    emitBody(s->thenBody);
    if (!b_->GetInsertBlock()->getTerminator()) b_->CreateBr(endBB);

    if (hasElse) {
        b_->SetInsertPoint(elseBB);
        emitBody(s->elseBody);
        if (!b_->GetInsertBlock()->getTerminator()) b_->CreateBr(endBB);
    }

    b_->SetInsertPoint(endBB);
}

void CodeGen::emitWhile(WhileLoop* s) {
    Function* fn = b_->GetInsertBlock()->getParent();
    auto* condBB = BasicBlock::Create(*ctx_, "while.cond", fn);
    auto* bodyBB = BasicBlock::Create(*ctx_, "while.body", fn);
    auto* endBB  = BasicBlock::Create(*ctx_, "while.end",  fn);

    b_->CreateBr(condBB);
    b_->SetInsertPoint(condBB);
    Value* c = emitExpr(s->condition.get());
    b_->CreateCondBr(c, bodyBB, endBB);

    b_->SetInsertPoint(bodyBB);
    emitBody(s->body);
    if (!b_->GetInsertBlock()->getTerminator()) b_->CreateBr(condBB);

    b_->SetInsertPoint(endBB);
}

void CodeGen::emitReturn(ReturnStatement* s) {
    bool voidCtx = isVoid(currentReturnType_);
    if (s->value) {
        Value* v = emitExpr(s->value.get());
        if (voidCtx) return;
        b_->CreateRet(coerce(v, s->value->resolvedType, currentReturnType_));
    } else {
        b_->CreateRetVoid();
    }
}

Value* CodeGen::emitExpr(Expression* e) {
    if (auto* il = dynamic_cast<IntegerLiteral*>(e))
        return ConstantInt::get(Type::getInt64Ty(*ctx_), il->value, true);
    if (auto* rl = dynamic_cast<RealLiteral*>(e))
        return ConstantFP::get(Type::getDoubleTy(*ctx_), rl->value);
    if (auto* bl = dynamic_cast<BooleanLiteral*>(e))
        return bl->value ? ConstantInt::getTrue(*ctx_) : ConstantInt::getFalse(*ctx_);
    if (auto* sl = dynamic_cast<StringLiteral*>(e)) {
        auto* gs = b_->CreateGlobalString(sl->value, ".str");
        return b_->CreateCall(module_->getFunction("ol_str_new"), {gs});
    }
    if (dynamic_cast<ThisExpression*>(e)) {
        return b_->CreateLoad(PointerType::getUnqual(*ctx_), thisSlot_);
    }
    if (auto* id = dynamic_cast<Identifier*>(e)) {
        if (auto* v = findVar(id->name)) {
            return b_->CreateLoad(llTy(v->type), v->slot, id->name);
        }
        if (currentClass_) {
            int idx = fieldIndex(currentClass_, id->name);
            if (idx >= 0) {
                auto ft = fieldType(currentClass_, id->name);
                Value* thisV = b_->CreateLoad(PointerType::getUnqual(*ctx_), thisSlot_);
                Value* gep = b_->CreateStructGEP(structTy(currentClass_), thisV, idx);
                return b_->CreateLoad(llTy(ft), gep, id->name);
            }
        }
        return zeroOf(e->resolvedType);
    }
    if (auto* fc = dynamic_cast<FunctionCall*>(e))           return emitCall(fc);
    if (auto* ci = dynamic_cast<ConstructorInvocation*>(e))  return emitCtor(ci);
    if (auto* ma = dynamic_cast<MemberAccess*>(e))           return emitMember(ma);
    return zeroOf(e->resolvedType);
}

Value* CodeGen::emitMember(MemberAccess* m) {
    Value* obj = emitExpr(m->object.get());
    auto objT = m->object->resolvedType;
    int idx = fieldIndex(objT, m->member->name);
    if (idx < 0) return zeroOf(m->resolvedType);
    auto ft = fieldType(objT, m->member->name);
    Value* gep = b_->CreateStructGEP(structTy(objT), obj, idx);
    return b_->CreateLoad(llTy(ft), gep, m->member->name);
}

int CodeGen::findMethodOverloadIdx(const ClassInfoPtr& cls,
                                   const std::string& name,
                                   const std::vector<ClassInfoPtr>& argTypes,
                                   ClassInfoPtr& ownerOut) {
    ClassInfoPtr walk = cls;
    while (walk) {
        auto it = walk->methods.find(name);
        if (it != walk->methods.end()) {
            int i = 0;
            for (auto& m : it->second) {
                if (m.isForward) { i++; continue; }
                if (m.parameters.size() != argTypes.size()) { i++; continue; }
                bool ok = true;
                for (size_t k = 0; k < argTypes.size(); ++k) {
                    if (!argTypes[k] || !m.parameters[k].type ||
                        !argTypes[k]->isSubclassOf(m.parameters[k].type)) { ok = false; break; }
                }
                if (ok) { ownerOut = walk; return i; }
                i++;
            }
        }
        walk = walk->baseClass;
    }
    ownerOut = cls;
    return 0;
}

int CodeGen::findCtorIdx(const ClassInfoPtr& cls, const std::vector<ClassInfoPtr>& argTypes) {
    int i = 0;
    for (auto& ctor : cls->constructors) {
        if (ctor.parameters.size() == argTypes.size()) {
            bool ok = true;
            for (size_t k = 0; k < argTypes.size(); ++k) {
                if (!argTypes[k] || !ctor.parameters[k].type ||
                    !argTypes[k]->isSubclassOf(ctor.parameters[k].type)) { ok = false; break; }
            }
            if (ok) return i;
        }
        i++;
    }
    return 0;
}

Value* CodeGen::emitCall(FunctionCall* c) {
    std::vector<Value*> args;
    std::vector<ClassInfoPtr> argTypes;
    for (auto& a : c->arguments) {
        argTypes.push_back(a->resolvedType);
        args.push_back(emitExpr(a.get()));
    }

    auto emitVirtualCall = [&](Value* recv, const ClassInfoPtr& staticT,
                               const std::string& mname) -> Value* {
        ClassInfoPtr owner;
        int idx = findMethodOverloadIdx(staticT, mname, argTypes, owner);
        Function* declFn = module_->getFunction(mangleMethod(owner, mname, idx));
        if (!declFn) return zeroOf(c->resolvedType);

        auto& mvec = owner->methods.at(mname);
        const MethodInfo* mi = nullptr;
        int seen = 0;
        for (auto& m : mvec) {
            if (m.isForward) continue;
            if (seen == idx) { mi = &m; break; }
            seen++;
        }

        std::vector<ClassInfoPtr> ptypes;
        if (mi) for (auto& p : mi->parameters) ptypes.push_back(p.type);
        int slot = findVTableSlot(owner, mname, ptypes);

        std::vector<Value*> callArgs{ recv };
        for (size_t k = 0; k < args.size(); ++k) {
            ClassInfoPtr to = mi ? mi->parameters[k].type : argTypes[k];
            callArgs.push_back(coerce(args[k], argTypes[k], to));
        }

        if (slot < 0) {
            return b_->CreateCall(declFn, callArgs);
        }

        auto* ptr = PointerType::getUnqual(*ctx_);
        Value* vtbl = b_->CreateLoad(ptr,
            b_->CreateStructGEP(structTy(staticT), recv, 0));
        size_t vsize = vtables_.at(staticT->name).size();
        auto* arrTy = ArrayType::get(ptr, vsize);
        Value* slotPtr = b_->CreateGEP(arrTy, vtbl,
            { ConstantInt::get(Type::getInt32Ty(*ctx_), 0),
              ConstantInt::get(Type::getInt32Ty(*ctx_), slot) });
        Value* fnPtr = b_->CreateLoad(ptr, slotPtr);
        return b_->CreateCall(declFn->getFunctionType(), fnPtr, callArgs);
    };

    if (auto* ma = dynamic_cast<MemberAccess*>(c->callee.get())) {
        Value* obj = emitExpr(ma->object.get());
        auto objT = ma->object->resolvedType;
        const std::string& mname = ma->member->name;

        if (objT && objT->isBuiltin) {
            return emitBuiltinMethod(objT, mname, obj, args, argTypes);
        }
        return emitVirtualCall(obj, objT, mname);
    }

    if (auto* id = dynamic_cast<Identifier*>(c->callee.get())) {
        if (currentClass_) {
            Value* thisV = b_->CreateLoad(PointerType::getUnqual(*ctx_), thisSlot_);
            return emitVirtualCall(thisV, currentClass_, id->name);
        }
    }
    return zeroOf(c->resolvedType);
}

Value* CodeGen::emitCtor(ConstructorInvocation* c) {
    auto it = classTable_.find(c->className->name);
    if (it == classTable_.end()) return ConstantPointerNull::get(PointerType::getUnqual(*ctx_));
    auto cls = it->second;

    std::vector<Value*> args;
    std::vector<ClassInfoPtr> argTypes;
    for (auto& a : c->arguments) {
        argTypes.push_back(a->resolvedType);
        args.push_back(emitExpr(a.get()));
    }

    if (cls->isBuiltin) return emitBuiltinCtor(cls, args, argTypes);

    auto* st = structTy(cls);
    auto* i64 = Type::getInt64Ty(*ctx_);
    Value* nullPtr = ConstantPointerNull::get(PointerType::getUnqual(*ctx_));
    Value* sizePtr = b_->CreateGEP(st, nullPtr,
                                   { ConstantInt::get(i64, 1) });
    Value* size = b_->CreatePtrToInt(sizePtr, i64);
    Value* obj = b_->CreateCall(module_->getFunction("ol_alloc"), {size});

    int idx = findCtorIdx(cls, argTypes);
    Function* fn = module_->getFunction(mangleCtor(cls, idx));
    if (!fn) return obj;

    auto& ctorInfo = cls->constructors[idx];
    std::vector<Value*> callArgs{ obj };
    for (size_t k = 0; k < args.size(); ++k) {
        callArgs.push_back(coerce(args[k], argTypes[k], ctorInfo.parameters[k].type));
    }
    b_->CreateCall(fn, callArgs);
    return obj;
}

Value* CodeGen::emitBuiltinCtor(const ClassInfoPtr& cls,
                                const std::vector<Value*>& args,
                                const std::vector<ClassInfoPtr>& argTypes) {
    auto* i64 = Type::getInt64Ty(*ctx_);
    auto* dbl = Type::getDoubleTy(*ctx_);

    if (isInteger(cls)) {
        if (args.empty()) return ConstantInt::get(i64, 0);
        if (isReal(argTypes[0])) return b_->CreateFPToSI(args[0], i64);
        return args[0];
    }
    if (isReal(cls)) {
        if (args.empty()) return ConstantFP::get(dbl, 0.0);
        if (isInteger(argTypes[0])) return b_->CreateSIToFP(args[0], dbl);
        return args[0];
    }
    if (isBoolean(cls)) {
        if (args.empty()) return ConstantInt::getFalse(*ctx_);
        return args[0];
    }
    if (isString(cls)) {
        if (args.empty()) return b_->CreateCall(module_->getFunction("ol_str_empty"), {});
        return args[0];
    }
    if (isIO(cls)) {
        return ConstantPointerNull::get(PointerType::getUnqual(*ctx_));
    }
    return ConstantPointerNull::get(PointerType::getUnqual(*ctx_));
}

Value* CodeGen::coerce(Value* v, const ClassInfoPtr& from, const ClassInfoPtr& to) {
    if (!from || !to) return v;
    if (from == to || from->name == to->name) return v;
    if (isReal(to) && isInteger(from))
        return b_->CreateSIToFP(v, Type::getDoubleTy(*ctx_));
    if (isInteger(to) && isReal(from))
        return b_->CreateFPToSI(v, Type::getInt64Ty(*ctx_));
    return v;
}

Value* CodeGen::emitBuiltinMethod(const ClassInfoPtr& cls,
                                  const std::string& m,
                                  Value* obj,
                                  const std::vector<Value*>& a,
                                  const std::vector<ClassInfoPtr>& at) {
    auto* i64 = Type::getInt64Ty(*ctx_);
    auto* dbl = Type::getDoubleTy(*ctx_);

    auto promote = [&](Value* x, const ClassInfoPtr& t) {
        if (isReal(t)) return x;
        return (Value*)b_->CreateSIToFP(x, dbl);
    };

    if (isInteger(cls)) {
        bool useReal = !at.empty() && isReal(at[0]);
        if (m == "Plus" || m == "Minus" || m == "Mult" || m == "Div" || m == "Rem") {
            if (useReal) {
                Value* l = promote(obj, cls);
                Value* r = a[0];
                if (m == "Plus")  return b_->CreateFAdd(l, r);
                if (m == "Minus") return b_->CreateFSub(l, r);
                if (m == "Mult")  return b_->CreateFMul(l, r);
                if (m == "Div")   return b_->CreateFDiv(l, r);
                return b_->CreateFRem(l, r);
            }
            if (m == "Plus")  return b_->CreateAdd(obj, a[0]);
            if (m == "Minus") return b_->CreateSub(obj, a[0]);
            if (m == "Mult")  return b_->CreateMul(obj, a[0]);
            if (m == "Div")   return b_->CreateSDiv(obj, a[0]);
            return b_->CreateSRem(obj, a[0]);
        }
        if (m == "Less" || m == "LessEqual" || m == "Greater" || m == "GreaterEqual" || m == "Equal") {
            if (useReal) {
                Value* l = promote(obj, cls);
                Value* r = a[0];
                if (m == "Less")         return b_->CreateFCmpOLT(l, r);
                if (m == "LessEqual")    return b_->CreateFCmpOLE(l, r);
                if (m == "Greater")      return b_->CreateFCmpOGT(l, r);
                if (m == "GreaterEqual") return b_->CreateFCmpOGE(l, r);
                return b_->CreateFCmpOEQ(l, r);
            }
            if (m == "Less")         return b_->CreateICmpSLT(obj, a[0]);
            if (m == "LessEqual")    return b_->CreateICmpSLE(obj, a[0]);
            if (m == "Greater")      return b_->CreateICmpSGT(obj, a[0]);
            if (m == "GreaterEqual") return b_->CreateICmpSGE(obj, a[0]);
            return b_->CreateICmpEQ(obj, a[0]);
        }
        if (m == "UnaryMinus") return b_->CreateNeg(obj);
        if (m == "toReal")     return b_->CreateSIToFP(obj, dbl);
        if (m == "toBoolean")  return b_->CreateICmpNE(obj, ConstantInt::get(i64, 0));
        if (m == "ToString")
            return b_->CreateCall(module_->getFunction("ol_int_to_string"), {obj});
    }

    if (isReal(cls)) {
        auto realArg = [&]() -> Value* {
            if (at.empty()) return ConstantFP::get(dbl, 0.0);
            if (isReal(at[0])) return a[0];
            return b_->CreateSIToFP(a[0], dbl);
        };
        if (m == "Plus")  return b_->CreateFAdd(obj, realArg());
        if (m == "Minus") return b_->CreateFSub(obj, realArg());
        if (m == "Mult")  return b_->CreateFMul(obj, realArg());
        if (m == "Div")   return b_->CreateFDiv(obj, realArg());
        if (m == "Rem")   return b_->CreateFRem(obj, realArg());
        if (m == "Less")         return b_->CreateFCmpOLT(obj, realArg());
        if (m == "LessEqual")    return b_->CreateFCmpOLE(obj, realArg());
        if (m == "Greater")      return b_->CreateFCmpOGT(obj, realArg());
        if (m == "GreaterEqual") return b_->CreateFCmpOGE(obj, realArg());
        if (m == "Equal")        return b_->CreateFCmpOEQ(obj, realArg());
        if (m == "UnaryMinus")   return b_->CreateFNeg(obj);
        if (m == "toInteger")    return b_->CreateFPToSI(obj, i64);
        if (m == "ToString")
            return b_->CreateCall(module_->getFunction("ol_real_to_string"), {obj});
    }

    if (isBoolean(cls)) {
        if (m == "Or")  return b_->CreateOr(obj, a[0]);
        if (m == "And") return b_->CreateAnd(obj, a[0]);
        if (m == "Xor") return b_->CreateXor(obj, a[0]);
        if (m == "Not") return b_->CreateNot(obj);
        if (m == "toInteger") return b_->CreateZExt(obj, i64);
        if (m == "ToString")
            return b_->CreateCall(module_->getFunction("ol_bool_to_string"), {obj});
    }

    if (isString(cls)) {
        if (m == "Concatenate") return b_->CreateCall(module_->getFunction("ol_str_concat"), {obj, a[0]});
        if (m == "At")          return b_->CreateCall(module_->getFunction("ol_str_at"), {obj, a[0]});
        if (m == "Length")      return b_->CreateCall(module_->getFunction("ol_str_length"), {obj});
        if (m == "Equal")       return b_->CreateCall(module_->getFunction("ol_str_equal"), {obj, a[0]});
        if (m == "Substring")   return b_->CreateCall(module_->getFunction("ol_str_substring"), {obj, a[0], a[1]});
        if (m == "ToString")    return obj;
    }

    if (isIO(cls)) {
        if (m == "Write")     { b_->CreateCall(module_->getFunction("ol_io_write"),    {a[0]}); return obj; }
        if (m == "WriteLine") { b_->CreateCall(module_->getFunction("ol_io_writeln"),  {a[0]}); return obj; }
        if (m == "Read")      return b_->CreateCall(module_->getFunction("ol_io_read"), {});
        if (m == "ReadLine")  return b_->CreateCall(module_->getFunction("ol_io_readline"), {});
    }

    return ConstantPointerNull::get(PointerType::getUnqual(*ctx_));
}

void CodeGen::emitEntry() {
    auto it = classTable_.find("Main");
    if (it == classTable_.end()) return;
    auto cls = it->second;

    auto* i32 = Type::getInt32Ty(*ctx_);
    auto* i64 = Type::getInt64Ty(*ctx_);
    auto* ptr = PointerType::getUnqual(*ctx_);

    auto* mainTy = FunctionType::get(i32, {i32, ptr}, false);
    auto* mainFn = Function::Create(mainTy, Function::ExternalLinkage, "main", module_.get());
    auto argIt = mainFn->arg_begin();
    Argument* argc = &*argIt++; argc->setName("argc");
    Argument* argv = &*argIt;   argv->setName("argv");

    auto* entry = BasicBlock::Create(*ctx_, "entry", mainFn);
    b_->SetInsertPoint(entry);

    auto* st = structTy(cls);
    Value* sizePtr = b_->CreateGEP(st, ConstantPointerNull::get(ptr),
                                   {ConstantInt::get(i64, 1)});
    Value* size = b_->CreatePtrToInt(sizePtr, i64);
    Value* obj  = b_->CreateCall(module_->getFunction("ol_alloc"), {size});
    Value* nargs = b_->CreateSub(argc, ConstantInt::get(i32, 1), "nargs");

    if (cls->constructors.empty()) {
        b_->CreateRet(ConstantInt::get(i32, 0));
        return;
    }

    std::vector<int> byArity;
    int defaultIdx = -1;
    for (size_t i = 0; i < cls->constructors.size(); ++i) {
        size_t n = cls->constructors[i].parameters.size();
        if (byArity.size() < n + 1) byArity.resize(n + 1, -1);
        if (byArity[n] < 0) byArity[n] = (int)i;
        if (n == 0) defaultIdx = (int)i;
    }

    auto callCtor = [&](int idx) {
        auto& ci = cls->constructors[idx];
        std::vector<Value*> callArgs{ obj };
        for (size_t k = 0; k < ci.parameters.size(); ++k) {

            Value* slot = b_->CreateGEP(ptr, argv,
                                        {ConstantInt::get(i64, (int64_t)(k+1))});
            Value* str  = b_->CreateLoad(ptr, slot);
            auto pt = ci.parameters[k].type;
            if (isInteger(pt)) {
                callArgs.push_back(b_->CreateCall(module_->getFunction("atoll"), {str}));
            } else if (isReal(pt)) {
                callArgs.push_back(b_->CreateCall(module_->getFunction("atof"), {str}));
            } else if (isBoolean(pt)) {
                Value* n = b_->CreateCall(module_->getFunction("atoll"), {str});
                callArgs.push_back(b_->CreateICmpNE(n, ConstantInt::get(i64, 0)));
            } else if (isString(pt)) {
                callArgs.push_back(b_->CreateCall(module_->getFunction("ol_str_new"), {str}));
            } else {
                callArgs.push_back(ConstantPointerNull::get(ptr));
            }
        }
        b_->CreateCall(module_->getFunction(mangleCtor(cls, idx)), callArgs);
        b_->CreateRet(ConstantInt::get(i32, 0));
    };

    for (size_t arity = 0; arity < byArity.size(); ++arity) {
        if (byArity[arity] < 0) continue;
        auto* match = BasicBlock::Create(*ctx_, "arity" + std::to_string(arity), mainFn);
        auto* next  = BasicBlock::Create(*ctx_, "next"  + std::to_string(arity), mainFn);
        Value* cmp = b_->CreateICmpEQ(nargs, ConstantInt::get(i32, (int)arity));
        b_->CreateCondBr(cmp, match, next);
        b_->SetInsertPoint(match);
        callCtor(byArity[arity]);
        b_->SetInsertPoint(next);
    }

    if (defaultIdx >= 0) {
        b_->CreateCall(module_->getFunction(mangleCtor(cls, defaultIdx)), {obj});
    }
    b_->CreateRet(ConstantInt::get(i32, 0));
}

}
