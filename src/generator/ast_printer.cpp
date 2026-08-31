// src/generator/ast_printer.cpp
#include "ast_printer.hpp"
#include <stdexcept>

namespace coracle {

using namespace ast;

namespace {

std::string print_expr(const Expr& expr) {
    switch (expr.kind) {
        case ExprKind::IntLiteral: {
            const auto& lit = static_cast<const IntLiteral&>(expr);
            return std::to_string(lit.value);
        }
        case ExprKind::VarRef: {
            const auto& ref = static_cast<const VarRef&>(expr);
            return ref.name;
        }
        case ExprKind::BinaryExpr: {
            const auto& bin = static_cast<const BinaryExpr&>(expr);
            std::string op_str = (bin.op == BinaryOp::Add) ? "+" : "-";
            // Parenthesize defensively so nested expressions never
            // accidentally change meaning due to C operator precedence.
            return "(" + print_expr(*bin.lhs) + " " + op_str + " " +
                   print_expr(*bin.rhs) + ")";
        }
    }
    throw std::logic_error("print_expr: unhandled ExprKind");
}

std::string print_stmt(const Stmt& stmt) {
    const std::string indent = "    ";

    switch (stmt.kind) {
        case StmtKind::VarDecl: {
            const auto& decl = static_cast<const VarDecl&>(stmt);
            return indent + "int " + decl.name + " = " +
                   print_expr(*decl.init) + ";\n";
        }
        case StmtKind::Assign: {
            const auto& assign = static_cast<const Assign&>(stmt);
            return indent + assign.name + " = " +
                   print_expr(*assign.value) + ";\n";
        }
        case StmtKind::PrintVar: {
            const auto& print = static_cast<const PrintVar&>(stmt);
            return indent + "printf(\"%d\\n\", " + print.name + ");\n";
        }
    }
    throw std::logic_error("print_stmt: unhandled StmtKind");
}

} // namespace

std::string print_program(const Program& program) {
    std::string out;
    out += "#include <stdio.h>\n\n";
    out += "int main(void) {\n";

    for (const auto& stmt : program.statements) {
        out += print_stmt(*stmt);
    }

    out += "    return 0;\n";
    out += "}\n";
    return out;
}

} // namespace coracle