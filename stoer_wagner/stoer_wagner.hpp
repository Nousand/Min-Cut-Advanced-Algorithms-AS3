// Stoer-Wagner deterministic global minimum cut
// Primary author: Aarav Khemani
//
// Two implementations of the SAME algorithm (maximum adjacency ordering, take the
// cut-of-the-phase, merge the last two vertices, repeat n-1 times). They differ only
// in the data structure used to find "the vertex most strongly connected so far":
//
//   stoer_wagner(n, edges)     matrix version, scan for the maximum. O(n^3) time, O(n^2) memory
//   stoer_wagner_pq(n, edges)  priority queue version, adjacency maps + lazy-deletion heap.
//                              O(n * m * log n) time, O(n + m) memory
//
// Input contract (shared by the whole group): vertices 0..n-1, undirected edges listed
// once, unweighted, connected, no self-loops. Both functions return the min cut size.

#pragma once

#include <algorithm>
#include <climits>
#include <queue>
#include <unordered_map>
#include <utility>
#include <vector>

// ---------------------------------------------------------------------------
// Matrix version
// ---------------------------------------------------------------------------
inline int stoer_wagner(int n, const std::vector<std::pair<int, int>>& edges) {
    if (n < 2) return 0;

    // Contraction creates parallel edges, so we store edge counts (weights).
    std::vector<std::vector<int>> w(n, std::vector<int>(n, 0));
    for (const auto& e : edges) {
        w[e.first][e.second]++;
        w[e.second][e.first]++;
    }

    std::vector<char> merged(n, 0);
    int best = INT_MAX;

    for (int phase = 0; phase < n - 1; ++phase) {
        std::vector<int> key(n, 0);
        std::vector<char> added(n, 0);
        int prev = -1, last = -1;

        // Maximum adjacency ordering over the n - phase remaining vertices.
        for (int i = 0; i < n - phase; ++i) {
            int sel = -1;
            for (int j = 0; j < n; ++j)
                if (!merged[j] && !added[j] && (sel == -1 || key[j] > key[sel]))
                    sel = j;
            added[sel] = 1;
            prev = last;
            last = sel;
            for (int j = 0; j < n; ++j)
                if (!merged[j] && !added[j]) key[j] += w[sel][j];
        }

        // key[last] = weight of the cut separating `last` from everything else.
        best = std::min(best, key[last]);

        // Merge `last` into `prev`.
        for (int j = 0; j < n; ++j) {
            if (j == prev || j == last) continue;
            w[prev][j] += w[last][j];
            w[j][prev] = w[prev][j];
        }
        merged[last] = 1;
    }
    return best;
}

// ---------------------------------------------------------------------------
// Priority queue version
// ---------------------------------------------------------------------------
inline int stoer_wagner_pq(int n, const std::vector<std::pair<int, int>>& edges) {
    if (n < 2) return 0;

    // adj[v] maps neighbour -> number of parallel edges. Only active vertices appear.
    std::vector<std::unordered_map<int, int>> adj(n);
    for (const auto& e : edges) {
        adj[e.first][e.second]++;
        adj[e.second][e.first]++;
    }

    std::vector<char> merged(n, 0);
    int best = INT_MAX;

    for (int phase = 0; phase < n - 1; ++phase) {
        const int remaining = n - phase;
        std::vector<int> key(n, 0);
        std::vector<char> added(n, 0);

        // Max-heap of (key, vertex). std::priority_queue cannot decrease/increase a key
        // in place, so we push a fresh entry on every update and skip stale ones on pop.
        std::priority_queue<std::pair<int, int>> pq;
        for (int v = 0; v < n; ++v)
            if (!merged[v]) pq.push({0, v});

        int prev = -1, last = -1;
        int count = 0;
        while (count < remaining) {
            auto top = pq.top();
            pq.pop();
            int v = top.second;
            if (added[v] || top.first != key[v]) continue;  // stale entry

            added[v] = 1;
            prev = last;
            last = v;
            ++count;

            for (const auto& nb : adj[v]) {
                int u = nb.first;
                if (!added[u]) {
                    key[u] += nb.second;
                    pq.push({key[u], u});
                }
            }
        }

        best = std::min(best, key[last]);

        // Merge `last` into `prev`, keeping adjacency maps consistent.
        for (const auto& nb : adj[last]) {
            int u = nb.first;
            if (u == prev) continue;
            adj[prev][u] += nb.second;
            adj[u][prev] += nb.second;
            adj[u].erase(last);
        }
        adj[prev].erase(last);
        adj[last].clear();
        merged[last] = 1;
    }
    return best;
}
