import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
import numpy as np

# ── Data ──────────────────────────────────────────────────────────────────────
nprocs = [2, 4, 8, 16, 32, 64]

# Each row = one nprocs level; columns = [prog1, prog2, prog3, prog4]
times = {
    2:  [1.17,  0.0401, 0.0384, 0.0428],
    4:  [5.96,  0.0927, 0.0411, 0.0457],
    8:  [16.5,  0.106,  0.0766, 0.0737],
    16: [36.4,  0.181,  0.166,  0.139 ],
    32: [80.8,  0.126,  0.343,  0.259 ],
    64: [295,   0.161,  0.846,  0.513 ],
}

programs = ["Program A", "Program D", "Program B", "Program C"]
colors   = ["#e63946", "#2a9d8f", "#e9c46a", "#457b9d"]
markers  = ["o", "s", "^", "D"]

data = np.array([times[n] for n in nprocs])  # shape (6, 4)

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

# ── Left: all 4 programs, log-y ───────────────────────────────────────────────
ax = axes[0]
for i, (prog, col, mk) in enumerate(zip(programs, colors, markers)):
    ax.plot(nprocs, data[:, i], marker=mk, color=col, linewidth=2,
            markersize=7, label=prog)

ax.set_yscale("log")
ax.set_xscale("log", base=2)
ax.set_xticks(nprocs)
ax.get_xaxis().set_major_formatter(ticker.ScalarFormatter())
ax.set_xlabel("Number of Processes", fontsize=11)
ax.set_ylabel("Time (s)", fontsize=11)
ax.set_title("All Programs — Log Scale", fontsize=13, fontweight="bold", pad=10)
ax.grid(True, which="both", color="#30363d", linewidth=0.6, linestyle="--")
ax.legend(facecolor="#21262d", edgecolor="#30363d", labelcolor="#c9d1d9",
          fontsize=9, loc="upper left")

# ── Right: programs 2-4 only, linear-y ───────────────────────────────────────
ax = axes[1]
for i in range(1, 4):
    ax.plot(nprocs, data[:, i], marker=markers[i], color=colors[i],
            linewidth=2, markersize=7, label=programs[i])

ax.set_xscale("log", base=2)
ax.set_xticks(nprocs)
ax.get_xaxis().set_major_formatter(ticker.ScalarFormatter())
ax.set_xlabel("Number of Processes", fontsize=11)
ax.set_ylabel("Time (s)", fontsize=11)
ax.set_title("Programs BCD — Linear Scale\n",
             fontsize=12, fontweight="bold", pad=10)
ax.grid(True, which="both", color="#30363d", linewidth=0.6, linestyle="--")
ax.legend(facecolor="#21262d", edgecolor="#30363d", labelcolor="#c9d1d9",
          fontsize=9)

# ── Shared subtitle ───────────────────────────────────────────────────────────
fig.suptitle(
    "Execution Time vs. nprocs  (iterations=10, model size=1,000,000)",
    fontsize=14, fontweight="bold", color="#f0f6fc", y=1.02
)

plt.tight_layout()
plt.show()