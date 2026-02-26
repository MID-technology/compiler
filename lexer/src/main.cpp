#include "lexer.h"
#include <iostream>
#include <fstream>
#include <sstream>

std::string readFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + filename);
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <source_file.ol>" << std::endl;
        return 1;
    }
    
    try {
        std::string source = readFile(argv[1]);
        olang::Lexer lexer(source);
        
        std::cout << "Tokenizing file: " << argv[1] << std::endl;
        std::cout << std::string(50, '=') << std::endl;
        
        std::vector<olang::Token> tokens = lexer.tokenize();
        
        for (const auto& token : tokens) {
            std::cout << token << std::endl;
        }
        
        std::cout << std::string(50, '=') << std::endl;
        std::cout << "Total tokens: " << tokens.size() << std::endl;
        
    } catch (const olang::LexerError& e) {
        std::cerr << "Lexer error at " << e.line() << ":" << e.column() 
                  << " - " << e.what() << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}