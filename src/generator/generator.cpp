// src/generator/generator.cpp
#include "generator.hpp"
#include <vector>
#include <string>

namespace coracle {

using namespace ast;

namespace {

// Builds a random expression, recursing up to `depth` levels deep.
// `declared_vars` may be referenced by VarRef leaves.
std::unique_ptr<Expr> gen_expr(Rng& rng, const GeneratorConfig& config,
                                const std::vector<std::string>& declared_vars,
                                int depth) {
    // Decide whether this node is a leaf (literal/var) or a binary op.
    // Force a leaf once depth is exhausted, so recursion always terminates.
    bool make_leaf = (depth <= 0) || rng.next_bool(0.5);

    if (make_leaf) {
        // Only offer VarRef as an option if a variable actually exists.
        bool use_var = !declared_vars.empty() && rng.next_bool(0.5);
        if (use_var) {
            return std::make_unique<VarRef>(rng.pick_one(declared_vars));
        }
        int64_t value = rng.next_int(config.int_min, config.int_max);
        return std::make_unique<IntLiteral>(value);
    }

    // Binary expression: recurse into two sub-expressions at depth - 1.
    std::vector<BinaryOp> ops = {BinaryOp::Add, BinaryOp::Sub};
    BinaryOp op = rng.pick_one(ops);
    auto lhs = gen_expr(rng, config, declared_vars, depth - 1);
    auto rhs = gen_expr(rng, config, declared_vars, depth - 1);
    return std::make_unique<BinaryExpr>(op, std::move(lhs), std::move(rhs));
}

} // namespace

ast::Program generate_program(Rng& rng, const GeneratorConfig& config) {
    Program program;
    std::vector<std::string> declared_vars;
    int next_var_id = 0;

    for (int i = 0; i < config.num_statements; ++i) {
        // The very first statement must be a VarDecl: nothing else is
        // valid to generate yet, since no variable exists.
        bool must_declare = declared_vars.empty();

        enum class Choice { Declare, DoAssign, DoPrint };
        Choice choice;

        if (must_declare) {
            choice = Choice::Declare;
        } else {
            std::vector<Choice> options = {
                Choice::Declare, Choice::DoAssign, Choice::DoPrint
            };
            choice = rng.pick_one(options);
        }

        switch (choice) {
            case Choice::Declare: {
                std::string name = "v" + std::to_string(next_var_id++);
                auto init = gen_expr(rng, config, declared_vars, config.max_expr_depth);
                program.statements.push_back(
                    std::make_unique<VarDecl>(name, std::move(init)));
                declared_vars.push_back(name);
                break;
            }
            case Choice::DoAssign: {
                std::string target = rng.pick_one(declared_vars);
                auto value = gen_expr(rng, config, declared_vars, config.max_expr_depth);
                program.statements.push_back(
                    std::make_unique<Assign>(target, std::move(value)));
                break;
            }
            case Choice::DoPrint: {
                std::string target = rng.pick_one(declared_vars);
                program.statements.push_back(
                    std::make_unique<PrintVar>(target));
                break;
            }
        }
    }

    return program;
}

} // namespace coracle