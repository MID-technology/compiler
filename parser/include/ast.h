#pragma once

#include <memory>
#include <string>
#include <vector>
#include <variant>

namespace olang {

struct ClassInfo;
using ClassInfoPtr = std::shared_ptr<ClassInfo>;

struct ASTNode {
    virtual ~ASTNode() = default;
};

struct Expression : ASTNode {
    ClassInfoPtr resolvedType;
};
struct Statement : ASTNode {};
struct MemberDeclaration : ASTNode {};

struct Identifier : Expression {
    std::string name;
    explicit Identifier(std::string name) : name(std::move(name)) {}
};

struct ClassName : ASTNode {
    std::string name;
    std::vector<std::unique_ptr<ClassName>> typeParameters;
    
    explicit ClassName(std::string name) : name(std::move(name)) {}
};

struct IntegerLiteral : Expression {
    int64_t value;
    explicit IntegerLiteral(int64_t value) : value(value) {}
};

struct RealLiteral : Expression {
    double value;
    explicit RealLiteral(double value) : value(value) {}
};

struct BooleanLiteral : Expression {
    bool value;
    explicit BooleanLiteral(bool value) : value(value) {}
};

struct StringLiteral : Expression {
    std::string value;
    explicit StringLiteral(std::string value) : value(std::move(value)) {}
};

struct ThisExpression : Expression {};

struct ConstructorInvocation : Expression {
    std::unique_ptr<ClassName> className;
    std::vector<std::unique_ptr<Expression>> arguments;
    
    explicit ConstructorInvocation(std::unique_ptr<ClassName> className)
        : className(std::move(className)) {}
};

struct FunctionCall : Expression {
    std::unique_ptr<Expression> callee;
    std::vector<std::unique_ptr<Expression>> arguments;
    
    explicit FunctionCall(std::unique_ptr<Expression> callee)
        : callee(std::move(callee)) {}
};

struct MemberAccess : Expression {
    std::unique_ptr<Expression> object;
    std::unique_ptr<Identifier> member;
    
    MemberAccess(std::unique_ptr<Expression> object, std::unique_ptr<Identifier> member)
        : object(std::move(object)), member(std::move(member)) {}
};

struct Assignment : Statement {
    std::unique_ptr<Expression> target;
    std::unique_ptr<Expression> value;
    
    Assignment(std::unique_ptr<Expression> target, std::unique_ptr<Expression> value)
        : target(std::move(target)), value(std::move(value)) {}
};

struct WhileLoop : Statement {
    std::unique_ptr<Expression> condition;
    std::vector<std::unique_ptr<ASTNode>> body;
    
    explicit WhileLoop(std::unique_ptr<Expression> condition)
        : condition(std::move(condition)) {}
};

struct IfStatement : Statement {
    std::unique_ptr<Expression> condition;
    std::vector<std::unique_ptr<ASTNode>> thenBody;
    std::vector<std::unique_ptr<ASTNode>> elseBody;
    
    explicit IfStatement(std::unique_ptr<Expression> condition)
        : condition(std::move(condition)) {}
};

struct ReturnStatement : Statement {
    std::unique_ptr<Expression> value;
    
    ReturnStatement() = default;
    explicit ReturnStatement(std::unique_ptr<Expression> value)
        : value(std::move(value)) {}
};

struct ParameterDeclaration : ASTNode {
    std::unique_ptr<Identifier> name;
    std::unique_ptr<ClassName> type;
    
    ParameterDeclaration(std::unique_ptr<Identifier> name, std::unique_ptr<ClassName> type)
        : name(std::move(name)), type(std::move(type)) {}
};

struct VariableDeclaration : MemberDeclaration {
    std::unique_ptr<Identifier> name;
    std::unique_ptr<Expression> initializer;
    
    VariableDeclaration(std::unique_ptr<Identifier> name, std::unique_ptr<Expression> initializer)
        : name(std::move(name)), initializer(std::move(initializer)) {}
};

struct MethodDeclaration : MemberDeclaration {
    std::unique_ptr<Identifier> name;
    std::vector<std::unique_ptr<ParameterDeclaration>> parameters;
    std::unique_ptr<ClassName> returnType;
    std::vector<std::unique_ptr<ASTNode>> body;
    bool isForward;
    
    explicit MethodDeclaration(std::unique_ptr<Identifier> name)
        : name(std::move(name)), isForward(false) {}
};

struct ConstructorDeclaration : MemberDeclaration {
    std::vector<std::unique_ptr<ParameterDeclaration>> parameters;
    std::vector<std::unique_ptr<ASTNode>> body;
};

struct ClassDeclaration : ASTNode {
    std::unique_ptr<ClassName> name;
    std::unique_ptr<ClassName> baseClass;
    std::vector<std::unique_ptr<MemberDeclaration>> members;
    
    explicit ClassDeclaration(std::unique_ptr<ClassName> name)
        : name(std::move(name)) {}
};

struct Program : ASTNode {
    std::vector<std::unique_ptr<ClassDeclaration>> classes;
};

}
