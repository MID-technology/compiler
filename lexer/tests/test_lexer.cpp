#include "lexer.h"
#include <cassert>
#include <iostream>
#include <vector>

void testKeywords() {
    std::cout << "Testing keywords..." << std::endl;
    
    olang::Lexer lexer("class is end extends var method this if then else while loop return true false base");
    auto tokens = lexer.tokenize();
    
    assert(tokens.size() == 17);
    assert(tokens[0].type == olang::TokenType::CLASS);
    assert(tokens[1].type == olang::TokenType::IS);
    assert(tokens[2].type == olang::TokenType::END);
    assert(tokens[3].type == olang::TokenType::EXTENDS);
    assert(tokens[4].type == olang::TokenType::VAR);
    assert(tokens[5].type == olang::TokenType::METHOD);
    assert(tokens[6].type == olang::TokenType::THIS);
    assert(tokens[7].type == olang::TokenType::IF);
    assert(tokens[8].type == olang::TokenType::THEN);
    assert(tokens[9].type == olang::TokenType::ELSE);
    assert(tokens[10].type == olang::TokenType::WHILE);
    assert(tokens[11].type == olang::TokenType::LOOP);
    assert(tokens[12].type == olang::TokenType::RETURN);
    assert(tokens[13].type == olang::TokenType::TRUE);
    assert(tokens[14].type == olang::TokenType::FALSE);
    assert(tokens[15].type == olang::TokenType::BASE);
    assert(tokens[16].type == olang::TokenType::END_OF_FILE);
    
    std::cout << "  ✓ Keywords test passed" << std::endl;
}

void testIdentifiers() {
    std::cout << "Testing identifiers..." << std::endl;
    
    olang::Lexer lexer("myVar _private MyClass test123");
    auto tokens = lexer.tokenize();
    
    assert(tokens.size() == 5);
    assert(tokens[0].type == olang::TokenType::IDENTIFIER);
    assert(tokens[0].lexeme == "myVar");
    assert(tokens[1].type == olang::TokenType::IDENTIFIER);
    assert(tokens[1].lexeme == "_private");
    assert(tokens[2].type == olang::TokenType::IDENTIFIER);
    assert(tokens[2].lexeme == "MyClass");
    assert(tokens[3].type == olang::TokenType::IDENTIFIER);
    assert(tokens[3].lexeme == "test123");
    assert(tokens[4].type == olang::TokenType::END_OF_FILE);
    
    std::cout << "  ✓ Identifiers test passed" << std::endl;
}

void testNumbers() {
    std::cout << "Testing numbers..." << std::endl;
    
    olang::Lexer lexer("42 3.14 0 123.456");
    auto tokens = lexer.tokenize();
    
    assert(tokens.size() == 5);
    assert(tokens[0].type == olang::TokenType::INTEGER_LITERAL);
    assert(std::get<int64_t>(tokens[0].value) == 42);
    assert(tokens[1].type == olang::TokenType::REAL_LITERAL);
    assert(std::get<double>(tokens[1].value) == 3.14);
    assert(tokens[2].type == olang::TokenType::INTEGER_LITERAL);
    assert(std::get<int64_t>(tokens[2].value) == 0);
    assert(tokens[3].type == olang::TokenType::REAL_LITERAL);
    assert(std::get<double>(tokens[3].value) == 123.456);
    assert(tokens[4].type == olang::TokenType::END_OF_FILE);
    
    std::cout << "  ✓ Numbers test passed" << std::endl;
}

void testStrings() {
    std::cout << "Testing strings..." << std::endl;
    
    olang::Lexer lexer(R"("hello" "world with spaces" "escaped\"quote")");
    auto tokens = lexer.tokenize();
    
    assert(tokens.size() == 4);
    assert(tokens[0].type == olang::TokenType::STRING_LITERAL);
    assert(std::get<std::string>(tokens[0].value) == "hello");
    assert(tokens[1].type == olang::TokenType::STRING_LITERAL);
    assert(std::get<std::string>(tokens[1].value) == "world with spaces");
    assert(tokens[2].type == olang::TokenType::STRING_LITERAL);
    assert(std::get<std::string>(tokens[2].value) == "escaped\"quote");
    assert(tokens[3].type == olang::TokenType::END_OF_FILE);
    
    std::cout << "  ✓ Strings test passed" << std::endl;
}

