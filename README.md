# Minimum Cut: Stoer-Wagner vs Karger vs Karger's Tree Packing

Programming Assignment 2 (Advanced Algorithms): a group comparison of three algorithms for the global minimum cut problem (Theme 2: deterministic vs randomized).

Given a connected, undirected, unweighted graph, find the smallest number of edges whose removal disconnects it. Every algorithm returns the size of that cut.

- Video: https://youtu.be/49uuL96k92g
- Report: "PA2 Report Minimum Cut Algorithm Comparison.pdf" in project root

## The algorithms

| Algorithm | Header | Main function(s) | Primary author |
|---|---|---|---|
| Stoer-Wagner (deterministic, exact) | `stoer_wagner/stoer_wagner.hpp` | `stoer_wagner(n, edges)` (matrix version), `stoer_wagner_pq(n, edges)` (priority queue version) | Aarav Khemani |
| Karger's contraction algorithm (randomized) | `karger/karger.hpp` | `karger_min_cut(n, edges, t, seed)` | Rodrigo Uzal Sequeros |
| Karger's tree packing (simplified, randomized version of "Minimum Cuts in Near-Linear Time") | `karger-tree-packing/karger_tree_packing.hpp` | `karger_tree_packing_min_cut(n, edges, num_trees, seed)` (an overload without `seed` uses a fresh random seed) | Rentaro Beattie |

The two Stoer-Wagner functions are the same algorithm with different data structures: the matrix version scans an n x n table for the next vertex, the priority queue version uses adjacency maps and a heap.

Tree packing builds `num_trees` spanning trees greedily (each new tree uses the least-loaded edges, with ties broken randomly) and returns the smallest cut that crosses some packed tree in at most two edges. It always returns the size of a real cut, so it can overestimate the minimum cut but never underestimate it. It returns the exact minimum cut only if some tree crosses it at most twice, which becomes more likely as `num_trees` grows.

## Shared interface

All algorithms use the same graph format, defined in `graphs/graphs.hpp`:

- `n` vertices, numbered `0` to `n-1`
- `edges` is a `std::vector<std::pair<int, int>>` (the type `Edges`), with each undirected edge listed once
- unweighted, connected, no self-loops
- every algorithm returns the minimum cut size as an `int`
- randomized algorithms take extra arguments at the end (the number of runs `t` for Karger, the number of trees `num_trees` for tree packing, and a `seed`)

`graphs/graphs.hpp` contains the graph builders `make_cycle`, `make_complete`, `make_two_clusters` and `make_random_graph`. The benchmark and test files also define a few extra builders locally (`make_two_circulants`, `make_path`, `make_star`).

## Repository layout

```
graphs/                  shared graph builders
stoer_wagner/            Stoer-Wagner (matrix and priority queue versions)
karger/                  Karger's contraction algorithm
karger-tree-packing/     Karger's tree packing
tests/                   correctness tests
bench/                   benchmark programs (write CSV files to results/)
scripts/                 Python scripts that turn the CSVs into plots
results/                 CSV files and plots produced by the benchmarks
requirements.txt         Python packages needed for the plot scripts
```

## Requirements

- A C++17 compiler. Either:
  - **MSVC**: Visual Studio (2022 or later) with the "Desktop development with C++" workload, used from a *Developer PowerShell for VS* (check that `cl` prints an **x64** banner), or
  - **g++ / clang++** with `-std=c++17`.
- Python 3 with the plotting packages: `pip install -r requirements.txt`

Use the same compiler and flags for every program you want to compare. The standard library does not guarantee that the same seed produces the same random numbers across compilers, so the same seed can give different graphs under MSVC and g++.

## Building and running

Run every command from the **repository root**, because the programs read and write paths like `results/...`. The `results/` folder must exist.

Examples below use MSVC. For g++, replace each `cl ...` line with `g++ -std=c++17 -O2 <source> -o <name>` and run the program as `./<name>`.

### Tests

```
cl /std:c++17 /O2 /EHsc tests\test_stoer_wagner.cpp /Fe:test_sw.exe
.\test_sw.exe

cl /std:c++17 /O2 /EHsc tests\test_karger_tree_packing.cpp /Fe:test_ktp.exe
.\test_ktp.exe

cl /std:c++17 /O2 /EHsc tests\test_karger.cpp /Fe:test_karger.exe
.\test_karger.exe
```

Each program prints `ALL ... TESTS PASSED` at the end if everything is correct. `test_stoer_wagner` checks both Stoer-Wagner versions against known answers and a brute-force search on small random graphs, and checks that the two versions agree. `test_karger` checks that a single Karger run always finds the minimum cut on cycles, trees and a doubled-cycle multigraph, that a single run never returns less than the minimum cut, and that Karger matches known answers and Stoer-Wagner. `test_karger_tree_packing` checks the tree packing against known answers and Stoer-Wagner.

### Benchmarks and plots

