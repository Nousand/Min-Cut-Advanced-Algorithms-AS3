// Karger's greedy spanning-tree packing
//
// A tree packing is a collection of spanning trees of the same graph. Karger's
// tree-packing approach to minimum cuts uses the following greedy construction:
//
//   * give every graph edge a weight equal to the number of trees that already
//     use it (its current "load"),
//   * compute a minimum spanning tree (MST) under those weights,
//   * add that MST to the packing and increase the load of its edges,
//   * repeat.
//
// Putting the next tree on the least-used edges keeps the loads balanced and
// makes the packing approximate a maximum (fractional) tree packing.  If a
// packing contains k trees and the largest edge load is L, scaling every tree
// by 1/L gives a valid weighted packing of value k/L.
//
// The implementation follows the same Kruskal + union-find style as the
// contraction algorithm in karger.hpp.
//
// Input contract (shared by the whole group): vertices 0..n-1, undirected
// edges listed once, unweighted, connected, no self-loops.

#pragma once

#include <algorithm>
#include <numeric>
#include <utility>
#include <vector>

#include "karger.hpp"  // UnionFind

// A packed tree is stored as the indices of its edges in the original edge
// list.  This makes it easy to check the load of every edge and to recover the
// actual endpoints when needed.
struct TreePacking {
    std::vector<std::vector<int>> trees;
    std::vector<int> load;  // load[e] = number of packed trees containing edge e

    int size() const { return static_cast<int>(trees.size()); }

    int max_load() const {
        int result = 0;
        for (int value : load) result = std::max(result, value);
        return result;
    }

    // Value of the packing after scaling each tree by 1 / max_load().
    double value() const {
        int heaviest = max_load();
        return heaviest == 0 ? 0.0 : static_cast<double>(size()) / heaviest;
    }
};

// Builds `num_trees` spanning trees.  At every step Kruskal is run with edge
// weight = current load, so the next tree uses the least-used edges first.
// Ties are broken by the original edge order, which keeps the result
// deterministic.
//
// If the graph is disconnected, fewer than `num_trees` trees are returned (the
// construction stops as soon as a spanning tree cannot be completed).
inline TreePacking karger_tree_packing(
    int n, const std::vector<std::pair<int, int>>& edges, int num_trees) {
    TreePacking packing;
    packing.load.assign(edges.size(), 0);

    if (n <= 1 || num_trees <= 0) return packing;

    std::vector<int> order(edges.size());
    std::iota(order.begin(), order.end(), 0);

    for (int t = 0; t < num_trees; ++t) {
        // Kruskal: consider edges from lightest (least used) to heaviest.
        // stable_sort preserves the previous order among equal loads.
        std::stable_sort(order.begin(), order.end(), [&](int a, int b) {
            return packing.load[a] < packing.load[b];
        });

        UnionFind uf(n);
        std::vector<int> tree;
        tree.reserve(n - 1);

        for (int edge_index : order) {
            int u = edges[edge_index].first;
            int v = edges[edge_index].second;
            if (uf.unite(u, v)) {
                tree.push_back(edge_index);
                if (static_cast<int>(tree.size()) == n - 1) break;
            }
        }

        // The graph is disconnected: no spanning tree can be completed.
        if (static_cast<int>(tree.size()) != n - 1) break;

        for (int edge_index : tree) ++packing.load[edge_index];
        packing.trees.push_back(std::move(tree));
    }

    return packing;
}

// ---------------------------------------------------------------------------
// Extracting a minimum cut from a packing
// ---------------------------------------------------------------------------
//
// For a fixed spanning tree T, every cut crosses at least one tree edge.
// Karger's key observation is that a minimum cut is crossed by at most two
// edges of at least one tree in a sufficiently large packing.  This helper
// enumerates, for every tree in the packing, all cuts that cross at most two
// tree edges and returns the smallest one found.
//
// Removing one tree edge splits T into two parts; removing two tree edges
// splits T into three parts, and the unique cut crossing exactly those two
// edges is the "middle" part.  The enumeration below is O(k * n^2 * m) for a
// packing of k trees, which is fine for the small/medium graphs used in the
// tests; Karger's near-linear implementation replaces this enumeration with a
// dynamic program on the tree.

namespace karger_tree_packing_detail {

inline int cut_size(const std::vector<std::pair<int, int>>& edges,
                    const std::vector<char>& side) {
    int cut = 0;
    for (const auto& e : edges) {
        if (side[e.first] != side[e.second]) ++cut;
    }
    return cut;
}

}  // namespace karger_tree_packing_detail

