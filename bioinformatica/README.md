# Desafio HPC: caracterização do IQ-TREE 2 em uma máquina local

Estudo de desempenho do IQ-TREE 2 (inferência filogenética por máxima verossimilhança)
com paralelismo de memória compartilhada (OpenMP), no âmbito do Programa de Formação
em Computação de Alto Desempenho (CESAR, 2026/2027).

## Estrutura
Veja a árvore de pastas em `docs/` e o guia de contribuição em `CONTRIBUTING.md`.

## Reprodução
1. Clone com submódulos: `git clone --recurse-submodules <url-deste-repo>`
2. Registre o ambiente: `scripts/collect_env.sh`
3. Compile o IQ-TREE conforme `docs/compilacao.md`
4. Execute os experimentos conforme `experiments/configs/`

## Versão
Ver `CHANGELOG.md`.
