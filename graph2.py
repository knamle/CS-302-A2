import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
import numpy as np

# ── Data ──────────────────────────────────────────────────────────────────────
model_sizes = [100, 1000, 10000, 100000]

times = {
    100:    [0.00294,  0.000371, 0.000511, 0.00137],
    1000:   [0.0322,   0.00108,  0.00118,  0.00225],
    10000:  [0.5,      0.00256,  0.00332,  0.00551],
    100000: [7.37,     0.00789,  0.0236,   0.0268 ],
}

programs = ["Program A", "Program D", "Program B", "Program C"]
colors   = ["#e63946", "#2a9d8f", "#e9c46a", "#457b9d"]
markers  = ["o", "s", "^", "D"]

data = np.array([times[n] for n in model_sizes])  # shape (4, 4)

# ── Figure layout: 1 row × 2 columns ─────────────────────────────────────────
fig, axes = plt.subplots(1, 2, figsize=(14, 5.5))
fig.patch.set_facecolor("#0d1117")
for ax in axes:
    ax.set_facecolor("#161b22")
    ax.tick_params(colors="#c9d1d9")
    ax.xaxis.label.set_color("#c9d1d9")
    ax.yaxis.label.set_color("#c9d1d9")
    ax.title.set_color("#f0f6fc")
    for spine in ax.spines.values():
        spine.set_edgecolor("#30363d")

# ── Left: all 4 programs, log-log ─────────────────────────────────────────────
ax = axes[0]
for i, (prog, col, mk) in enumerate(zip(programs, colors, markers)):
    ax.plot(model_sizes, data[:, i], marker=mk, color=col, linewidth=2,
            markersize=7, label=prog)

ax.set_xscale("log")
ax.set_yscale("log")
ax.set_xticks(model_sizes)
ax.get_xaxis().set_major_formatter(ticker.ScalarFormatter())
ax.set_xlabel("Model Size", fontsize=11)
ax.set_ylabel("Time (s)", fontsize=11)
ax.set_title("All Programs — Log Scale", fontsize=13, fontweight="bold", pad=10)
ax.grid(True, which="both", color="#30363d", linewidth=0.6, linestyle="--")
ax.legend(facecolor="#21262d", edgecolor="#30363d", labelcolor="#c9d1d9",
          fontsize=9, loc="upper left")

# ── Right: programs 2-4 only, linear-y ───────────────────────────────────────
ax = axes[1]
for i in range(1, 4):
    ax.plot(model_sizes, data[:, i], marker=markers[i], color=colors[i],
            linewidth=2, markersize=7, label=programs[i])

ax.set_xscale("log")
ax.set_xticks(model_sizes)
ax.get_xaxis().set_major_formatter(ticker.ScalarFormatter())
ax.set_xlabel("Model Size", fontsize=11)
ax.set_ylabel("Time (s)", fontsize=11)
ax.set_title("Programs BCD — Linear Scale",
             fontsize=12, fontweight="bold", pad=10)
ax.grid(True, which="both", color="#30363d", linewidth=0.6, linestyle="--")
ax.legend(facecolor="#21262d", edgecolor="#30363d", labelcolor="#c9d1d9",
          fontsize=9)

# ── Shared subtitle ───────────────────────────────────────────────────────────
fig.suptitle(
    "Execution Time vs. Model Size  (nprocs=32, iterations=10)",
    fontsize=14, fontweight="bold", color="#f0f6fc", y=1.02
)

plt.tight_layout()
plt.show()