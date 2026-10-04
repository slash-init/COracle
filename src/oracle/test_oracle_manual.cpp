#include "differential_oracle.hpp"
#include <iostream>
#include <cassert>

using namespace coracle;

const char* verdict_name(Verdict v) {
    switch (v) {
        case Verdict::Match: return "Match";
        case Verdict::Mismatch: return "Mismatch";
        case Verdict::GccCompileFailed: return "GccCompileFailed";
        case Verdict::ClangCompileFailed: return "ClangCompileFailed";
        case Verdict::GccRunTimedOut: return "GccRunTimedOut";
        case Verdict::ClangRunTimedOut: return "ClangRunTimedOut";
    }
    return "?";
}

int main() {
    // Case 1: a normal, well-defined program -> expect Match.
    {
        std::string src =
            "#include <stdio.h>\n"
            "int main(void){ int x = 3; int y = x - 1; printf(\"%d\\n%d\\n\", x, y); return 0; }\n";
        DifferentialResult r = run_differential_test(src, 10, 5);
        std::cout << "[well-defined] verdict=" << verdict_name(r.verdict) << "\n";
        assert(r.verdict == Verdict::Match);
    }

    // Case 2: deliberately broken C -> expect GccCompileFailed.
    {
        std::string src = "int main(void) { this is not valid C }\n";
        DifferentialResult r = run_differential_test(src, 10, 5);
        std::cout << "[broken] verdict=" << verdict_name(r.verdict) << "\n";
        assert(r.verdict == Verdict::GccCompileFailed);
        assert(!r.gcc_compile_stderr.empty());
    }

    std::cout << "OK: oracle tests passed.\n";
    return 0;
}