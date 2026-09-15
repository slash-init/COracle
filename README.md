# COracle

**COracle** is an experimental C/C++ compiler bug detection framework based on **differential testing**. The project generates valid, syntactically well-formed C programs, compiles and executes them across multiple compilers (or across different optimization levels of the same compiler), and compares their observable runtime behavior to uncover compiler miscompilations, crashes, and code-generation discrepancies.

## Overview

Compilers such as GCC and Clang are complex software systems performing aggressive code optimizations. Verifying their correctness is notoriously difficult because there is rarely an automated formal "oracle" that can evaluate whether an arbitrary optimized binary preserves the semantics of an arbitrary C source program.

**Differential testing** addresses this test-oracle problem by compiling the same source program using multiple distinct compilers (e.g., GCC and Clang) or multiple optimization levels (e.g., `-O0`, `-O2`, `-O3`). Because all conforming compilers must adhere to the ISO C standard, the compiled binaries should produce identical observable output for any well-defined program. If one compiler's binary produces an output or exit code that differs from the others—or crashes during compilation—a potential compiler bug has been identified.

```text
                     ┌──────────────────┐
                     │ Generated Program│
                     └────────┬─────────┘
          ┌───────────────────┼───────────────────┐
          ▼                   ▼                   ▼
    ┌───────────┐       ┌───────────┐       ┌───────────┐
    │  GCC -O0  │       │  GCC -O3  │       │ Clang -O3 │
    └─────┬─────┘       └─────┬─────┘       └─────┬─────┘
          ▼                   ▼                   ▼
     [Binary A]          [Binary B]          [Binary C]
          │                   │                   │
          └───────────────────┼───────────────────┘
                              ▼
                 ┌────────────────────────┐
                 │  Differential Oracle   │
                 │   (Compare Outputs)    │
                 └────────────┬───────────┘
                              │
                    Discrepancy Found?
                    ├── No  ──> Continue Campaign
                    └── Yes ──> Report / Reduce
```

COracle is currently being built from the ground up as a research project. The current repository contains the foundational program generation and subprocess execution pipeline; the multi-compiler comparison oracle and test-case reduction stages are scheduled for subsequent development phases.

## Current Status

The table below reflects the **actual status of components implemented in the repository today**:

| Component | Status | Description |
| :--- | :--- | :--- |
| **Deterministic RNG** (`coracle::Rng`) | **Implemented** | Seeded PRNG wrapping `std::mt19937_64` ensuring fully reproducible generation sequences. |
| **AST Representation** (`coracle::ast`) | **Implemented** | Strongly-typed node hierarchy for expressions, variable declarations, assignments, and print statements. |
| **AST Printer** (`coracle::print_program`) | **Implemented** | Converts an AST into compilable, well-formatted C source text with defensive parenthesization. |
| **Random Program Generator** (`coracle::generate_program`) | **Implemented** | Generates constrained, scoped random ASTs within depth and variable-tracking bounds. |
| **Process Runner** (`coracle::run_process`) | **Implemented** | POSIX subprocess executor using `fork`, `execvp`, and pipes with timeout detection and output capping. |
| **Compiler Toolchain Wrapper** | *Planned* | Abstraction layer to invoke external compilers (`gcc`, `clang`) with varying optimization flags. |
| **Differential Oracle** | *Planned* | Comparison engine evaluating stdout, stderr, and exit codes across compiler outputs. |
| **Result Classifier & Triage** | *Planned* | Classifies findings into compiler crashes (ICE), execution divergence, timeouts, or anomalies. |
| **Test-Case Reducer** | *Planned* | Minimizes discrepancy-inducing programs down to small, isolated bug reports. |
| **Continuous Campaign Scheduler** | *Planned* | High-throughput fuzzing loop automating generation, execution, and artifact archiving. |

## Architecture

### Current Implemented Pipeline

