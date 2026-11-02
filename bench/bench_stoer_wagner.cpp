// Timing benchmark: Stoer-Wagner matrix version vs priority queue version.
// Primary author: Aarav Khemani
//
// Build (from the repo root):
//   g++ -std=c++17 -O2 bench/bench_stoer_wagner.cpp -o bench_sw
// Run:
//   ./bench_sw                (Windows: .\bench_sw.exe)   -> writes results/stoer_wagner_bench.csv
//
// Method: every instance is built BEFORE timing starts; each algorithm gets 1 untimed
// warm-up run then REPS timed runs on the identical edge list; we report the median.
// The two versions must return the same cut size, otherwise the benchmark aborts.

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
#include "../stoer_wagner/stoer_wagner.hpp"

static const int REPS = 5;
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

static void run_instance(std::ofstream& out, const std::string& experiment, const std::string& family,
                         int n, const Edges& edges) {
    int m = (int)edges.size();
    Timing a = time_it([&] { return stoer_wagner(n, edges); });
    Timing b = time_it([&] { return stoer_wagner_pq(n, edges); });
    if (a.result != b.result) {
        std::fprintf(stderr, "MISMATCH on %s n=%d m=%d: matrix=%d pq=%d\n", family.c_str(), n, m, a.result, b.result);
        std::exit(1);
    }
    auto row = [&](const char* algo, const Timing& t) {
        out << experiment << ',' << family << ',' << n << ',' << m << ',' << algo << ','
            << t.median_us << ',' << t.min_us << ',' << REPS << ',' << t.result << '\n';
    };
    row("matrix", a);
    row("priority_queue", b);
    std::printf("%-16s %-14s n=%-5d m=%-7d matrix %10.0f us   pq %10.0f us\n", experiment.c_str(), family.c_str(),
                n, m, a.median_us, b.median_us);
    out.flush();
}

int main(int argc, char** argv) {
    std::string path = argc > 1 ? argv[1] : "results/stoer_wagner_bench.csv";
    std::ofstream out(path);
    if (!out) { std::fprintf(stderr, "Cannot open %s (does the results/ folder exist?)\n", path.c_str()); return 1; }
    out << "experiment,family,n,m,algorithm,median_us,min_us,reps,result\n";

    // Experiment 1: growth with n on SPARSE graphs (m = 3n)
    for (int n : {50, 100, 200, 400, 800, 1600})
        run_instance(out, "size_sparse", "random_m=3n", n, make_random_graph(n, 3 * n, SEED + n));

    // Experiment 2: growth with n on DENSE graphs (half of all possible edges)
    for (int n : {50, 100, 200, 300, 400})
        run_instance(out, "size_dense", "random_half", n, make_random_graph(n, n * (n - 1) / 4, SEED + n));

    // Experiment 3: density sweep at a fixed n, to find where matrix and priority queue cross over
    const int fixed_n = 300;
    const long long max_m = (long long)fixed_n * (fixed_n - 1) / 2;
    for (double d : {0.01, 0.02, 0.05, 0.1, 0.2, 0.4, 0.7, 1.0}) {
        int m = std::max(fixed_n - 1, (int)std::llround(d * max_m));
        Edges e = (d >= 1.0) ? make_complete(fixed_n) : make_random_graph(fixed_n, m, SEED + m);
        run_instance(out, "density_sweep", "random_density=" + std::to_string(d).substr(0, 4), fixed_n, e);
    }

    std::printf("\nWrote %s\n", path.c_str());
    return 0;
}