| Program | Source | Writes | Plot script | Plots |
|---|---|---|---|---|
| Stoer-Wagner, matrix vs priority queue | `bench/bench_stoer_wagner.cpp` | `results/stoer_wagner_bench.csv` | `scripts/plot_stoer_wagner.py` | `sw_sparse.png`, `sw_dense.png`, `sw_density.png` |
| All algorithms compared | `bench/bench_all.cpp` | `results/bench_time.csv`, `results/bench_success.csv`, `results/bench_ksweep.csv`, `results/bench_treesweep.csv` | `scripts/plot_all.py` | `all_time_sparse.png`, `all_time_dense.png`, `all_success.png`, `all_repetitions.png`, `ksweep.png`, `treesweep.png` |
| Tree packing on its own | `bench/bench_karger_tree_packing.cpp` | `results/bench_karger_tree_packing.csv` | `scripts/plot_tree_packing.py` | `tp_scaling.png`, `tp_density.png`, `tp_trees.png` |
| Giant super-vertex check (report Section 5.1) | `bench/check_giant.cpp` | prints to the screen | none | none |

Each plot script needs the CSV files written by its benchmark, so run the benchmark first.

Stoer-Wagner benchmark:

```
cl /std:c++17 /O2 /EHsc bench\bench_stoer_wagner.cpp /Fe:bench_sw.exe
.\bench_sw.exe
python scripts\plot_stoer_wagner.py
```

Comparison of all algorithms (takes a few minutes, mostly because Karger needs many repetitions):

```
cl /std:c++17 /O2 /EHsc bench\bench_all.cpp /Fe:bench_all.exe
.\bench_all.exe
python scripts\plot_all.py
```

Tree packing benchmark:

```
cl /std:c++17 /O2 /EHsc bench\bench_karger_tree_packing.cpp /Fe:bench_ktp.exe
.\bench_ktp.exe
python scripts\plot_tree_packing.py
```

Close other programs while a benchmark runs, because background load adds noise to the timings.

**Note on `bench_karger_tree_packing.csv`:** the `tree_packing_pack_only` rows time the packing alone, so their `result` column holds the number of trees, not a cut size. Only the `tree_packing` rows should be compared with the `exact` column.

### Reproducing all results

Run the tests first (a benchmark of wrong answers is meaningless), then every benchmark and its plot script. To regenerate everything from scratch, run these in order from the repository root (PowerShell):

```
Remove-Item results\*.csv, results\*.png

cl /std:c++17 /O2 /EHsc tests\test_stoer_wagner.cpp /Fe:test_sw.exe
.\test_sw.exe
cl /std:c++17 /O2 /EHsc tests\test_karger_tree_packing.cpp /Fe:test_ktp.exe
.\test_ktp.exe
cl /std:c++17 /O2 /EHsc tests\test_karger.cpp /Fe:test_karger.exe
.\test_karger.exe

cl /std:c++17 /O2 /EHsc bench\bench_stoer_wagner.cpp /Fe:bench_sw.exe
.\bench_sw.exe
python scripts\plot_stoer_wagner.py

cl /std:c++17 /O2 /EHsc bench\bench_all.cpp /Fe:bench_all.exe
.\bench_all.exe
python scripts\plot_all.py

cl /std:c++17 /O2 /EHsc bench\bench_karger_tree_packing.cpp /Fe:bench_ktp.exe
.\bench_ktp.exe
python scripts\plot_tree_packing.py
```

## Methodology in brief

The full description is in section 3 of the report. In short:

- **Inputs:** every instance is built before timing starts, from fixed seeds, and every algorithm runs on the identical edge list. The input families are cycles, complete graphs, paths, stars, two clusters joined by `k` edges, two circulant graphs joined by `k` edges, and random connected graphs of varying density.
- **Exact answer:** Stoer-Wagner is the reference for checking the randomized algorithms.
- **Timing:** `std::chrono::steady_clock`, timing only the algorithm call. One untimed warm-up run, then repeated timed runs (5 for Stoer-Wagner, 3 for the slower randomized algorithms), reporting the median.
- **Time to about 99% success (`bench_all`):** Karger is run `ceil(ln(100) * n(n-1)/2)` times, the worst-case count for 99% success. Stoer-Wagner is run once, since it is exact. Tree packing uses `2n` trees, a chosen setting that is checked against the exact answer, not a guaranteed 99%.
- **Karger's success rate:** measured with single runs (`t = 1`) over many different seeds, with 95% Wilson confidence intervals, and compared with the 2/(n(n-1)) bound.
- **Tree packing's success rate:** measured over 200 seeds for each number of trees, on graphs whose minimum cut is not a single vertex (two clusters and two circulants), with 95% Wilson confidence intervals.

## Results in this repository

The files in `results/` were produced with:

- Compiler and flags: MSVC 19.51.36257 for x64 (Visual Studio 2026), `/std:c++17 /O2 /EHsc`
- Hardware: Lenovo Legion Slim 5 16AHP9, AMD Ryzen 7 8845HS (8 cores, 16 threads), 16 GB RAM, Windows 11 Home (build 10.0.26200), plugged in, Quiet Mode

## AI use

AI tools were used in this project. The per-member disclosure is in section 8 of the report.