The currently implemented codebase takes an explicit 64-bit random seed and generator configuration, deterministically constructs a valid Abstract Syntax Tree (AST), prints that AST into standard C code, and provides a POSIX subprocess runner to execute external binaries safely.

```mermaid
flowchart TD
    Seed["Seed (uint64_t)"] --> RNG["Deterministic RNG\n(std::mt19937_64)"]
    Config["GeneratorConfig\n(depth, statement count, int range)"] --> Gen["Program Generator\n(coracle::generate_program)"]
    RNG --> Gen
    Gen --> AST["Abstract Syntax Tree\n(coracle::ast::Program)"]
    AST --> Printer["AST Printer\n(coracle::print_program)"]
    Printer --> CSource["Generated C Source\n(#include <stdio.h>, main)"]
    CSource -.->|"Manual compilation\n& execution"| Runner["Process Runner\n(coracle::run_process)"]
    Runner --> Result["ProcessResult\n(exit code, stdout, stderr, timeout)"]
```

### Planned Architecture

The target architecture will close the loop into an automated differential-testing fuzzing engine:

```mermaid
flowchart TD
    subgraph CurrentGeneration["Implemented Generation Pipeline"]
        SeedIn["Seed"] --> PRNG["Rng Engine"]
        PRNG --> GenMod["Program Generator"]
        GenMod --> ASTMod["AST"]
        ASTMod --> PrintMod["AST Printer"]
        PrintMod --> SourceCode["C Source File"]
    end

    subgraph PlannedDifferential["Planned Differential Engine"]
        SourceCode --> CompA["Compiler Target A\n(e.g., GCC -O3)"]
        SourceCode --> CompB["Compiler Target B\n(e.g., Clang -O3)"]
        SourceCode --> CompBase["Baseline Compiler\n(e.g., GCC -O0)"]

        CompA --> RunA["Process Runner\n(Child Subprocess)"]
        CompB --> RunB["Process Runner\n(Child Subprocess)"]
        CompBase --> RunBase["Process Runner\n(Child Subprocess)"]

        RunA --> Oracle["Differential Oracle\n(Output & Exit Code Comparator)"]
        RunB --> Oracle
        RunBase --> Oracle

        Oracle --> Decision{"Discrepancy\nDetected?"}
        Decision -->|"No"| NextTest["Next Iteration"]
        Decision -->|"Yes"| Classifier["Result Classifier\n(ICE / Miscompilation / Hang)"]
        Classifier --> Reducer["Test-Case Reducer\n(Delta Debugging / AST Pruner)"]
        Reducer --> Report["Reproducible Bug Report\n(Seed + Minimal C Program)"]
    end
```

## Current Implementation

The active codebase is organized into two primary modules: `src/generator/` and `src/executor/`.

### 1. Deterministic Random Number Generator (`src/generator/rng.hpp`)

All random choices made during program generation pass through the `coracle::Rng` class rather than calling standard library distribution primitives directly.
* **Deterministic Engine**: Wraps `std::mt19937_64`, an engine with a large period ($2^{19937}-1$) and good statistical distribution properties.
* **Reproducibility**: Initialized with an explicit `uint64_t` seed. Given the same seed, `Rng` produces the exact same sequence of integers, booleans, and element choices.
* **Core API**:
  * `next_int(lo, hi)`: Uniform distribution over `[lo, hi]` (inclusive). Throws `std::invalid_argument` if `lo > hi`.
  * `next_bool(probability_true)`: Bernoulli distribution returning true with probability $p \in [0.0, 1.0]$.
  * `pick_one(vector<T>)`: Uniformly selects a reference to an element from a non-empty vector.

### 2. Abstract Syntax Tree (`src/generator/ast.hpp`)

Directly generating random C code as raw strings frequently produces syntactically invalid programs (e.g., mismatched braces, malformed expressions) that compilers reject in the parsing stage. Instead, COracle constructs a structured Abstract Syntax Tree (AST) using modern C++ memory management (`std::unique_ptr` ownership).

