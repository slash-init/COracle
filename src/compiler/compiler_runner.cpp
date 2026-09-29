// src/compiler/compiler_runner.cpp
#include "compiler_runner.hpp"
#include "../executor/process_runner.hpp"
#include <vector>

namespace coracle {

std::string compiler_command(CompilerKind kind) {
    switch (kind) {
        case CompilerKind::GCC:   return "gcc";
        case CompilerKind::Clang: return "clang";
    }
    return ""; // unreachable; enum is exhaustively handled above
}

CompileResult compile_source(const std::string& source_path,
                              const std::string& output_path,
                              CompilerKind compiler,
                              int timeout_seconds) {
    CompileResult result;

    std::vector<std::string> args = {
        compiler_command(compiler),
        "-o", output_path,
        source_path
    };

    ProcessResult proc = run_process(args, timeout_seconds);

    result.stderr_output = proc.stderr_output;
    result.timed_out = proc.timed_out;

    // A compiler that timed out did not successfully produce a binary.
    if (proc.timed_out) {
        result.success = false;
        return result;
    }

    // Non-zero exit (rejected program) or negative exit code (compiler
    // itself crashed, e.g. internal compiler error) both mean no usable
    // executable exists.
    result.success = (proc.exit_code == 0);
    if (result.success) {
        result.executable_path = output_path;
    }

    return result;
}

} // namespace coracle