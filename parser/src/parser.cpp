#include "../include/parser.h"
#include <sstream>

namespace olang {

Parser::Parser(std::vector<Token> tokens)
    : tokens_(std::move(tokens)), current_(0) {}

std::unique_ptr<Program> Parser::parse() {
    auto program = std::make_unique<Program>();

    while (!isAtEnd()) {
        try {
            program->classes.push_back(parseClassDeclaration());
        } catch (const ParserError& e) {
            errors_.push_back(e);
            recoverToClass();
        }
    }

    return program;
}

Token Parser::peek() const {
    return tokens_[current_];
}

Token Parser::previous() const {
    return tokens_[current_ - 1];
}

Token Parser::advance() {
    if (!isAtEnd()) {
        current_++;
    }
    return previous();
}

bool Parser::check(TokenType type) const {
    if (isAtEnd()) return false;
    return peek().type == type;
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

bool Parser::match(const std::vector<TokenType>& types) {
    for (TokenType type : types) {
        if (check(type)) {
            advance();
            return true;
        }
    }
    return false;
}

bool Parser::isAtEnd() const {
    return peek().type == TokenType::END_OF_FILE;
}

Token Parser::consume(TokenType type, const std::string& message) {
    if (check(type)) return advance();
    error(message, peek());
}

void Parser::error(const std::string& message) {
    Token token = peek();
    throw ParserError(message, token.line, token.column);
}

void Parser::error(const std::string& message, const Token& token) {
    throw ParserError(message, token.line, token.column);
}

void Parser::recoverToClass() {
    while (!isAtEnd() && !check(TokenType::CLASS)) {
        advance();
    }
}

void Parser::synchronizeMember() {
    while (!isAtEnd()) {
        switch (peek().type) {
            case TokenType::VAR:
            case TokenType::METHOD:
            case TokenType::THIS:
            case TokenType::END:
            case TokenType::CLASS:
                return;
            default:
                advance();
        }
    }
}

std::unique_ptr<ClassDeclaration> Parser::parseClassDeclaration() {
    consume(TokenType::CLASS, "Expected 'class' keyword");
    
    auto className = parseClassName();
    auto classDecl = std::make_unique<ClassDeclaration>(std::move(className));
    
    if (match(TokenType::EXTENDS)) {
        classDecl->baseClass = parseClassName();
    }
    
    consume(TokenType::IS, "Expected 'is' keyword after class name");

    while (!check(TokenType::END) && !check(TokenType::CLASS) && !isAtEnd()) {
        try {
            classDecl->members.push_back(parseMemberDeclaration());
        } catch (const ParserError& e) {
            errors_.push_back(e);
            synchronizeMember();
        }
    }

    consume(TokenType::END, "Expected 'end' keyword to close class declaration");
    
    return classDecl;
}

std::unique_ptr<ClassName> Parser::parseClassName() {
    Token nameToken = consume(TokenType::IDENTIFIER, "Expected class name");
    auto className = std::make_unique<ClassName>(nameToken.lexeme);
    
    if (match(TokenType::LBRACKET)) {
        do {
            className->typeParameters.push_back(parseClassName());
        } while (match(TokenType::COMMA));
        
        consume(TokenType::RBRACKET, "Expected ']' after type parameters");
    }
    
    return className;
}

std::unique_ptr<MemberDeclaration> Parser::parseMemberDeclaration() {
    if (check(TokenType::VAR)) {
        return parseVariableDeclaration();
    } else if (check(TokenType::METHOD)) {
        return parseMethodDeclaration();
    } else if (check(TokenType::THIS)) {
        return parseConstructorDeclaration();
    } else {
        error("Expected member declaration (var, method, or this)");
    }
}

std::unique_ptr<VariableDeclaration> Parser::parseVariableDeclaration() {
    consume(TokenType::VAR, "Expected 'var' keyword");
    
    Token nameToken = consume(TokenType::IDENTIFIER, "Expected variable name");
    auto name = std::make_unique<Identifier>(nameToken.lexeme);
    
    if (!match(TokenType::COLON) && !match(TokenType::EQUAL)) {
        error("Expected ':' or '=' after variable name");
    }
    
    auto initializer = parseExpression();
    
    return std::make_unique<VariableDeclaration>(std::move(name), std::move(initializer));
}

std::unique_ptr<MethodDeclaration> Parser::parseMethodDeclaration() {
    consume(TokenType::METHOD, "Expected 'method' keyword");
    
    Token nameToken = consume(TokenType::IDENTIFIER, "Expected method name");
    auto method = std::make_unique<MethodDeclaration>(std::make_unique<Identifier>(nameToken.lexeme));
    
    if (check(TokenType::LPAREN)) {
        method->parameters = parseParameters();
    }
    
    if (match(TokenType::COLON)) {
        Token returnTypeToken = consume(TokenType::IDENTIFIER, "Expected return type");
        method->returnType = std::make_unique<ClassName>(returnTypeToken.lexeme);
    }
    
    if (match(TokenType::IS)) {
        method->body = parseBody();
        consume(TokenType::END, "Expected 'end' keyword to close method body");
        method->isForward = false;
    } else if (match(TokenType::ARROW)) {
        auto expr = parseExpression();
        auto returnStmt = std::make_unique<ReturnStatement>(std::move(expr));
        method->body.push_back(std::move(returnStmt));
        method->isForward = false;
    } else {
        method->isForward = true;
    }
    
    return method;
}

std::unique_ptr<ConstructorDeclaration> Parser::parseConstructorDeclaration() {
    consume(TokenType::THIS, "Expected 'this' keyword");
    
    auto constructor = std::make_unique<ConstructorDeclaration>();
    
    if (check(TokenType::LPAREN)) {
        constructor->parameters = parseParameters();
    }
    
    consume(TokenType::IS, "Expected 'is' keyword after constructor parameters");
    
    constructor->body = parseBody();
    
    consume(TokenType::END, "Expected 'end' keyword to close constructor body");
    
    return constructor;
}

std::vector<std::unique_ptr<ParameterDeclaration>> Parser::parseParameters() {
    consume(TokenType::LPAREN, "Expected '(' to start parameters");
    
    std::vector<std::unique_ptr<ParameterDeclaration>> parameters;
    
    if (!check(TokenType::RPAREN)) {
        do {
            parameters.push_back(parseParameterDeclaration());
        } while (match(TokenType::COMMA));
    }
    
    consume(TokenType::RPAREN, "Expected ')' after parameters");
    
    return parameters;
}

std::unique_ptr<ParameterDeclaration> Parser::parseParameterDeclaration() {
    Token nameToken = consume(TokenType::IDENTIFIER, "Expected parameter name");
    auto name = std::make_unique<Identifier>(nameToken.lexeme);
    
    consume(TokenType::COLON, "Expected ':' after parameter name");
    
    auto type = parseClassName();
    
    return std::make_unique<ParameterDeclaration>(std::move(name), std::move(type));
}

std::vector<std::unique_ptr<ASTNode>> Parser::parseBody() {
    std::vector<std::unique_ptr<ASTNode>> body;
    
    while (!check(TokenType::END) && !isAtEnd()) {
        if (check(TokenType::VAR)) {
            body.push_back(parseVariableDeclaration());
        } else {
            body.push_back(parseStatement());
        }
    }
    
    return body;
}

std::unique_ptr<Statement> Parser::parseStatement() {
    if (check(TokenType::IDENTIFIER) || check(TokenType::THIS) || check(TokenType::BASE)) {
        auto expr = parseExpression();
        
        if (check(TokenType::ASSIGN)) {
            advance();
            auto value = parseExpression();
            return std::make_unique<Assignment>(std::move(expr), std::move(value));
        } else {
            return std::make_unique<ReturnStatement>(std::move(expr));
        }
    } else if (check(TokenType::WHILE)) {
        return parseWhileLoop();
    } else if (check(TokenType::IF)) {
        return parseIfStatement();
    } else if (check(TokenType::RETURN)) {
        return parseReturnStatement();
    } else {
        error("Expected statement");
    }
}

std::unique_ptr<Assignment> Parser::parseAssignment() {
    Token nameToken = consume(TokenType::IDENTIFIER, "Expected variable name");
    auto name = std::make_unique<Identifier>(nameToken.lexeme);
    
    consume(TokenType::ASSIGN, "Expected ':=' in assignment");
    
    auto value = parseExpression();
    
    return std::make_unique<Assignment>(std::move(name), std::move(value));
}

std::unique_ptr<WhileLoop> Parser::parseWhileLoop() {
    consume(TokenType::WHILE, "Expected 'while' keyword");
    
    auto condition = parseExpression();
    auto whileLoop = std::make_unique<WhileLoop>(std::move(condition));
    
    consume(TokenType::LOOP, "Expected 'loop' keyword after while condition");
    
    whileLoop->body = parseBody();
    
    consume(TokenType::END, "Expected 'end' keyword to close while loop");
    
    return whileLoop;
}

std::unique_ptr<IfStatement> Parser::parseIfStatement() {
    consume(TokenType::IF, "Expected 'if' keyword");
    
    auto condition = parseExpression();
    auto ifStmt = std::make_unique<IfStatement>(std::move(condition));
    
    consume(TokenType::THEN, "Expected 'then' keyword after if condition");
    
    while (!check(TokenType::ELSE) && !check(TokenType::END) && !isAtEnd()) {
        if (check(TokenType::VAR)) {
            ifStmt->thenBody.push_back(parseVariableDeclaration());
        } else {
            ifStmt->thenBody.push_back(parseStatement());
        }
    }
    
    if (match(TokenType::ELSE)) {
        while (!check(TokenType::END) && !isAtEnd()) {
            if (check(TokenType::VAR)) {
                ifStmt->elseBody.push_back(parseVariableDeclaration());
            } else {
                ifStmt->elseBody.push_back(parseStatement());
            }
        }
    }
    
    consume(TokenType::END, "Expected 'end' keyword to close if statement");
    
    return ifStmt;
}

std::unique_ptr<ReturnStatement> Parser::parseReturnStatement() {
    consume(TokenType::RETURN, "Expected 'return' keyword");
    
    if (check(TokenType::END) || isAtEnd()) {
        return std::make_unique<ReturnStatement>();
    }
    
    auto value = parseExpression();
    return std::make_unique<ReturnStatement>(std::move(value));
}

std::unique_ptr<Expression> Parser::parseExpression() {
    auto expr = parsePrimary();
    return parsePostfix(std::move(expr));
}

std::unique_ptr<Expression> Parser::parsePrimary() {
    if (match(TokenType::INTEGER_LITERAL)) {
        int64_t value = std::get<int64_t>(previous().value);
        return std::make_unique<IntegerLiteral>(value);
    }
    
    if (match(TokenType::REAL_LITERAL)) {
        double value = std::get<double>(previous().value);
        return std::make_unique<RealLiteral>(value);
    }
    
    if (match(TokenType::TRUE)) {
        return std::make_unique<BooleanLiteral>(true);
    }
    
    if (match(TokenType::FALSE)) {
        return std::make_unique<BooleanLiteral>(false);
    }
    
    if (match(TokenType::STRING_LITERAL)) {
        std::string value = std::get<std::string>(previous().value);
        return std::make_unique<StringLiteral>(value);
    }
    
    if (match(TokenType::THIS)) {
        return std::make_unique<ThisExpression>();
    }
    
    if (match(TokenType::BASE)) {
        if (check(TokenType::LPAREN)) {
            auto arguments = parseArguments();
            auto baseCall = std::make_unique<FunctionCall>(std::make_unique<Identifier>("base"));
            baseCall->arguments = std::move(arguments);
            return baseCall;
        }
        return std::make_unique<Identifier>("base");
    }
    
    if (check(TokenType::IDENTIFIER)) {
        size_t savedPos = current_;
        auto className = parseClassName();
        
        if (check(TokenType::LPAREN)) {
            return parseConstructorInvocation();
        } else {
            current_ = savedPos;
            Token nameToken = advance();
            return std::make_unique<Identifier>(nameToken.lexeme);
        }
    }
    
    if (match(TokenType::LPAREN)) {
        auto expr = parseExpression();
        consume(TokenType::RPAREN, "Expected ')' after expression");
        return expr;
    }
    
    error("Expected expression");
}

std::unique_ptr<Expression> Parser::parsePostfix(std::unique_ptr<Expression> expr) {
    while (true) {
        if (match(TokenType::DOT)) {
            Token memberToken = consume(TokenType::IDENTIFIER, "Expected member name after '.'");
            auto member = std::make_unique<Identifier>(memberToken.lexeme);
            expr = std::make_unique<MemberAccess>(std::move(expr), std::move(member));
        } else if (check(TokenType::LPAREN)) {
            auto arguments = parseArguments();
            auto call = std::make_unique<FunctionCall>(std::move(expr));
            call->arguments = std::move(arguments);
            expr = std::move(call);
        } else {
            break;
        }
    }
    
    return expr;
}

std::unique_ptr<ConstructorInvocation> Parser::parseConstructorInvocation() {
    current_--;
    auto className = parseClassName();
    auto constructor = std::make_unique<ConstructorInvocation>(std::move(className));
    
    if (check(TokenType::LPAREN)) {
        constructor->arguments = parseArguments();
    }
    
    return constructor;
}

std::vector<std::unique_ptr<Expression>> Parser::parseArguments() {
    consume(TokenType::LPAREN, "Expected '(' to start arguments");
    
    std::vector<std::unique_ptr<Expression>> arguments;
    
    if (!check(TokenType::RPAREN)) {
        do {
            arguments.push_back(parseExpression());
        } while (match(TokenType::COMMA));
    }
    
    consume(TokenType::RPAREN, "Expected ')' after arguments");
    
    return arguments;
}

}