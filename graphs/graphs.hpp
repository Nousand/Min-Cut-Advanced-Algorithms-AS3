// Graph builders shared by tests and the benchmark
// Every builder returns an edge list on vertices 0, ... ,n-1

#pragma once

#include <vector>
#include <utility>
#include <random>
#include <set>

// Predefine the vector edges' structure
using Edges = std::vector<std::pair<int, int>>;

// First builder constructs a cycle graph where each vertex has degree 2, the previous and next vertex
// Here we can determine the number of vertices of the graph. Min cut = 2.

inline Edges make_cycle(int n){
    Edges edges;
    for (int i= 0; i < n; i++){
        edges.push_back({i, (i+1) % n});
    }
    return edges;
}

// Complete graph: every possible pair of vertices is joined once. Min cut = n-1. It is interesting as it is the most possible dense graph.
// We expect Karger to be more precise for this kind of graphs, without its efficiency taking a big toll, while Stoer-Wagner and karger-Stein
// drastically decreasing in performance.

inline Edges make_complete(int n){
    Edges edges;
    for (int i = 0; i < n; i++){
        for (int j = i + 1; j < n; j++){
            edges.push_back({i, j});
        }
    }
    return edges;
}

// Two complete graphs of s vertices each, joined by k edges st 1 <= k < s-1 so that min cut = k.
// Cluster A formed by vertices 0, ..., s-1 and cluster B with 0, ..., s-1 st n = 2s.

inline Edges make_two_clusters(int s, int k){
    Edges edges;
    for (int i = 0; i < s; i++){
        for (int j = i + 1; j < s; j++){
            edges.push_back({i, j});
            edges.push_back({i + s, j + s});
        }
    }
    for (int i = 0; i < k; i++){
        edges.push_back({i, i + s});
        // Here we are connecting edges from cluster A to cluster B, but since we ensure k < s-1 this will be the minimal cut.
    }
    return edges;
}


// IMPORTANT REQUIREMENT: n - 1 <= m <= n(n-1)/2
inline Edges make_random_graph(int n, int m, unsigned int seed){
    Edges edges;
    std::set<std::pair<int, int>> used;
    std::mt19937 rng(seed);

    // First we must ensure the graph is connected by building a random spanning tree and then adding rangom extra edges.
    // So each vertex i connects to a random earlier vertex j from 0, 1, 2, ..., i-1
    for (int i = 1; i < n; i++){
        std::uniform_int_distribution<int> pick_earlier(0, i - 1);
        int j = pick_earlier(rng);
        edges.push_back({j, i});
        used.insert({j, i});
    }

    // After, we just need to randomly assign the remaining m - n edges to our graph
    std::uniform_int_distribution<int> pick(0, n - 1); // we define the range of number we want each round
    while((int)edges.size() < m){
        int u = pick(rng); // It gives a random integer in range [0, n-1]
        int v = pick(rng);
        if(u == v) continue; // Skip the rest of this round as we don't want selfloops
        if(u > v) std::swap(u, v);
        if(used.count({u, v}) == 0){
            used.insert({u, v});
            edges.push_back({u, v});
        }
    }
    return edges;
}

