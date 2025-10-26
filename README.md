# 🧭 Traveling Salesman Problem (TSP) — Brute Force & Genetic Algorithm

### Author: **Ben Mitchell**  
**Language:** C (GCC, macOS ARM64)  
**Demonstrates:** algorithmic optimization, memory management, and comparative performance between exhaustive and heuristic methods

---

## 📘 Overview

This project explores two classic approaches to the **Traveling Salesman Problem (TSP)** — one of computer science’s most well-known NP-hard problems.  

The goal is to find the shortest possible route that visits all cities exactly once and returns to the origin city.  

| Algorithm | Description | Complexity |
|------------|--------------|-------------|
| **Brute Force** | Evaluates all permutations to guarantee the optimal path. | O(n!) |
| **Genetic Algorithm (GA)** | Uses evolutionary heuristics (selection, mutation, elitism) to approximate the best route efficiently. | O(n × g) |

Together, they highlight the trade-off between **accuracy** and **efficiency** in combinatorial optimization.

---

## ⚙️ Program Behavior

When you run the program, it performs the following steps:

1. Reads a square matrix of distances between cities from `cities.dat`.
2. Prints the full distance matrix for verification.
3. Executes the **Brute Force algorithm** and displays every route and cost.
4. Executes the **Genetic Algorithm**, evolving over multiple generations.
5. Displays the final optimal cost and runtime for both methods.

---

## 📂 File Structure

| File | Description |
|------|--------------|
| `graph.c / graph.h` | Handles graph initialization and distance matrix parsing. |
| `queue.c / queue.h` | Implements a heap-based priority queue for population management. |
| `function.c / function.h` | Contains both the brute-force and genetic algorithms plus helper functions. |
| `tester.c` | Entry point used by the instructor’s test harness. |
| `makefile` | Automates compilation and linking (`make`, `make clean`, `make run`). |

---

## 💾 Input Format (`cities.dat`)

Provide a symmetric distance matrix (N×N) with zero diagonals.  
Example for **5 cities**:

```
0 29 20 21 16
29 0 15 17 28
20 15 0 28 39
21 17 28 0 13
16 28 39 13 0
```

---

## 🧮 Brute Force Algorithm

- Prints **every permutation** of city orders.
- Calculates and displays the **total travel cost** for each.
- Tracks and reports the **minimum cost path**.
- Execution time measured via `gettimeofday()`.

Example output excerpt:
```
|0||1||2||3||4||0|  Cost = 125.00  
|0||1||3||2||4||0|  Cost = 122.00  
...
Optimal Cost: 112.00
It took 1 second
```

---

## 🧬 Genetic Algorithm

The GA begins with a randomized population of possible tours and evolves them over multiple generations.

### ✳️ Key Features
- **Randomized initialization** (new each run)
- **User-controlled mutation rate** (number of cities swapped)
- **Elitism** (top performers survive)
- **Heap-based selection**
- **Optional fixed seed for reproducibility**

### 🧠 Runtime Parameters

| Prompt | Description | Typical Range |
|--------|--------------|----------------|
| `How many generations to run?` | Total iterations of evolution | 10–50 |
| `Population size? (0–12)` | Number of candidate routes | 4–12 |
| `How many cities are you visiting? (0–19)` | Subset of cities | 4–10 |
| `Percentage of population to keep as elites?` | Controls elitism | 10–40 |
| `How many cities to swap on mutation?` | Mutation strength | 1–3 |

---

## 🧪 Example Run

```
Brute Force Optimal Cost: 1596.232086
It took 0 Seconds

Genetic Algorithm
Generation 1 → Best Cost: 2600.898034  
Generation 5 → Best Cost: 2398.615478  
Generation 10 → Best Cost: 1911.716072  
Generation 15 → Best Cost: 1596.232086 ✅

GA matched the Brute Force optimum at generation 15
```

---

## 🧰 Enhancements & Safeguards

- ✅ **Memory safety:** `memset` and `memcpy` corrected to avoid leaks  
- ✅ **Symmetry validation:** ensures matrix consistency  
- ✅ **Input validation:** clamps city and population values  
- ✅ **Optional early-stop:** halts GA if no improvement over several generations  
- ✅ **Reproducible runs:** use `srand(12345)` instead of `srand(time(NULL))`  

---

## ⏱️ Performance Notes

| Metric | Brute Force | Genetic Algorithm |
|---------|--------------|-------------------|
| Optimality | Always exact | Typically near or equal |
| Speed (5 cities) | Instant | Instant |
| Speed (10 cities) | Exponential growth | Linear per generation |
| Memory | Factorial growth | Constant population-based |

---

## 🧾 Sample Output Summary

- Prints complete **distance matrix**
- Displays every permutation and **cost**
- Shows **generation progress** for GA
- Reports **best cost and runtime**

```
Optimal cost 1596.232086
GA best cost 1596.232086 (reached at generation 15)
It took 0 Seconds
```

---

## 🧩 Concepts Demonstrated

- Brute-force recursion and permutation generation  
- Priority queues (min-heaps)  
- Randomized algorithms and mutation control  
- Comparative algorithmic efficiency  
- Input validation and memory management  
- Time measurement and performance profiling  

---

## 🧠 Future Improvements

- Implement **2-opt** local search for GA refinement  
- Add **CSV logging** of generation vs cost  
- Visualize paths using a simple Python or C graphics extension  
- Multi-thread the brute-force search for benchmarking

---

### ✨ Summary
This project provides a clear demonstration of how **heuristic algorithms** like Genetic Algorithms can approximate NP-hard solutions with remarkable efficiency.  
It also highlights the contrast between exhaustive accuracy and practical optimization — the cornerstone of modern algorithmic design.

---

```bash
$ make clean && make && ./tsp
```
Enjoy exploring the balance between **precision** and **performance** 🚀