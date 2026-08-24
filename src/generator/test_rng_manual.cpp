#include "rng.hpp"
#include <iostream>
#include <cassert>

int main() {
    coracle::Rng a(42);
    coracle::Rng b(42);
    coracle::Rng c(1337);

    bool same_sequence = true;
    bool different_sequence = false;

    for (int i = 0; i < 20; ++i) {
        int64_t va = a.next_int(0, 1000000);
        int64_t vb = b.next_int(0, 1000000);
        int64_t vc = c.next_int(0, 1000000);

        std::cout << "a=" << va << "  b=" << vb << "  c=" << vc << "\n";

        if (va != vb) same_sequence = false;
        if (va != vc) different_sequence = true;
    }

    assert(same_sequence && "Same seed produced different sequences!");
    assert(different_sequence && "Different seeds produced identical sequences!");

    std::cout << "OK: reproducibility test passed.\n";
    return 0;
}