The AST currently implements a focused subset of C expressions and statements:
* **Expressions (`coracle::ast::Expr`)**:
  * `IntLiteral`: 64-bit integer constant (e.g., `42`, `-15`).
  * `VarRef`: Reference to an existing variable identifier (e.g., `v0`).
  * `BinaryExpr`: Binary operations holding owned left-hand and right-hand sub-expressions (`op` is either `BinaryOp::Add` or `BinaryOp::Sub`).
* **Statements (`coracle::ast::Stmt`)**:
  * `VarDecl`: Variable declaration with an initializing expression (`int <name> = <init>;`).
  * `Assign`: Reassignment of an existing variable (`<name> = <value>;`).
  * `PrintVar`: Printing a variable's value to standard output via `printf("%d\n", <name>);`.
* **Program (`coracle::ast::Program`)**:
  * Represents the body of a C `main` function as a flat list of `std::unique_ptr<Stmt>`.

### 3. AST Printer (`src/generator/ast_printer.hpp`, `ast_printer.cpp`)

The AST printer provides `std::string print_program(const ast::Program& program)`.
* **Pure Functionality**: Has no internal state, no random decisions, and no filesystem I/O.
* **Valid C Boilerplate**: Wraps generated statements within standard `#include <stdio.h>`, an `int main(void)` signature, and `return 0;`.
* **Defensive Parenthesization**: Every binary expression is rendered with explicit parentheses (e.g., `((a + b) - c)`), eliminating operator precedence ambiguities.
* **Deterministic Formatting**: Indents statement blocks cleanly with 4 spaces.

### 4. Random Program Generator (`src/generator/generator.hpp`, `generator.cpp`)

The generator converts pseudo-random decisions into a valid AST via `coracle::generate_program(rng, config)`.

Generation is parameterized by `coracle::GeneratorConfig`:
* `num_statements`: Total statements generated (default: `6`).
* `max_expr_depth`: Upper bound on recursive expression tree depth (default: `3`).
* `int_min` / `int_max`: Integer literal range (default: `[-100, 100]`).

Key generation mechanisms and constraints:
* **Scope Tracking**: Maintains a list of `declared_vars` as variables are created.
* **Valid Variable References**: A `VarRef` or assignment target can only be selected from `declared_vars`. The generator never references an undeclared variable.
* **Mandatory Initial Declaration**: The first statement of any generated program is forced to be a `VarDecl`, ensuring subsequent statements have a valid variable to reference or print.
* **Bounded Recursion**: The helper function `gen_expr` decrements `depth` at each binary node. When `depth <= 0`, it is forced to create a leaf (`IntLiteral` or `VarRef`), guaranteeing termination.

### 5. Process Runner (`src/executor/process_runner.hpp`, `process_runner.cpp`)

Compiling and executing untrusted or randomly generated programs carries risks of infinite loops, hangs, resource exhaustion, or terminal crashes. The process runner (`coracle::run_process`) executes external binaries safely in an isolated POSIX subprocess:
* **No Shell Execution**: Uses `execvp` directly without invoking `/bin/sh`. Arguments are passed as an exact argument vector (`argv`), preventing shell injection or metacharacter interpretation.
* **Subprocess Isolation**: Uses POSIX `fork` to isolate child execution from the main runner.
* **Piped Stream Capture**: Uses unidirectional POSIX `pipe`s and `dup2` to capture both `stdout` and `stderr` independently.
* **Non-Blocking Polling & Timeout Enforcement**: Uses `waitpid(..., WNOHANG)` alongside microsecond sleeps (`usleep`) to track child progress against a specified `timeout_seconds`. If a process hangs, `kill(pid, SIGKILL)` terminates it, followed by a blocking `waitpid` to reap the process and prevent zombie processes.
* **Output Size Capping**: Implements a per-stream output buffer cap (`kMaxOutputBytes = 1 MB`). If a program generates runaway output, the buffer stops appending and marks `output_truncated = true`, while continuing to drain the pipe so the child process does not deadlock on a full pipe buffer.
* **Structured Result (`ProcessResult`)**:
  * Captures `stdout_output` and `stderr_output`.
  * Records `exit_code` for normal exits (`WIFEXITED`).
  * Records signal terminations (`WIFSIGNALED`) as negative values (e.g., `-11` for `SIGSEGV`), allowing callers to distinguish between standard exits and abnormal crashes.
  * Sets `timed_out` boolean flag.

