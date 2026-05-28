#include "../include/semantic.h"
#include <sstream>

namespace olang {

void SemanticAnalyzer::pass1RegisterClasses(Program* program) {
    for (const auto& classDecl : program->classes) {
        const auto& name = classDecl->name->name;
        if (classTable_.count(name)) {
            reportError("Class '" + name + "' is already defined");
            continue;
        }
        addClass(name, nullptr, false);
    }
}

void SemanticAnalyzer::pass2ResolveInheritance(Program* program) {
    for (const auto& classDecl : program->classes) {
        const auto& name = classDecl->name->name;
        auto classInfo = resolveType(name);
        if (!classInfo) continue;

        if (classDecl->baseClass) {
            const auto& baseName = classDecl->baseClass->name;
            auto baseInfo = resolveType(baseName);
            if (!baseInfo) {
                reportError("Class '" + name + "' extends unknown class '" + baseName + "'");
                continue;
            }
            if (baseInfo == classInfo) {
                reportError("Class '" + name + "' cannot extend itself");
                continue;
            }
            classInfo->baseClass = baseInfo;
        }
    }

    for (const auto& classDecl : program->classes) {
        const auto& name = classDecl->name->name;
        auto classInfo = resolveType(name);
        if (!classInfo) continue;

        auto current = classInfo->baseClass;
        while (current) {
            if (current == classInfo) {
                reportError("Circular inheritance detected involving class '" + name + "'");
                classInfo->baseClass = nullptr;
                break;
            }
            current = current->baseClass;
        }
    }
}

void SemanticAnalyzer::pass3RegisterMembers(Program* program) {
    for (const auto& classDecl : program->classes) {
        const auto& name = classDecl->name->name;
        auto classInfo = resolveType(name);
        if (!classInfo) continue;

        for (const auto& member : classDecl->members) {
            if (auto* varDecl = dynamic_cast<VariableDeclaration*>(member.get())) {
                ClassInfoPtr fieldType;
                auto* init = varDecl->initializer.get();
                if (auto* ci = dynamic_cast<ConstructorInvocation*>(init))
                    fieldType = resolveClassName(ci->className.get());
                else if (dynamic_cast<IntegerLiteral*>(init))
                    fieldType = integerClass_;
                else if (dynamic_cast<RealLiteral*>(init))
                    fieldType = realClass_;
                else if (dynamic_cast<BooleanLiteral*>(init))
                    fieldType = booleanClass_;
                else if (dynamic_cast<StringLiteral*>(init))
                    fieldType = stringClass_;
                if (!fieldType) fieldType = classClass_;
                addField(classInfo, varDecl->name->name, fieldType);

            } else if (auto* methodDecl = dynamic_cast<MethodDeclaration*>(member.get())) {
                MethodInfo method;
                method.name = methodDecl->name->name;
                method.ownerClass = classInfo;
                method.isForward = methodDecl->isForward;
                for (const auto& param : methodDecl->parameters) {
                    ParamInfo pi;
                    pi.name = param->name->name;
                    pi.type = resolveClassName(param->type.get());
                    if (!pi.type)
                        reportError("Unknown type '" + param->type->name +
                                    "' for parameter '" + pi.name +
                                    "' in method '" + method.name +
                                    "' of class '" + name + "'");
                    method.parameters.push_back(std::move(pi));
                }
                if (methodDecl->returnType) {
                    method.returnType = resolveClassName(methodDecl->returnType.get());
                    if (!method.returnType)
                        reportError("Unknown return type '" + methodDecl->returnType->name +
                                    "' for method '" + method.name +
                                    "' of class '" + name + "'");
                }
                classInfo->methods[method.name].push_back(std::move(method));

            } else if (auto* ctorDecl = dynamic_cast<ConstructorDeclaration*>(member.get())) {
                ConstructorInfo ctor;
                ctor.ownerClass = classInfo;
                for (const auto& param : ctorDecl->parameters) {
                    ParamInfo pi;
                    pi.name = param->name->name;
                    pi.type = resolveClassName(param->type.get());
                    if (!pi.type)
                        reportError("Unknown type '" + param->type->name +
                                    "' for parameter '" + pi.name +
                                    "' in constructor of class '" + name + "'");
                    ctor.parameters.push_back(std::move(pi));
                }
                classInfo->constructors.push_back(std::move(ctor));
            }
        }

        for (auto& [methodName, overloads] : classInfo->methods) {
            std::vector<MethodInfo*> forwards, definitions;
            for (auto& m : overloads) {
                if (m.isForward) forwards.push_back(&m);
                else definitions.push_back(&m);
            }
            for (auto* fwd : forwards) {
                bool found = false;
                for (auto* def : definitions) {
                    if (fwd->parameters.size() != def->parameters.size()) continue;
                    bool match = true;
                    for (size_t i = 0; i < fwd->parameters.size(); ++i) {
                        if (fwd->parameters[i].type != def->parameters[i].type) {
                            match = false;
                            break;
                        }
                    }
                    if (match) { found = true; break; }
                }
                if (!found)
                    reportError("Forward declaration of method '" + methodName +
                                "' in class '" + name + "' has no matching definition");
            }
        }
    }
}

void SemanticAnalyzer::pass4CheckBodies(Program* program) {
    for (const auto& classDecl : program->classes) {
        const auto& name = classDecl->name->name;
        auto classInfo = resolveType(name);
        if (!classInfo) continue;

        currentClass_ = classInfo;

        for (const auto& member : classDecl->members) {
            if (auto* methodDecl = dynamic_cast<MethodDeclaration*>(member.get())) {
                if (!methodDecl->isForward)
                    checkMethodBody(methodDecl, classInfo);
            } else if (auto* ctorDecl = dynamic_cast<ConstructorDeclaration*>(member.get())) {
                checkConstructorBody(ctorDecl, classInfo);
            }
        }

        currentClass_ = nullptr;
    }
}

}
