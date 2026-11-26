# Plano do desafio OpenMP: histograma de letras

Criado em 2026-11-19. Estimativa total: cerca de 11 h em três sessões.

| Sessão | Etapa | O que fazer | Tempo |
| --- | --- | --- | --- |
| A | 0 | Montar o kit, gerar entradas, rodar a serial | 30 min |
| A | 1 | Versão ingênua (ver a condição de corrida) | 30 min |
| A | 2 | critical e atomic | 45 min |
| A | 3 | Histograma local por thread | 45 min |
| A | 4 | reduction e testes de schedule | 45 min |
| B | 5 | Sua versão (sugestão: matriz [thread][26] com e sem padding) | 1 h 30 |
| B | 6 | Script de validação contra a serial (confere.sh) | 30 min |
| B | 7 | Receita do contêiner (hist.def) | 1 h 30 |
| C | 8 | Medir, speedup e eficiência, gráfico (mede.sh, analise.py) | 1 h 30 |
| C | 9 | Hipóteses e análise | 2 h |
| C | 10 | Revisão e entrega | 45 min |

Observação: com `schedule(runtime)` e `OMP_SCHEDULE` indefinida, o GCC usa `dynamic` com bloco 1 (conferido com `OMP_DISPLAY_ENV=true`).
