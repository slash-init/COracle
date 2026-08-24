// src/generator/ast.hpp
#pragma once

#include <memory>
#include <string>
#include <vector>

namespace coracle::ast {

// ---------- Expressions ----------

enum class ExprKind {
    IntLiteral,
    VarRef,
    BinaryExpr
};

// Base class for all expression nodes.
struct Expr {
    ExprKind kind;
    explicit Expr(ExprKind k) : kind(k) {}
    virtual ~Expr() = default;
};

// A constant integer, e.g. `42`.
struct IntLiteral : Expr {
    int64_t value;
    explicit IntLiteral(int64_t v)
        : Expr(ExprKind::IntLiteral), value(v) {}
};

// A reference to a previously declared variable, e.g. `x`.
struct VarRef : Expr {
    std::string name;
    explicit VarRef(std::string n)
        : Expr(ExprKind::VarRef), name(std::move(n)) {}
};

enum class BinaryOp {
    Add,
    Sub
};

// A binary operation, e.g. `lhs + rhs`.
struct BinaryExpr : Expr {
    BinaryOp op;
    std::unique_ptr<Expr> lhs;
    std::unique_ptr<Expr> rhs;

    BinaryExpr(BinaryOp o, std::unique_ptr<Expr> l, std::unique_ptr<Expr> r)
        : Expr(ExprKind::BinaryExpr), op(o), lhs(std::move(l)), rhs(std::move(r)) {}
};

// ---------- Statements ----------

enum class StmtKind {
    VarDecl,
    Assign,
    PrintVar
};

// Base class for all statement nodes.
struct Stmt {
    StmtKind kind;
    explicit Stmt(StmtKind k) : kind(k) {}
    virtual ~Stmt() = default;
};

// `int <name> = <init>;`
struct VarDecl : Stmt {
    std::string name;
    std::unique_ptr<Expr> init;

    VarDecl(std::string n, std::unique_ptr<Expr> i)
        : Stmt(StmtKind::VarDecl), name(std::move(n)), init(std::move(i)) {}
};

// `<name> = <value>;` (name must refer to an already-declared variable)
struct Assign : Stmt {
    std::string name;
    std::unique_ptr<Expr> value;

    Assign(std::string n, std::unique_ptr<Expr> v)
        : Stmt(StmtKind::Assign), name(std::move(n)), value(std::move(v)) {}
};

// `printf("%d\n", <name>);`
struct PrintVar : Stmt {
    std::string name;

    explicit PrintVar(std::string n)
        : Stmt(StmtKind::PrintVar), name(std::move(n)) {}
};

// ---------- Program ----------

// A program, for v1, is just the body of `main`: a flat list of statements.
struct Program {
    std::vector<std::unique_ptr<Stmt>> statements;
};

} // namespace coracle::ast