#pragma once

#include "token.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <stdexcept>

namespace olang {

class LexerError : public std::runtime_error {
private:
    size_t line_;
    size_t column_;
    
public:
    LexerError(const std::string& message, size_t line, size_t column)
        : std::runtime_error(message), line_(line), column_(column) {}
    
    size_t line() const { return line_; }
    size_t column() const { return column_; }
};

class Lexer {
private:
    std::string source_;
    size_t current_;
    size_t line_;
    size_t column_;
    
    static const std::unordered_map<std::string, TokenType> keywords_;
    
public:
    explicit Lexer(std::string source);
    
    std::vector<Token> tokenize();
    Token nextToken();
    
private:
    char peek() const;
    char peekNext() const;
    char advance();
    bool match(char expected);
    bool isAtEnd() const;
    
    void skipWhitespace();
    void skipLineComment();
    void skipBlockComment();
    
    Token makeToken(TokenType type, const std::string& lexeme);
    Token makeToken(TokenType type, const std::string& lexeme, TokenValue value);
    
    Token identifier();
    Token number();
    Token string();
    
    bool isAlpha(char c) const;
    bool isDigit(char c) const;
    bool isAlphaNumeric(char c) const;
};

}