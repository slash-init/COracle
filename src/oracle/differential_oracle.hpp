// src/oracle/differential_oracle.hpp
#pragma once

#include "../compiler/compiler_runner.hpp"
#include <string>

namespace coracle {

// Distinct outcomes of running one program through both compilers.
// Kept as separate enum values (rather than a bool "found_bug") so
// that non-comparable cases (a compile failure, a timeout) are never
// silently counted as a semantic mismatch. This matters for
// experimental validity: a "mismatch count" that secretly includes
// compile failures is not measuring what it claims to measure.
enum class Verdict {
    Match,                // both compiled, both ran, same observable behavior
    Mismatch,              // both compiled, both ran, DIFFERENT behavior -- the interesting case
    GccCompileFailed,      // GCC rejected or crashed on the program
    ClangCompileFailed,    // Clang rejected or crashed on the program
    GccRunTimedOut,        // GCC's executable hung when run
    ClangRunTimedOut       // Clang's executable hung when run
};

// Full record of one differential test, including enough raw detail
// to investigate a Mismatch (or any failure) later -- this is the
// data a future reducer / research log entry will need.
struct DifferentialResult {
    Verdict verdict;

    std::string gcc_stdout;
    std::string clang_stdout;
    int gcc_exit_code = 0;
    int clang_exit_code = 0;

    // Compiler diagnostics, kept even on success (warnings can be
    // interesting data), and essential on a *CompileFailed verdict.
    std::string gcc_compile_stderr;
    std::string clang_compile_stderr;
};

// Runs one differential test: compiles `source` with both GCC and
// Clang, runs both resulting executables (if compilation succeeded),
// and compares their observable behavior (stdout + exit code).
//
// All temp files created for this single test (source file, two
// executables) live under one throwaway directory that is deleted
// before this function returns, regardless of which verdict is
// reached or whether an exception propagates.
DifferentialResult run_differential_test(const std::string& source,
                                          int compile_timeout_seconds,
                                          int run_timeout_seconds);

} // namespace coracle