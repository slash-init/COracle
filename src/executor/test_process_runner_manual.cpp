// src/executor/test_process_runner_manual.cpp
#include "process_runner.hpp"
#include <iostream>
#include <cassert>

using namespace coracle;

int main() {
    // Test 1: a normal, quick command.
    {
        ProcessResult r = run_process({"/bin/echo", "hello from child"}, 5);
        std::cout << "[echo] exit=" << r.exit_code
                  << " timed_out=" << r.timed_out
                  << " stdout=" << r.stdout_output;
        assert(r.exit_code == 0);
        assert(!r.timed_out);
        assert(r.stdout_output == "hello from child\n");
    }

    // Test 2: a command that hangs, to verify the timeout kills it.
    {
        ProcessResult r = run_process({"/bin/sleep", "10"}, 2);
        std::cout << "[sleep 10, timeout 2] timed_out=" << r.timed_out << "\n";
        assert(r.timed_out);
    }

    // Test 3: a command that doesn't exist, to verify exec failure is handled.
    {
        ProcessResult r = run_process({"/no/such/binary"}, 5);
        std::cout << "[bad path] exit=" << r.exit_code << "\n";
        assert(r.exit_code == 127);
    }

    std::cout << "OK: process_runner tests passed.\n";
    return 0;
}