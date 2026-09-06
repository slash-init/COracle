// src/executor/process_runner.hpp
#pragma once

#include <string>
#include <vector>

namespace coracle {

// Result of running an external process to completion (or timeout).
struct ProcessResult {
    std::string stdout_output;
    std::string stderr_output;
    int exit_code = -1;      // meaningless if timed_out is true
    bool timed_out = false;
    bool output_truncated = false;
};

// Runs an external program as a subprocess.
//
// args[0] is the executable path/name (resolved via PATH, like execvp),
// args[1..] are its argv. No shell is invoked, so no shell metacharacters
// (;, |, `, $, etc.) are interpreted — the arguments are passed exactly
// as given.
//
// The process is killed if it runs longer than timeout_seconds.
// Captured stdout/stderr are capped (see kMaxOutputBytes in the .cpp)
// to protect against runaway output from generated programs.
//
// Throws std::runtime_error only for infrastructure failures (pipe/fork
// setup failing) -- NOT for the child process itself failing, crashing,
// or timing out; those are reported via ProcessResult.
ProcessResult run_process(const std::vector<std::string>& args,
                           int timeout_seconds);

} // namespace coracle