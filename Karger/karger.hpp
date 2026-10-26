// Karger's randomized minimum cut algorithm
// Primary author: Rodrigo Uzal Sequeros

#pragma once

#include <random>
#include <algorithm>

int mainloop(int t, int n, std::vector<std::pair<int, int>> edges, unsigned int seed)

int karger_alg(int n, std::vector<std::pair<int, int>> edges)