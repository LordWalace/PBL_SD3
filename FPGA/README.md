**Universidade Estadual de Feira de Santana (UEFS)**

**Disciplina:** Sistemas Digitais (TEC499) - 2025.2

**Equipe:** Luis Felipe Carneiro Pimentel e Walace de Jesus Venas

---
Coprocessador FPGA para Processamento de Imagens em Tons de Cinza
---

Sumário
=================
  * [1. Softwares Utilizados](#1-softwares-utilizados)
  * [2. Hardware Usado nos Testes](#2-hardware-usado-nos-testes)
  * [3. Arquitetura do Sistema](#3-arquitetura-do-sistema)
  * [4. Funcionalidades e ISA](#4-funcionalidades-e-isa)
  * [5. Barramentos PIO e Sinais de Comunicação](#5-barramentos-pio-e-sinais-de-comunicação)
  * [6. Análise de resultados](#6-análise-de-resultados)


## Descrição do projeto.

A segunda etapa do projeto **Zoom Digital** Sistemas Digitais (MI) teve foco na interação entre a unidade de processamento principal, o HPS (Hard Processor System), e o chip reconfigurável, o FPGA (Field Programmable Gate Array), ambos presentes na placa DE1-SoC. A conexão entre essas duas áreas foi estabelecida utilizando interfaces PIOs (Parallel Input/Output) customizadas, definidas pelo Platform Designer e integradas através da arquitetura de barramento AXI. Essa transmissão de dados otimizada entre os dois componentes viabiliza o encaminhamento de comandos, informações e sinais de feedback para a execução das tarefas de processamento de matrizes. Na terceira etapa do projeto não houve alterações drásticas na parte do FPGA do projeto. 

O cooprocessador e sua implementação em verilog pode ser visualizado no [Repositório da etapa 1](https://github.com/LordWalace/PBL_SD1/blob/main/README.md).

Nessa etapa os arquivos "*main.v*", "*mem1.v*" e "*memory_control.v*" assim como o contéudo presente na pasta "*aux_files*" vem de um repósitorio externo usado para a elaboração do Verilog desse projeto. Origem dos arquivos não feitos pela equipe que foram citados: [Repositório externo](https://github.com/DestinyWolf/Problema-SD-2025-2?tab=readme-ov-file#estrutura-do-coprocessador)

---

## 1. Softwares Utilizados.
- **IDEs de Desenvolvimento:** *Intel Quartus Prime Lite Edition (23.1std.0),Visual Studio Code*
- **Simulador:** *ModelSim - Intel FPGA Edition (2020.1)*
- **Linguagem HDL:*** Verilog-2001*

---

## 2. Hardware Usado nos Testes.
- **Placa de Desenvolvimento:** Terasic DE1-SoC
- **FPGA:** Intel Cyclone V SE 5CSEMA5F31C6N
- **Memória de Vídeo (Frame Buffer):** RAM de dupla porta (76.800 palavras x 8 bits)
- **Monitor:** Philips VGA (640x480 @ 60Hz)

---

<div align="center">
  <img src="https://i.postimg.cc/9FxMzNZK/Captura-de-imagem-20251205-222707.png"><br>
  <strong>Componentes necessários para a utilização do projeto.</strong><br><br>
</div>

---

## 3. Arquitetura do Sistema

A arquitetura se baseia em uma divisão clara entre software e hardware para isolar o processamento de pixels das operações de deslocamento/zoom. A comunicação entre o HPS e o Coprocessador é realizada através de Barramentos PIO (Parallel Input/Output). Nessa seção haverá um foco nos arquivos "top_level" e "main" do Verilog, já que, eles foram modificados e criados para auxiliar no desenvolvimento da etapa 2 do projeto **Zoom Digital**

### 3.1. Blocos Principais.

A solução é baseada em uma arquitetura de hardware customizada, centralizada pelo módulo main.v, que atua como a Unidade de Controle (FSM) e o Processador de Dados.

**Blocos funcionais:**
- Geração de Clock (PLL): Responsável por gerar os sinais de clock necessários a partir do clock de entrada do FPGA.
- Unidade de Controle (FSM): Implementada como uma Máquina de Estados Finitos. Recebe instruções de um barramento de controle externo (implícito via ghrd_top.v e entradas como INSTRUCTION, ENABLE) e gerencia o fluxo de dados e a execução das operações.
- Sistema de Gerenciamento de Memórias (Três Bancos): Três instâncias de memória RAM de porta simples (módulos mem1 implícitos) são usadas para segregação de dados.
- Pipeline do Algoritmo: Conjunto de registradores e lógica para executar as transformações de imagem (Zoom/Média).
- Controlador VGA: Gerencia o endereçamento de leitura da memória de exibição e a sincronização para a saída de vídeo.

**Fluxo de Controle:** O pulso *ENABLE* inicia a FSM. A *INSTRUCTION* decide o próximo estado: *READ_AND_WRITE* (para *LOAD/STORE*), *ALGORITHM* (para *PR_ALG/BA_ALG/NHI_ALG/NH_ALG*), *RESET*, ou *COPY_READ/COPY_WRITE* (para *REFRESH_SCREEN*).

**Fluxo de Dados:** 
- Escrita Externa (*STORE*): *DATA_IN* é escrito na memory1 (Memória Original) no endereço *MEM_ADDR*.
- Leitura Externa (*LOAD*): A FSM endereça *MEM_ADDR* em *memory1* ou *memory3* e o dado lido (*data_out_mem1* ou *data_out_mem3*) é enviado para a saída *DATA_OUT*.
- Atualização de Tela: Os estados *COPY_READ/COPY_WRITE* copiam o conteúdo da *memory1* (ou *memory3* após um algoritmo) para a *memory2* (Memória de Exibição).
- Exibição: O Controlador VGA lê continuamente a *memory2* para a saída VGA.
- Algoritmo: A FSM no estado *ALGORITHM* lê dados da *memory1* , o Pipeline do Algoritmo processa esses dados (*data_out_mem1*) e escreve o resultado (*data_to_write*) na *memory3* (Memória de Trabalho).

A solução resolve o problema de processamento de imagem permitindo que a CPU (ou HPS) realize tarefas complexas e demoradas (como Zoom e o Resize) para a FSM otimizada em hardware, liberando o processador e fornecendo feedback de status (*FLAG_DONE*, *FLAG_ERROR*).

### 3.2 Explicação dos blocos.

#### Geração de Clock (Instância pll0).
**Função:** Sincronização do sistema e geração de clocks com diferentes frequências.
**Dados de Entrada:** refclk (*CLOCK_50*, 50 MHz): Clock de referência da placa.
**Processamento:** O módulo pll (implícito, geralmente um *Megafunction* da Altera/Intel FPGA) gera clocks derivados.
**Dados de Saída:** *outclk_0* (*clk_100*, 100 MHz): Usado para a FSM e lógica principal ; *outclk_1* (clk_25_vga, 25 MHz): Usado para o controlador VGA.

---

#### Sistema de Gerenciamento de Memórias.
Três instâncias de memória mem1 (assumidas como RAM de porta simples) são utilizadas: 
| Memória | Função | Endereçamento (Bits) | Tamanho Máx (Bytes) |
| :--- | :--- | :--- | :--- | 
| memory1 | Imagem Original | 17 ([16:0]) | 131072 (Limite: 76800 para 320x240) | 
| memory2 | Imagem de Exibição (VGA) | 17 ([16:0]) | 131072 | 
| memory3 | Memória de Trabalho (Algoritmo) | 17 ([16:0]) | 131072 |

Processamento de Dados: Cada memória recebe um endereço de leitura (*rdaddress*), um endereço de escrita (*wraddress*), o dado de entrada (*data*) e um sinal de escrita (*wren*) no clock (*clk_100*), retornando o dado de saída (q) no clock seguinte.

---

#### Unidade de Controle (FSM - Registrador uc_state).

| Estado (uc_state)	| Código	| Descrição| 
| --- | --- | --- |
| IDLE	| 3'b000	| Estado inicial. Aguarda o pulso enable_pulse e a instrução (INSTRUCTION). Ativa FLAG_DONE. |
| READ_AND_WRITE | 3'b001 | Executa operações de LOAD ou STORE. Escreve DATA_IN em memory1 se for STORE. Prepara o endereço para LOAD. |
| ALGORITHM | 3'b010 | Executa o algoritmo de processamento de imagem (multi-ciclos). Controla o pipeline de leitura de memory1 e escrita em memory3. |
| RESET | 3'b011 | Reseta o estado do sistema e a variável de zoom para o valor padrão (3'b100, 4x). |
| COPY_READ | 3'b100 | Lê o próximo dado da memória fonte (memory1 ou memory3), incrementando counter_rd_wr. |
| COPY_WRITE | 3'b101 | Escreve o dado lido na memory2 (memória de exibição). |
| WAIT_WR_OR_RD | 3'b111 | Aguarda a conclusão das operações de leitura ou escrita das memórias no ciclo de clock seguinte. |

Entrada de Dados: INSTRUCTION[2:0], ENABLE (pulso).
Saída de Dados: FLAG_DONE, FLAG_ERROR, sinais de wren das memórias.

---

#### Pipeline do Algoritmo (Lógica no estado ALGORITHM).

A lógica de processamento de imagem é executada sequencialmente dentro do estado ALGORITHM, controlada por um sub-estado op_step.
**PR_ALG (Zoom In/Smooth Zoom):** Processamento: Implementa um tipo de interpolação/duplicação (zoom in). A lógica lê o pixel original em memory1 (endereçado por old_x, old_y) e o escreve múltiplas vezes em memory3 (endereçado por new_x, new_y). Os offsets (old_x/old_y) são ajustados com base no nível de zoom (next_zoom) para determinar qual pixel de origem deve ser lido para a nova coordenada de escrita.

**BA_ALG (Zoom Out/Average):** Processamento: Implementa uma média de pixels (zoom out/redução). O algoritmo lê um bloco de 4 pixels (data_out_mem1) de memory1 (e.g., 2x2 para 2x zoom out) em 4 ciclos de clock, armazena em data_to_avg e calcula a média aritmética para o novo pixel de saída, que é escrito em memory3.

**NH_ALG/NHI_ALG (Nearest Neighbor/Crop):** Processamento: Para NHI_ALG (Zoom In), duplica pixels de forma análoga a PR_ALG. Para NH_ALG (Zoom Out/Crop), implementa um corte (crop) na imagem, lendo os pixels apenas de uma região central de memory1 para memory3. Fora da área de corte (o resto da tela 320x240), é escrito um valor de pixel preto (8'b0).

---

#### Lógica do VGA.
**Função:** Mapeia coordenadas de tela para endereços de memória, garantindo que apenas a imagem seja lida na área de exibição.

**Dados de Entrada:** clk_25_vga, coordenadas next_x e next_y (do vga_module instanciado).
**Dados de Saída:** Endereço de leitura addr_mem2 (via addr_from_vga), dado de cor data_to_vga_pipe para o vga_module.

**Processamento:** O bloco verifica se as coordenadas (next_x, next_y) estão dentro da "caixa" de exibição (exemplo: X_START=159, Y_START=119 a X_END=479, Y_END=359 para uma imagem de 320x240 no centro). Se estiverem, calcula o endereço de memória vga_offset para memory2. O dado (data_out_mem2) é pipelineado para a saída de cor VGA.

---

## 4. Funcionalidades e ISA

O coprocessador implementa uma ISA enxuta com três classes de instrução, focadas em transferência de dados e execução de zoom:

| Classe | Descrição |
| --- | --- |
| LOAD	| Leitura de dado da memória de imagem. |
| STORE	| Escrita de dado na memória de imagem. | 
| ZOOM	| Execução da operação de ampliação/redução sobre uma região. |README HPS](https://github.com/LordWalace/PBL_SD2/blob/main/HPS/README.md)

#### 4.1. Formato da Instrução (Palavra de 32 bits).

| Bits	| Função |
| --- | --- |
| [2:0]	| Código da operação (OpCode) |
| [19:3]	| Endereço de memória|
| [28:21]	| Dado de entrada (apenas para STORE) |
| [31:29]	| Reservado |

---

## 5. Barramentos PIO e Sinais de Comunicação

Nesse projeto, os barramentos PIO servem como a ponte de comunicação entre o processador HPS (ARM) e o coprocessador FPGA, permitindo que o código Assembly realize operações diretas sobre o hardware customizado via mapeamento de memória. 

Cada função em Assembly acessa registradores PIO específicos que foram cuidadosamente definidos em Verilog, escrevendo ou lendo comandos, dados e flags para executar ações, verificar resultados ou controlar estados internos da FPGA. Essa arquitetura, baseada em leitura e escrita sequencial de offsets de memória correspondentes aos registradores, permite que o software de alto nível desencadeie e sincronize operações especializadas. Essa seção vai exibir elaborar mais afundo sobre os Barramentos PIO.

 | Sinal |	Direção	Função |	Largura |
 | --- | --- | --- |
| INSTRUCT |	Entrada	Palavra de comando (ISA, endereço, dado) |	32 |
| ENABLE |	Entrada	Pulso de ativação do coprocessador |	1 |
| FLAGS |	Saída	Sinalização de status (done, erro, limites) |	4 |
| DATA_OUT |	Saída	Retorno para leitura de dados (LOAD) |	8 |

### 5.1. Detalhamento dos Sinais de Saída.

Os 4 bits do sinal flags indicam o status da operação:

- **DONE:** Processamento da instrução concluído.
- **ERROR:** Instrução incorreta, endereço fora do mapeamento ou dado inválido.
- **ZOOM_MIN:** Tentativa de zoom abaixo do limite permitido.
- **ZOOM_MAX:** Tentativa de zoom acima do limite permitido.
- **Protocolo de Comunicação:** É mandatório que o sinal enable seja desativado após cada operação para garantir a sincronização entre software (HPS) e hardware (FPGA).

---

## 6. Análise de resultados

Essa seção vai detalhar os resultados do projeto.

O projeto final demonstrou conseguir ser capaz de cumprir todas as tarefas que foram requisitadas pelo problema. Ou seja, imagens podem ser carregadas, exibidas e transformadas corretamente desde que estejam na escala cinza e na resolução correta, o projeto oferece os mesmos algoritmos de zoom que foram providos na etapa 1, ou seja, duas opções de *Zoom-In* e duas opções de *Zoom-Out*. O usuário consegue interagir de maneira simples e simplificada com o projeto a partir do Makefile

**Resumo.**
O projeto foi implementado com as seguintes funcionalidades:
- Suporte à uma interface de usuário prática
- Execução e ligação dos arquivos a partir de um "Makefile"
- Tratamento de erro para imagens incompatíveis

É possível saber mais dos testes realizados e dos resultados a partir do [README HPS](https://github.com/LordWalace/PBL_SD3/blob/main/HPS/README.md).
###


