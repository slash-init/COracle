// src/generator/rng.hpp
#pragma once

#include <cstdint>
#include <random>
#include <vector>
#include <stdexcept>

namespace coracle {

// Rng wraps a deterministic pseudo-random number generator.
//
// Every other part of the generator (statement choice, expression choice,
// literal values, variable names, etc.) should go through this class
// instead of touching <random> directly, for two reasons:
//
//   1. Reproducibility: given the same seed, a run must produce the exact
//      same program. This matters for debugging and for research logging —
//      a discovered compiler bug should be regenerable later from its seed.
//   2. Centralization: if we ever change the underlying engine or how
//      ranges/probabilities are computed, we only change it here.
class Rng {
public:
    explicit Rng(uint64_t seed)
        : seed_(seed), engine_(seed) {}

    // Returns the seed this Rng was constructed with.
    uint64_t seed() const { return seed_; }

    // Returns an integer in [lo, hi], inclusive on both ends.
    int64_t next_int(int64_t lo, int64_t hi) {
        if (lo > hi) {
            throw std::invalid_argument("Rng::next_int: lo > hi");
        }
        std::uniform_int_distribution<int64_t> dist(lo, hi);
        return dist(engine_);
    }

    // Returns true with the given probability (default: fair coin flip).
    bool next_bool(double probability_true = 0.5) {
        if (probability_true < 0.0 || probability_true > 1.0) {
            throw std::invalid_argument("Rng::next_bool: probability out of range");
        }
        std::bernoulli_distribution dist(probability_true);
        return dist(engine_);
    }

    // Picks and returns one element from a non-empty vector of options.
    template <typename T>
    const T& pick_one(const std::vector<T>& options) {
        if (options.empty()) {
            throw std::invalid_argument("Rng::pick_one: options is empty");
        }
        size_t index = static_cast<size_t>(
            next_int(0, static_cast<int64_t>(options.size()) - 1));
        return options[index];
    }

private:
    uint64_t seed_;
    std::mt19937_64 engine_;
};

} // namespace coracle