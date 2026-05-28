#include "../include/semantic.h"
#include <sstream>

namespace olang {

SemanticAnalyzer::SemanticAnalyzer() {
    registerBuiltins();
}

void SemanticAnalyzer::analyze(Program* program) {
    pass1RegisterClasses(program);
    if (hasErrors()) return;
    pass2ResolveInheritance(program);
    if (hasErrors()) return;
    pass3RegisterMembers(program);
    if (hasErrors()) return;
    pass4CheckBodies(program);
}

ClassInfoPtr SemanticAnalyzer::addClass(const std::string& name, ClassInfoPtr base, bool builtin) {
    auto info = std::make_shared<ClassInfo>();
    info->name = name;
    info->baseClass = std::move(base);
    info->isBuiltin = builtin;
    classTable_[name] = info;
    return info;
}

void SemanticAnalyzer::addMethod(const ClassInfoPtr& cls, const std::string& name,
                                  std::vector<ParamInfo> params, ClassInfoPtr returnType) {
    MethodInfo method;
    method.name = name;
    method.parameters = std::move(params);
    method.returnType = std::move(returnType);
    method.ownerClass = cls;
    method.isForward = false;
    cls->methods[name].push_back(std::move(method));
}

void SemanticAnalyzer::addField(const ClassInfoPtr& cls, const std::string& name, ClassInfoPtr type) {
    FieldInfo field;
    field.name = name;
    field.type = std::move(type);
    field.ownerClass = cls;
    cls->fields.push_back(std::move(field));
}

void SemanticAnalyzer::addConstructor(const ClassInfoPtr& cls, std::vector<ParamInfo> params) {
    ConstructorInfo ctor;
    ctor.parameters = std::move(params);
    ctor.ownerClass = cls;
    cls->constructors.push_back(std::move(ctor));
}

ClassInfoPtr SemanticAnalyzer::resolveClassName(const ClassName* name) {
    if (!name) return nullptr;
    return resolveType(name->name);
}

ClassInfoPtr SemanticAnalyzer::resolveType(const std::string& name) {
    auto it = classTable_.find(name);
    if (it != classTable_.end()) return it->second;
    return nullptr;
}

void SemanticAnalyzer::reportError(const std::string& message) {
    errors_.push_back(message);
}

bool SemanticAnalyzer::isAssignableFrom(const ClassInfoPtr& target, const ClassInfoPtr& source) {
    if (!target || !source) return true;
    return source->isSubclassOf(target);
}

}
