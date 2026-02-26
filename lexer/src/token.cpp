#include "token.h"
#include <sstream>

namespace olang {

std::string tokenTypeToString(TokenType type) {
    switch (type) {
        case TokenType::CLASS: return "CLASS";
        case TokenType::IS: return "IS";
        case TokenType::END: return "END";
        case TokenType::EXTENDS: return "EXTENDS";
        case TokenType::VAR: return "VAR";
        case TokenType::METHOD: return "METHOD";
        case TokenType::THIS: return "THIS";
        case TokenType::IF: return "IF";
        case TokenType::THEN: return "THEN";
        case TokenType::ELSE: return "ELSE";
        case TokenType::WHILE: return "WHILE";
        case TokenType::LOOP: return "LOOP";
        case TokenType::RETURN: return "RETURN";
        case TokenType::TRUE: return "TRUE";
        case TokenType::FALSE: return "FALSE";
        case TokenType::BASE: return "BASE";
        case TokenType::IDENTIFIER: return "IDENTIFIER";
        case TokenType::INTEGER_LITERAL: return "INTEGER_LITERAL";
        case TokenType::REAL_LITERAL: return "REAL_LITERAL";
        case TokenType::STRING_LITERAL: return "STRING_LITERAL";
        case TokenType::COLON: return "COLON";
        case TokenType::ASSIGN: return "ASSIGN";
        case TokenType::EQUAL: return "EQUAL";
        case TokenType::ARROW: return "ARROW";
        case TokenType::DOT: return "DOT";
        case TokenType::COMMA: return "COMMA";
        case TokenType::LPAREN: return "LPAREN";
        case TokenType::RPAREN: return "RPAREN";
        case TokenType::LBRACKET: return "LBRACKET";
        case TokenType::RBRACKET: return "RBRACKET";
        case TokenType::LBRACE: return "LBRACE";
        case TokenType::RBRACE: return "RBRACE";
        case TokenType::LANGLE: return "LANGLE";
        case TokenType::RANGLE: return "RANGLE";
        case TokenType::END_OF_FILE: return "END_OF_FILE";
        case TokenType::INVALID: return "INVALID";
        default: return "UNKNOWN";
    }
}

std::ostream& operator<<(std::ostream& os, const Token& token) {
    os << tokenTypeToString(token.type) << " '" << token.lexeme << "' at " 
       << token.line << ":" << token.column;
    
    if (std::holds_alternative<int64_t>(token.value)) {
        os << " (value: " << std::get<int64_t>(token.value) << ")";
    } else if (std::holds_alternative<double>(token.value)) {
        os << " (value: " << std::get<double>(token.value) << ")";
    } else if (std::holds_alternative<bool>(token.value)) {
        os << " (value: " << (std::get<bool>(token.value) ? "true" : "false") << ")";
    } else if (std::holds_alternative<std::string>(token.value)) {
        os << " (value: \"" << std::get<std::string>(token.value) << "\")";
    }
    
    return os;
}

}