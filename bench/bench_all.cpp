// Shared benchmark: Stoer-Wagner (matrix + priority queue), Karger, and Karger's tree packing.
// Written to the group's agreed interface: n, edges, returns the min cut size as an int.
//
// Build (from the repo root). MSVC, in a Developer PowerShell:
//   cl /std:c++17 /O2 /EHsc bench\bench_all.cpp /Fe:bench_all.exe
// or g++:
//   g++ -std=c++17 -O2 bench/bench_all.cpp -o bench_all
// Run:
//   ./bench_all                  (Windows: .\bench_all.exe)
// Writes:
//   results/bench_time.csv       time to reach ~99% success (Karger runs t = ceil(ln(100) * n(n-1)/2) times;
//                                tree packing uses 2n trees)
//   results/bench_success.csv    Karger single-run success rate vs the 2/(n(n-1)) bound, per graph family
//   results/bench_ksweep.csv     Karger success rate on two-cluster graphs as the planted cut k grows
//   results/bench_treesweep.csv  tree packing success rate vs number of trees, on structured graphs

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <string>
#include <vector>

#include "../graphs/graphs.hpp"
#include "../karger/karger.hpp"
#include "../stoer_wagner/stoer_wagner.hpp"
#include "../karger-tree-packing/karger_tree_packing.hpp"


// ----------------------------------------------------------------- settings
static const unsigned SEED = 2026;
static const int TIMING_REPS = 5;        // timed repetitions for the deterministic algorithms
static const int KARGER_REPS = 3;        // Karger with t ~ thousands of runs is slow, so fewer repetitions
static const int SUCCESS_TRIALS = 20000; // single Karger runs per instance in the success-rate experiment
static const double TARGET_FAIL = 0.01;  // 99% success

// ----------------------------------------------------------------- extra family (not in graphs.hpp)
// Two circulant graphs C_s(1,2) (each vertex joined to the two vertices on either side, degree 4),
// joined by k edges (k < 4). Unique minimum cut = k, while every vertex has degree >= 4 > k:
// the cut is only slightly smaller than the vertex degrees, which is hard for random contraction.
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

// ----------------------------------------------------------------- timing helper
struct Timing { double median_us, min_us; int result; };

static Timing time_it(const std::function<int()>& fn, int reps) {
    volatile int sink = fn();  // untimed warm-up
    (void)sink;
    std::vector<double> t;
    int result = 0;
    for (int r = 0; r < reps; ++r) {
        auto a = std::chrono::steady_clock::now();
        result = fn();
        auto b = std::chrono::steady_clock::now();
        t.push_back(std::chrono::duration<double, std::micro>(b - a).count());
    }
    std::sort(t.begin(), t.end());
    return {t[t.size() / 2], t.front(), result};
}

static void wilson(int successes, int trials, double& lo, double& hi) {
    const double z = 1.96, nn = trials, p = (double)successes / trials;
    double denom = 1 + z * z / nn;
    double centre = (p + z * z / (2 * nn)) / denom;
    double half = z * std::sqrt(p * (1 - p) / nn + z * z / (4 * nn * nn)) / denom;
    lo = std::max(0.0, centre - half);
    hi = std::min(1.0, centre + half);
}

// ----------------------------------------------------------------- experiment 1: time to ~99% success
static void time_instance(std::ofstream& out, const std::string& family, int n, const Edges& edges) {
    int m = (int)edges.size();
    int exact = stoer_wagner(n, edges);
    int t = (int)std::ceil(std::log(1.0 / TARGET_FAIL) * n * (n - 1) / 2.0);

    auto row = [&](const char* algo, int runs, const Timing& tm, int reps) {
        out << family << ',' << n << ',' << m << ',' << algo << ',' << runs << ',' << tm.median_us << ','
            << tm.min_us << ',' << reps << ',' << tm.result << ',' << exact << '\n';
    };
    Timing a = time_it([&] { return stoer_wagner(n, edges); }, TIMING_REPS);
    Timing b = time_it([&] { return stoer_wagner_pq(n, edges); }, TIMING_REPS);
    Timing c = time_it([&] { return karger_min_cut(n, edges, t, 7u * SEED + n); }, KARGER_REPS); // separate seed stream for the timing runs
    row("stoer_wagner_matrix", 1, a, TIMING_REPS);
    row("stoer_wagner_pq", 1, b, TIMING_REPS);
    row("karger", t, c, KARGER_REPS);
    const int tp_trees = 2 * n;  // number of trees used for timing; Experiment 4 measures how many are actually needed
    Timing d = time_it([&] { return karger_tree_packing_min_cut(n, edges, tp_trees, SEED + n); }, KARGER_REPS);
    row("tree_packing", tp_trees, d, KARGER_REPS);
    std::printf("time    %-18s n=%-4d m=%-6d matrix %9.0f us  pq %9.0f us  karger(t=%d) %10.0f us %s  tree packing(k=%d) %10.0f us %s\n",
                family.c_str(), n, m, a.median_us, b.median_us, t, c.median_us, c.result == exact ? "" : "(missed!)",
                tp_trees, d.median_us, d.result == exact ? "" : "(missed!)");
    out.flush();
}