## Example

The transformation pipeline converts an initial seed into concrete, compilable C code:

```text
Seed: 7
  │
  ▼
[Deterministic RNG]
  │
  ▼
[Generator Decisions]
  ├── Statement 0: Must declare variable -> name="v0", init=(79 - -89)
  ├── Statement 1: Picked DoPrint -> target="v0"
  ├── Statement 2: Picked Declare -> name="v1", init=(( -39 + ( -47 - -42 ) ) - v0)
  ├── Statement 3: Picked Declare -> name="v2", init=v1
  ├── Statement 4: Picked DoAssign -> target="v1", value=v2
  └── Statement 5: Picked DoAssign -> target="v2", value=(-72 + (v1 - v1))
  │
  ▼
[AST Printer Output]
```

Actual output produced by the current printer with seed `7`:

```c
#include <stdio.h>

int main(void) {
    int v0 = (79 - -89);
    printf("%d\n", v0);
    int v1 = ((-39 + (-47 - -42)) - v0);
    int v2 = v1;
    v1 = v2;
    v2 = (-72 + (v1 - v1));
    return 0;
}
```

When compiled and executed, this program prints `168` to stdout and exits with code `0`.

## Reproducibility

Reproducibility is a primary design objective of COracle:

$$\text{Seed} + \text{GeneratorConfig} \longrightarrow \text{Exact Decision Sequence} \longrightarrow \text{Identical AST} \longrightarrow \text{Identical C Code}$$

Why this matters:
1. **Bug Isolation**: When differential testing discovers an output discrepancy or an Internal Compiler Error (ICE), saving the 64-bit seed and configuration is sufficient to reproduce the test case.
2. **Minimal Test Artifacts**: Large corpora of test files do not need to be saved to disk; seeds can be stored, logged, and replayed on demand.
3. **Deterministic Debugging**: Regressions in the generator or AST transformations can be caught immediately via unit and regression tests.

## Safety and Generation Constraints

The generator enforces strict structural and scoping constraints to produce valid programs:
* **Undeclared Variable Prevention**: Variable references (`VarRef`) and assignments (`Assign`) can only select from previously declared variables (`declared_vars`).
* **Strict Type Homogeneity**: Currently, all variables and expressions are typed as signed integers (`int`).
* **Controlled Value Ranges**: Integer literals are bounded by default to $[-100, 100]$.
* **Restricted Operator Subset**: Only binary addition (`+`) and subtraction (`-`) are generated.

### Important Note on Undefined Behavior (UB)

> [!WARNING]
> The current generator **does not claim or guarantee complete absence of Undefined Behavior (UB)**.

* **What is prevented**: The generator avoids obvious syntax errors, undeclared variable references, uninitialized variable reads, and operations known for trivial UB like division-by-zero or out-of-bounds pointer arithmetic (since division, pointers, arrays, and shift operators are not yet part of the AST).
* **What can still happen**: While literal values and expression depths are small, deep compositions of arithmetic operations could theoretically trigger signed integer overflow (which is undefined behavior in the C standard).
* **Design Strategy**: COracle intentionally begins with a minimal, tightly bounded language subset to keep the search space manageable and to minimize accidental UB. As language features expand (e.g., loops, conditionals, pointers), dedicated semantic validation and UB-avoidance passes will be incorporated into `src/validator/`.

## Project Structure

