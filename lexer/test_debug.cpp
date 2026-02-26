#include "include/lexer.h"
#include <iostream>

int main() {
    olang::Lexer lexer("myVar _private MyClass test123");
    auto tokens = lexer.tokenize();
    
    std::cout << "Total tokens: " << tokens.size() << std::endl;
    for (size_t i = 0; i < tokens.size(); ++i) {
        std::cout << i << ": " << tokens[i] << std::endl;
    }
    
    return 0;
}
