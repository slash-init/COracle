// src/compiler/test_compiler_runner_manual.cpp
#include "compiler_runner.hpp"
#include <iostream>
#include <fstream>
#include <cassert>

using namespace coracle;

int main() {
    // Write a tiny valid C program to a temp file.
    std::string src_path = "/tmp/coracle_test.c";
    std::string out_path = "/tmp/coracle_test_exe";
    {
        std::ofstream f(src_path);
        f << "#include <stdio.h>\nint main(void){printf(\"hi\\n\");return 0;}\n";
    }

    CompileResult gcc_result = compile_source(src_path, out_path, CompilerKind::GCC, 10);
    std::cout << "[GCC] success=" << gcc_result.success
              << " stderr=\"" << gcc_result.stderr_output << "\"\n";
    assert(gcc_result.success);
    assert(!gcc_result.executable_path.empty());

    // Now try compiling a deliberately broken program.
    std::string bad_src_path = "/tmp/coracle_test_bad.c";
    {
        std::ofstream f(bad_src_path);
        f << "int main(void) { this is not valid C }\n";
    }
    CompileResult bad_result = compile_source(bad_src_path, "/tmp/coracle_test_bad_exe",
                                               CompilerKind::GCC, 10);
    std::cout << "[GCC bad input] success=" << bad_result.success << "\n";
    std::cout << "stderr (should be non-empty):\n" << bad_result.stderr_output << "\n";
    assert(!bad_result.success);
    assert(!bad_result.stderr_output.empty());

    std::cout << "OK: compiler_runner tests passed.\n";
    return 0;
}