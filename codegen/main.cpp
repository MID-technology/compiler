#include "include/codegen.h"
#include "../parser/include/parser.h"
#include "../lexer/include/lexer.h"
#include "../semantic/include/semantic.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <system_error>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/IR/Module.h>

using namespace olang;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <source.ol> [-o out.ll]\n";
        return 1;
    }
    std::string srcPath = argv[1];
    std::string outPath;
    for (int i = 2; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "-o" && i + 1 < argc) outPath = argv[++i];
    }

    std::ifstream f(srcPath);
    if (!f) { std::cerr << "cannot open " << srcPath << "\n"; return 1; }
    std::stringstream buf;
    buf << f.rdbuf();
    std::string src = buf.str();

    try {
        Lexer lex(src);
        auto toks = lex.tokenize();
        if (lex.hadErrors()) {
            for (auto& e : lex.errors())
                std::cerr << "Lexer error at line " << e.line() << ", column "
                          << e.column() << ": " << e.what() << "\n";
            return 1;
        }
        Parser parser(std::move(toks));
        auto ast = parser.parse();
        if (parser.hadErrors()) {
            for (auto& e : parser.errors())
                std::cerr << "Parser error at line " << e.line() << ", column "
                          << e.column() << ": " << e.what() << "\n";
            return 1;
        }

        SemanticAnalyzer sem;
        sem.analyze(ast.get());
        if (sem.hasErrors()) {
            for (auto& e : sem.errors()) std::cerr << "Error: " << e << "\n";
            return 1;
        }

        CodeGen cg(ast.get(), sem.classTable());
        auto mod = cg.generate();

        if (outPath.empty()) {
            mod->print(llvm::outs(), nullptr);
        } else {
            std::error_code ec;
            llvm::raw_fd_ostream os(outPath, ec, llvm::sys::fs::OF_None);
            if (ec) { std::cerr << "cannot open " << outPath << ": " << ec.message() << "\n"; return 1; }
            mod->print(os, nullptr);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
