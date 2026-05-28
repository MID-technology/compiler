#include "include/semantic.h"
#include "../parser/include/parser.h"
#include "../lexer/include/lexer.h"
#include <iostream>
#include <fstream>
#include <sstream>

using namespace olang;

void printIndent(int level) {
    for (int i = 0; i < level; i++)
        std::cout << "  ";
}

void printAST(const ASTNode* node, int indent = 0);

void printExpression(const Expression* expr, int indent) {
    if (auto* intLit = dynamic_cast<const IntegerLiteral*>(expr)) {
        printIndent(indent);
        std::cout << "IntegerLiteral: " << intLit->value;
        if (expr->resolvedType) std::cout << " [" << expr->resolvedType->name << "]";
        std::cout << "\n";
    } else if (auto* realLit = dynamic_cast<const RealLiteral*>(expr)) {
        printIndent(indent);
        std::cout << "RealLiteral: " << realLit->value;
        if (expr->resolvedType) std::cout << " [" << expr->resolvedType->name << "]";
        std::cout << "\n";
    } else if (auto* boolLit = dynamic_cast<const BooleanLiteral*>(expr)) {
        printIndent(indent);
        std::cout << "BooleanLiteral: " << (boolLit->value ? "true" : "false");
        if (expr->resolvedType) std::cout << " [" << expr->resolvedType->name << "]";
        std::cout << "\n";
    } else if (auto* strLit = dynamic_cast<const StringLiteral*>(expr)) {
        printIndent(indent);
        std::cout << "StringLiteral: \"" << strLit->value << "\"";
        if (expr->resolvedType) std::cout << " [" << expr->resolvedType->name << "]";
        std::cout << "\n";
    } else if (dynamic_cast<const ThisExpression*>(expr)) {
        printIndent(indent);
        std::cout << "ThisExpression";
        if (expr->resolvedType) std::cout << " [" << expr->resolvedType->name << "]";
        std::cout << "\n";
    } else if (auto* ident = dynamic_cast<const Identifier*>(expr)) {
        printIndent(indent);
        std::cout << "Identifier: " << ident->name;
        if (expr->resolvedType) std::cout << " [" << expr->resolvedType->name << "]";
        std::cout << "\n";
    } else if (auto* ma = dynamic_cast<const MemberAccess*>(expr)) {
        printIndent(indent);
        std::cout << "MemberAccess";
        if (expr->resolvedType) std::cout << " [" << expr->resolvedType->name << "]";
        std::cout << ":\n";
        printIndent(indent + 1);
        std::cout << "Object:\n";
        printExpression(ma->object.get(), indent + 2);
        printIndent(indent + 1);
        std::cout << "Member: " << ma->member->name << "\n";
    } else if (auto* fc = dynamic_cast<const FunctionCall*>(expr)) {
        printIndent(indent);
        std::cout << "FunctionCall";
        if (expr->resolvedType) std::cout << " [" << expr->resolvedType->name << "]";
        std::cout << ":\n";
        printIndent(indent + 1);
        std::cout << "Callee:\n";
        printExpression(fc->callee.get(), indent + 2);
        printIndent(indent + 1);
        std::cout << "Arguments:\n";
        for (const auto& arg : fc->arguments)
            printExpression(arg.get(), indent + 2);
    } else if (auto* ci = dynamic_cast<const ConstructorInvocation*>(expr)) {
        printIndent(indent);
        std::cout << "ConstructorInvocation: " << ci->className->name;
        if (expr->resolvedType) std::cout << " [" << expr->resolvedType->name << "]";
        std::cout << "\n";
        if (!ci->arguments.empty()) {
            printIndent(indent + 1);
            std::cout << "Arguments:\n";
            for (const auto& arg : ci->arguments)
                printExpression(arg.get(), indent + 2);
        }
    }
}

void printStatement(const Statement* stmt, int indent) {
    if (auto* assignment = dynamic_cast<const Assignment*>(stmt)) {
        printIndent(indent);
        std::cout << "Assignment:\n";
        printIndent(indent + 1);
        std::cout << "Target:\n";
        printExpression(assignment->target.get(), indent + 2);
        printIndent(indent + 1);
        std::cout << "Value:\n";
        printExpression(assignment->value.get(), indent + 2);
    } else if (auto* whileLoop = dynamic_cast<const WhileLoop*>(stmt)) {
        printIndent(indent);
        std::cout << "WhileLoop:\n";
        printIndent(indent + 1);
        std::cout << "Condition:\n";
        printExpression(whileLoop->condition.get(), indent + 2);
        printIndent(indent + 1);
        std::cout << "Body:\n";
        for (const auto& node : whileLoop->body)
            printAST(node.get(), indent + 2);
    } else if (auto* ifStmt = dynamic_cast<const IfStatement*>(stmt)) {
        printIndent(indent);
        std::cout << "IfStatement:\n";
        printIndent(indent + 1);
        std::cout << "Condition:\n";
        printExpression(ifStmt->condition.get(), indent + 2);
        printIndent(indent + 1);
        std::cout << "Then:\n";
        for (const auto& node : ifStmt->thenBody)
            printAST(node.get(), indent + 2);
        if (!ifStmt->elseBody.empty()) {
            printIndent(indent + 1);
            std::cout << "Else:\n";
            for (const auto& node : ifStmt->elseBody)
                printAST(node.get(), indent + 2);
        }
    } else if (auto* returnStmt = dynamic_cast<const ReturnStatement*>(stmt)) {
        printIndent(indent);
        std::cout << "ReturnStatement:\n";
        if (returnStmt->value)
            printExpression(returnStmt->value.get(), indent + 1);
    }
}

