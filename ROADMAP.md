# Project Roadmap

This roadmap records the intended evolution of the project while preserving
its original algorithms and educational purpose.

## 1. Original coursework

The project began as university data-structures coursework written in C. Its
core implementation uses a fixed adjacency matrix, a one-based binary
min-heap, brute-force Traveling Salesman Problem enumeration, and a
mutation-and-elitism evolutionary heuristic.

## 2. Rehabilitation

The v1.0.0 rehabilitation release established a known-good portfolio baseline.
That work corrected solver and heap behavior, made fixed-seed execution
deterministic, added focused tests and sanitizer coverage, enabled modern
compiler warnings, and updated the documentation to describe the implemented
behavior accurately.

## 3. Core and API architecture

The v1.1.0 architecture separates the presentation-independent algorithms
into a reusable C library with a small public API. The command-line program is
a client of that API rather than a separate implementation. This boundary
keeps graph storage, route generation, fitness calculation, exhaustive search,
heap operations, elitism, and mutation in the C core while leaving argument
parsing and output formatting to the CLI.

## 4. Future visualization

A later phase may add a cross-platform C++ desktop application targeting
Windows, macOS, and Linux. That application will consume the same C API as the
CLI. The graph, heap, brute-force solver, and evolutionary solver will remain
implemented in C; the desktop layer will be responsible only for interaction
and visualization.

No GUI toolkit or C++ application is part of v1.1.0.

## 5. Potential visualization capabilities

A future visualization client could use structured solver results together
with its retained run configuration to present:

- route drawings;
- evolutionary convergence charts;
- brute-force and heuristic comparisons;
- exhaustive search-space size;
- candidates evaluated;
- algorithm configuration and deterministic seed information; and
- the heuristic optimality gap when a brute-force result is available.

These capabilities are directions for later client work, not commitments to a
specific GUI framework or delivery schedule.
