#pragma once

#include "class_info.h"
#include "symbol_table.h"
#include "semantic_error.h"
#include "../../parser/include/ast.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace olang {

class SemanticAnalyzer {
    std::unordered_map<std::string, ClassInfoPtr> classTable_;
    SymbolTable symbolTable_;
    std::vector<std::string> errors_;

    ClassInfoPtr currentClass_;
    MethodInfo* currentMethod_ = nullptr;

    ClassInfoPtr integerClass_;
    ClassInfoPtr realClass_;
    ClassInfoPtr booleanClass_;
    ClassInfoPtr stringClass_;
    ClassInfoPtr ioClass_;
    ClassInfoPtr classClass_;
    ClassInfoPtr anyValueClass_;
    ClassInfoPtr anyRefClass_;
    ClassInfoPtr arrayClass_;
    ClassInfoPtr listClass_;
    ClassInfoPtr voidType_;

public:
    SemanticAnalyzer();

    void analyze(Program* program);
    const std::vector<std::string>& errors() const { return errors_; }
    bool hasErrors() const { return !errors_.empty(); }
    const std::unordered_map<std::string, ClassInfoPtr>& classTable() const { return classTable_; }

private:
    void registerBuiltins();
    void registerIntegerClass();
    void registerRealClass();
    void registerBooleanClass();
    void registerStringClass();
    void registerIOClass();

    ClassInfoPtr addClass(const std::string& name, ClassInfoPtr base, bool builtin);
    void addMethod(const ClassInfoPtr& cls, const std::string& name,
                   std::vector<ParamInfo> params, ClassInfoPtr returnType);
    void addField(const ClassInfoPtr& cls, const std::string& name, ClassInfoPtr type);
    void addConstructor(const ClassInfoPtr& cls, std::vector<ParamInfo> params);

    void pass1RegisterClasses(Program* program);
    void pass2ResolveInheritance(Program* program);
    void pass3RegisterMembers(Program* program);
    void pass4CheckBodies(Program* program);

    ClassInfoPtr resolveClassName(const ClassName* name);
    ClassInfoPtr resolveType(const std::string& name);

    void checkMethodBody(MethodDeclaration* methodDecl, const ClassInfoPtr& classInfo);
    void checkConstructorBody(ConstructorDeclaration* ctorDecl, const ClassInfoPtr& classInfo);
    void checkBody(const std::vector<std::unique_ptr<ASTNode>>& body);
    void checkStatement(ASTNode* node);
    void checkVariableDeclaration(VariableDeclaration* varDecl);
    void checkAssignment(Assignment* assignment);
    void checkWhileLoop(WhileLoop* whileLoop);
    void checkIfStatement(IfStatement* ifStmt);
    void checkReturnStatement(ReturnStatement* returnStmt);

    ClassInfoPtr checkExpression(Expression* expr);
    ClassInfoPtr checkIdentifier(Identifier* ident);
    ClassInfoPtr checkMemberAccess(MemberAccess* access);
    ClassInfoPtr checkFunctionCall(FunctionCall* call);
    ClassInfoPtr checkConstructorInvocation(ConstructorInvocation* ctor);

    void reportError(const std::string& message);
    bool isAssignableFrom(const ClassInfoPtr& target, const ClassInfoPtr& source);
};

}
