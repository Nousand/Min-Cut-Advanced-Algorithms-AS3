// Correctness tests for Stoer-Wagner (matrix and priority queue versions).
// Primary author: <your name>
//
// Build (from the repo root):
//   g++ -std=c++17 -O2 -Wall -Wextra tests/test_stoer_wagner.cpp -o test_sw
// Run:  ./test_sw        (Windows: test_sw.exe)

#include <cstdio>
#include <vector>

#include "../graphs/graphs.hpp"
#include "../karger/karger.hpp"
#include "../stoer_wagner/stoer_wagner.hpp"

static int failures = 0;

static void check(bool ok, const char* what, int n, int expected, int got) {
    if (!ok) {
        ++failures;
        std::printf("FAIL  %-28s n=%-4d expected %d, got %d\n", what, n, expected, got);
    }
}

// Exhaustive minimum cut: try every bipartition with vertex 0 fixed on side A.
static int brute_force(int n, const Edges& edges) {
    int best = (int)edges.size();
    for (long long mask = 1; mask < (1LL << (n - 1)); ++mask) {  // bit i-1 set => vertex i on side B
        int cut = 0;
        for (const auto& e : edges) {
            int a = e.first == 0 ? 0 : (int)((mask >> (e.first - 1)) & 1);
            int b = e.second == 0 ? 0 : (int)((mask >> (e.second - 1)) & 1);
            if (a != b) ++cut;
        }
        best = std::min(best, cut);
    }
    return best;
}

static void check_both(const char* what, int n, const Edges& edges, int expected) {
    check(stoer_wagner(n, edges) == expected, "matrix", n, expected, stoer_wagner(n, edges));
    check(stoer_wagner_pq(n, edges) == expected, "priority queue", n, expected, stoer_wagner_pq(n, edges));
    (void)what;
}

int main() {
    // 1. Graphs with a known answer
    for (int n = 3; n <= 60; ++n) {
        check_both("cycle", n, make_cylce(n), 2);
        check_both("complete", n, make_complete(n), n - 1);
        Edges path, star;
        for (int i = 0; i + 1 < n; ++i) path.push_back({i, i + 1});
        for (int i = 1; i < n; ++i) star.push_back({0, i});
        check_both("path", n, path, 1);
        check_both("star", n, star, 1);
    }
    for (int s = 4; s <= 20; ++s)
        for (int k = 1; k < s - 1; ++k)
            check_both("two clusters", 2 * s, make_two_clusters(s, k), k);
    check_both("single edge", 2, Edges{{0, 1}}, 1);
    std::puts("known-answer tests done");

    // 2. Against brute force on small random connected graphs
    int trials = 0;
    for (int n = 2; n <= 14; ++n) {
        int max_m = n * (n - 1) / 2;
        for (int rep = 0; rep < 40; ++rep) {
            int m = (n - 1) + (rep * (max_m - (n - 1))) / 39;  // sweep sparse -> dense
            Edges edges = make_random_graph(n, m, 1000u * n + rep);
            int expected = brute_force(n, edges);
            check(stoer_wagner(n, edges) == expected, "matrix vs brute force", n, expected, stoer_wagner(n, edges));
            check(stoer_wagner_pq(n, edges) == expected, "pq vs brute force", n, expected, stoer_wagner_pq(n, edges));
            ++trials;
        }
    }
    std::printf("brute-force comparison done (%d graphs)\n", trials);

    // 3. The two versions must agree on larger graphs
    int big = 0;
    for (int n : {30, 60, 100, 150}) {
        for (int m : {n - 1, 2 * n, 5 * n, n * (n - 1) / 4, n * (n - 1) / 2}) {
            if (m < n - 1 || m > n * (n - 1) / 2) continue;
            Edges edges = make_random_graph(n, m, 7u * n + m);
            int a = stoer_wagner(n, edges), b = stoer_wagner_pq(n, edges);
            check(a == b, "matrix vs pq (large)", n, a, b);
            ++big;
        }
    }
    std::printf("matrix vs priority queue agreement done (%d graphs)\n", big);

    // 4. Sanity check of the teammate's Karger against the exact answer (probabilistic, so informational)
    int karger_miss = 0, karger_runs = 0;
    for (int n : {10, 20, 30}) {
        Edges edges = make_random_graph(n, 3 * n, 99u + n);
        int exact = stoer_wagner(n, edges);
        int t = 3 * n * (n - 1) / 2;
        int got = karger_min_cut(n, edges, t, 12345u);
        ++karger_runs;
        if (got != exact) { ++karger_miss; std::printf("note: karger gave %d, exact %d (n=%d, t=%d)\n", got, exact, n, t); }
    }
    std::printf("karger vs exact: %d/%d matched\n", karger_runs - karger_miss, karger_runs);

    if (failures == 0) std::puts("ALL STOER-WAGNER TESTS PASSED");
    else std::printf("%d FAILURES\n", failures);
    return failures == 0 ? 0 : 1;
}
