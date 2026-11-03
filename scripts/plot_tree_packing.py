"""Plot the tree packing benchmark (bench/bench_karger_tree_packing.cpp).

Usage (from the repo root):
    python scripts/plot_tree_packing.py
Reads  results/bench_karger_tree_packing.csv
Writes results/tp_scaling.png, results/tp_density.png, results/tp_trees.png

'tree_packing' rows time packing + cut extraction together; 'tree_packing_pack_only' rows time
the packing alone, so the gap between the two lines is the cost of finding the cut.
"""
import pandas as pd
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

df = pd.read_csv("results/bench_karger_tree_packing.csv")
df["ms"] = df["median_us"] / 1000.0
total = df[df["algorithm"] == "tree_packing"]
pack = df[df["algorithm"] == "tree_packing_pack_only"]


def lines(ax, exp, x):
    t = total[total["experiment"] == exp].sort_values(x)
    p = pack[pack["experiment"] == exp].sort_values(x)
    ax.plot(t[x], t["ms"], marker="o", label="Packing + cut extraction")
    ax.plot(p[x], p["ms"], marker="s", label="Packing only")
    ax.set_yscale("log")
    ax.grid(True, which="both", alpha=0.3)
    ax.set_ylabel("Median time (ms)")


# 1. Growth with n (k = 2n trees): sparse and dense side by side
fig, axes = plt.subplots(1, 2, figsize=(11, 4.5), sharey=True)
for ax, exp, title in [(axes[0], "size_sparse", "Sparse graphs (m = 3n)"),
                       (axes[1], "size_dense", "Dense graphs (m = n(n-1)/4)")]:
    lines(ax, exp, "n")
    ax.set_xscale("log")
    ax.set_xlabel("Number of vertices n (2n trees)")
    ax.set_title(title)
axes[0].legend()
fig.tight_layout()
fig.savefig("results/tp_scaling.png", dpi=200)
plt.close(fig)
print("wrote results/tp_scaling.png")

# 2. Density sweep at fixed n
fig, ax = plt.subplots(figsize=(6.5, 4.5))
lines(ax, "density_sweep", "m")
ax.set_xscale("log")
n_fixed = int(total[total["experiment"] == "density_sweep"]["n"].iloc[0])
ax.set_xlabel(f"Number of edges m (n = {n_fixed})")
ax.set_title("Tree packing: density sweep")
ax.legend()
fig.tight_layout()
fig.savefig("results/tp_density.png", dpi=200)
plt.close(fig)
print("wrote results/tp_density.png")

# 3. Number of trees: time, and the value of the packing
fig, axes = plt.subplots(1, 2, figsize=(11, 4.5))
lines(axes[0], "tree_count_sweep", "num_trees")
axes[0].set_xscale("log")
axes[0].set_xlabel("Number of trees k")
axes[0].set_title("Time vs number of trees")
axes[0].legend()
v = total[total["experiment"] == "tree_count_sweep"].sort_values("num_trees")
axes[1].plot(v["num_trees"], v["packing_value"], marker="o")
axes[1].set_xscale("log")
axes[1].set_xlabel("Number of trees k")
axes[1].set_ylabel("Packing value k / (max edge load)")
axes[1].set_title("Packing value vs number of trees")
axes[1].grid(True, which="both", alpha=0.3)
fig.tight_layout()
fig.savefig("results/tp_trees.png", dpi=200)
plt.close(fig)
print("wrote results/tp_trees.png")
