// Giant super-vertex check for Karger's algorithm (Section 5.1 of the report).
// Author: Rodrigo Uzal Sequeros, written with the help of Claude (Anthropic, Opus 5.5).
//
// Tests our explanation of why Karger succeeds so often on complete graphs: one super-vertex
// grows into a giant that absorbs the other vertices one at a time. Each run uses the same
// contraction as karger_alg in karger/karger.hpp, but stops when 40, 20, 10 or 3 super-vertices
// remain and records the size of the largest one. The complete graph is compared with the
// two-circulants graph, where we expect no giant to form. Results are printed to the screen.
//
// Build (from the repo root). MSVC, in a Developer PowerShell:
//   cl /std:c++17 /O2 /EHsc bench\check_giant.cpp /Fe:check_giant.exe
// or g++:
//   g++ -std=c++17 -O2 bench/check_giant.cpp -o check_giant
// Run:  .\check_giant.exe        (Mac/Linux: ./check_giant)

#include <cstdio>
#include <random>
#include <algorithm>
#include <vector>
#include <utility>
#include "../graphs/graphs.hpp"
#include "../karger/karger.hpp"

// Copied from bench/bench_all.cpp: two rings C_s(1,2) joined by k edges (unique minimum cut k, every degree >= 4)
static Edges make_two_circulants(int s, int k) {
    Edges e;
    for (int copy = 0; copy < 2; ++copy)
        for (int i = 0; i < s; ++i) {
            e.push_back({copy * s + i, copy * s + (i + 1) % s});
            e.push_back({copy * s + i, copy * s + (i + 2) % s});
        }
    for (int i = 0; i < k; ++i) e.push_back({i, s + i});
    return e;
}

// Same contraction as karger_alg, but it stops when 'stop_at' groups remain and returns the size of the largest group
static int largest_group(int n, Edges edges, int stop_at, std::mt19937& rng) {
    std::shuffle(edges.begin(), edges.end(), rng);
    UnionFind uf(n);
    for (int i = 0; uf.components > stop_at; i++)
        uf.unite(edges[i].first, edges[i].second);
    int largest = 0;
    for (int v = 0; v < n; v++)
        if (uf.find(v) == v) largest = std::max(largest, uf.size[v]);
    return largest;
}

static void check(const char* name, int n, const Edges& edges, int runs) {
    std::mt19937 rng(2026);
    for (int stop_at : {40, 20, 10, 3}) {
        double total = 0;
        int giant_and_lone = 0; // runs where every other group is a single vertex
        for (int r = 0; r < runs; r++) {
            int big = largest_group(n, edges, stop_at, rng);
            total += big;
            if (big == n - (stop_at - 1)) giant_and_lone++;
        }
        std::printf("%-15s n=%d  %2d groups left: largest group %5.1f vertices on average, "
                    "giant + lone vertices in %5.1f%% of runs\n",
                    name, n, stop_at, total / runs, 100.0 * giant_and_lone / runs);
    }
}

int main() {
    const int n = 80, runs = 2000;
    check("complete", n, make_complete(n), runs);
    check("two circulants", n, make_two_circulants(n / 2, 3), runs);
    return 0;
}