// ----------------------------------------------------------------- experiment 2: Karger single-run success rate
static void success_instance(std::ofstream& out, const std::string& family, int n, const Edges& edges) {
    int m = (int)edges.size();
    int exact = stoer_wagner(n, edges);
    int successes = 0;
    for (int i = 0; i < SUCCESS_TRIALS; ++i)
        if (karger_min_cut(n, edges, 1, SEED * 31u + (unsigned)i) == exact) ++successes;

    double p = (double)successes / SUCCESS_TRIALS, lo, hi;
    wilson(successes, SUCCESS_TRIALS, lo, hi);
    double bound = 2.0 / ((double)n * (n - 1));
    double t_formula = std::log(1.0 / TARGET_FAIL) / bound;  // worst-case repetitions for 99%
    double t_needed = (p <= 0) ? -1 : (p >= 1 ? 1 : std::log(TARGET_FAIL) / std::log(1 - p));

    out << family << ',' << n << ',' << m << ',' << exact << ',' << SUCCESS_TRIALS << ',' << successes << ','
        << p << ',' << lo << ',' << hi << ',' << bound << ',' << t_formula << ',' << t_needed << '\n';
    std::printf("success %-18s n=%-4d m=%-6d p_hat=%.4f [%.4f, %.4f]  bound=%.5f  t needed=%.1f vs formula=%.0f\n",
                family.c_str(), n, m, p, lo, hi, bound, t_needed, t_formula);
    out.flush();
}


// ----------------------------------------------------------------- experiment 4: tree packing success vs number of trees
// Karger's tree packing only returns the true minimum cut if some packed tree crosses it at most twice.
// Here we measure how often that happens as the number of trees grows, on graphs whose minimum cut is NOT
// simply a single vertex (so a 1-respecting cut of a vertex cannot find it by luck).
static const int TREE_TRIALS = 200;  // independent seeds per (instance, number of trees)

static void treesweep_instance(std::ofstream& out, const std::string& family, int n, const Edges& edges,
                               const std::vector<int>& tree_counts) {
    int m = (int)edges.size();
    int exact = stoer_wagner(n, edges);
    for (int k : tree_counts) {
        int successes = 0;
        for (int i = 0; i < TREE_TRIALS; ++i)
            if (karger_tree_packing_min_cut(n, edges, k, SEED * 17u + 1000u * (unsigned)k + (unsigned)i) == exact)
                ++successes;
        double p = (double)successes / TREE_TRIALS, lo, hi;
        wilson(successes, TREE_TRIALS, lo, hi);
        out << family << ',' << n << ',' << m << ',' << exact << ',' << k << ',' << TREE_TRIALS << ','
            << successes << ',' << p << ',' << lo << ',' << hi << '\n';
        std::printf("trees   %-24s n=%-4d trees=%-3d p_hat=%.3f [%.3f, %.3f]\n", family.c_str(), n, k, p, lo, hi);
        out.flush();
    }
}

int main() {
    std::ofstream tout("results/bench_time.csv"), sout("results/bench_success.csv");
    if (!tout || !sout) { std::fprintf(stderr, "Cannot open results/ files (does the results/ folder exist?)\n"); return 1; }
    tout << "family,n,m,algorithm,runs,median_us,min_us,reps,result,exact\n";
    sout << "family,n,m,exact,trials,successes,p_hat,ci_lo,ci_hi,bound,t_formula,t_needed\n";
    // A third results file stores Karger's success rate on two-cluster graphs as the cut size k grows
    // A fourth results file stores tree packing's success rate as the number of trees grows
    std::ofstream trout("results/bench_treesweep.csv");
    trout << "family,n,m,exact,num_trees,trials,successes,p_hat,ci_lo,ci_hi\n";
    std::ofstream kout("results/bench_ksweep.csv");
    kout << "family,n,m,exact,trials,successes,p_hat,ci_lo,ci_hi,bound,t_formula,t_needed\n";

    // Experiment 1: time to reach ~99% success. Sizes are small because Karger needs ~2.3 n^2 runs.
    for (int n : {20, 40, 80, 160})
        time_instance(tout, "sparse_m=3n", n, make_random_graph(n, 3 * n, SEED + n));
    for (int n : {20, 40, 80})
        time_instance(tout, "dense_m=n(n-1)/4", n, make_random_graph(n, n * (n - 1) / 4, SEED + n));

    // Experiment 2: single-run success rate on structured families.
    for (int n : {10, 20, 40, 80}) {
        int s = n / 2;
        success_instance(sout, "cycle", n, make_cycle(n));
        success_instance(sout, "complete", n, make_complete(n));
        success_instance(sout, "two_clusters_k=2", n, make_two_clusters(s, 2));
        success_instance(sout, "two_circulants_k=3", n, make_two_circulants(s, 3));
        success_instance(sout, "random_m=3n", n, make_random_graph(n, 3 * n, SEED + n));
        success_instance(sout, "random_dense", n, make_random_graph(n, n * (n - 1) / 4, SEED + n));
    }

    // Experiment 3: two-cluster graphs with a growing planted cut k (prediction 4)
    const int cluster_size = 20;
    for (int k : {1, 2, 4, 8, 12, 16, 18})
        success_instance(kout, "two_clusters_s=20_k=" + std::to_string(k), 2 * cluster_size,
                         make_two_clusters(cluster_size, k));

    // Experiment 4: tree packing success rate vs number of trees, on structured graphs with a non-trivial minimum cut
    const std::vector<int> tree_counts = {1, 2, 3, 4, 6, 8, 12, 16, 24, 32, 48, 80};
    for (int k : {2, 8, 18})
        treesweep_instance(trout, "two_clusters_s=20_k=" + std::to_string(k), 40, make_two_clusters(20, k), tree_counts);
    treesweep_instance(trout, "two_circulants_s=20_k=3", 40, make_two_circulants(20, 3), tree_counts);
    treesweep_instance(trout, "two_circulants_s=40_k=3", 80, make_two_circulants(40, 3), tree_counts);

    std::puts("\nWrote results/bench_time.csv, results/bench_success.csv, results/bench_ksweep.csv and results/bench_treesweep.csv");
    return 0;
}
