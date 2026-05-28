#include "../include/semantic.h"
#include <sstream>

namespace olang {

void SemanticAnalyzer::checkMethodBody(MethodDeclaration* methodDecl, const ClassInfoPtr& classInfo) {
    const auto& methodName = methodDecl->name->name;
    MethodInfo* methodInfo = nullptr;
    auto it = classInfo->methods.find(methodName);
    if (it != classInfo->methods.end()) {
        for (auto& m : it->second) {
            if (!m.isForward && m.parameters.size() == methodDecl->parameters.size()) {
                methodInfo = &m;
                break;
            }
        }
    }
    currentMethod_ = methodInfo;
    symbolTable_.pushScope();
    for (const auto& param : methodDecl->parameters) {
        auto paramType = resolveClassName(param->type.get());
        if (paramType)
            symbolTable_.define(param->name->name, paramType, SymbolKind::Parameter);
    }
    checkBody(methodDecl->body);
    symbolTable_.popScope();
    currentMethod_ = nullptr;
}

void SemanticAnalyzer::checkConstructorBody(ConstructorDeclaration* ctorDecl, const ClassInfoPtr&) {
    currentMethod_ = nullptr;
    symbolTable_.pushScope();
    for (const auto& param : ctorDecl->parameters) {
        auto paramType = resolveClassName(param->type.get());
        if (paramType)
            symbolTable_.define(param->name->name, paramType, SymbolKind::Parameter);
    }
    checkBody(ctorDecl->body);
    symbolTable_.popScope();
}

void SemanticAnalyzer::checkBody(const std::vector<std::unique_ptr<ASTNode>>& body) {
    for (const auto& node : body)
        checkStatement(node.get());
}

void SemanticAnalyzer::checkStatement(ASTNode* node) {
    if (auto* varDecl = dynamic_cast<VariableDeclaration*>(node))
        checkVariableDeclaration(varDecl);
    else if (auto* assignment = dynamic_cast<Assignment*>(node))
        checkAssignment(assignment);
    else if (auto* whileLoop = dynamic_cast<WhileLoop*>(node))
        checkWhileLoop(whileLoop);
    else if (auto* ifStmt = dynamic_cast<IfStatement*>(node))
        checkIfStatement(ifStmt);
    else if (auto* returnStmt = dynamic_cast<ReturnStatement*>(node))
        checkReturnStatement(returnStmt);
    else if (auto* expr = dynamic_cast<Expression*>(node))
        checkExpression(expr);
}

void SemanticAnalyzer::checkVariableDeclaration(VariableDeclaration* varDecl) {
    auto initType = checkExpression(varDecl->initializer.get());
    if (!symbolTable_.define(varDecl->name->name, initType, SymbolKind::Variable))
        reportError("Variable '" + varDecl->name->name + "' is already defined in this scope");
}

void SemanticAnalyzer::checkAssignment(Assignment* assignment) {
    auto targetType = checkExpression(assignment->target.get());
    auto valueType = checkExpression(assignment->value.get());
    if (targetType && valueType && !isAssignableFrom(targetType, valueType))
        reportError("Cannot assign value of type '" + valueType->name +
                    "' to target of type '" + targetType->name + "'");
    if (auto* ma = dynamic_cast<MemberAccess*>(assignment->target.get())) {
        if (!dynamic_cast<ThisExpression*>(ma->object.get())) {
            auto objType = ma->object->resolvedType;
            if (objType && currentClass_ && !currentClass_->isSubclassOf(objType))
                reportError("Cannot assign to field '" + ma->member->name +
                            "' of class '" + objType->name + "' from outside the class");
        }
    }
}

void SemanticAnalyzer::checkWhileLoop(WhileLoop* whileLoop) {
    auto condType = checkExpression(whileLoop->condition.get());
    if (condType && condType != booleanClass_)
        reportError("While loop condition must be of type Boolean, got '" + condType->name + "'");
    symbolTable_.pushScope();
    checkBody(whileLoop->body);
    symbolTable_.popScope();
}

void SemanticAnalyzer::checkIfStatement(IfStatement* ifStmt) {
    auto condType = checkExpression(ifStmt->condition.get());
    if (condType && condType != booleanClass_)
        reportError("If condition must be of type Boolean, got '" + condType->name + "'");
    symbolTable_.pushScope();
    checkBody(ifStmt->thenBody);
    symbolTable_.popScope();
    if (!ifStmt->elseBody.empty()) {
        symbolTable_.pushScope();
        checkBody(ifStmt->elseBody);
        symbolTable_.popScope();
    }
}

void SemanticAnalyzer::checkReturnStatement(ReturnStatement* returnStmt) {
    if (returnStmt->value) {
        auto valueType = checkExpression(returnStmt->value.get());
        if (currentMethod_ && currentMethod_->returnType && valueType) {
            if (!isAssignableFrom(currentMethod_->returnType, valueType))
                reportError("Return type mismatch: expected '" +
                            currentMethod_->returnType->name + "', got '" +
                            valueType->name + "'");
        }
    } else {
        if (currentMethod_ && currentMethod_->returnType)
            reportError("Method '" + currentMethod_->name +
                        "' must return a value of type '" +
                        currentMethod_->returnType->name + "'");
    }
}

ClassInfoPtr SemanticAnalyzer::checkExpression(Expression* expr) {
    if (!expr) return nullptr;
    ClassInfoPtr type;
    if (dynamic_cast<IntegerLiteral*>(expr))
        type = integerClass_;
    else if (dynamic_cast<RealLiteral*>(expr))
        type = realClass_;
    else if (dynamic_cast<BooleanLiteral*>(expr))
        type = booleanClass_;
    else if (dynamic_cast<StringLiteral*>(expr))
        type = stringClass_;
    else if (dynamic_cast<ThisExpression*>(expr))
        type = currentClass_;
    else if (auto* ident = dynamic_cast<Identifier*>(expr))
        type = checkIdentifier(ident);
    else if (auto* ma = dynamic_cast<MemberAccess*>(expr))
        type = checkMemberAccess(ma);
    else if (auto* fc = dynamic_cast<FunctionCall*>(expr))
        type = checkFunctionCall(fc);
    else if (auto* ci = dynamic_cast<ConstructorInvocation*>(expr))
        type = checkConstructorInvocation(ci);
    expr->resolvedType = type;
    return type;
}

ClassInfoPtr SemanticAnalyzer::checkIdentifier(Identifier* ident) {
    if (ident->name == "base") {
        if (currentClass_ && currentClass_->baseClass)
            return currentClass_->baseClass;
        reportError("'base' used but class has no base class");
        return nullptr;
    }
    auto sym = symbolTable_.lookup(ident->name);
    if (sym) return sym->type;
    if (currentClass_) {
        auto field = currentClass_->findField(ident->name);
        if (field) return field->type;
    }
    reportError("Undefined identifier '" + ident->name + "'");
    return nullptr;
}

ClassInfoPtr SemanticAnalyzer::checkMemberAccess(MemberAccess* access) {
    auto objType = checkExpression(access->object.get());
    if (!objType) return nullptr;
    const auto& memberName = access->member->name;
    auto field = objType->findField(memberName);
    if (field) return field->type;
    auto method = objType->findMethodByName(memberName);
    if (method)
        return method->returnType ? method->returnType : voidType_;
    reportError("Class '" + objType->name + "' has no member '" + memberName + "'");
    return nullptr;
}

ClassInfoPtr SemanticAnalyzer::checkFunctionCall(FunctionCall* call) {
    std::vector<ClassInfoPtr> argTypes;
    for (auto& arg : call->arguments)
        argTypes.push_back(checkExpression(arg.get()));

    if (auto* ma = dynamic_cast<MemberAccess*>(call->callee.get())) {
        auto objType = checkExpression(ma->object.get());
        if (!objType) return nullptr;
        const auto& methodName = ma->member->name;
        auto method = objType->findMethod(methodName, argTypes);
        if (method) {
            call->callee->resolvedType = method->returnType;
            return method->returnType ? method->returnType : voidType_;
        }
        auto anyMethod = objType->findMethodByName(methodName);
        if (anyMethod) {
            std::ostringstream oss;
            oss << "No matching overload for method '" << methodName
                << "' in class '" << objType->name << "' with argument types (";
            for (size_t i = 0; i < argTypes.size(); ++i) {
                if (i > 0) oss << ", ";
                oss << (argTypes[i] ? argTypes[i]->name : "unknown");
            }
            oss << ")";
            reportError(oss.str());
            return anyMethod->returnType;
        }
        reportError("Class '" + objType->name + "' has no method '" + methodName + "'");
        return nullptr;
    }

    if (auto* ident = dynamic_cast<Identifier*>(call->callee.get())) {
        if (ident->name == "base") {
            if (currentClass_ && currentClass_->baseClass) {
                auto ctor = currentClass_->baseClass->findConstructor(argTypes);
                if (ctor) return currentClass_->baseClass;
                reportError("No matching constructor in base class '" +
                            currentClass_->baseClass->name + "'");
            } else {
                reportError("'base' call but class has no base class");
            }
            return nullptr;
        }
        if (currentClass_) {
            auto method = currentClass_->findMethod(ident->name, argTypes);
            if (method) {
                ident->resolvedType = method->returnType;
                return method->returnType ? method->returnType : voidType_;
            }
        }
        reportError("Cannot resolve call to '" + ident->name + "'");
        return nullptr;
    }

    auto calleeType = checkExpression(call->callee.get());
    return calleeType;
}

ClassInfoPtr SemanticAnalyzer::checkConstructorInvocation(ConstructorInvocation* ctor) {
    auto classInfo = resolveClassName(ctor->className.get());
    if (!classInfo) {
        reportError("Unknown class '" + ctor->className->name + "' in constructor invocation");
        return nullptr;
    }
    std::vector<ClassInfoPtr> argTypes;
    for (auto& arg : ctor->arguments)
        argTypes.push_back(checkExpression(arg.get()));
    if (!classInfo->constructors.empty()) {
        auto ctorInfo = classInfo->findConstructor(argTypes);
        if (!ctorInfo) {
            std::ostringstream oss;
            oss << "No matching constructor for class '" << classInfo->name
                << "' with argument types (";
            for (size_t i = 0; i < argTypes.size(); ++i) {
                if (i > 0) oss << ", ";
                oss << (argTypes[i] ? argTypes[i]->name : "unknown");
            }
            oss << ")";
            reportError(oss.str());
        }
    }
    return classInfo;
}

}
