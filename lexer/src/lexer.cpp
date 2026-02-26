#include "lexer.h"
#include <cctype>
#include <sstream>

namespace olang {

const std::unordered_map<std::string, TokenType> Lexer::keywords_ = {
    {"class", TokenType::CLASS},
    {"is", TokenType::IS},
    {"end", TokenType::END},
    {"extends", TokenType::EXTENDS},
    {"var", TokenType::VAR},
    {"method", TokenType::METHOD},
    {"this", TokenType::THIS},
    {"if", TokenType::IF},
    {"then", TokenType::THEN},
    {"else", TokenType::ELSE},
    {"while", TokenType::WHILE},
    {"loop", TokenType::LOOP},
    {"return", TokenType::RETURN},
    {"true", TokenType::TRUE},
    {"false", TokenType::FALSE},
    {"base", TokenType::BASE}
};

Lexer::Lexer(std::string source)
    : source_(std::move(source)), current_(0), line_(1), column_(1) {}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    
    while (!isAtEnd()) {
        Token token = nextToken();
        if (token.type != TokenType::INVALID) {
            tokens.push_back(token);
        }
        if (token.type == TokenType::END_OF_FILE) {
            break;
        }
    }
    
    if (tokens.empty() || tokens.back().type != TokenType::END_OF_FILE) {
        tokens.push_back(makeToken(TokenType::END_OF_FILE, ""));
    }
    
    return tokens;
}

Token Lexer::nextToken() {
    skipWhitespace();
    
    if (isAtEnd()) {
        return makeToken(TokenType::END_OF_FILE, "");
    }
    
    size_t tokenLine = line_;
    size_t tokenColumn = column_;
    char c = advance();
    
    if (isAlpha(c)) {
        current_--;
        column_--;
        return identifier();
    }
    
    if (isDigit(c)) {
        current_--;
        column_--;
        return number();
    }
    
    switch (c) {
        case '"':
            return string();
        case ':':
            if (match('=')) {
                return makeToken(TokenType::ASSIGN, ":=");
            }
            return makeToken(TokenType::COLON, ":");
        case '=':
            if (match('>')) {
                return makeToken(TokenType::ARROW, "=>");
            }
            return makeToken(TokenType::EQUAL, "=");
        case '.':
            return makeToken(TokenType::DOT, ".");
        case ',':
            return makeToken(TokenType::COMMA, ",");
        case '(':
            return makeToken(TokenType::LPAREN, "(");
        case ')':
            return makeToken(TokenType::RPAREN, ")");
        case '[':
            return makeToken(TokenType::LBRACKET, "[");
        case ']':
            return makeToken(TokenType::RBRACKET, "]");
        case '{':
            return makeToken(TokenType::LBRACE, "{");
        case '}':
            return makeToken(TokenType::RBRACE, "}");
        case '<':
            return makeToken(TokenType::LANGLE, "<");
        case '>':
            return makeToken(TokenType::RANGLE, ">");
        case '/':
            if (match('/')) {
                skipLineComment();
                return nextToken();
            } else if (match('*')) {
                skipBlockComment();
                return nextToken();
            }
            break;
    }
    
    std::ostringstream oss;
    oss << "Unexpected character '" << c << "'";
    throw LexerError(oss.str(), tokenLine, tokenColumn);
}

char Lexer::peek() const {
    if (isAtEnd()) return '\0';
    return source_[current_];
}

char Lexer::peekNext() const {
    if (current_ + 1 >= source_.length()) return '\0';
    return source_[current_ + 1];
}

char Lexer::advance() {
    char c = source_[current_++];
    if (c == '\n') {
        line_++;
        column_ = 1;
    } else {
        column_++;
    }
    return c;
}

bool Lexer::match(char expected) {
    if (isAtEnd()) return false;
    if (source_[current_] != expected) return false;
    advance();
    return true;
}

bool Lexer::isAtEnd() const {
    return current_ >= source_.length();
}

void Lexer::skipWhitespace() {
    while (!isAtEnd()) {
        char c = peek();
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            advance();
        } else {
            break;
        }
    }
}

void Lexer::skipLineComment() {
    while (!isAtEnd() && peek() != '\n') {
        advance();
    }
}

void Lexer::skipBlockComment() {
    int depth = 1;
    
    while (!isAtEnd() && depth > 0) {
        if (peek() == '/' && peekNext() == '*') {
            advance();
            advance();
            depth++;
        } else if (peek() == '*' && peekNext() == '/') {
            advance();
            advance();
            depth--;
        } else {
            advance();
        }
    }
    
    if (depth > 0) {
        throw LexerError("Unterminated block comment", line_, column_);
    }
}

Token Lexer::makeToken(TokenType type, const std::string& lexeme) {
    return Token(type, lexeme, line_, column_ - lexeme.length());
}

Token Lexer::makeToken(TokenType type, const std::string& lexeme, TokenValue value) {
    return Token(type, lexeme, value, line_, column_ - lexeme.length());
}

Token Lexer::identifier() {
    size_t start = current_;
    size_t startColumn = column_;
    
    while (!isAtEnd() && isAlphaNumeric(peek())) {
        advance();
    }
    
    std::string text = source_.substr(start, current_ - start);
    
    auto it = keywords_.find(text);
    if (it != keywords_.end()) {
        TokenType type = it->second;
        if (type == TokenType::TRUE) {
            return Token(type, text, true, line_, startColumn);
        } else if (type == TokenType::FALSE) {
            return Token(type, text, false, line_, startColumn);
        }
        return Token(type, text, line_, startColumn);
    }
    
    return Token(TokenType::IDENTIFIER, text, line_, startColumn);
}

Token Lexer::number() {
    size_t start = current_;
    size_t startColumn = column_;
    bool isReal = false;
    
    while (!isAtEnd() && isDigit(peek())) {
        advance();
    }
    
    if (!isAtEnd() && peek() == '.' && isDigit(peekNext())) {
        isReal = true;
        advance();
        
        while (!isAtEnd() && isDigit(peek())) {
            advance();
        }
    }
    
    std::string text = source_.substr(start, current_ - start);
    
    if (isReal) {
        double value = std::stod(text);
        return Token(TokenType::REAL_LITERAL, text, value, line_, startColumn);
    } else {
        int64_t value = std::stoll(text);
        return Token(TokenType::INTEGER_LITERAL, text, value, line_, startColumn);
    }
}

Token Lexer::string() {
    size_t start = current_;
    size_t startColumn = column_ - 1;
    std::string value;
    
    while (!isAtEnd() && peek() != '"') {
        if (peek() == '\\') {
            advance();
            if (isAtEnd()) {
                throw LexerError("Unterminated string literal", line_, column_);
            }
            char escaped = advance();
            switch (escaped) {
                case 'n': value += '\n'; break;
                case 't': value += '\t'; break;
                case 'r': value += '\r'; break;
                case '\\': value += '\\'; break;
                case '"': value += '"'; break;
                default:
                    value += '\\';
                    value += escaped;
                    break;
            }
        } else {
            value += advance();
        }
    }
    
    if (isAtEnd()) {
        throw LexerError("Unterminated string literal", line_, column_);
    }
    
    advance();
    
    std::string lexeme = source_.substr(start - 1, current_ - start + 1);
    return Token(TokenType::STRING_LITERAL, lexeme, value, line_, startColumn);
}

bool Lexer::isAlpha(char c) const {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

bool Lexer::isDigit(char c) const {
    return c >= '0' && c <= '9';
}

bool Lexer::isAlphaNumeric(char c) const {
    return isAlpha(c) || isDigit(c);
}

}