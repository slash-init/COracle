// src/executor/process_runner.cpp
#include "process_runner.hpp"

#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <cstring>
#include <cerrno>
#include <stdexcept>
#include <vector>
#include <array>
#include <chrono>

namespace coracle {

namespace {

constexpr size_t kMaxOutputBytes = 1 * 1024 * 1024; // 1 MB cap per stream

// Reads from fd into `out`, appending up to kMaxOutputBytes total.
// Returns false (and sets truncated=true) once the cap is hit, but keeps
// draining the pipe without storing more, so the child never blocks
// on a full pipe buffer.
void drain_fd(int fd, std::string& out, bool& truncated) {
    std::array<char, 4096> buf{};
    ssize_t n;
    while ((n = read(fd, buf.data(), buf.size())) > 0) {
        if (out.size() < kMaxOutputBytes) {
            size_t remaining = kMaxOutputBytes - out.size();
            size_t to_append = std::min(static_cast<size_t>(n), remaining);
            out.append(buf.data(), to_append);
            if (to_append < static_cast<size_t>(n)) {
                truncated = true;
            }
        } else {
            truncated = true;
        }
    }
}

} // namespace

ProcessResult run_process(const std::vector<std::string>& args,
                           int timeout_seconds) {
    if (args.empty()) {
        throw std::runtime_error("run_process: args must contain at least the executable");
    }

    int stdout_pipe[2];
    int stderr_pipe[2];
    if (pipe(stdout_pipe) != 0 || pipe(stderr_pipe) != 0) {
        throw std::runtime_error(std::string("run_process: pipe() failed: ") + strerror(errno));
    }

    pid_t pid = fork();
    if (pid < 0) {
        throw std::runtime_error(std::string("run_process: fork() failed: ") + strerror(errno));
    }

    if (pid == 0) {
        // ---- Child process ----
        // Redirect our stdout/stderr into the write ends of the pipes.
        dup2(stdout_pipe[1], STDOUT_FILENO);
        dup2(stderr_pipe[1], STDERR_FILENO);

        // Close all pipe fds in the child; only the redirected
        // STDOUT/STDERR matter now.
        close(stdout_pipe[0]);
        close(stdout_pipe[1]);
        close(stderr_pipe[0]);
        close(stderr_pipe[1]);

        // Build a raw char* argv[] for execvp (it needs a
        // null-terminated array of C strings, not std::string).
        std::vector<char*> argv;
        argv.reserve(args.size() + 1);
        for (const auto& a : args) {
            argv.push_back(const_cast<char*>(a.c_str()));
        }
        argv.push_back(nullptr);

        execvp(argv[0], argv.data());

        // execvp only returns on failure (e.g. binary not found).
        // We're still "inside" the forked child here, so just exit
        // immediately with a distinct code rather than returning up
        // through normal control flow.
        _exit(127);
    }

    // ---- Parent process ----
    close(stdout_pipe[1]);
    close(stderr_pipe[1]);

    ProcessResult result;
    bool stdout_truncated = false;
    bool stderr_truncated = false;

    // Poll: read available output and check for exit, until timeout.
    auto start = std::chrono::steady_clock::now();
    bool child_exited = false;
    int status = 0;

    while (true) {
        pid_t wait_ret = waitpid(pid, &status, WNOHANG);
        if (wait_ret == pid) {
            child_exited = true;
            break;
        }

        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - start).count();
        if (elapsed >= timeout_seconds) {
            break; // timed out; handled below
        }

        // Small sleep to avoid busy-spinning the CPU while waiting.
        usleep(10 * 1000); // 10ms
    }

    if (!child_exited) {
        // Timed out: kill the whole process, then reap it so it
        // doesn't become a zombie.
        kill(pid, SIGKILL);
        waitpid(pid, &status, 0);
        result.timed_out = true;
    }

    // Drain whatever output exists, regardless of timeout/exit,
    // so partial output before a hang/crash is still captured.
    drain_fd(stdout_pipe[0], result.stdout_output, stdout_truncated);
    drain_fd(stderr_pipe[0], result.stderr_output, stderr_truncated);
    close(stdout_pipe[0]);
    close(stderr_pipe[0]);

    result.output_truncated = stdout_truncated || stderr_truncated;

    if (!result.timed_out) {
        if (WIFEXITED(status)) {
            result.exit_code = WEXITSTATUS(status);
        } else if (WIFSIGNALED(status)) {
            // Convention: report signal deaths as negative "exit codes"
            // (e.g. -11 for SIGSEGV) so callers can distinguish a crash
            // from a normal exit without a separate bool field.
            result.exit_code = -WTERMSIG(status);
        }
    }

    return result;
}

} // namespace coracle