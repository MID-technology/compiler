#include "include/codegen.h"
#include "../parser/include/parser.h"
#include "../lexer/include/lexer.h"
#include "../semantic/include/semantic.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <system_error>
#include <filesystem>
#include <vector>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/wait.h>
#include <spawn.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/IR/Module.h>

#ifndef OLC_CLANG_PATH
#define OLC_CLANG_PATH "clang"
#endif
#ifndef OLC_RUNTIME_OBJ
#define OLC_RUNTIME_OBJ ""
#endif

extern char** environ;

using namespace olang;
namespace fs = std::filesystem;

static int spawnAndWait(const std::vector<std::string>& argv) {
    std::vector<char*> cargs;
    cargs.reserve(argv.size() + 1);
    for (auto& s : argv) cargs.push_back(const_cast<char*>(s.c_str()));
    cargs.push_back(nullptr);

    pid_t pid = 0;
    int rc = posix_spawnp(&pid, cargs[0], nullptr, nullptr, cargs.data(), environ);
    if (rc != 0) {
        std::cerr << "spawn failed: " << argv[0] << ": " << std::strerror(rc) << "\n";
        return -1;
    }
    int status = 0;
    waitpid(pid, &status, 0);
    if (WIFEXITED(status)) return WEXITSTATUS(status);
    return -1;
}

static void usage(const char* prog) {
    std::cerr <<
        "usage: " << prog << " <source.ol> [options] [-- prog-args...]\n"
        "  -o <path>     output path (binary by default, or .ll with -emit-llvm)\n"
        "  -emit-llvm    write LLVM IR instead of linking a binary\n"
        "  -run          run the produced binary after building (passes prog-args)\n"
        "  -h, --help    show this help\n";
}

int main(int argc, char* argv[]) {
    std::string srcPath;
    std::string outPath;
    bool emitLLVM = false;
    bool runAfter = false;
    std::vector<std::string> progArgs;

    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "-h" || a == "--help") { usage(argv[0]); return 0; }
        if (a == "--") { for (int j = i + 1; j < argc; ++j) progArgs.emplace_back(argv[j]); break; }
        if (a == "-emit-llvm") { emitLLVM = true; continue; }
        if (a == "-run")       { runAfter = true; continue; }
        if (a == "-o" && i + 1 < argc) { outPath = argv[++i]; continue; }
        if (!a.empty() && a[0] == '-') {
            std::cerr << "unknown option: " << a << "\n"; usage(argv[0]); return 1;
        }
        if (srcPath.empty()) { srcPath = a; continue; }
        std::cerr << "unexpected argument: " << a << "\n"; usage(argv[0]); return 1;
    }
    if (srcPath.empty()) { usage(argv[0]); return 1; }

    std::ifstream f(srcPath);
    if (!f) { std::cerr << "cannot open " << srcPath << "\n"; return 1; }
    std::stringstream buf;
    buf << f.rdbuf();
    std::string src = buf.str();

    std::unique_ptr<Parser> parser;
    std::unique_ptr<SemanticAnalyzer> sem;
    std::unique_ptr<CodeGen> cg;
    std::unique_ptr<ASTNode> astHolder;
    Program* astProgram = nullptr;
    std::unique_ptr<llvm::Module> mod;
    try {
        Lexer lex(src);
        auto toks = lex.tokenize();
        if (lex.hadErrors()) {
            for (auto& e : lex.errors())
                std::cerr << "Lexer error at line " << e.line() << ", column "
                          << e.column() << ": " << e.what() << "\n";
            return 1;
        }
        parser = std::make_unique<Parser>(std::move(toks));
        auto ast = parser->parse();
        if (parser->hadErrors()) {
            for (auto& e : parser->errors())
                std::cerr << "Parser error at line " << e.line() << ", column "
                          << e.column() << ": " << e.what() << "\n";
            return 1;
        }
        astProgram = ast.get();
        astHolder.reset(ast.release());

        sem = std::make_unique<SemanticAnalyzer>();
        sem->analyze(astProgram);
        if (sem->hasErrors()) {
            for (auto& e : sem->errors()) std::cerr << "Error: " << e << "\n";
            return 1;
        }

        cg = std::make_unique<CodeGen>(astProgram, sem->classTable());
        mod = cg->generate();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    if (emitLLVM) {
        if (outPath.empty()) {
            mod->print(llvm::outs(), nullptr);
        } else {
            std::error_code ec;
            llvm::raw_fd_ostream os(outPath, ec, llvm::sys::fs::OF_None);
            if (ec) { std::cerr << "cannot open " << outPath << ": " << ec.message() << "\n"; return 1; }
            mod->print(os, nullptr);
        }
        return 0;
    }

    fs::path tmpDir = fs::temp_directory_path() /
                      ("olc-" + std::to_string(::getpid()));
    std::error_code ec;
    fs::create_directories(tmpDir, ec);
    fs::path llPath = tmpDir / "out.ll";

    {
        llvm::raw_fd_ostream os(llPath.string(), ec, llvm::sys::fs::OF_None);
        if (ec) { std::cerr << "cannot write IR: " << ec.message() << "\n"; return 1; }
        mod->print(os, nullptr);
    }

    if (outPath.empty()) {
        outPath = fs::path(srcPath).stem().string();
    }

    std::string runtimeObj = OLC_RUNTIME_OBJ;
    if (runtimeObj.empty() || !fs::exists(runtimeObj)) {
        std::cerr << "runtime.o not found at " << runtimeObj
                  << " (rebuild olc with CMake)\n";
        return 1;
    }

    int rc = spawnAndWait({
        OLC_CLANG_PATH,
        "-Wno-override-module",
        "-o", outPath,
        llPath.string(),
        runtimeObj
    });
    fs::remove_all(tmpDir, ec);
    if (rc != 0) {
        std::cerr << "linker failed (exit " << rc << ")\n";
        return rc;
    }

    if (runAfter) {
        std::vector<std::string> argv2{ "./" + outPath };
        for (auto& a : progArgs) argv2.push_back(a);
        return spawnAndWait(argv2);
    }
    return 0;
}
