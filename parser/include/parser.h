#pragma once

#include "ast.h"
#include "../../lexer/include/token.h"
#include <vector>
#include <stdexcept>
#include <memory>

namespace olang {

class ParserError : public std::runtime_error {
private:
    size_t line_;
    size_t column_;
    
public:
    ParserError(const std::string& message, size_t line, size_t column)
        : std::runtime_error(message), line_(line), column_(column) {}
    
    size_t line() const { return line_; }
    size_t column() const { return column_; }
};

class Parser {
private:
    std::vector<Token> tokens_;
    size_t current_;

    std::vector<ParserError> errors_;

public:
    explicit Parser(std::vector<Token> tokens);

    std::unique_ptr<Program> parse();

    bool hadErrors() const { return !errors_.empty(); }
    const std::vector<ParserError>& errors() const { return errors_; }

private:
    Token peek() const;
    Token previous() const;
    Token advance();
    bool check(TokenType type) const;
    bool match(TokenType type);
    bool match(const std::vector<TokenType>& types);
    bool isAtEnd() const;
    
    Token consume(TokenType type, const std::string& message);
    
    [[noreturn]] void error(const std::string& message);
    [[noreturn]] void error(const std::string& message, const Token& token);

    void recoverToClass();
    void synchronizeMember();
    
    std::unique_ptr<ClassDeclaration> parseClassDeclaration();
    std::unique_ptr<ClassName> parseClassName();
    std::unique_ptr<MemberDeclaration> parseMemberDeclaration();
    std::unique_ptr<VariableDeclaration> parseVariableDeclaration();
    std::unique_ptr<MethodDeclaration> parseMethodDeclaration();
    std::unique_ptr<ConstructorDeclaration> parseConstructorDeclaration();
    
    std::vector<std::unique_ptr<ParameterDeclaration>> parseParameters();
    std::unique_ptr<ParameterDeclaration> parseParameterDeclaration();
    
    std::vector<std::unique_ptr<ASTNode>> parseBody();
    
    std::unique_ptr<Statement> parseStatement();
    std::unique_ptr<Assignment> parseAssignment();
    std::unique_ptr<WhileLoop> parseWhileLoop();
    std::unique_ptr<IfStatement> parseIfStatement();
    std::unique_ptr<ReturnStatement> parseReturnStatement();
    
    std::unique_ptr<Expression> parseExpression();
    std::unique_ptr<Expression> parsePrimary();
    std::unique_ptr<Expression> parsePostfix(std::unique_ptr<Expression> expr);
    std::unique_ptr<ConstructorInvocation> parseConstructorInvocation();
    std::vector<std::unique_ptr<Expression>> parseArguments();
};

}