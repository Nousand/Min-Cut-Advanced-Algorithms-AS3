// Karger's randomized minimum cut algorithm
// Primary author: Rodrigo Uzal Sequeros

#pragma once

#include <random>
#include <algorithm>
#include <vector>
#include <utility>


// We need to define the unionFind structure as I will be following the implementation of Karger recommended from the slides
// where we adapt it to follow the already seen algorithm from week 5 of Kruskal MST

// It was assumed that this code would only be run with connected and simple graphs.

struct UnionFind{
    std::vector<int> parent; // Vertex that v points to
    std::vector<int> size; // How many vertices are in the group whose root is r
    int components;

    UnionFind(int n) : parent(n), size(n,1), components(n){
        for(int i = 0; i < n; i++){
            parent[i] = i; // Each node is its own parent
        }
    }
    // Find returns the name of the super vertex p belongs to
    int find(int p){
        while(parent[p] != p){
            p = parent[p];
        }
        return p;
    }
    // Unite merges the two super vertex
    bool unite(int p, int q){
        int rp = find(p);
        int rq = find(q);
        if(rp == rq) return false;
        if(size[rp] < size[rq]){
            // We reorder their values so that the first tree is bigger than the second as this one is the onw we will be linking to the first.
            int z = rp;
            rp = rq;
            rq = z;
        }
        parent[rq] = rp;
        size[rp] = size[rp] + size[rq];
        components--;
        return true;
    }
};

// Edges are passed by reference not just a copy
inline int karger_alg(int n, const std::vector<std::pair<int, int>>& edges){
    UnionFind uf(n);
    int cut = 0;

    for (int i = 0; uf.components > 2; i++){
        // I first extract the corresponding index of the vertices that form the i-th edge
        int u = edges[i].first;
        int v = edges[i].second;

        // Now we call unite which will merge them if they do not belong to the same super vertex   
        uf.unite(u,v);

    }
    for (int i = 0; i < (int)edges.size(); i++){
        int u = edges[i].first;
        int v = edges[i].second;
        if(uf.find(u) != uf.find(v)){
            cut++;
        }
    }
    return cut;
}

// This is the main loop through which we call Karger and compare the cuts obtained in each of the t iterations
inline int karger_min_cut(int n, std::vector<std::pair<int, int>> edges, int t, unsigned int seed){
    // For each iteration we will need to randomly organise the set of edges of our graph before going through them. As it was discussed
    // in the notes, we could introduce randomization in each step of contraction or order the edges randomly before hand and follow the
    // resulting order.

    // Create the generator of the random seed once before the loop starts
    std::mt19937 rng(seed);

    // We initialize the variable best which will store the current smallest cut size found
    int best = static_cast<int>(edges.size());

    // When we run Karger t >= 3n(n-1)/2 times it gives us a success probability of at least 9/10, as seen from class.
    for (int i = 0; i < t; i++){
        // We randomly shuffle the edges using the generator rng, costing us O(m) in efficiency
        std::shuffle(edges.begin(), edges.end(), rng);
        // Call the algorithm itself and save the cut size in cut
        int cut = karger_alg(n, edges);
        if(cut < best){
            best = cut;
        }
    }
    return best;
}


