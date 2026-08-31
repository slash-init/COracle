// src/generator/generator.hpp
#pragma once

#include "ast.hpp"
#include "rng.hpp"

namespace coracle {

// Configuration for program generation. Kept as a plain struct so
// tuning generation (e.g. for later experiments) means changing
// these values, not editing generation logic.
struct GeneratorConfig {
    int num_statements = 6;      // total statements in the program
    int max_expr_depth = 3;      // max recursion depth for expressions
    int64_t int_min = -100;      // range for generated integer literals
    int64_t int_max = 100;
};

// Generates a random, structurally valid C program (as an AST) using
// the given Rng. Deterministic: same Rng seed + same config -> same tree.
ast::Program generate_program(Rng& rng, const GeneratorConfig& config);

} // namespace coracle