// Returns the smallest cut found by inspecting all cuts that are 1- or
// 2-respecting with respect to a tree in `packing`.  Every returned value is
// the size of a real cut of the graph, so it is always >= the true min cut; if
// some packed tree 2-respects a minimum cut, the result is exactly the min cut.
inline int karger_tree_packing_min_cut(
    int n, const std::vector<std::pair<int, int>>& edges,
    const TreePacking& packing) {
    using karger_tree_packing_detail::cut_size;

    if (n <= 1) return 0;

    int best = static_cast<int>(edges.size());
    std::vector<char> side(n);

    for (const auto& tree : packing.trees) {
        // Adjacency of the tree, with the position of the tree edge used to
        // reach each neighbour.
        std::vector<std::vector<std::pair<int, int>>> adj(n);
        for (int pos = 0; pos < static_cast<int>(tree.size()); ++pos) {
            int u = edges[tree[pos]].first;
            int v = edges[tree[pos]].second;
            adj[u].push_back({v, pos});
            adj[v].push_back({u, pos});
        }

        // Root the tree at vertex 0 and compute DFS entry/exit times.  The
        // subtree of a vertex v is exactly the interval [tin[v], tout[v]).
        std::vector<int> parent(n, -1);
        std::vector<int> tin(n, 0), tout(n, 0), subtree_size(n, 1);
        std::vector<int> dfs_order;
        dfs_order.reserve(n);

        // Iterative DFS preorder.  Pushing neighbours in reverse order is not
        // required for correctness; it just makes the order predictable.
        std::vector<int> stack;
        stack.push_back(0);
        parent[0] = 0;
        while (!stack.empty()) {
            int v = stack.back();
            stack.pop_back();
            tin[v] = static_cast<int>(dfs_order.size());
            dfs_order.push_back(v);

            for (const auto& next : adj[v]) {
                int u = next.first;
                if (parent[u] != -1) continue;
                parent[u] = v;
                stack.push_back(u);
            }
        }

        // In preorder the subtree of v occupies [tin[v], tin[v] + size[v]).
        for (int i = n - 1; i > 0; --i) {
            int v = dfs_order[i];
            subtree_size[parent[v]] += subtree_size[v];
        }
        for (int v = 0; v < n; ++v) tout[v] = tin[v] + subtree_size[v];

        auto is_ancestor = [&](int a, int b) {
            return tin[a] <= tin[b] && tin[b] < tout[a];
        };

        // 1-respecting cuts: one side is the subtree of a non-root vertex.
        for (int v = 1; v < n; ++v) {
            std::fill(side.begin(), side.end(), 0);
            for (int u = 0; u < n; ++u) {
                if (tin[v] <= tin[u] && tin[u] < tout[v]) side[u] = 1;
            }
            best = std::min(best, cut_size(edges, side));
        }

        // 2-respecting cuts.
        for (int a = 1; a < n; ++a) {
            for (int b = a + 1; b < n; ++b) {
                std::fill(side.begin(), side.end(), 0);

                if (is_ancestor(a, b)) {
                    // Middle part: subtree(a) \ subtree(b).
                    for (int u = 0; u < n; ++u) {
                        bool in_a = tin[a] <= tin[u] && tin[u] < tout[a];
                        bool in_b = tin[b] <= tin[u] && tin[u] < tout[b];
                        side[u] = in_a && !in_b;
                    }
                } else if (is_ancestor(b, a)) {
                    // Middle part: subtree(b) \ subtree(a).
                    for (int u = 0; u < n; ++u) {
                        bool in_a = tin[a] <= tin[u] && tin[u] < tout[a];
                        bool in_b = tin[b] <= tin[u] && tin[u] < tout[b];
                        side[u] = in_b && !in_a;
                    }
                } else {
                    // Middle part: everything outside subtree(a) U subtree(b).
                    for (int u = 0; u < n; ++u) {
                        bool in_a = tin[a] <= tin[u] && tin[u] < tout[a];
                        bool in_b = tin[b] <= tin[u] && tin[u] < tout[b];
                        side[u] = !in_a && !in_b;
                    }
                }

                best = std::min(best, cut_size(edges, side));
            }
        }
    }

    return best;
}

// Convenience overload: pack `num_trees` trees and immediately extract the best
// 1-/2-respecting cut from the resulting packing.
inline int karger_tree_packing_min_cut(
    int n, const std::vector<std::pair<int, int>>& edges, int num_trees) {
    return karger_tree_packing_min_cut(
        n, edges, karger_tree_packing(n, edges, num_trees));
}
