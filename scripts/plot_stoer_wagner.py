"""Plot the Stoer-Wagner benchmark results.

Usage (from the repo root):
    python scripts/plot_stoer_wagner.py
Reads  results/stoer_wagner_bench.csv
Writes results/sw_sparse.png, results/sw_dense.png, results/sw_density.png
"""
import sys
import pandas as pd
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

csv_path = sys.argv[1] if len(sys.argv) > 1 else "results/stoer_wagner_bench.csv"
df = pd.read_csv(csv_path)
df["ms"] = df["median_us"] / 1000.0
styles = {"matrix": ("tab:blue", "o"), "priority_queue": ("tab:orange", "s")}
labels = {"matrix": "Matrix version", "priority_queue": "Priority queue version"}


def plot(experiment, x, xlabel, title, outfile, logx=True):
    sub = df[df["experiment"] == experiment]
    fig, ax = plt.subplots(figsize=(6.5, 4.5))
    for algo, g in sub.groupby("algorithm"):
        g = g.sort_values(x)
        color, marker = styles[algo]
        ax.plot(g[x], g["ms"], marker=marker, color=color, label=labels[algo])
    ax.set_xlabel(xlabel)
    ax.set_ylabel("Median time (ms)")
    ax.set_title(title)
    if logx:
        ax.set_xscale("log")
    ax.set_yscale("log")
    ax.grid(True, which="both", alpha=0.3)
    ax.legend()
    fig.tight_layout()
    fig.savefig(outfile, dpi=200)
    plt.close(fig)
    print("wrote", outfile)


plot("size_sparse", "n", "Number of vertices n", "Sparse graphs (m = 3n)", "results/sw_sparse.png")
plot("size_dense", "n", "Number of vertices n", "Dense graphs (m = n(n-1)/4)", "results/sw_dense.png")
plot("density_sweep", "m", "Number of edges m (n = 300)", "Density sweep at n = 300", "results/sw_density.png")
