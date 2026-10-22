# Cerebras: o que existe por trás do código que executamos?

> 📽️ **[Abrir a apresentação no Google Slides](https://docs.google.com/presentation/d/1vHQtJ6lO5HGWStWyTaQkpKTts6UJy6HmRdGkFc4zOX8/edit)**

Trabalho do Grupo 02 para a disciplina **Arquiteturas para Desempenho** (CESAR School). A Cerebras, com seu chip WSE-3, é a arquitetura estudada. Abaixo, cada ponto do desafio é respondido com a explicação sobre a Cerebras.

---

## 1. Arquitetura

### Qual é sua ISA?

A Cerebras não publica uma ISA comparável ao x86 ou ao ARM. O que se conhece são as categorias de instrução: aritméticas, lógicas, de acesso à memória, de comparação e de desvio, com instruções de tensor por cima. Os programadores não escrevem essas instruções diretamente; escrevem em uma linguagem de alto nível própria, a CSL, que gera o código de cada núcleo.

Algumas características da ISA:

- **Tensores são operandos de primeira classe:** uma instrução pode operar sobre um array inteiro da memória, sem laços explícitos.
- **Comunicação faz parte da instrução:** enviar dados a um núcleo vizinho é quase tão rápido quanto uma operação aritmética.
- **Esparsidade no hardware:** zeros não são enviados, e o cálculo só acontece quando há dados úteis.
- **Controle e tensor juntos:** há instruções de controle comuns, e sobre elas suporte a operações de tensor.

### Organização do processamento

O WSE-3 é um único chip do tamanho de um wafer, com cerca de 900 mil núcleos organizados em uma malha. Cada núcleo tem uma unidade de cálculo, um roteador que o liga aos quatro vizinhos diretos e uma pequena memória local. Não há cache: quem decide onde cada dado fica é o programador.

### Hierarquia de memória

Em CPUs e GPUs, o dado passa por vários níveis de cache até chegar ao processador. No WSE, a hierarquia é mais curta: o núcleo usa a própria memória local e, quando precisa, pega dados dos vizinhos pela malha.

O ganho é uma banda de memória muito alta, de 21 PB/s. O custo é a capacidade total de 44 GB.

### Paralelismo de execução

O paralelismo é espacial: centenas de milhares de núcleos executam ao mesmo tempo, cada um com seus dados. Dentro de cada núcleo, a execução é guiada por eventos, como a chegada de dados vindos de um vizinho.

### Vetorização

A vetorização clássica, como as instruções AVX do x86, não é o eixo do WSE. Operações sobre arrays inteiros são feitas por descrições de dados que percorrem a memória com uma única instrução. O núcleo é otimizado para precisões reduzidas, como fp16 e bf16.

---

## 2. Programação

### Compiladores e linguagens

- **CSL:** linguagem de estilo C, de baixo nível, que descreve o código de cada núcleo e as rotas de comunicação entre eles.
- **SDK e host:** um programa em Python, no computador host, envia os dados ao chip, dispara o cálculo e recebe o resultado.
- **Compiladores de alto nível:** há um pipeline baseado em MLIR para stencils, com desempenho próximo ao código escrito à mão, segundo os autores.

### Bibliotecas e frameworks

Para IA, frameworks como o PyTorch já estão integrados à pilha de software, e bibliotecas prontas reduzem a necessidade de escrever CSL. Para HPC, ainda é comum escrever o código manualmente.

### Ferramentas de profiling

O host permite medir a execução do programa. A ferramenta específica de profiling não foi confirmada, por isso não é citada nos slides.

### Extensões específicas

O modelo tem conceitos próprios: mensagens que carregam um canal virtual de comunicação, descritores de estrutura de dados e tarefas disparadas por eventos. Eles não existem em linguagens de uso geral.

---

## 3. Desempenho

### Cores, frequência e SIMD

- **Cores:** cerca de 900 mil núcleos. É o principal mecanismo de desempenho.
- **Frequência:** modesta, em torno de 875 MHz. O WSE não compete por clock.
- **SIMD:** existe por meio das descrições de dados, mas não é o fator principal.

### Largura de memória e cache

Não há cache. A largura de memória vem da memória local distribuída, que dá 21 PB/s de banda no total. Cada núcleo acessa a própria memória diretamente, com alta largura de banda.

### Paralelismo e aceleradores

O WSE é um acelerador especializado. Ele tira proveito do paralelismo espacial e da comunicação local, e é mais eficiente em problemas com essa estrutura, como malhas, stencils e álgebra linear.

### Outros mecanismos

- **Dataflow e esparsidade:** o cálculo acontece quando os dados chegam, e zeros não são processados.
- **Roteamento reconfigurável:** as rotas de comunicação podem mudar durante a execução.

### Resultados publicados

- **Dinâmica molecular:** 1,144 milhão de passos por segundo para 200 mil átomos (Perez et al., *J. Chem. Phys.*, 2025).
- **Dinâmica molecular vs. Frontier:** 179× mais passos por segundo, para 800 mil átomos (Cerebras, 2024).
- **Stencil 2D:** até 342× sobre uma implementação adaptada em GPU A100 (Belli & De Sensi, arXiv:2605.07954).

Esses números são medições dos próprios autores ou da Cerebras, e devem ser lidos com cautela.

---

## 4. Aplicação prática

### Dinâmica molecular (HPC)

- **Problema:** simular o movimento de átomos por milhões de passos de tempo. Em supercomputadores, o tempo simulado por dia é curto.
- **Por que o WSE ajuda:** cada átomo ocupa um núcleo, que conversa só com vizinhos, o que combina com a estrutura do chip.
- **Resultado:** 179× mais rápido que o Frontier, segundo a Cerebras. A colaboração envolveu Sandia, Lawrence Livermore e Los Alamos.

### Stencils (HPC)

Um stencil atualiza cada ponto de uma grade a partir de seus vizinhos, o que aparece em problemas de calor, ondas e fluidos. No WSE, a grade é dividida entre os núcleos, que trocam as bordas com os vizinhos. Esse padrão combina bem com a malha do chip.

---

## 5. Avaliação crítica

### Vantagens

1. **Memória muito rápida:** banda de 21 PB/s, sem gargalo de memória externa.
2. **Comunicação local de baixa latência:** cada núcleo fala com seus vizinhos, sem barramentos globais.
3. **Paralelismo massivo com esparsidade:** centenas de milhares de núcleos, com zeros tratados no hardware.

### Limitações

1. **Capacidade de 44 GB:** modelos e problemas maiores precisam ser divididos entre sistemas.
2. **Programação de baixo nível:** a CSL exige pensar em rotas de dados e sincronização. A curva de aprendizado é alta e a comunidade é pequena.
3. **Benchmarks do fabricante e custo elevado:** os resultados vêm da própria empresa, e o consumo é alto.

**Conclusão:** o WSE compensa quando o problema cabe na memória do chip e tem estrutura de malha ou de vizinhança. Fora desse caso, a generalidade de CPUs e GPUs ainda é mais vantajosa.

---

## Metodologia de pesquisa

As informações foram levantadas por buscas na web e pela leitura direta das fontes. Buscas feitas (em inglês):

1. `arXiv 2405.07898 molecular dynamics Cerebras wafer-scale Santos`
2. `arXiv 2411.10532 molecular dynamics EAM Cerebras Wafer Scale Engine`
3. `Cerebras press release May 15 2024 Sandia Lawrence Livermore Los Alamos molecular dynamics`
4. `Hot Chips 34 Cerebras WSE-2 presentation 2022`
5. `Cerebras WSE-3 technical specifications 900,000 cores 44 GB SRAM 21 PB/s`
6. `Cerebras Hot Chips 31 2019 wafer scale engine 1.2 trillion transistors 400,000 cores`
7. `Cerebras announces third generation wafer-scale engine WSE-3 5nm 4 trillion transistors March 2024 press release`
8. `Cerebras WSE-3 21 PB/s memory bandwidth 215 PB/s fabric bandwidth`

Buscas em bases acadêmicas (arXiv, IEEE Xplore e ACM Digital Library), em inglês:

9. `Cerebras wafer-scale engine architecture dataflow sparse computation`
10. `wafer-scale engine high performance computing scientific simulation evaluation`
11. `Cerebras WSE CSL programming model stencil molecular dynamics`
12. `Cerebras WSE-2 WSE-3 wafer-scale vs GPU benchmark comparison`

Trabalhos acadêmicos encontrados nessas buscas que complementam o estudo (os de referência foram lidos; os demais, apenas identificados pelo título):

- *Cerebras Architecture Deep Dive: First Look Inside the Hardware/Software Co-Design for Deep Learning*. IEEE Micro, 2023. https://dl.acm.org/doi/abs/10.1109/MM.2023.3256384 (lido: origem das afirmações sobre a ISA)
- *A Comparison of the Cerebras Wafer-Scale Integration Technology with Nvidia GPU-based Systems for Artificial Intelligence*. arXiv:2503.11698. https://arxiv.org/abs/2503.11698 (identificado)
- *Benchmarking the Cerebras Wafer Scale Engine-2 Architecture*. ACM, 2025. https://dl.acm.org/doi/10.1145/3731599.3767438 (identificado)
- *Benchmarking the Performance of Large Language Models on the Cerebras Wafer Scale Engine*. arXiv:2409.00287. https://arxiv.org/abs/2409.00287 (identificado)
- *Record Acceleration of the Two-Dimensional Ising Model Using High-Performance Wafer Scale Engine*. arXiv:2404.16990. https://arxiv.org/abs/2404.16990 (identificado)
- *SPADA: A Spatial Dataflow Architecture Programming Language*. arXiv:2511.09447. https://arxiv.org/abs/2511.09447 (identificado)
- *Massively scalable stencil algorithm*. Jacquelin et al. (Cerebras Systems). arXiv:2204.03775. https://arxiv.org/abs/2204.03775 (identificado)

Além das buscas, foram lidos diretamente os artigos (arXiv e *J. Chem. Phys.*), os slides do Hot Chips 2024, as notas de imprensa e os blogs da Cerebras, e o white paper do SDK. Quando uma afirmação não pôde ser confirmada na fonte primária, isso foi registrado no texto.

---

## Referências principais

- Belli, E.; De Sensi, D. "Stencil Computations on Cerebras Wafer-Scale Engine". arXiv:2605.07954. https://arxiv.org/abs/2605.07954
- Perez, D. et al. "Breaking the mold: Overcoming the time constraints of molecular dynamics on general-purpose hardware". *J. Chem. Phys.* 162, 072501 (2025). https://doi.org/10.1063/5.0249193
- Santos, K. et al. "Breaking the Molecular Dynamics Timescale Barrier Using a Wafer-Scale System". arXiv:2405.07898. https://arxiv.org/abs/2405.07898
- Stawinoga, N. et al. "An MLIR Lowering Pipeline for Stencils at Wafer-Scale". arXiv:2601.17754. https://arxiv.org/abs/2601.17754
- Cerebras. Materiais técnicos e notas de imprensa sobre o WSE-3 e sobre dinâmica molecular (2024).