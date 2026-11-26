#!/usr/bin/env python3
"""uso: python3 analise.py resultados.csv [outro.csv]

Lê um CSV com colunas versao,threads,rep,tempo_s (saída do mede.sh) e gera:
- tabela.csv: mediana, mínimo, máximo, speedup e eficiência por (versão, threads)
- speedup.png: gráficos de speedup e eficiência
Com um segundo CSV (por exemplo, nativo x contêiner), compara as medianas: a
coluna razao_2_sobre_1 é o tempo do segundo dividido pelo do primeiro, e um
valor acima de 1 significa que o segundo ambiente foi mais lento.
"""
import sys
import pandas as pd
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt


def carrega(caminho):
    df = pd.read_csv(caminho).dropna(subset=["tempo_s"])
    # A serial ignora OMP_NUM_THREADS: mediana de todas as repetições = T_serial.
    t_serial = df[df.versao == "omp-letters"]["tempo_s"].median()
    med = (df[df.versao != "omp-letters"]
           .groupby(["versao", "threads"])["tempo_s"]
           .agg(mediana="median", minimo="min", maximo="max")
           .reset_index())
    med["speedup"] = t_serial / med["mediana"]
    med["eficiencia"] = med["speedup"] / med["threads"]
    return t_serial, med


if len(sys.argv) < 2:
    print(__doc__, file=sys.stderr)
    sys.exit(2)

t_serial, med = carrega(sys.argv[1])
print(f"T_serial (mediana) = {t_serial:.6f} s")
print(med.round(4).to_string(index=False))
med.to_csv("tabela.csv", index=False)

fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(11, 4))
for v, g in med.groupby("versao"):
    ax1.plot(g.threads, g.speedup, marker="o", label=v)
    ax2.plot(g.threads, g.eficiencia, marker="o", label=v)
maxp = int(med.threads.max())
ax1.plot([1, maxp], [1, maxp], "k--", label="ideal")
ax1.set(xlabel="threads", ylabel="speedup", title="Speedup (entrada grande)")
ax2.axhline(1.0, color="k", ls="--")
ax2.set(xlabel="threads", ylabel="eficiência", title="Eficiência (entrada grande)")
ax1.legend(fontsize=8)
ax2.legend(fontsize=8)
fig.tight_layout()
fig.savefig("speedup.png", dpi=150)
print("gerados: tabela.csv, speedup.png")

if len(sys.argv) > 2:
    t2, med2 = carrega(sys.argv[2])
    comp = med.merge(med2, on=["versao", "threads"], suffixes=("_1", "_2"))
    comp["razao_2_sobre_1"] = comp["mediana_2"] / comp["mediana_1"]
    print(f"\nT_serial (1) = {t_serial:.6f} s   T_serial (2) = {t2:.6f} s")
    print(comp[["versao", "threads", "mediana_1", "mediana_2", "razao_2_sobre_1"]]
          .round(4).to_string(index=False))
