#include "ast.hpp"
#include <iostream>
#include <cassert>

using namespace coracle::ast;

int main() {
    Program program;

    // int x = 3;
    program.statements.push_back(
        std::make_unique<VarDecl>("x", std::make_unique<IntLiteral>(3)));

    // int y = x - 1;
    program.statements.push_back(
        std::make_unique<VarDecl>(
            "y",
            std::make_unique<BinaryExpr>(
                BinaryOp::Sub,
                std::make_unique<VarRef>("x"),
                std::make_unique<IntLiteral>(1))));

    // printf x; printf y;
    program.statements.push_back(std::make_unique<PrintVar>("x"));
    program.statements.push_back(std::make_unique<PrintVar>("y"));

    assert(program.statements.size() == 4);

    // Sanity-check node kinds are what we expect.
    assert(program.statements[0]->kind == StmtKind::VarDecl);
    assert(program.statements[1]->kind == StmtKind::VarDecl);
    assert(program.statements[2]->kind == StmtKind::PrintVar);
    assert(program.statements[3]->kind == StmtKind::PrintVar);

    auto* y_decl = static_cast<VarDecl*>(program.statements[1].get());
    assert(y_decl->init->kind == ExprKind::BinaryExpr);

    std::cout << "OK: AST construction test passed. "
              << program.statements.size() << " statements built.\n";
    return 0;
}