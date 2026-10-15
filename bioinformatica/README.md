 Caracterização de desempenho do IQ-TREE 2 com OpenMP
 
Estudo de desempenho do [IQ-TREE 2](http://www.iqtree.org/), um software de inferência filogenética por máxima verossimilhança, executado em uma máquina local. O trabalho faz parte do Programa de Formação em Computação de Alto Desempenho (CESAR, 2026/2027), no desafio de escolher um software open source de uma área de aplicação real e documentar sua compilação e execução.
 
## Contexto
 
A filogenética estuda a relação evolutiva entre espécies ou sequências. Dado um alinhamento de sequências, o objetivo é encontrar a árvore que melhor explica os dados. O IQ-TREE faz essa busca por máxima verossimilhança, o que exige avaliar muitas árvores candidatas e calcular a verossimilhança de cada uma. Esse cálculo é caro e pode ser paralelizado, que é o foco deste estudo.
 
## Ambiente
 
- **Máquina:** notebook com AMD Ryzen 5 PRO 4650U (6 núcleos físicos, 12 lógicos)
- **Sistema:** Windows 11 com WSL2 (Ubuntu)
- **Memória disponível para o WSL:** 3 GB
- **Compilador:** GCC
- **Paralelismo:** OpenMP (ativo por padrão no CMake do IQ-TREE)
Os registros completos do ambiente ficam em `docs/`.
 
## Estrutura
 
```
bioinformatica/
├── docs/              # documentação dos testes e do procedimento
│   └── teste-inicial.md
├── data/              # alinhamentos usados nos experimentos (sem versionamento)
├── iqtree2/           # código-fonte do IQ-TREE 2
└── README.md
```
 
## Como reproduzir
 
1. Clone o IQ-TREE 2 com submódulos:
```bash
   git clone --recursive https://github.com/iqtree/iqtree2.git
```
 
2. Instale as dependências (CMake, GCC, Eigen3, Boost, zlib).
3. Compile:
```bash
   cd iqtree2
   mkdir build && cd build
   cmake -DCMAKE_BUILD_TYPE=Release ..
   make -j6
```
 
4. Execute o teste inicial:
```bash
   ./iqtree2 -s ../example/example.phy -m GTR+G -T 2 -seed 1 --prefix teste_inicial
```
 
Para o procedimento completo, veja `docs/teste-inicial.md`.
 
## Status
 
- [x] Compilação do IQ-TREE 2 com OpenMP
- [x] Teste inicial com 1 e 2 threads (`docs/teste-inicial.md`)
- [ ] Fixar o IQ-TREE como submódulo em uma versão específica
- [ ] Experimento com alinhamento maior (escalonamento forte e tamanho do problema)
- [ ] Repetições com mediana e dispersão
## Versão
 
Commit de referência do código: `a00094e0` (branch `master` do IQ-TREE 2). Esse valor será substituído pela tag escolhida quando o submódulo for fixado.
 