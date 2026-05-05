import matplotlib.pyplot as plt
import numpy as np

b1 = [64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384]
b2 = [64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384]

# timeB[i][j] = time for B1=b1[i], B2=b2[j]
timeB = [
    [2.59, 1.42, 0.903, 0.603, 0.455, 0.365, 0.370, 0.367, 0.363],
    [2.60, 1.42, 0.902, 0.601, 0.445, 0.358, 0.361, 0.357, 0.355],
    [2.58, 1.41, 0.884, 0.598, 0.441, 0.353, 0.358, 0.355, 0.349],
    [2.53, 1.41, 0.885, 0.590, 0.439, 0.351, 0.354, 0.349, 0.348],
    [2.58, 1.41, 0.889, 0.595, 0.442, 0.351, 0.356, 0.352, 0.349],
    [2.56, 1.41, 0.877, 0.578, 0.431, 0.344, 0.347, 0.344, 0.340],
    [2.54, 1.40, 0.874, 0.585, 0.432, 0.341, 0.348, 0.342, 0.339],
    [2.50, 1.40, 0.876, 0.578, 0.429, 0.343, 0.348, 0.341, 0.339],
    [2.49, 1.41, 0.874, 0.582, 0.431, 0.342, 0.349, 0.344, 0.338],
]

# timeC[i][j] = time for B1=b1[i], B2=b2[j]
timeC = [
    [1.73, 1.11, 0.692, 0.426, 0.364, 0.313, 0.399, 0.364, 0.346],
    [1.73, 0.958, 0.591, 0.358, 0.298, 0.269, 0.373, 0.340, 0.320],
    [1.96, 0.970, 0.587, 0.358, 0.303, 0.262, 0.367, 0.332, 0.315],
    [1.93, 1.030, 0.608, 0.359, 0.295, 0.260, 0.362, 0.333, 0.314],
    [1.93, 1.040, 0.625, 0.374, 0.299, 0.263, 0.362, 0.334, 0.318],
    [1.92, 1.030, 0.625, 0.365, 0.299, 0.259, 0.354, 0.333, 0.312],
    [1.74, 1.020, 0.657, 0.427, 0.373, 0.336, 0.427, 0.400, 0.370],
    [1.69, 1.030, 0.648, 0.431, 0.372, 0.338, 0.429, 0.395, 0.374],
    [1.68, 1.020, 0.652, 0.426, 0.377, 0.337, 0.427, 0.396, 0.376],
]

def _plot_heatmap(matrix, title, filename):
    fig, ax = plt.subplots(figsize=(9, 7))
    # transpose so B1 is x-axis, B2 is y-axis
    im = ax.imshow(matrix.T, aspect="auto", cmap="viridis_r", origin="lower")
    ax.set_xticks(range(len(b1)))
    ax.set_xticklabels(b1)
    ax.set_yticks(range(len(b2)))
    ax.set_yticklabels(b2)
    ax.set_xlabel("B1")
    ax.set_ylabel("B2")
    ax.set_title(title)

    threshold = 0.5 * (matrix.T.max() + matrix.T.min())
    for i in range(len(b1)):
        for j in range(len(b2)):
            ax.text(i, j, f"{matrix[i, j]:.2f}",
                    ha="center", va="center",
                    color="white" if matrix[i, j] > threshold else "black",
                    fontsize=7)
    fig.colorbar(im, ax=ax, label="Time (s)")
    plt.tight_layout()
    plt.savefig(filename, dpi=150)
    plt.show()

_plot_heatmap(np.array(timeB),
              "progB: Execution Time (s) - 32 procs, S=1M, 10 iterations",
              "heatmap_progB.png")

_plot_heatmap(np.array(timeC),
                "progC: Execution Time (s) - 32 procs, S=1M, 10 iterations",
                "heatmap_progC.png")