```text
COracle/
├── src/
│   ├── generator/                     # Random program generation pipeline
│   │   ├── rng.hpp                    # Deterministic pseudo-random number generator
│   │   ├── ast.hpp                    # AST node hierarchy (Expr, Stmt, Program)
│   │   ├── ast_printer.hpp            # AST printer header
│   │   ├── ast_printer.cpp            # AST printer implementation
│   │   ├── generator.hpp              # Generator configuration and interface
│   │   ├── generator.cpp              # Random AST generation logic
│   │   ├── test_rng_manual.cpp        # Manual test: RNG seed determinism
│   │   ├── test_ast_manual.cpp        # Manual test: AST construction
│   │   ├── test_printer_manual.cpp    # Manual test: AST code formatting
│   │   └── test_generator_manual.cpp  # Manual test: End-to-end program generation
│   │
│   ├── executor/                      # Subprocess management and execution
│   │   ├── process_runner.hpp         # Subprocess runner interface & ProcessResult
│   │   ├── process_runner.cpp         # POSIX fork/execvp/pipe implementation
│   │   └── test_process_runner_manual.cpp # Manual test: Subprocess execution & timeouts
│   │
│   ├── compiler/                      # (Planned) Compiler wrapper abstraction
│   ├── coverage/                      # (Planned) Coverage feedback and instrumentation
│   ├── oracle/                        # (Planned) Differential output comparison
│   ├── reducer/                       # (Planned) Test-case reduction / delta debugging
│   ├── reporting/                     # (Planned) Bug reproduction logging
│   ├── scheduler/                     # (Planned) Fuzzing campaign loop
│   ├── validator/                     # (Planned) Semantic validation & UB avoidance
│   └── main.cpp                       # (Planned) Main entrypoint CLI
│
├── config/                            # Configuration files (e.g., config.toml)
├── corpus/                            # Test cases (crashes/, interesting/, seeds/)
├── docs/                              # Project and research documentation
├── results/                           # Experiment outputs and discrepancy logs
├── scripts/                           # Auxiliary scripts
├── tests/                             # Automated test suite (future)
├── CMakeLists.txt                     # Build configuration (placeholder)
├── .gitignore                         # Git exclusion rules
└── README.md                          # Project documentation
```

## Building and Testing

### Prerequisites

* Linux-based operating system (the `process_runner` relies on POSIX APIs: `fork`, `execvp`, `pipe`, `waitpid`).
* C++17 compliant compiler (`g++` or `clang++`).
* A C compiler (e.g., `gcc`) for compiling generated test programs.

### Compiling and Running Implemented Components

Currently, the project does not require an automated build tool like CMake or Make. The manual test drivers for each implemented component can be compiled directly:

#### 1. Test Deterministic RNG
Verifies that identical seeds yield identical pseudo-random number sequences, while different seeds diverge:
```bash
g++ -std=c++17 -Wall -Wextra src/generator/test_rng_manual.cpp -o test_rng
./test_rng
```

#### 2. Test AST Construction
Verifies that statement and expression nodes can be created and structured in memory:
```bash
g++ -std=c++17 -Wall -Wextra src/generator/test_ast_manual.cpp -o test_ast
./test_ast
```

#### 3. Test AST Printer
Verifies that an AST is rendered into valid C source text:
```bash
g++ -std=c++17 -Wall -Wextra src/generator/test_printer_manual.cpp src/generator/ast_printer.cpp -o test_printer
./test_printer
```

#### 4. Test Program Generator
Generates programs using fixed seeds and asserts reproducibility:
```bash
g++ -std=c++17 -Wall -Wextra src/generator/test_generator_manual.cpp src/generator/generator.cpp src/generator/ast_printer.cpp -o test_generator
./test_generator
```

#### 5. Test Subprocess Runner
Verifies command execution, standard output capture, timeout enforcement (killing a hanging command), and missing binary detection:
```bash
g++ -std=c++17 -Wall -Wextra src/executor/test_process_runner_manual.cpp src/executor/process_runner.cpp -o test_process_runner
./test_process_runner
```

