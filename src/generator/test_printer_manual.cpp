// src/generator/test_printer_manual.cpp
#include "ast.hpp"
#include "ast_printer.hpp"
#include <iostream>

using namespace coracle;
using namespace coracle::ast;

int main() {
    Program program;

    program.statements.push_back(
        std::make_unique<VarDecl>("x", std::make_unique<IntLiteral>(3)));

    program.statements.push_back(
        std::make_unique<VarDecl>(
            "y",
            std::make_unique<BinaryExpr>(
                BinaryOp::Sub,
                std::make_unique<VarRef>("x"),
                std::make_unique<IntLiteral>(1))));

    program.statements.push_back(std::make_unique<PrintVar>("x"));
    program.statements.push_back(std::make_unique<PrintVar>("y"));

    std::string code = print_program(program);
    std::cout << code;

    return 0;
}