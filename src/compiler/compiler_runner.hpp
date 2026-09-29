// src/compiler/compiler_runner.hpp
#pragma once

#include <string>

namespace coracle {

enum class CompilerKind {
    GCC,
    Clang
};

// Returns the executable name to invoke for a given compiler kind
// (resolved via PATH by execvp, same as process_runner).
std::string compiler_command(CompilerKind kind);

struct CompileResult {
    bool success = false;
    bool timed_out = false;
    std::string stderr_output;   // compiler diagnostics, useful even on success (warnings)
    std::string executable_path; // valid only if success == true
};

// Compiles a single C source file into an executable using the given
// compiler. Uses a fixed, minimal invocation (no optimization flags) --
// intentionally identical across compilers so the baseline comparison
// is fair. Any future change to compilation flags (e.g. -O2) should be
// an explicit, documented experimental variable, not silently added here.
CompileResult compile_source(const std::string& source_path,
                              const std::string& output_path,
                              CompilerKind compiler,
                              int timeout_seconds);

} // namespace coracle