#pragma once

#include "class_info.h"
#include <string>
#include <unordered_map>
#include <memory>
#include <vector>

namespace olang {

enum class SymbolKind {
    Variable,
    Parameter,
    Field
};

struct SymbolInfo {
    std::string name;
    ClassInfoPtr type;
    SymbolKind kind;
};

class Scope {
    std::shared_ptr<Scope> parent_;
    std::unordered_map<std::string, SymbolInfo> symbols_;

public:
    explicit Scope(std::shared_ptr<Scope> parent = nullptr)
        : parent_(std::move(parent)) {}

    bool define(const std::string& name, ClassInfoPtr type, SymbolKind kind) {
        if (symbols_.count(name)) return false;
        symbols_[name] = {name, std::move(type), kind};
        return true;
    }

    const SymbolInfo* lookup(const std::string& name) const {
        auto it = symbols_.find(name);
        if (it != symbols_.end()) return &it->second;
        if (parent_) return parent_->lookup(name);
        return nullptr;
    }

    const SymbolInfo* lookupLocal(const std::string& name) const {
        auto it = symbols_.find(name);
        if (it != symbols_.end()) return &it->second;
        return nullptr;
    }

    std::shared_ptr<Scope> parent() const { return parent_; }
};

class SymbolTable {
    std::shared_ptr<Scope> current_;

public:
    void pushScope() {
        current_ = std::make_shared<Scope>(current_);
    }

    void popScope() {
        if (current_) {
            current_ = current_->parent();
        }
    }

    bool define(const std::string& name, ClassInfoPtr type, SymbolKind kind) {
        if (!current_) return false;
        return current_->define(name, std::move(type), kind);
    }

    const SymbolInfo* lookup(const std::string& name) const {
        if (!current_) return nullptr;
        return current_->lookup(name);
    }
};

}
