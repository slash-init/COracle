// src/generator/test_generator_manual.cpp
#include "ast.hpp"
#include "ast_printer.hpp"
#include "generator.hpp"
#include "rng.hpp"
#include <iostream>
#include <cassert>

using namespace coracle;

int main() {
    GeneratorConfig config; // defaults

    Rng rng1(7);
    ast::Program prog1 = generate_program(rng1, config);
    std::string code1 = print_program(prog1);

    Rng rng2(7);
    ast::Program prog2 = generate_program(rng2, config);
    std::string code2 = print_program(prog2);

    assert(code1 == code2 && "Same seed produced different programs!");

    std::cout << code1;
    std::cout << "\n// OK: reproducibility check passed.\n";

    return 0;
}