void printAST(const ASTNode* node, int indent) {
    if (auto* expr = dynamic_cast<const Expression*>(node)) {
        printExpression(expr, indent);
    } else if (auto* stmt = dynamic_cast<const Statement*>(node)) {
        printStatement(stmt, indent);
    } else if (auto* varDecl = dynamic_cast<const VariableDeclaration*>(node)) {
        printIndent(indent);
        std::cout << "VariableDeclaration: " << varDecl->name->name << "\n";
        printIndent(indent + 1);
        std::cout << "Initializer:\n";
        printExpression(varDecl->initializer.get(), indent + 2);
    } else if (auto* methodDecl = dynamic_cast<const MethodDeclaration*>(node)) {
        printIndent(indent);
        std::cout << "MethodDeclaration: " << methodDecl->name->name;
        if (methodDecl->isForward) std::cout << " (forward)";
        std::cout << "\n";
        if (!methodDecl->parameters.empty()) {
            printIndent(indent + 1);
            std::cout << "Parameters:\n";
            for (const auto& param : methodDecl->parameters) {
                printIndent(indent + 2);
                std::cout << param->name->name << ": " << param->type->name << "\n";
            }
        }
        if (methodDecl->returnType) {
            printIndent(indent + 1);
            std::cout << "ReturnType: " << methodDecl->returnType->name << "\n";
        }
        if (!methodDecl->body.empty()) {
            printIndent(indent + 1);
            std::cout << "Body:\n";
            for (const auto& bodyNode : methodDecl->body)
                printAST(bodyNode.get(), indent + 2);
        }
    } else if (auto* ctorDecl = dynamic_cast<const ConstructorDeclaration*>(node)) {
        printIndent(indent);
        std::cout << "ConstructorDeclaration\n";
        if (!ctorDecl->parameters.empty()) {
            printIndent(indent + 1);
            std::cout << "Parameters:\n";
            for (const auto& param : ctorDecl->parameters) {
                printIndent(indent + 2);
                std::cout << param->name->name << ": " << param->type->name << "\n";
            }
        }
        printIndent(indent + 1);
        std::cout << "Body:\n";
        for (const auto& bodyNode : ctorDecl->body)
            printAST(bodyNode.get(), indent + 2);
    } else if (auto* classDecl = dynamic_cast<const ClassDeclaration*>(node)) {
        printIndent(indent);
        std::cout << "ClassDeclaration: " << classDecl->name->name << "\n";
        if (classDecl->baseClass) {
            printIndent(indent + 1);
            std::cout << "Extends: " << classDecl->baseClass->name << "\n";
        }
        printIndent(indent + 1);
        std::cout << "Members:\n";
        for (const auto& member : classDecl->members)
            printAST(member.get(), indent + 2);
    } else if (auto* program = dynamic_cast<const Program*>(node)) {
        std::cout << "Program:\n";
        for (const auto& classDecl : program->classes)
            printAST(classDecl.get(), indent + 1);
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <source_file>\n";
        return 1;
    }

    std::ifstream file(argv[1]);
    if (!file) {
        std::cerr << "Error: Could not open file " << argv[1] << "\n";
        return 1;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();

    try {
        Lexer lexer(source);
        auto tokens = lexer.tokenize();

        if (lexer.hadErrors()) {
            for (const auto& e : lexer.errors()) {
                std::cerr << "Lexer error at line " << e.line() << ", column "
                          << e.column() << ": " << e.what() << "\n";
            }
            return 1;
        }

        Parser parser(std::move(tokens));
        auto ast = parser.parse();

        if (parser.hadErrors()) {
            for (const auto& e : parser.errors()) {
                std::cerr << "Parser error at line " << e.line() << ", column "
                          << e.column() << ": " << e.what() << "\n";
            }
            return 1;
        }

        std::cout << "=== AST ===\n";
        printAST(ast.get());
        std::cout << "\n";

        SemanticAnalyzer analyzer;
        analyzer.analyze(ast.get());

        if (analyzer.hasErrors()) {
            std::cerr << "=== Semantic Errors ===\n";
            for (const auto& error : analyzer.errors())
                std::cerr << "  Error: " << error << "\n";
            return 1;
        }

        std::cout << "=== Annotated AST ===\n";
        printAST(ast.get());
        std::cout << "\nSemantic analysis passed successfully.\n";

    } catch (const LexerError& e) {
        std::cerr << "Lexer error at line " << e.line() << ", column " << e.column()
                  << ": " << e.what() << "\n";
        return 1;
    } catch (const ParserError& e) {
        std::cerr << "Parser error at line " << e.line() << ", column " << e.column()
                  << ": " << e.what() << "\n";
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
