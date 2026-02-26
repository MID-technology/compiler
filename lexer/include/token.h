#pragma once

#include <string>
#include <variant>
#include <ostream>

namespace olang {

enum class TokenType {
    CLASS,
    IS,
    END,
    EXTENDS,
    VAR,
    METHOD,
    THIS,
    IF,
    THEN,
    ELSE,
    WHILE,
    LOOP,
    RETURN,
    TRUE,
    FALSE,
    BASE,
    
    IDENTIFIER,
    
    INTEGER_LITERAL,
    REAL_LITERAL,
    STRING_LITERAL,
    
    COLON,
    ASSIGN,
    EQUAL,
    ARROW,
    DOT,
    COMMA,
    LPAREN,
    RPAREN,
    LBRACKET,
    RBRACKET,
    LBRACE,
    RBRACE,
    LANGLE,
    RANGLE,
    
    END_OF_FILE,
    INVALID
};

using TokenValue = std::variant<std::monostate, int64_t, double, bool, std::string>;

struct Token {
    TokenType type;
    std::string lexeme;
    TokenValue value;
    size_t line;
    size_t column;
    
    Token(TokenType type, std::string lexeme, size_t line, size_t column)
        : type(type), lexeme(std::move(lexeme)), value(std::monostate{}), line(line), column(column) {}
    
    Token(TokenType type, std::string lexeme, TokenValue value, size_t line, size_t column)
        : type(type), lexeme(std::move(lexeme)), value(std::move(value)), line(line), column(column) {}
};

std::string tokenTypeToString(TokenType type);
std::ostream& operator<<(std::ostream& os, const Token& token);

}