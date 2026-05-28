#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <optional>

namespace olang {

struct ClassInfo;
using ClassInfoPtr = std::shared_ptr<ClassInfo>;

struct FieldInfo {
    std::string name;
    ClassInfoPtr type;
    std::weak_ptr<ClassInfo> ownerClass;
};

struct ParamInfo {
    std::string name;
    ClassInfoPtr type;
};

struct MethodInfo {
    std::string name;
    std::vector<ParamInfo> parameters;
    ClassInfoPtr returnType;
    std::weak_ptr<ClassInfo> ownerClass;
    bool isForward = false;
};

struct ConstructorInfo {
    std::vector<ParamInfo> parameters;
    std::weak_ptr<ClassInfo> ownerClass;
};

struct ClassInfo : std::enable_shared_from_this<ClassInfo> {
    std::string name;
    ClassInfoPtr baseClass;
    bool isBuiltin = false;

    std::vector<FieldInfo> fields;
    std::unordered_map<std::string, std::vector<MethodInfo>> methods;
    std::vector<ConstructorInfo> constructors;

    bool isSubclassOf(const ClassInfoPtr& other) const {
        if (!other) return false;
        if (this == other.get()) return true;
        if (baseClass) return baseClass->isSubclassOf(other);
        return false;
    }

    const MethodInfo* findMethod(const std::string& methodName,
                                  const std::vector<ClassInfoPtr>& argTypes) const {
        auto it = methods.find(methodName);
        if (it != methods.end()) {
            for (const auto& method : it->second) {
                if (method.isForward) continue;
                if (method.parameters.size() != argTypes.size()) continue;
                bool match = true;
                for (size_t i = 0; i < argTypes.size(); ++i) {
                    if (!argTypes[i] || !method.parameters[i].type) {
                        match = false;
                        break;
                    }
                    if (!argTypes[i]->isSubclassOf(method.parameters[i].type)) {
                        match = false;
                        break;
                    }
                }
                if (match) return &method;
            }
        }
        if (baseClass) return baseClass->findMethod(methodName, argTypes);
        return nullptr;
    }

    const MethodInfo* findMethodByName(const std::string& methodName) const {
        auto it = methods.find(methodName);
        if (it != methods.end()) {
            for (const auto& method : it->second) {
                if (!method.isForward) return &method;
            }
        }
        if (baseClass) return baseClass->findMethodByName(methodName);
        return nullptr;
    }

    const FieldInfo* findField(const std::string& fieldName) const {
        for (const auto& field : fields) {
            if (field.name == fieldName) return &field;
        }
        if (baseClass) return baseClass->findField(fieldName);
        return nullptr;
    }

    const ConstructorInfo* findConstructor(const std::vector<ClassInfoPtr>& argTypes) const {
        for (const auto& ctor : constructors) {
            if (ctor.parameters.size() != argTypes.size()) continue;
            bool match = true;
            for (size_t i = 0; i < argTypes.size(); ++i) {
                if (!argTypes[i] || !ctor.parameters[i].type) {
                    match = false;
                    break;
                }
                if (!argTypes[i]->isSubclassOf(ctor.parameters[i].type)) {
                    match = false;
                    break;
                }
            }
            if (match) return &ctor;
        }
        return nullptr;
    }
};

}
