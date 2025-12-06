# Programação Assembly e Construção de Driver de Software.

**Universidade Estadual de Feira de Santana (UEFS)**

**Disciplina:** Sistemas Digitais (TEC499) - 2025.2, UEFS

**Equipe:** Luis Felipe Carneiro Pimentel e Walace de Jesus Venas

---

## Índice

- [Índice](#índice)
- [1. Descrição e Objetivo do Projeto](#1-descrição-e-objetivo-do-projeto)
- [2. Levantamento de Requisitos e Solução](#2-levantamento-de-requisitos-e-solução)
  - [Requisitos Gerais](#requisitos-gerais)
  - [Solução Adotada](#solução-adotada)
- [3. Arquitetura Geral da Solução](#3-arquitetura-geral-da-solução)
- [4. Estrutura dos Módulos (HPS e FPGA)](#4-estrutura-dos-módulos-hps-e-fpga)
  - [HPS (ARM)](#hps-arm)
  - [FPGA (Cyclone V)](#fpga-cyclone-v)
- [5. Instalação, Configuração e Integração](#5-instalação-configuração-e-integração)
- [6. Fluxo de Dados Entre HPS e FPGA](#6-fluxo-de-dados-entre-hps-e-fpga)
- [7. Testes, Resultados e Validação](#7-testes-resultados-e-validação)
- [8. Referências Cruzadas: Documentação Específica](#8-referências-cruzadas-documentação-específica)

---

## 1. Descrição e Objetivo do Projeto

Esse _README_ é a versão atualizada para a terceira etapa do projeto de Zoom em imagens de escala cinza. A terceira etapa teve foco na atualização das funções já presentes na parte HPS do projeto, com a adição da funcionalidade de aplicar zoom-in sobre determinada área da janela fazendo uso do mouse e teclado. O _README_ da segunda etapa desse projeto pode ser encontrado aqui: [Segunda Etapa](https://github.com/LordWalace/PBL_SD2/blob/main/FPGA/README.md)  

Este projeto integra um sistema completo de processamento de zoom de imagens em escala de cinza (8 bits/pixel) para o kit DE1-SoC. A lógica central reside na divisão funcional:
- O **HPS (ARM)** cuida da interface com o usuário, da manipulação de arquivos BMP por meio de mouse e teclado, comandos via menu interativo, comunicação com o hardware através de Assembly.
- O **FPGA (Cyclone V)** implementa o coprocessador dedicado, recebendo comandos, processando algoritmos de zoom, e retornando dados e flags de estado.
O objetivo é criar uma solução robusta, modular e expansível, documentada de forma que futuros usuários/engenheiros possam compreender, replicar, manter e aprimorar.

---

## 2. Levantamento de Requisitos e Solução

### Requisitos Gerais
- Manipulação e carragemento correto de imagens BMP.
- Comunicação eficiente entre HPS e FPGA via barramentos PIO (Parallel Input/Output), com controle de delays e sinalização de status.
- Interface de usuário clara e direta.
#### Requisitos da terceira etapa
- Uso do mouse para seleção de uma região da janela para a aplicação de _zoom-in_.
- Coordenadas presentes no sistema para auxiliar o usuário na escolha da área desejada para aplicar zoom.
- Fazer uso das teclas "+" e "-" do teclado para aplicar _zoom-in_ e _zoom-out_ respectivamente


### Solução Adotada
- HPS e FPGA conectados via sistema Qsys(Platform Designer).
- Separação total entre lógica de interface/controle (HPS) e lógica de processamento (FPGA).
- Estrutura modular, favorecendo manutenção e expansão.

![Diagrama de Interconexão HPS ↔️ FPGA via AXI BRIDGE](https://raw.githubusercontent.com/Marcelosgc1/SistemasDigitais_Problema2/refs/heads/main/imagens/fpga-hps.png)

Mais detalhes sobre os requisitos e solução específica em cada componente estão nos seus respectivos READMEs:  
• [README HPS](https://github.com/LordWalace/PBL_SD3/blob/main/HPS/README.md)  
• [README FPGA](https://github.com/LordWalace/PBL_SD3/blob/main/FPGA/README.md)

---

## 3. Arquitetura Geral da Solução

```graph
    U[Usuário]
    M[Menu de Alta Nível (HPS)]
    BMP[BMP Loader]
    ASM[API Assembly]
    PIO[Registradores PIO]
    FPGA[Coprocessador FPGA]
    VGA[Monitor VGA]

    U --> M
    M --> BMP
    M --> ASM
    ASM --> PIO
    PIO --> FPGA
    FPGA --> VGA
    FPGA --> PIO
    PIO --> ASM
```


**Descrição:**  
- O usuário interage com o menu HPS.
- Imagens BMP são selecionadas e carregadas pelo HPS.
- API Assembly monta e envia comandos via registradores PIO.
- FPGA recebe a instrução, processa e retorna status/resultados.
- A imagem processada é visualizada via monitor VGA.

---

## 4. Estrutura dos Módulos (HPS e FPGA)

### HPS (ARM)
- Interface de navegação via menus.
- Gerenciamento de arquivos BMP.
- Manipulação de arquivos BMP fazendo uso do teclado.
- Lógica do cursor para seleção de área para aplicação de algoritmos de zoom.
- API Assembly para controle direto do hardware e sincronização.

Mais detalhes: [README HPS](https://github.com/LordWalace/PBL_SD3/blob/main/HPS/README.md)

### FPGA (Cyclone V)
- Implementação da ISA customizada.
- FSM e Datapath para zoom e manipulação de VRAM.
- Flags de status para monitoramento e depuração.

Mais detalhes: [README FPGA](https://github.com/LordWalace/PBL_SD3/blob/main/FPGA/README.md)

---

## 5. Instalação, Configuração e Integração

**Resumo dos passos principais:**
1. Instalar ambiente Linux e ferramentas ID/compilação HPS.
2. Realizar upload do bitstream do FPGA (projeto pronto e validado).
3. Instalar, compilar e executar o software HPS conforme instruções individuais.
4. Verificar conexão física HPS ↔ FPGA.
5. Conectar um mouse diretamente no HPS.
6. Testar operações iniciais (carregar imagem, zoom, reset, status).

Para instruções detalhadas de instalação/configuração, acesse:
- [README HPS](https://github.com/LordWalace/PBL_SD2/blob/main/HPS/README.md)
- [README FPGA](https://github.com/LordWalace/PBL_SD2/blob/main/FPGA/README.md)

---

## 6. Fluxo de Dados Entre HPS e FPGA

**Resumo do protocolo:**
- HPS monta instrução (LOAD, STORE, ZOOM) em formato de 32 bits.
- A instrução é escrita em registrador PIO na FPGA.
- Pulso de enable ativa o processamento.
- FPGA processa, atualiza flags de status, retorna resultado.
- HPS lê flags, exibe resultados para usuário e/ou manipula imagem conforme saída.

**Diagrama de sinalização PIO (ver detalhes em cada README):**
| Sinal        | Direção | Finalidade                      | Largura    |
|--------------|---------|----------------------------------|------------|
| instruct     | Entrada | Palavra de comando               | 32 bits    |
| enable       | Entrada | Disparo de processamento         | 1 bit      |
| flags        | Saída   | Status do processamento          | 4 bits     |
| data_out     | Saída   | Dados lidos pelo HPS             | 8 bits     |

---

## 7. Testes, Resultados e Validação

- Testes unitários e integrados validaram todas operações existentes.
- Erros identificados e corrigidos incluem dimensionamento inadequado de imagens, erros de sincronização entre HPS/FPGA e limites de zoom atingidos corretamente.
- Resultados detalhados com logs, imagens e exemplos estão disponíveis nas seções específicas dos respectivos READMEs.

---

## 8. Referências Cruzadas: Documentação Específica

- **Documentação HPS (interface, menus, API Assembly):** [README HPS](https://github.com/LordWalace/PBL_SD3/blob/main/HPS/README.md)
- **Documentação FPGA (coprocessador, ISA, blocos internos):** [README FPGA](https://github.com/LordWalace/PBL_SD3/blob/main/FPGA/README.md)
- **Este README** serve como visão geral e guia para integração entre os dois sistemas.

---

> Para dúvidas sobre integração, arquitetura e fluxograma do sistema, consulte os documentos da referência.  
> Para detalhes de implementação, operação ou depuração, acesse os READMEs específicos de HPS e FPGA acima.
