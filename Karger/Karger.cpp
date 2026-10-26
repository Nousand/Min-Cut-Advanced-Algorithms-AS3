// Karger's randomized minimum cut algorithm
// Primary author: Rodrigo Uzal Sequeros

#include <random>
#include <algorithm>
#include <vector>
#include <utility>


// We need to define the unionFind structure as I will be following the implementation of Krager recommended from the slides
// where we adapt it to follow the already seen algorithm from week 5 of Kruskal MST

struct UnionFind{
    std::vector<int> parent; // Vertex that points to v
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
    int unite(int p, int q){
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
}

// This is the main loop through which we call Karger and compare the cuts obtained in each of the t iterations
int mainloop(int t, int n, std::vector<std::pair<int, int>> edges, unsigned int seed ){
    // For each iteration we will need to randomnly organise the set of edges of our graph before going through them. As it was discussed
    // in the notes, we could introduce randomization in each step of contraction or order the edges randomly before hand and follow the
    // resulting order.

    // Create the generator of the random seed once before the loop starts
    std:: mt19937 rng(seed);

    // We initialize the variable best which will store the current smallest cut size found
    int best = static_cast<int>(edges.size());

    // We run Karger t times which gives us a success probability of at least 9/10, as seen from class.
    for (int i = 0; i < t; i++){
        // We randomly shuffle the edges using the generator rng, costing us O(n) in efficiency
        std::shuffle(edges.begin(), edges.end(), rng);
        // Call the algorithm itself and save the cut size in cut
        int cut = karger_alg(n, edges);
        if(cut < best){
            best = cut;
        }
    }
    return best;
}

int karger_alg(int n, std::vector<std::pair<int, int>> edges){
    // Main algorithm
}

