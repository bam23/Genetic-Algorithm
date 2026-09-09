# Traveling Salesman Problem: Brute-Force and Evolutionary Search in C

A C11 implementation of the Traveling Salesman Problem (TSP) that compares a
brute-force exhaustive solver with a deterministic mutation-and-elitism
evolutionary search. The project emphasizes algorithms and data structures
implemented directly in C rather than external libraries.

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

The presentation-independent algorithms are built as a reusable C library.
The command-line program includes only the public API at `include/tsp/tsp.h`
and links against `build/libtsp.a`:

```text
C Core Library
    └── C CLI
```

A later phase may add a cross-platform C++ desktop visualization that consumes
the same C API:

```text
C Core Library
    ├── C CLI
    └── C++ Desktop Visualization (planned, not implemented)
```

The core algorithms remain implemented in C. See [ROADMAP.md](ROADMAP.md) for
the planned project evolution.

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

Requirements are CMake 3.21 or newer and a C11 compiler such as Clang, GCC, or
MSVC. CMake is the canonical build definition and supports a core-only build
without GUI dependencies:

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
coursework. It was later revisited to improve algorithmic correctness,
reproducibility, input safety, automated testing, and documentation while
preserving its original C implementation and core algorithms.

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
- The implementation uses fixed arrays and is single-threaded, consistent with
  its data-structures coursework scope.
