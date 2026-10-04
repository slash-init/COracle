// src/oracle/differential_oracle.cpp
#include "differential_oracle.hpp"
#include "../executor/process_runner.hpp"

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <ftw.h>
#include <unistd.h>

namespace coracle {

namespace {

// Recursively removes a directory tree. Used only by TempDir's
// destructor, on a path TempDir itself created via mkdtemp -- never
// on a path supplied by the caller.
int remove_entry(const char* path, const struct stat*, int type, struct FTW*) {
    if (type == FTW_DP) {
        rmdir(path);
    } else {
        unlink(path);
    }
    return 0;
}

void remove_tree(const std::string& path) {
    nftw(path.c_str(), remove_entry, 16, FTW_DEPTH | FTW_PHYS);
}

// RAII guard for a throwaway temp directory, unique per test case.
// Guarantees cleanup on every exit path (normal return, early return,
// or an exception unwinding through this scope) -- this matters once
// run_differential_test runs in a loop many thousands of times.
class TempDir {
public:
    TempDir() {
        char tmpl[] = "/tmp/coracle_test_XXXXXX";
        char* result = mkdtemp(tmpl);
        if (result == nullptr) {
            throw std::runtime_error(std::string("TempDir: mkdtemp failed: ") + strerror(errno));
        }
        path_ = result;
    }

    ~TempDir() {
        remove_tree(path_);
    }

    TempDir(const TempDir&) = delete;
    TempDir& operator=(const TempDir&) = delete;

    const std::string& path() const { return path_; }

private:
    std::string path_;
};

void write_file(const std::string& path, const std::string& contents) {
    std::ofstream f(path);
    if (!f) {
        throw std::runtime_error("write_file: could not open " + path + " for writing");
    }
    f << contents;
}

} // namespace

DifferentialResult run_differential_test(const std::string& source,
                                          int compile_timeout_seconds,
                                          int run_timeout_seconds) {
    DifferentialResult result;

    TempDir dir; // cleaned up automatically on every return path below

    std::string source_path = dir.path() + "/test.c";
    write_file(source_path, source);

    std::string gcc_exe = dir.path() + "/gcc_out";
    std::string clang_exe = dir.path() + "/clang_out";

    CompileResult gcc_compile = compile_source(source_path, gcc_exe,
                                                CompilerKind::GCC, compile_timeout_seconds);
    result.gcc_compile_stderr = gcc_compile.stderr_output;

    CompileResult clang_compile = compile_source(source_path, clang_exe,
                                                  CompilerKind::Clang, compile_timeout_seconds);
    result.clang_compile_stderr = clang_compile.stderr_output;

    // Compile failures are reported distinctly from Mismatch: these
    // cases are not comparable, so they must never be counted as a
    // behavioral disagreement between the two compilers.
    if (!gcc_compile.success) {
        result.verdict = Verdict::GccCompileFailed;
        return result;
    }
    if (!clang_compile.success) {
        result.verdict = Verdict::ClangCompileFailed;
        return result;
    }

    ProcessResult gcc_run = run_process({gcc_exe}, run_timeout_seconds);
    if (gcc_run.timed_out) {
        result.verdict = Verdict::GccRunTimedOut;
        return result;
    }

    ProcessResult clang_run = run_process({clang_exe}, run_timeout_seconds);
    if (clang_run.timed_out) {
        result.verdict = Verdict::ClangRunTimedOut;
        return result;
    }

    result.gcc_stdout = gcc_run.stdout_output;
    result.clang_stdout = clang_run.stdout_output;
    result.gcc_exit_code = gcc_run.exit_code;
    result.clang_exit_code = clang_run.exit_code;

    bool same_output = (gcc_run.stdout_output == clang_run.stdout_output);
    bool same_exit_code = (gcc_run.exit_code == clang_run.exit_code);

    result.verdict = (same_output && same_exit_code) ? Verdict::Match : Verdict::Mismatch;
    return result;
}

} // namespace coracle