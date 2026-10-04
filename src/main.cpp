// src/main.cpp
//
// COracle entry point (v1): generates ONE C program from a given seed
// and runs it through the differential oracle (GCC vs Clang).
//
// Usage:
//   coracle --seed <number>
//
// This is deliberately minimal -- no config file, no looping over many
// seeds yet. It exists to prove the full pipeline (Rng -> generator ->
// printer -> oracle) composes correctly end-to-end on a real generated
// program, not just on hand-built test trees. The fuzzing loop (Phase 5)
// will build on this by running this same sequence many times.

#include "generator/rng.hpp"
#include "generator/generator.hpp"
#include "generator/ast_printer.hpp"
#include "oracle/differential_oracle.hpp"

#include <iostream>
#include <string>
#include <cstdlib>
#include <optional>

namespace {

// Parses "--seed <number>" from argv. Returns std::nullopt if the
// argument is missing or malformed, so main() can report a clear
// error instead of crashing on a bad std::stoull.
std::optional<uint64_t> parse_seed_arg(int argc, char** argv) {
    for (int i = 1; i < argc - 1; ++i) {
        if (std::string(argv[i]) == "--seed") {
            try {
                return std::stoull(argv[i + 1]);
            } catch (const std::exception&) {
                return std::nullopt;
            }
        }
    }
    return std::nullopt;
}

const char* verdict_name(coracle::Verdict v) {
    using coracle::Verdict;
    switch (v) {
        case Verdict::Match:               return "Match";
        case Verdict::Mismatch:             return "Mismatch";
        case Verdict::GccCompileFailed:     return "GccCompileFailed";
        case Verdict::ClangCompileFailed:   return "ClangCompileFailed";
        case Verdict::GccRunTimedOut:       return "GccRunTimedOut";
        case Verdict::ClangRunTimedOut:     return "ClangRunTimedOut";
    }
    return "Unknown";
}

constexpr int kCompileTimeoutSeconds = 10;
constexpr int kRunTimeoutSeconds = 5;

} // namespace

int main(int argc, char** argv) {
    std::optional<uint64_t> seed = parse_seed_arg(argc, argv);
    if (!seed.has_value()) {
        std::cerr << "Usage: " << argv[0] << " --seed <number>\n";
        return 2;
    }

    coracle::Rng rng(*seed);
    coracle::GeneratorConfig config; // defaults

    coracle::ast::Program program = coracle::generate_program(rng, config);
    std::string source = coracle::print_program(program);

    std::cout << "=== Seed: " << *seed << " ===\n";
    std::cout << "--- Generated program ---\n";
    std::cout << source;
    std::cout << "--------------------------\n";

    coracle::DifferentialResult result = coracle::run_differential_test(
        source, kCompileTimeoutSeconds, kRunTimeoutSeconds);

    std::cout << "Verdict: " << verdict_name(result.verdict) << "\n";

    switch (result.verdict) {
        case coracle::Verdict::Match:
            std::cout << "Both compilers agreed. Output:\n" << result.gcc_stdout;
            return 0;

        case coracle::Verdict::Mismatch:
            std::cout << "GCC stdout:\n" << result.gcc_stdout;
            std::cout << "GCC exit code: " << result.gcc_exit_code << "\n";
            std::cout << "Clang stdout:\n" << result.clang_stdout;
            std::cout << "Clang exit code: " << result.clang_exit_code << "\n";
            return 1;

        case coracle::Verdict::GccCompileFailed:
            std::cout << "GCC rejected the program. stderr:\n" << result.gcc_compile_stderr;
            return 0;

        case coracle::Verdict::ClangCompileFailed:
            std::cout << "Clang rejected the program. stderr:\n" << result.clang_compile_stderr;
            return 0;

        case coracle::Verdict::GccRunTimedOut:
            std::cout << "GCC's executable timed out.\n";
            return 0;

        case coracle::Verdict::ClangRunTimedOut:
            std::cout << "Clang's executable timed out.\n";
            return 0;
    }

    return 0;
}