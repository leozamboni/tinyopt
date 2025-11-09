import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

CSV_PATH = "results.csv" 

df = pd.read_csv(CSV_PATH)

df["Compilacao"] = df["Compiler"] + " -" + df["Opt level"]

df["Compilacao"] = df["Compilacao"].replace({
    "gcc -O0": "GCC -O0",
    "gcc -O2": "GCC -O2",
    "gcc -O3": "GCC -O3",
    "clang -O3": "Clang -O3",
    "tinyopt+gcc -O0": "TinyOpt + GCC -O0"
})

ordem_compilacoes = ["GCC -O0", "GCC -O2", "GCC -O3", "Clang -O3", "TinyOpt + GCC -O0"]

pivot_df = df.pivot_table(index="File", columns="Compilacao", values="Instructions")

pivot_df = pivot_df[ordem_compilacoes]

programas = pivot_df.index
configs = pivot_df.columns
n_configs = len(configs)
x = np.arange(len(programas))
width = 0.15

fig, ax = plt.subplots(figsize=(11, 6))

for i, config in enumerate(configs):
    bars = ax.bar(x + i * width, pivot_df[config], width, label=config)
    for bar in bars:
        height = bar.get_height()
        ax.text(bar.get_x() + bar.get_width() / 2, height + 1, f"{int(height)}",
                ha='center', va='bottom', fontsize=8)

ax.set_xlabel("Programa")
ax.set_ylabel("Número de Instruções (objdump)")
ax.set_title("Comparação de Otimizações entre Compiladores")
ax.set_xticks(x + width * (n_configs - 1) / 2)
ax.set_xticklabels(programas, rotation=45, ha="right")
ax.legend(title="Compilação")
ax.grid(axis="y", linestyle="--", alpha=0.6)

plt.tight_layout()
plt.savefig("opt_graph.png", dpi=300)
plt.show()
