// Correctness tests for Karger's greedy tree packing.
//
// Build (from the repo root):
//   g++ -std=c++17 -O2 -Wall -Wextra tests/test_karger_tree_packing.cpp -o test_ktp
// Run:  ./test_ktp        (Windows: test_ktp.exe)

#include <algorithm>
#include <cstdio>
#include <vector>

#include "../graphs/graphs.hpp"
#include "../karger/karger_tree_packing.hpp"
#include "../stoer_wagner/stoer_wagner.hpp"

static int failures = 0;

static void check(bool ok, const char* what, int n, int expected, int got) {
    if (!ok) {
        ++failures;
        std::printf("FAIL  %-30s n=%-4d expected %d, got %d\n", what, n, expected, got);
    }
}

// Verifies that every packed tree spans all n vertices, has exactly n-1
// distinct edges, and that the stored loads match the trees.
static bool valid_packing(int n, const Edges& edges, const TreePacking& packing) {
    std::vector<int> counted(edges.size(), 0);

    for (const auto& tree : packing.trees) {
        if (static_cast<int>(tree.size()) != n - 1) return false;

        std::vector<int> seen(edges.size(), 0);
        std::vector<int> parent(n);
        for (int i = 0; i < n; ++i) parent[i] = i;

        auto find = [&](int x) {
            while (parent[x] != x) x = parent[x];
            return x;
        };

        for (int edge_index : tree) {
            if (edge_index < 0 || edge_index >= static_cast<int>(edges.size())) return false;
            if (seen[edge_index]++) return false;  // duplicate edge in one tree
            ++counted[edge_index];

            int a = find(edges[edge_index].first);
            int b = find(edges[edge_index].second);
            if (a == b) return false;  // cycle
            parent[a] = b;
        }

        int root = find(0);
        for (int v = 1; v < n; ++v)
            if (find(v) != root) return false;  // not spanning
    }

    return counted == packing.load;
}

static void check_packing(const char* what, int n, const Edges& edges, int num_trees) {
    TreePacking packing = karger_tree_packing(n, edges, num_trees);
    check(static_cast<int>(packing.trees.size()) == num_trees, what, n, num_trees,
          static_cast<int>(packing.trees.size()));
    check(valid_packing(n, edges, packing), "valid packing", n, 1, 0);
}

static void check_known(const char* what, int n, const Edges& edges, int expected) {
    check_packing(what, n, edges, 2 * n);
    int got = karger_tree_packing_min_cut(n, edges, 2 * n);
    check(got == expected, what, n, expected, got);
}

int main() {
    // 1. Known-answer graphs.
    for (int n = 3; n <= 30; ++n) {
        check_known("cycle", n, make_cycle(n), 2);
        check_known("complete", n, make_complete(n), n - 1);
    }
    for (int s = 4; s <= 10; ++s) {
        for (int k = 1; k < s - 1; ++k) {
            check_known("two clusters", 2 * s, make_two_clusters(s, k), k);
        }
    }

    Edges path, star;
    for (int i = 0; i + 1 < 12; ++i) path.push_back({i, i + 1});
    for (int i = 1; i < 12; ++i) star.push_back({0, i});
    check_known("path", 12, path, 1);
    check_known("star", 12, star, 1);
    check_known("single edge", 2, Edges{{0, 1}}, 1);
    std::puts("known-answer tests done");

    // 2. Compare the cut extracted from the packing with the exact min cut on
    // small random connected graphs.
    int trials = 0;
    for (int n = 3; n <= 14; ++n) {
        int max_m = n * (n - 1) / 2;
        for (int rep = 0; rep < 25; ++rep) {
            int m = (n - 1) + (rep * (max_m - (n - 1))) / 24;
            Edges edges = make_random_graph(n, m, 1000u * n + rep);
            int expected = stoer_wagner(n, edges);
            int got = karger_tree_packing_min_cut(n, edges, 2 * n);
            check(got == expected, "packing vs exact", n, expected, got);
            ++trials;
        }
    }
    std::printf("exact comparison done (%d graphs)\n", trials);

    // 3. Packing value should be positive and no larger than the min cut.
    for (int n : {5, 10, 20}) {
        int m = std::min(3 * n, n * (n - 1) / 2);
        Edges edges = make_random_graph(n, m, 7u * n);
        TreePacking packing = karger_tree_packing(n, edges, 4 * n);
        int exact = stoer_wagner(n, edges);
        check(valid_packing(n, edges, packing), "valid packing (larger)", n, 1, 0);
        check(packing.value() <= exact + 1e-9, "packing value <= min cut", n, exact,
              static_cast<int>(packing.value()));
    }
    std::puts("packing-value tests done");

    // 4. A disconnected graph cannot yield a spanning tree.
    Edges disconnected = {{0, 1}, {2, 3}};
    TreePacking none = karger_tree_packing(4, disconnected, 3);
    check(none.trees.empty(), "disconnected graph", 4, 0,
          static_cast<int>(none.trees.size()));

    if (failures == 0) std::puts("ALL KARGER TREE-PACKING TESTS PASSED");
    else std::printf("%d FAILURES\n", failures);
    return failures == 0 ? 0 : 1;
}