void testOperators() {
    std::cout << "Testing operators..." << std::endl;
    
    olang::Lexer lexer(": := = => . , ( ) [ ] { } < >");
    auto tokens = lexer.tokenize();
    
    assert(tokens.size() == 15);
    assert(tokens[0].type == olang::TokenType::COLON);
    assert(tokens[1].type == olang::TokenType::ASSIGN);
    assert(tokens[2].type == olang::TokenType::EQUAL);
    assert(tokens[3].type == olang::TokenType::ARROW);
    assert(tokens[4].type == olang::TokenType::DOT);
    assert(tokens[5].type == olang::TokenType::COMMA);
    assert(tokens[6].type == olang::TokenType::LPAREN);
    assert(tokens[7].type == olang::TokenType::RPAREN);
    assert(tokens[8].type == olang::TokenType::LBRACKET);
    assert(tokens[9].type == olang::TokenType::RBRACKET);
    assert(tokens[10].type == olang::TokenType::LBRACE);
    assert(tokens[11].type == olang::TokenType::RBRACE);
    assert(tokens[12].type == olang::TokenType::LANGLE);
    assert(tokens[13].type == olang::TokenType::RANGLE);
    assert(tokens[14].type == olang::TokenType::END_OF_FILE);
    
    std::cout << "  ✓ Operators test passed" << std::endl;
}

void testComments() {
    std::cout << "Testing comments..." << std::endl;
    
    olang::Lexer lexer("var x // line comment\nvar y /* block comment */ var z");
    auto tokens = lexer.tokenize();
    
    assert(tokens.size() == 7);
    assert(tokens[0].type == olang::TokenType::VAR);
    assert(tokens[1].type == olang::TokenType::IDENTIFIER);
    assert(tokens[1].lexeme == "x");
    assert(tokens[2].type == olang::TokenType::VAR);
    assert(tokens[3].type == olang::TokenType::IDENTIFIER);
    assert(tokens[3].lexeme == "y");
    assert(tokens[4].type == olang::TokenType::VAR);
    assert(tokens[5].type == olang::TokenType::IDENTIFIER);
    assert(tokens[5].lexeme == "z");
    assert(tokens[6].type == olang::TokenType::END_OF_FILE);
    
    std::cout << "  ✓ Comments test passed" << std::endl;
}

void testNestedComments() {
    std::cout << "Testing nested comments..." << std::endl;
    
    olang::Lexer lexer("var x /* outer /* inner */ still in comment */ var y");
    auto tokens = lexer.tokenize();
    
    assert(tokens.size() == 5);
    assert(tokens[0].type == olang::TokenType::VAR);
    assert(tokens[1].type == olang::TokenType::IDENTIFIER);
    assert(tokens[1].lexeme == "x");
    assert(tokens[2].type == olang::TokenType::VAR);
    assert(tokens[3].type == olang::TokenType::IDENTIFIER);
    assert(tokens[3].lexeme == "y");
    assert(tokens[4].type == olang::TokenType::END_OF_FILE);
    
    std::cout << "  ✓ Nested comments test passed" << std::endl;
}

void testSimpleClass() {
    std::cout << "Testing simple class..." << std::endl;
    
    std::string source = R"(
        class Main is
            var x: Integer(42)
        end
    )";
    
    olang::Lexer lexer(source);
    auto tokens = lexer.tokenize();
    
    assert(tokens[0].type == olang::TokenType::CLASS);
    assert(tokens[1].type == olang::TokenType::IDENTIFIER);
    assert(tokens[1].lexeme == "Main");
    assert(tokens[2].type == olang::TokenType::IS);
    assert(tokens[3].type == olang::TokenType::VAR);
    assert(tokens[4].type == olang::TokenType::IDENTIFIER);
    assert(tokens[4].lexeme == "x");
    assert(tokens[5].type == olang::TokenType::COLON);
    assert(tokens[6].type == olang::TokenType::IDENTIFIER);
    assert(tokens[6].lexeme == "Integer");
    assert(tokens[7].type == olang::TokenType::LPAREN);
    assert(tokens[8].type == olang::TokenType::INTEGER_LITERAL);
    assert(tokens[9].type == olang::TokenType::RPAREN);
    assert(tokens[10].type == olang::TokenType::END);
    
    std::cout << "  ✓ Simple class test passed" << std::endl;
}

void testErrorHandling() {
    std::cout << "Testing error handling..." << std::endl;
    
    try {
        olang::Lexer lexer("var x = \"unterminated string");
        lexer.tokenize();
        assert(false);
    } catch (const olang::LexerError& e) {
        std::cout << "  ✓ Caught unterminated string error" << std::endl;
    }
    
    try {
        olang::Lexer lexer("var x = /* unterminated comment");
        lexer.tokenize();
        assert(false);
    } catch (const olang::LexerError& e) {
        std::cout << "  ✓ Caught unterminated comment error" << std::endl;
    }
    
    std::cout << "  ✓ Error handling test passed" << std::endl;
}

int main() {
    std::cout << "Running lexer tests..." << std::endl;
    std::cout << std::string(50, '=') << std::endl;
    
    try {
        testKeywords();
        testIdentifiers();
        testNumbers();
        testStrings();
        testOperators();
        testComments();
        testNestedComments();
        testSimpleClass();
        testErrorHandling();
        
        std::cout << std::string(50, '=') << std::endl;
        std::cout << "All tests passed! ✓" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << std::endl;
        return 1;
    }
}