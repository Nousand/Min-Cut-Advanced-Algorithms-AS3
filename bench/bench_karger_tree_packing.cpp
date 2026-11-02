// Timing benchmark: Karger greedy tree packing + 1-/2-respecting cut extraction.
//
// Build (from the repo root):
//   g++ -std=c++17 -O2 bench/bench_karger_tree_packing.cpp -o bench_ktp
// Run:
//   ./bench_ktp                (Windows: .\bench_ktp.exe)   -> writes results/bench_karger_tree_packing.csv
//
// Method: every instance is built BEFORE timing starts; each algorithm gets 1 untimed
// warm-up run then REPS timed runs on the identical edge list; we report the median.
//
// We time:
//   * tree_packing            : packing construction + min-cut extraction
//   * tree_packing_pack_only  : packing construction only (result = number of trees)
//
// The extracted cut is compared with Stoer-Wagner.  Tree packing is the simplified
// heuristic version, so a miss is reported in the CSV/console instead of aborting;
// a result smaller than the exact min cut is treated as a bug and aborts.

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
#include "../karger-tree-packing/karger_tree_packing.hpp"
#include "../stoer_wagner/stoer_wagner.hpp"

static const int REPS = 3;
static const unsigned SEED = 2026;

struct Timing { double median_us; double min_us; int result; };

static Timing time_it(const std::function<int()>& fn) {
    volatile int sink = fn();  // warm-up run, not timed
    (void)sink;
    std::vector<double> t;
    int result = 0;
    for (int r = 0; r < REPS; ++r) {
        auto start = std::chrono::steady_clock::now();
        result = fn();
        auto stop = std::chrono::steady_clock::now();
        t.push_back(std::chrono::duration<double, std::micro>(stop - start).count());
    }
    std::sort(t.begin(), t.end());
    return {t[t.size() / 2], t.front(), result};
}

// Two circulant graphs C_s(1,2), joined by k edges.  Not in graphs.hpp, but the
// same family is used by bench_all.cpp.
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

static Edges make_path(int n) {
    Edges e;
    for (int i = 0; i + 1 < n; ++i) e.push_back({i, i + 1});
    return e;
}

static Edges make_star(int n) {
    Edges e;
    for (int i = 1; i < n; ++i) e.push_back({0, i});
    return e;
}

static void run_instance(std::ofstream& out, const std::string& experiment, const std::string& family,
                         int n, const Edges& edges, int num_trees) {
    int m = (int)edges.size();
    int exact = stoer_wagner(n, edges);
    double packing_value = karger_tree_packing(n, edges, num_trees).value();

    Timing full = time_it([&] { return karger_tree_packing_min_cut(n, edges, num_trees); });
    Timing pack = time_it([&] { return karger_tree_packing(n, edges, num_trees).size(); });

    if (full.result < exact) {
        std::fprintf(stderr, "INVALID CUT on %s n=%d m=%d: tree_packing=%d < stoer_wagner=%d\n",
                     family.c_str(), n, m, full.result, exact);
        std::exit(1);
    }

    auto row = [&](const char* algo, const Timing& t) {
        out << experiment << ',' << family << ',' << n << ',' << m << ',' << algo << ','
            << num_trees << ',' << packing_value << ',' << t.median_us << ',' << t.min_us << ','
            << REPS << ',' << t.result << ',' << exact << '\n';
    };

    row("tree_packing", full);
    row("tree_packing_pack_only", pack);

    std::printf("%-16s %-22s n=%-5d m=%-7d trees=%-4d  total %9.0f us  pack %9.0f us  %s\n",
                experiment.c_str(), family.c_str(), n, m, num_trees, full.median_us, pack.median_us,
                full.result == exact ? "" : "(missed exact)");
    out.flush();
}

int main(int argc, char** argv) {
    std::string path = argc > 1 ? argv[1] : "results/bench_karger_tree_packing.csv";
    std::ofstream out(path);
    if (!out) { std::fprintf(stderr, "Cannot open %s (does the results/ folder exist?)\n", path.c_str()); return 1; }
    out << "experiment,family,n,m,algorithm,num_trees,packing_value,median_us,min_us,reps,result,exact\n";

    // Experiment 1: growth with n on sparse graphs (m = 3n), k = 2n trees.
    for (int n : {50, 100, 200, 400})
        run_instance(out, "size_sparse", "random_m=3n", n,
                     make_random_graph(n, 3 * n, SEED + n), 2 * n);

    // Experiment 2: growth with n on dense graphs (half of all possible edges), k = 2n trees.
    for (int n : {30, 60, 90, 120, 160}) {
        int m = n * (n - 1) / 4;
        run_instance(out, "size_dense", "random_half", n,
                     make_random_graph(n, m, SEED + n + 1234), 2 * n);
    }

    // Experiment 3: density sweep at a fixed n, k = 2n trees.
    const int fixed_n = 60;
    const long long max_m = (long long)fixed_n * (fixed_n - 1) / 2;
    for (double d : {0.01, 0.05, 0.1, 0.2, 0.4, 0.7, 1.0}) {
        int m = std::max(fixed_n - 1, (int)std::llround(d * max_m));
        Edges e = (d >= 1.0) ? make_complete(fixed_n) : make_random_graph(fixed_n, m, SEED + m + 777);
        std::string fam = "random_density=" + std::to_string(d).substr(0, 4);
        run_instance(out, "density_sweep", fam, fixed_n, e, 2 * fixed_n);
    }

    // Experiment 4: effect of the number of packed trees at fixed n.
    {
        const int n = 80;
        Edges e = make_random_graph(n, 3 * n, SEED + n + 9001);
        for (int k_trees : {n / 2, n, 2 * n, 4 * n})
            run_instance(out, "tree_count_sweep", "random_m=3n_k=" + std::to_string(k_trees),
                         n, e, k_trees);
    }

    // Experiment 5: structured families.
    {
        const int n = 60, s = n / 2;
        run_instance(out, "structured", "cycle", n, make_cycle(n), 2 * n);
        run_instance(out, "structured", "complete", n, make_complete(n), 2 * n);
        run_instance(out, "structured", "two_clusters_k=2", n, make_two_clusters(s, 2), 2 * n);
        run_instance(out, "structured", "two_circulants_k=3", n, make_two_circulants(s, 3), 2 * n);
        run_instance(out, "structured", "path", 80, make_path(80), 2 * 80);
        run_instance(out, "structured", "star", 80, make_star(80), 2 * 80);
    }

    std::printf("\nWrote %s\n", path.c_str());
    return 0;
}
