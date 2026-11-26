#!/usr/bin/env python3
"""uso: python3 analise.py resultados.csv [resultados-openmp.csv]

Lê o CSV do mede.sh (versao,p,rep,distribuir,contar,combinar,total) e gera:
- tabela.csv: mediana de cada fase e do total, mínimo e máximo do total,
  speedup e eficiência por (versão, p)
- speedup.png: speedup e eficiência
- fases.png: mediana de cada fase por p, uma coluna de gráficos por versão
T_serial é a mediana do tempo de make_hist() da serial (letters).
Com o CSV do desafio OpenMP (versao,threads,rep,tempo_s), acrescenta a melhor
versão OpenMP (a de menor tempo) na tabela de comparação e no gráfico de speedup.
Os máximos de cada fase podem vir de processos diferentes: a soma das fases
pode passar do total.
"""
import sys
import pandas as pd
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

FASES = ["distribuir", "contar", "combinar"]

if len(sys.argv) < 2:
    print(__doc__, file=sys.stderr)
    sys.exit(2)

df = pd.read_csv(sys.argv[1])
t_serial = df[df.versao == "letters"]["total"].median()
par = df[df.versao != "letters"]

med = (par.groupby(["versao", "p"])
       .agg(distribuir=("distribuir", "median"), contar=("contar", "median"),
            combinar=("combinar", "median"), total=("total", "median"),
            minimo=("total", "min"), maximo=("total", "max"))
       .reset_index())
med["speedup"] = t_serial / med["total"]
med["eficiencia"] = med["speedup"] / med["p"]
med["mib_s"] = 256 / med["total"]

print(f"T_serial (mediana) = {t_serial:.6f} s")
print(med.round(4).to_string(index=False))
med.to_csv("tabela.csv", index=False)

omp = None
if len(sys.argv) > 2:
    o = pd.read_csv(sys.argv[2])
    o = o[o.versao != "omp-letters"]
    om = o.groupby(["versao", "threads"])["tempo_s"].median().reset_index()
    melhor = om.loc[om.tempo_s.idxmin(), "versao"]
    omp = om[om.versao == melhor].rename(columns={"threads": "p", "tempo_s": "total"})
    omp["speedup"] = t_serial / omp["total"]
    print(f"\nMelhor OpenMP: {melhor} (speedup calculado com o T_serial do MPI)")
    comp = med.pivot(index="p", columns="versao", values="total")
    comp[melhor] = omp.set_index("p")["total"]
    print(comp.round(4).to_string())

fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(11, 4))
for v, g in med.groupby("versao"):
    ax1.plot(g.p, g.speedup, marker="o", label=v)
    ax2.plot(g.p, g.eficiencia, marker="o", label=v)
if omp is not None:
    ax1.plot(omp.p, omp.speedup, marker="s", ls=":", label=omp.versao.iloc[0])
    ax2.plot(omp.p, omp.speedup / omp.p, marker="s", ls=":", label=omp.versao.iloc[0])
maxp = int(med.p.max())
ax1.plot([1, maxp], [1, maxp], "k--", label="ideal")
ax1.set(xlabel="processos (threads no OpenMP)", ylabel="speedup", title="Speedup (entrada grande)")
ax2.axhline(1.0, color="k", ls="--")
ax2.set(xlabel="processos (threads no OpenMP)", ylabel="eficiência", title="Eficiência (entrada grande)")
ax1.legend(fontsize=8)
ax2.legend(fontsize=8)
fig.tight_layout()
fig.savefig("speedup.png", dpi=150)

versoes = sorted(med.versao.unique())
fig, axs = plt.subplots(1, len(versoes), figsize=(4 * len(versoes), 4), sharey=True, squeeze=False)
for ax, v in zip(axs[0], versoes):
    g = med[med.versao == v]
    x = [str(p) for p in g.p]
    base = [0.0] * len(g)
    for f in FASES:
        nome = "ler" if (f == "distribuir" and v.endswith("arquivo")) else f
        ax.bar(x, g[f], bottom=base, label=nome)
        base = [b + t for b, t in zip(base, g[f])]
    ax.set(title=v.replace("mpi-letters-", ""), xlabel="processos")
    ax.legend(fontsize=8)
axs[0][0].set_ylabel("tempo (s), mediana")
fig.tight_layout()
fig.savefig("fases.png", dpi=150)
print("gerados: tabela.csv, speedup.png, fases.png")
