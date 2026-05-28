#pragma once

#include "../../parser/include/ast.h"
#include "../../semantic/include/class_info.h"

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace olang {

class CodeGen {
public:
    CodeGen(Program* program,
            const std::unordered_map<std::string, ClassInfoPtr>& classTable,
            const std::string& moduleName = "olang");

    std::unique_ptr<llvm::Module> generate();

private:
    struct VarInfo {
        llvm::AllocaInst* slot;
        ClassInfoPtr type;
    };

    Program* program_;
    const std::unordered_map<std::string, ClassInfoPtr>& classTable_;

    std::unique_ptr<llvm::LLVMContext> ctx_;
    std::unique_ptr<llvm::Module> module_;
    std::unique_ptr<llvm::IRBuilder<>> b_;

    std::unordered_map<std::string, llvm::StructType*> structTypes_;
    std::unordered_map<std::string, llvm::Function*> functions_;

    std::vector<std::unordered_map<std::string, VarInfo>> scopes_;
    ClassInfoPtr currentClass_;
    ClassInfoPtr currentReturnType_;
    llvm::AllocaInst* thisSlot_ = nullptr;

    bool isInteger(const ClassInfoPtr& c) const;
    bool isReal(const ClassInfoPtr& c) const;
    bool isBoolean(const ClassInfoPtr& c) const;
    bool isString(const ClassInfoPtr& c) const;
    bool isIO(const ClassInfoPtr& c) const;
    bool isVoid(const ClassInfoPtr& c) const;

    llvm::Type* llTy(const ClassInfoPtr& c);
    llvm::StructType* structTy(const ClassInfoPtr& c);
    llvm::Constant* zeroOf(const ClassInfoPtr& c);

    void collectFields(const ClassInfoPtr& c, std::vector<FieldInfo>& out) const;
    int fieldIndex(const ClassInfoPtr& c, const std::string& name) const;
    ClassInfoPtr fieldType(const ClassInfoPtr& c, const std::string& name) const;

    std::string mangleMethod(const ClassInfoPtr& cls, const std::string& name, int idx) const;
    std::string mangleCtor(const ClassInfoPtr& cls, int idx) const;

    void declareRuntime();
    void declareStructs();
    void declareUserFunctions();

    void emitClass(ClassDeclaration* cd);
    void emitMethod(MethodDeclaration* md, const ClassInfoPtr& cls, int overloadIdx);
    void emitConstructor(ConstructorDeclaration* cd, const ClassInfoPtr& cls, int idx);
    void emitFunctionPrologue(llvm::Function* fn,
                              const std::vector<std::pair<std::string, ClassInfoPtr>>& params,
                              const ClassInfoPtr& selfCls);
    void emitBody(const std::vector<std::unique_ptr<ASTNode>>& body);
    void emitStatement(ASTNode* node);
    void emitVarDecl(VariableDeclaration* vd);
    void emitAssign(Assignment* a);
    void emitIf(IfStatement* s);
    void emitWhile(WhileLoop* s);
    void emitReturn(ReturnStatement* s);

    llvm::Value* emitExpr(Expression* e);
    llvm::Value* emitCall(FunctionCall* c);
    llvm::Value* emitCtor(ConstructorInvocation* c);
    llvm::Value* emitMember(MemberAccess* m);

    llvm::Value* emitBuiltinMethod(const ClassInfoPtr& objCls,
                                   const std::string& method,
                                   llvm::Value* obj,
                                   const std::vector<llvm::Value*>& args,
                                   const std::vector<ClassInfoPtr>& argTypes);
    llvm::Value* emitBuiltinCtor(const ClassInfoPtr& cls,
                                 const std::vector<llvm::Value*>& args,
                                 const std::vector<ClassInfoPtr>& argTypes);

    VarInfo* findVar(const std::string& name);

    int findMethodOverloadIdx(const ClassInfoPtr& cls,
                              const std::string& name,
                              const std::vector<ClassInfoPtr>& argTypes,
                              ClassInfoPtr& ownerOut);
    int findCtorIdx(const ClassInfoPtr& cls,
                    const std::vector<ClassInfoPtr>& argTypes);

    llvm::Value* coerce(llvm::Value* v, const ClassInfoPtr& from, const ClassInfoPtr& to);

    void emitEntry();

    llvm::FunctionCallee rt(const std::string& name);
};

}
