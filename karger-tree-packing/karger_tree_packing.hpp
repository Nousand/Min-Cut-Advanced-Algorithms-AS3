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
// The MST choice is randomized by shuffling equal-load edges before each
// Kruskal pass.  This keeps the load-balancing rule but removes the deterministic
// dependence on the original edge order.
//
// The implementation follows the same Kruskal + union-find style as the
// contraction algorithm in karger.hpp.
//
// Input contract (shared by the whole group): vertices 0..n-1, undirected
// edges listed once, unweighted, connected, no self-loops.

#pragma once

#include <algorithm>
#include <numeric>
#include <random>
#include <utility>
#include <vector>

#include "../karger/karger.hpp"  // UnionFind

namespace karger_tree_packing_detail {

// Fresh seed for callers that do not pass one explicitly.
inline unsigned random_seed() {
    static std::random_device device;
    return device();
}

}  // namespace karger_tree_packing_detail

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
// Equal-load edges are shuffled before each Kruskal pass, so the MST choice is
// randomized instead of being tied to the original edge order.  Pass `seed` for
// a reproducible run; omitting it uses a fresh random seed.
//
// If the graph is disconnected, fewer than `num_trees` trees are returned (the
// construction stops as soon as a spanning tree cannot be completed).
inline TreePacking karger_tree_packing(
    int n, const std::vector<std::pair<int, int>>& edges, int num_trees,
    unsigned seed) {
    TreePacking packing;
    packing.load.assign(edges.size(), 0);

    if (n <= 1 || num_trees <= 0) return packing;

    std::mt19937 rng(seed);
    std::vector<int> order(edges.size());
    std::iota(order.begin(), order.end(), 0);

    for (int t = 0; t < num_trees; ++t) {
        // Randomize the order of equal-load edges.  stable_sort then keeps the
        // shuffled order among ties, producing a random minimum spanning tree.
        std::shuffle(order.begin(), order.end(), rng);
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

// Fresh-random-seed convenience overload.
inline TreePacking karger_tree_packing(
    int n, const std::vector<std::pair<int, int>>& edges, int num_trees) {
    return karger_tree_packing(n, edges, num_trees,
                               karger_tree_packing_detail::random_seed());
}

// ---------------------------------------------------------------------------
// Extracting a minimum cut from a packing
// ---------------------------------------------------------------------------
//
// For a fixed spanning tree T, every cut crosses at least one tree edge.
// Karger's key observation is that a minimum cut is crossed by at most two
// edges of at least one tree in a sufficiently large packing.
//
// We avoid link-cut trees and dynamic LCA structures.  Instead, for every tree
// we root it once with an ordinary DFS and use array-based subtree sums:
//
//   * 1-respecting cuts are obtained from cut[subtree(v)].
//   * For 2-respecting cuts, fix one tree edge e_a and let A be the subtree
//     below it.  For every other edge e_b whose child is not an ancestor of a,
//     the middle piece of the cut is either A \ subtree(b) or its complement's
//     difference with subtree(b).  In both cases
//
//         cut(middle) = cut(A) + cut(subtree(b)) - 2 * cross[b],
//
//     where cross[b] counts graph edges from subtree(b) to the opposite side
//     of e_a.  The array cross[] is built with one DFS and a bottom-up sum.
//
// This gives O(k * n * (n + m)) for a packing of k trees.  With k = O(log n)
// that is the simplified O(n^2 log n) / O(m n log n) routine, instead of the
// near-linear implementation that needs a link-cut tree.

// Returns the smallest cut found by inspecting all cuts that are 1- or
// 2-respecting with respect to a tree in `packing`.  Every returned value is
// the size of a real cut of the graph, so it is always >= the true min cut; if
// some packed tree 2-respects a minimum cut, the result is exactly the min cut.
inline int karger_tree_packing_min_cut(
    int n, const std::vector<std::pair<int, int>>& edges,
    const TreePacking& packing) {
    if (n <= 1) return 0;

    int best = static_cast<int>(edges.size());

    for (const auto& tree : packing.trees) {
        // Adjacency of the packed tree.
        std::vector<std::vector<int>> adj(n);
        for (int edge_index : tree) {
            int u = edges[edge_index].first;
            int v = edges[edge_index].second;
            adj[u].push_back(v);
            adj[v].push_back(u);
        }

        // Root the tree at vertex 0 and compute DFS order / subtree intervals.
        std::vector<int> parent(n, -1);
        std::vector<int> tin(n, 0), tout(n, 0), subtree_size(n, 1);
        std::vector<int> dfs_order;
        dfs_order.reserve(n);

        // Iterative DFS preorder.
        std::vector<int> stack;
        stack.push_back(0);
        parent[0] = 0;
        while (!stack.empty()) {
            int v = stack.back();
            stack.pop_back();
            tin[v] = static_cast<int>(dfs_order.size());
            dfs_order.push_back(v);

            for (int u : adj[v]) {
                if (parent[u] != -1) continue;
                parent[u] = v;
                stack.push_back(u);
            }
        }

        // In preorder, subtree(v) = [tin[v], tin[v] + size[v]).
        for (int i = n - 1; i > 0; --i) {
            int v = dfs_order[i];
            subtree_size[parent[v]] += subtree_size[v];
        }
        for (int v = 0; v < n; ++v) tout[v] = tin[v] + subtree_size[v];

        auto is_ancestor = [&](int a, int b) {
            return tin[a] <= tin[b] && tin[b] < tout[a];
        };
        auto in_subtree = [&](int root, int v) {
            return tin[root] <= tin[v] && tin[v] < tout[root];
        };

        // cut[v] = number of graph edges leaving subtree(v), for every
        // non-root vertex v (the root's subtree is the whole graph).
        std::vector<int> cut(n, 0);
        for (int v = 1; v < n; ++v) {
            int count = 0;
            for (const auto& e : edges) {
                if (in_subtree(v, e.first) != in_subtree(v, e.second)) ++count;
            }
            cut[v] = count;
            best = std::min(best, count);
        }

        std::vector<char> in_a(n);
        std::vector<int> opposite(n, 0), cross(n, 0);

        // Fix the first tree edge via its child vertex a.
        for (int a = 1; a < n; ++a) {
            for (int v = 0; v < n; ++v) in_a[v] = in_subtree(a, v) ? 1 : 0;

            // opposite[v] counts graph edges crossing e_a and incident to v.
            std::fill(opposite.begin(), opposite.end(), 0);
            for (const auto& e : edges) {
                if (in_a[e.first] != in_a[e.second]) {
                    ++opposite[e.first];
                    ++opposite[e.second];
                }
            }

            // cross[v] = sum of opposite[x] over x in subtree(v).
            cross = opposite;
            for (int i = n - 1; i > 0; --i) {
                int v = dfs_order[i];
                cross[parent[v]] += cross[v];
            }

            // Try every second edge.  If b is an ancestor of a, the pair is
            // handled when a and b swap roles, so skip it here.
            for (int b = 1; b < n; ++b) {
                if (is_ancestor(b, a)) continue;
                int candidate = cut[a] + cut[b] - 2 * cross[b];
                best = std::min(best, candidate);
            }
        }
    }

    return best;
}

// Convenience overload: pack `num_trees` trees and immediately extract the best
// 1-/2-respecting cut from the resulting packing.  `seed` controls the
// randomized packing; omit it for a fresh random run.
inline int karger_tree_packing_min_cut(
    int n, const std::vector<std::pair<int, int>>& edges, int num_trees,
    unsigned seed) {
    return karger_tree_packing_min_cut(
        n, edges, karger_tree_packing(n, edges, num_trees, seed));
}

// Fresh-random-seed convenience overload.
inline int karger_tree_packing_min_cut(
    int n, const std::vector<std::pair<int, int>>& edges, int num_trees) {
    return karger_tree_packing_min_cut(
        n, edges, num_trees, karger_tree_packing_detail::random_seed());
}
