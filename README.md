# Traveling Salesman Problem: C Core, CLI, and Qt Desktop App

[![Build](https://github.com/bam23/Genetic-Algorithm/actions/workflows/build.yml/badge.svg)](https://github.com/bam23/Genetic-Algorithm/actions/workflows/build.yml)

A C11 implementation of the Traveling Salesman Problem (TSP), with a supported
command-line client and a C++17/Qt 6 desktop application. Both clients compare
a brute-force exhaustive solver with a deterministic mutation-and-elitism
evolutionary search. The algorithms and data structures remain implemented in
the reusable C core library.

## Quick Start

### Desktop application

The desktop application requires CMake 3.21 or newer, Qt 6, and a C/C++
compiler. It can run either solver, compare their results, chart evolutionary
convergence, and display both directed tours on the same schematic layout.
Exact search is intentionally limited to 12 cities because its search space
grows factorially.

```bash
git clone https://github.com/bam23/Genetic-Algorithm.git
cd Genetic-Algorithm
cmake --preset gui
cmake --build --preset gui
```

Launch the application from a single-configuration development build:

```bash
# macOS
open build/gui/gui/tsp-desktop.app

# Linux
./build/gui/gui/tsp-desktop
```

With a Visual Studio multi-configuration build, launch the Windows executable
from the selected configuration directory:

```powershell
.\build\gui\gui\RelWithDebInfo\tsp-desktop.exe
```

The route views are schematic: screen distance does not represent route cost.

### Command-line interface

The CLI remains permanently supported and requires no Qt installation:

```bash
make
make run
```

The CMake-only CLI workflow is documented in [Build and Run](#build-and-run).

## What This Demonstrates

- **Brute force vs. heuristic optimization:** exhaustive `(n - 1)!` permutation
  search compared with a population-based evolutionary search.
- **Manual data-structure implementation:** a one-based, array-backed binary
  min-heap / priority queue with percolate-up and percolate-down operations.
- **Graph representation:** a weighted directed graph stored as an adjacency
  matrix, with tours represented by fixed-size C arrays and structs.
- **Reproducible engineering:** configurable deterministic seeds, eleven focused
  tests, strict compiler warnings, and sanitizer targets.

## Overview

TSP asks for the lowest-cost closed tour that visits every selected city
exactly once. This project solves the same graph instance in two ways:

- the **brute-force solver** evaluates every tour anchored at city 0 and
  guarantees the optimum for supported input sizes;
- the **evolutionary search** preserves elite routes and generates new routes
  with swap mutation, but does not guarantee optimality.

The bundled dataset is a **directed/asymmetric** weighted graph: traveling from
city A to city B may have a different cost than traveling from B to A. Using
the same selected cities for both solvers makes the brute-force result a useful
baseline for evaluating the heuristic.

## Architecture

The presentation-independent algorithms are built once as a reusable C
library. Both clients consume that implementation through the public API at
`include/tsp/tsp.h`:

```text
C Core Library
    ├── C CLI
    └── C++/Qt Desktop Application
        ├── Private C++ adapter
        ├── Background solver worker
        ├── Convergence visualization
        └── Directed route visualization
```

The GUI uses one background worker thread so solver execution does not block
the interface. The adapter, worker, and visualizations do not reimplement the
algorithms. Qt remains optional: the default CMake configuration builds only
the C library, CLI, and C tests. See [ROADMAP.md](ROADMAP.md) for the project
evolution.

The canonical application artwork is
`resources/icons/tsp-app-icon-master.png`. The committed PNG, ICNS, and ICO
assets are derived from that project-owned master; builds do not require image
processing tools.

## Algorithms

### Exhaustive permutation search

The brute-force solver anchors city 0 because rotating a closed tour does not
create a distinct route. It generates the remaining tours one at a time with a
lexicographic next-permutation algorithm and evaluates all `(n - 1)!`
possibilities. The recursive factorial calculation includes overflow checks.

Because every anchored tour is evaluated, this solver reports the guaranteed
optimum for the selected cities.

### Mutation-and-elitism evolutionary search

The heuristic intentionally uses selection, elitism, and swap mutation without
crossover:

1. Create exactly `P` valid tours while keeping city 0 anchored.
2. Evaluate each route and rank the population with the binary min-heap.
3. Copy the best `E` elite candidates unchanged into the next generation.
4. Fill the remaining positions with mutated children selected from the best
   `max(P / 2, E)` parents, recalculating every child's route cost before heap
   insertion.

Every generation contains exactly `P` candidates. The search tracks the best
route seen across all generations and reports it as a heuristic result. It is
known to be optimal only when it matches an independently computed
brute-force result.

### Brute force vs. heuristic tradeoff

| Approach | Result | Search effort | Role in this project |
|---|---|---|---|
| Brute-force exhaustive search | Guaranteed optimum | Factorial growth | Optimal baseline for feasible instances |
| Evolutionary search | Best route found; no guarantee | Configurable population and generations | Bounded exploration of a candidate set |

For small city counts, brute-force search may evaluate fewer candidates and
finish faster. Its search space grows factorially as `(n - 1)!`, while the
evolutionary search evaluates a configurable, bounded number of candidates.
The evolutionary solver trades an optimality guarantee for scalability; it is
not inherently faster for every problem size or configuration.

## Data Structures

- **Adjacency matrix:** `src/graph.c` loads a fixed `20 x 20` directed cost
  matrix. The 380 off-diagonal values come from `cities.dat`; diagonal entries
  are set to zero.
- **Binary min-heap:** `src/heap.c` manually implements a one-based priority
  queue. Each node stores a route, active city count, and corresponding
  `double` cost.
- **Fixed route records:** tours and results use C arrays and structs. The
  implementation performs no dynamic allocation.

The heap retains the original coursework population limit of 12 while keeping
the representation and heap operations visible and independently testable.

## Example Results

The following result was generated from the final executable with a fixed seed:

```bash
./tsp --cities 5 --generations 10 --population 8 \
  --elite-percent 25 --mutation-swaps 1 --seed 12345
```

Output excerpt (machine-dependent timing lines omitted):

```text
Traveling Salesman Problem comparison
Cities: 5 | Population: 8 | Generations: 10 | Seed: 12345

Brute-force exhaustive search
  Cost: 1596.232086
  Tour: 0 -> 3 -> 2 -> 4 -> 1 -> 0
  Evaluated tours: 24

Mutation-and-elitism evolutionary search
  Cost: 1598.017482
  Tour: 0 -> 3 -> 4 -> 2 -> 1 -> 0
  Evaluated tours: 68

Comparison: heuristic gap = 1.785397 (0.11% above optimum).
```

This run illustrates the intended comparison: the brute-force solver
establishes the optimum, while the heuristic examines fewer candidates and
finds a route within 0.11% of it. A given seed and configuration reproduce the
same routes and costs.

## Build and Run

Core and CLI requirements are CMake 3.21 or newer and a C11 compiler such as
Clang, GCC, or MSVC. CMake is the canonical build definition and supports a
core-only build without Qt or C++ dependencies:

```bash
cmake --preset default
cmake --build --preset default
ctest --preset default
./build/tsp
```

Single-configuration generators commonly place the executable at `build/tsp`.
Multi-configuration generators such as Visual Studio place it in a
configuration subdirectory, for example `build/RelWithDebInfo/`.

The existing Make commands remain available as a compatibility workflow on a
POSIX-like environment. They configure and invoke the corresponding CMake
targets while preserving the root-level `./tsp` executable:

```bash
make           # Build tsp with strict warnings enabled
make run       # Build and run the default configuration
make test      # Build and run the deterministic test suite
make sanitize  # Test with AddressSanitizer and UndefinedBehaviorSanitizer
make fno-common # Test portability with common symbols disabled
make cpp-check # Verify that C++ can include the public C API
make clean     # Remove generated build files
```

Running `./tsp` without options uses eight cities, 50 generations, population
12, 25% elites, one swap per generated child, and seed `12345`.

```text
--cities N          Cities used by both solvers (3-12)
--generations N     Evolutionary generations (0-100000)
--population N      Population size (2-12)
--elite-percent N   Population retained unchanged (1-100)
--mutation-swaps N  City swaps per generated child (1-N-1)
--seed N            Deterministic 32-bit random seed
--data PATH         Off-diagonal 20-city distance data
--verbose           Print evolutionary convergence history
--print-graph       Print the selected adjacency matrix
--help              Show command-line help
```

Default output is concise. `--verbose` prints the best known evolutionary cost
for generation 0 and each configured generation.

## Testing

`make test` runs eleven deterministic C tests covering:

- tour cost calculation;
- binary min-heap ordering;
- anchored permutation validity and count;
- the brute-force optimum for a known four-city graph;
- mutation preserving a valid permutation;
- repeatable fixed-seed heuristic results;
- heuristic route/fitness agreement;
- recursive factorial overflow protection;
- public graph loading and result metadata;
- the convergence-history capacity contract; and
- the documented v1.0.0 deterministic example as an architecture regression.

`make sanitize` runs the same suite with AddressSanitizer and
UndefinedBehaviorSanitizer.

The optional desktop configuration also registers five C++ adapter tests that
exercise C/C++ ownership, deterministic results, convergence history, and
owned error handling:

```bash
cmake --preset desktop
cmake --build --preset desktop
ctest --preset desktop
```

GitHub Actions runs the core and Qt desktop configurations on macOS, Windows,
and Linux using Qt 6.8.3.

## Complexity

Let `n` be the selected city count, `P` the population size, `E` the elite
count, and `G` the number of generations.

| Operation | Complexity |
|---|---|
| Adjacency matrix storage | `O(TSP_MAX_CITIES^2)` |
| Route-cost calculation | `O(n)` |
| Next permutation | `O(n)` worst case |
| Heap insertion or deletion | `O(log P)` comparisons, plus fixed route-record copying |
| Exhaustive search | `O(n * (n - 1)!)` time and `O(n)` route workspace |
| Evolutionary search | Approximately `O(P*n + G*(P*log P + (P-E)*n))` time and `O(P*n)` population storage |

Verbose console output is intended for inspection rather than performance
measurement.

## Project Background

This project originated as university data structures and algorithms
coursework. It was later rehabilitated for algorithmic correctness,
reproducibility, input safety, automated testing, and documentation; then
reorganized as a reusable C library and expanded with a C++/Qt desktop client.
The original solver algorithms remain implemented in C.

The adjacency matrix, lexicographic permutation search, recursive factorial,
binary min-heap, elitism, swap mutation, and Make-based workflow remain central
to the project.

## Limitations

- Brute-force search is limited to 12 cities and becomes expensive near that
  limit because of factorial growth.
- The evolutionary search uses a maximum population of 12 and does not
  guarantee an optimum; it intentionally omits crossover and local-search
  refinements such as 2-opt.
- Input uses the project's fixed 380-value, 20-city directed graph format
  rather than a general TSP file format.
- The core solver implementation uses fixed arrays and remains single-threaded.
  The desktop application runs one solver workflow at a time on a background
  worker thread.
