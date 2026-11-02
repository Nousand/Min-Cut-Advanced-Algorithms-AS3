"""Plots for the three-algorithm benchmark (bench_all.cpp).

Usage (from the repo root):
    python scripts/plot_all.py
Reads  results/bench_time.csv and results/bench_success.csv
Writes results/all_time_sparse.png, all_time_dense.png, all_success.png, all_repetitions.png
Any algorithm present in bench_time.csv (e.g. tree_packing, once added) is plotted automatically.
"""
import pandas as pd
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

time_df = pd.read_csv("results/bench_time.csv")
succ_df = pd.read_csv("results/bench_success.csv")
time_df["ms"] = time_df["median_us"] / 1000.0

labels = {"stoer_wagner_matrix": "Stoer-Wagner (matrix)", "stoer_wagner_pq": "Stoer-Wagner (priority queue)",
          "karger": "Karger (t for 99% success)", "tree_packing": "Tree packing"}


def time_plot(family_prefix, title, outfile):
    sub = time_df[time_df["family"].str.startswith(family_prefix)]
    fig, ax = plt.subplots(figsize=(6.5, 4.5))
    for algo, g in sub.groupby("algorithm"):
        g = g.sort_values("n")
        ax.plot(g["n"], g["ms"], marker="o", label=labels.get(algo, algo))
    ax.set_xscale("log"); ax.set_yscale("log")
    ax.set_xlabel("Number of vertices n"); ax.set_ylabel("Median time (ms)")
    ax.set_title(title); ax.grid(True, which="both", alpha=0.3); ax.legend()
    fig.tight_layout(); fig.savefig(outfile, dpi=200); plt.close(fig); print("wrote", outfile)


time_plot("sparse", "Time to ~99% success, sparse graphs (m = 3n)", "results/all_time_sparse.png")
time_plot("dense", "Time to ~99% success, dense graphs (m = n(n-1)/4)", "results/all_time_dense.png")

# Karger single-run success rate vs the 2/(n(n-1)) bound
fig, ax = plt.subplots(figsize=(6.5, 4.5))
for fam, g in succ_df.groupby("family"):
    g = g.sort_values("n")
    ax.errorbar(g["n"], g["p_hat"], yerr=[g["p_hat"] - g["ci_lo"], g["ci_hi"] - g["p_hat"]],
                marker="o", capsize=3, label=fam)
b = succ_df.drop_duplicates("n").sort_values("n")
ax.plot(b["n"], b["bound"], "k--", label="bound 2/(n(n-1))")
ax.set_xscale("log"); ax.set_yscale("log")
ax.set_xlabel("Number of vertices n"); ax.set_ylabel("Success probability of ONE Karger run")
ax.set_title("Karger single-run success rate (95% Wilson intervals)")
ax.grid(True, which="both", alpha=0.3); ax.legend(fontsize=7)
fig.tight_layout(); fig.savefig("results/all_success.png", dpi=200); plt.close(fig); print("wrote results/all_success.png")

# Repetitions needed for 99% success: empirical vs worst-case formula
fig, ax = plt.subplots(figsize=(6.5, 4.5))
for fam, g in succ_df.groupby("family"):
    g = g.sort_values("n")
    ax.plot(g["n"], g["t_needed"], marker="o", label=fam)
f = succ_df.drop_duplicates("n").sort_values("n")
ax.plot(f["n"], f["t_formula"], "k--", label="formula ln(100)*n(n-1)/2")
ax.set_xscale("log"); ax.set_yscale("log")
ax.set_xlabel("Number of vertices n"); ax.set_ylabel("Runs needed for 99% success")
ax.set_title("Karger repetitions: observed vs worst-case formula")
ax.grid(True, which="both", alpha=0.3); ax.legend(fontsize=7)
fig.tight_layout(); fig.savefig("results/all_repetitions.png", dpi=200); plt.close(fig); print("wrote results/all_repetitions.png")

# Plotting logic for experiment 3, written with the help of Claude (Opus 5.5)

k_df = pd.read_csv("results/bench_ksweep.csv")
k_df["k"] = k_df["family"].str.extract(r"k=(\d+)", expand=False).astype(int)

def predicted_lower_bound(s, k):
    n, c = 2 * s, 2 * k / (s - 1)
    p = 1.0
    for j in range(1, n - 1):
        p *= j / (j + c)
    return p

k_df["predicted"] = [predicted_lower_bound(20, k) for k in k_df["k"]]

fig, ax = plt.subplots(figsize=(6.5, 4.5))
ax.errorbar(k_df["k"], k_df["p_hat"], yerr=[k_df["p_hat"] - k_df["ci_lo"], k_df["ci_hi"] - k_df["p_hat"]],
            marker="o", capsize=3, label="measured")
ax.plot(k_df["k"], k_df["predicted"], "s--", label="predicted lower bound")
ax.axhline(k_df["bound"].iloc[0], color="k", linestyle=":", label="general bound 2/(n(n-1))")
ax.set_yscale("log")
ax.set_xlabel("Cut size k (two clusters of s = 20)"); ax.set_ylabel("Success probability of ONE Karger run")
ax.set_title("Karger success vs planted cut size"); ax.grid(True, which="both", alpha=0.3); ax.legend()
fig.tight_layout(); fig.savefig("results/ksweep.png", dpi=200); plt.close(fig); print("wrote results/ksweep.png")