#### 6. Compiling a Generated Program
You can redirect the generator output to a C source file and compile it using GCC:
```bash
./test_generator > generated.c
gcc -O2 generated.c -o generated_exe
./generated_exe
```

## Research Direction

The primary research objective of COracle is to evaluate how effective grammar-directed and constraint-guided random testing can be at uncovering optimizer bugs in modern compilers.

The planned experimental methodology follows a 7-step differential testing workflow:
1. **Constrained Program Generation**: Generate structurally valid, syntactically correct C programs.
2. **Multi-Target Compilation**: Compile each program using multiple compiler configurations (e.g., GCC vs. Clang, and across optimization flags `-O0`, `-O1`, `-O2`, `-O3`, `-Os`).
3. **Isolated Execution**: Run the resulting binaries within isolated subprocesses under identical input and resource constraints.
4. **Behavioral Comparison**: Compare standard output, standard error, and exit codes across configurations.
5. **Discrepancy Triage**: Separate true miscompilations from compiler crashes (ICE) or non-deterministic program behavior.
6. **Automated Test Reduction**: Shrink discovered discrepancy-triggering programs using delta debugging or syntax-guided minimization while preserving the failure mode.
7. **Artifact Generation**: Produce minimal, self-contained C files with compiler flags and system details suitable for upstream bug reporting.

## Roadmap

### Phase 1: Core Foundation *(Current)*
- [x] Deterministic 64-bit RNG (`coracle::Rng`)
- [x] Initial AST node hierarchy (expressions, declarations, assignments, prints)
- [x] AST-to-C source printer (`coracle::print_program`)
- [x] Seed-driven random AST generator with basic scoping rules
- [x] Subprocess runner with timeout management and output limits (`coracle::run_process`)

### Phase 2: Compiler Invocation & Differential Oracle *(Next)*
- [ ] Compiler invocation abstraction (`src/compiler/`) to wrap `gcc` and `clang`
- [ ] Differential comparison oracle (`src/oracle/`) comparing outputs across optimization tiers
- [ ] Crash and Internal Compiler Error (ICE) classification
- [ ] CMake build system configuration for building targets and unit tests

### Phase 3: Language Expansion & Validation *(Planned)*
- [ ] Expansion of AST: unary operators, logical operators, `if`/`else` control flow, loops (`for`, `while`)
- [ ] Semantic validation and UB avoidance passes (`src/validator/`)
- [ ] Variable shadowing and block scope management

### Phase 4: Reduction & Automated Campaign *(Future)*
- [ ] Test-case reducer (`src/reducer/`) implementing hierarchical delta debugging
- [ ] Automated continuous fuzzing campaign loop (`src/scheduler/`)
- [ ] Automated bug reporting and seed replay utilities (`src/reporting/`)

## Design Principles

The design of COracle adheres to several key engineering principles:
* **Strict Reproducibility**: All random choices depend on explicit seeds passed through `coracle::Rng`. Any generated test case can be reproduced from its seed.
* **Structured AST Generation**: Generating an AST instead of random text guarantees syntactic validity and allows semantic constraints to be enforced before code generation.
* **Bounded Complexity**: Recursive expressions and statements are explicitly bounded to prevent infinite generation or excessively deep expression trees.
* **Process Isolation**: External programs and compilers are isolated via dedicated subprocesses with strict execution time limits and memory/output limits.
* **Independent Testability**: Every component (RNG, AST, Printer, Generator, Runner) is decoupled and independently verifiable.
* **Incremental Evolution**: Language features and compiler targets are added incrementally to ensure generator correctness at each stage.

## Research Context

COracle is being developed as a student research project investigating automated compiler testing and differential analysis techniques. It is designed to explore how constrained program synthesis and differential oracles can effectively detect subtle miscompilation bugs and crashes in modern production compilers.

