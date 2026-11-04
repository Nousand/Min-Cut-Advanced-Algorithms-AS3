// Correctness tests for Karger's contraction algorithm.
// Primary author: Rodrigo Uzal Sequeros
//
// Build (from the repo root). MSVC, in a Developer PowerShell:
//   cl /std:c++17 /O2 /EHsc tests\test_karger.cpp /Fe:test_karger.exe
// or g++:
//   g++ -std=c++17 -O2 -Wall -Wextra tests/test_karger.cpp -o test_karger
// Run:  .\test_karger.exe        (Mac/Linux: ./test_karger)

#include <cstdio>
#include <vector>

#include "../graphs/graphs.hpp"
#include "../karger/karger.hpp"
#include "../stoer_wagner/stoer_wagner.hpp"

static int failures = 0;

static void check(bool ok, const char* what, int n, int expected, int got) {
    if (!ok) {
        ++failures;
        std::printf("FAIL %s (n=%d): expected %d, got %d\n", what, n, expected, got);
    }
}

int main() {
    // On tests 1-3 we only run one iteration as on these graphs every run must find the minimum cut.
    // 1. Cycles: every run ends on one of the n(n-1)/2 minimum cuts, so even a single run (t = 1) must return 2
    for (int n = 3; n <= 30; ++n)
        for (unsigned seed = 0; seed < 20; ++seed) {
            int got = karger_min_cut(n, make_cycle(n), 1, seed);
            check(got == 2, "cycle, one run", n, 2, got);
        }
    std::puts("cycles done");

    // 2. Trees: every edge is a bridge, so a single run must return 1
    //    (make_random_graph with m = n - 1 builds only its random spanning tree)
    for (int n = 2; n <= 30; ++n)
        for (unsigned seed = 0; seed < 20; ++seed) {
            int got = karger_min_cut(n, make_random_graph(n, n - 1, 1000u + n), 1, seed);
            check(got == 1, "tree, one run", n, 1, got);
        }
    std::puts("trees done");

    // 3. Multigraph: a cycle with every edge doubled has minimum cut 4, and a single run must return it
    for (int n = 3; n <= 20; ++n) {
        Edges edges = make_cycle(n);
        Edges copy = edges;
        edges.insert(edges.end(), copy.begin(), copy.end());
        for (unsigned seed = 0; seed < 20; ++seed) {
            int got = karger_min_cut(n, edges, 1, seed);
            check(got == 4, "doubled cycle (multigraph), one run", n, 4, got);
        }
    }
    std::puts("multigraph done");

    // 4. Never below the minimum: every run returns a real cut, so a single run can only overestimate
    int single_runs = 0;
    for (int n = 8; n <= 30; ++n)
        for (int m : {3 * n, n * (n - 1) / 4}) {
            Edges edges = make_random_graph(n, m, 500u + n + m);
            int exact = stoer_wagner(n, edges);
            for (unsigned seed = 0; seed < 50; ++seed) {
                int got = karger_min_cut(n, edges, 1, seed);
                check(got >= exact, "single run below the minimum cut", n, exact, got);
                ++single_runs;
            }
        }
    std::printf("never below the minimum done (%d single runs)\n", single_runs);

    // 5. Known answers with t = 3n(n-1)/2 runs: for complete graphs we must obtain (n - 1) and for two clusters (k)
    for (int n = 4; n <= 20; ++n) {
        int got = karger_min_cut(n, make_complete(n), 3 * n * (n - 1) / 2, 7u);
        check(got == n - 1, "complete graph", n, n - 1, got);
    }
    for (int s = 5; s <= 12; ++s)
        for (int k = 1; k < s - 1; ++k) {
            int n = 2 * s;
            int got = karger_min_cut(n, make_two_clusters(s, k), 3 * n * (n - 1) / 2, 7u);
            check(got == k, "two clusters", n, k, got);
        }
    std::puts("known answers done");

    // 6. Against the exact answer from Stoer-Wagner on random graphs (moved from tests/test_stoer_wagner.cpp)
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

    if (failures == 0) std::puts("ALL KARGER TESTS PASSED");
    else std::printf("%d KARGER TESTS FAILED\n", failures);
    return failures == 0 ? 0 : 1;
}