// src/generator/ast_printer.hpp
#pragma once

#include "ast.hpp"
#include <string>

namespace coracle {

// Renders a Program's AST into compilable C source text.
// Pure function: no randomness, no I/O, no side effects.
std::string print_program(const ast::Program& program);

} // namespace coracle