**Universidade Estadual de Feira de Santana (UEFS)**

**Disciplina:** Sistemas Digitais (TEC499) - 2025.2, UEFS

**Equipe:** Luis Felipe Carneiro Pimentel e Walace de Jesus Venas

Sumário
=================
  * [1. Menu Principal](#1-menu-principal)
  * [2. Funções do código Assembly](#2-funções-do-código-assembly)
  * [3. Implementação do zoom em área específica](#3-implementação-do-zoom-em-área-específica)
  * [4. Makefile](#3-makefile)
  * [5. Erros Comuns e Mensagens de Alerta](#4-erros-comuns-e-mensagens-de-alerta)

## Descrição e Objetivo do Projeto

Este projeto consiste no desenvolvimento de uma **Interface de Programação de Aplicações (API)**, escrita em linguagem Assembly, para controlar um coprocessador de processamento de imagens embarcado em um sistema com o processador **ARM (HPS)**. A **API** deve implementar a **ISA** do coprocessador, reutilizando as operações previamente definidas via componentes físicos da placa, para manipular imagens em escala de cinza (8 bits por pixel) que são lidas de um arquivo e transferidas do HPS para o coprocessador. As imagens BMP exibidas em tela podem ser manipuladas fazendo o uso do mouse e teclado, permitindo o usuário selecionar a área que deseja aplicar o zoom.

Esse documento está voltado em descrever como foi o desenvolvimento da parte Assembly e C da terceira etapa do projeto. Um guia do usuário também se encontra nesse documento visto que é possível explicar os testes efetuados e conseguir demonstrar como replicar eles para que o programa apresente os resultados esperados.

> [!NOTE]
> O desenvolvimento da parte Assembly do projeto foi realizado primeiramente fazendo uso da linguagem C e depois convertendo os arquivos para Assembly.

---

## Navegação e Interfaces e Guia de Usuário

O sistema é operado através de um **menu de texto interativo**.

### 1. Menus
#### 1.1 Menu Principal

Ao iniciar o programa, este menu será exibido. Digite o número da opção desejada e ela sera acessada automaticamente.

> [!WARNING]
> A **primeira** inicialização do programa apresenta uma imagem, entretanto, essa imagem não consegue ser alterada fazendo uso dos algoritmos de zoom. Caso o usuário tente acessar o menu de zoom antes de selecionar alguma imagem (mesmo que alguma imagem esteja aparecendo na tela) o programa não permite. Para conseguir fazer uso dos algoritmos de zoom é preciso sempre selecionaruma imagem. Isso sempre será necessário quando for o **primeiro acesso ao sistema depois de fechar ele** ou após fazer uso da opção **_Reset_** do menu principal.

| Opção | Ação | Observação |
| :--- | :--- | :--- |
| **[1] Carregar Imagem** | Vai para o menu de seleção de imagens. | Caso nenhuma imagem seja selecionada uma imagem "padrão" já é carregada. |
| **[2] Aplicar Zoom** | Vai para o menu de algoritmos de zoom.| Só é possível com uma imagem carregada. |
| **[3] Reset do Sistema** | Limpa o estado atual do coprocessador FPGA. | Util para retornar a imagem para seu estado "original" (Sem zooms) |
| **[4] Status** | Faz o terminal exibir as propriedades atuais do sistema. | Exibe flags e informações sobre o estado atual do sistema e dimensões suportadas. |
| **[0] Sair** | Encerra o programa. | |

<div align="center">
  <img src="https://i.postimg.cc/cLnSZDm5/Menu1.png"><br>
  <strong>Menu principal</strong><br><br>
</div>

#### 1.2. Menu de Seleção de Imagens

Após escolher a opção **[1]**, uma lista de arquivos BMP disponíveis na pasta será exibida.

* Digite o número correspondente à imagem que deseja carregar (Ex: **2** para `Hornet.bmp`).
* A imagem selecionada será carregada e enviada para o coprocessador FPGA.


<div align="center">
  <img src="https://i.postimg.cc/26JsPhZM/Imagem-Select2.png"><br>
  <strong>Seleção de imagens</strong><br><br>
</div>

#### 1.3. Menu de Zoom

Após carregar uma imagem, a opção **[2]** do menu principal levará a este menu, que lista os algoritmos disponíveis:

| Opção | Algoritmo | Fator de Escala (Exemplos) | Efeito |
| :--- | :--- | :--- | :--- |
| **[1]** | **Vizinho Mais Próximo** | 2x, 4x, 8x | Zoom In (Aumentar) |
| **[2]** | **Replicação de Pixel** | 2x, 4x, 8x | Zoom In (Aumentar) |

* Selecione o número do algoritmo e pressione **ENTER**.
* O sistema irá processar a imagem no FPGA e exibir o resultado no monitor VGA (se conectado).
* Um passo de zoom é aplicado a cada execução (ex: se o fator é 1x, um zoom in resultará em 2x; se for 2x, resultará em 4x, e assim por diante).

> [!NOTE]
> Os algoritmos de _zoom-out_ não estão disponíveis para seleção, visto que a terceira etapa do problema exige que a imagem não pode ser menor do que seu tamanho original. Entretanto, após aplicar o _zoom-in_ na imagem, é possível dar _zoom-out_ posteriormente para fazê-la voltar ao seu tamanho original.



<div align="center">
  <img src="https://i.postimg.cc/Ss5BwCMP/Zoom-Select2.png"><br>
  <strong>Menu de seleção de algoritimos de zoom.</strong><br><br>
</div>

Ao selecionar o algoritmo de zoom o usuário deve selecionar a área desejada para aplicar o zoom na janela.

* A área deve ser selecionada fazendo uso do mouse conectado ao HPS. Os pontos 1 e 2 são selecionados com o clique esquerdo do mouse.
* Coodernadas são visiveis no programa e um cursor é visivel na tela conectada ao VGA para facilitar a visualização da área que o usuário vai selecionar.
* Ao selecionar a área desejada, o usuário pode realizar o _zoom-in_ com a tecla de "+" do teclado e o _zoom-out_ com a tecla de "-".
* 

<div align="center">
  <img src="https://i.postimg.cc/vBjJpfnR/Zoom-Select22.png"><br>
  <strong>Menu de seleção de área da imagem.</strong><br><br>
</div>

<div align="center">
  <img src="https://i.postimg.cc/PxcgBYwg/Area-Select2.png"><br>
  <strong>Menu de seleção de área após definir o espaço desejado para aplicar o zoom.</strong><br><br>
</div>

<div align="center">
  <img src="https://i.postimg.cc/4dMDrVcR/Status2.png"><br>
  <strong>Janela de status.</strong><br><br>
</div>

---

### 2. Funções do código Assembly

Nessa seção as funções do código serão explicadas, cada uma tem um papel essencial para que o projeto demonstre resultados corretos.

#### 1. Lib (Inicialização).
Função de inicialização da biblioteca.

- Responsável por abrir o arquivo especial /dev/mem (usando a syscall 5 - open) para ter acesso direto à memória física do sistema.
- Mapeia (usando a syscall 192 - mmap) a região de memória do hardware Light-Weight (LW) Bridges do FPGA na memória virtual do processo.
- O endereço retornado pelo mmap é armazenado em FPGA_ADRS e será o endereço base usado para acessar todos os registradores do coprocessador.
> [!NOTE]
> Retorna 0 em caso de sucesso ou -1 em caso de erro (open ou mmap falharem).

#### 2. encerraLib (Encerramento).
Função de encerramento da biblioteca.

- Desfaz o mapeamento de memória (usando a syscall 91 - munmap), liberando o espaço de memória virtual que apontava para o FPGA.
- Fecha o descritor de arquivo de /dev/mem (usando a syscall 6 - close).
> [!NOTE]
> Retorna 0 em caso de sucesso ou -1 se o munmap falhar.

#### 3. write_pixel.
Escreve um valor de pixel na VRAM do coprocessador.

- **Recebe o endereço do pixel (r0) e o valor do pixel (cor, r1).**
- Verifica se o endereço (r0) é válido (menor que VRAM_MAX_ADDR).
- Monta a instrução e a escreve no registrador PIO_INSTRUCT. O Assembly indica que o endereço é deslocado em 3 bits e o valor do pixel é deslocado em 21 bits (além de um bit de controle em 20).
- Dispara a operação escrevendo 1 e depois 0 no registrador PIO_ENABLE.
- Entra em um loop de espera (WAIT_LOOP_WR), verificando o flag FLAG_DONE_MASK no registrador PIO_FLAGS até que a operação seja concluída ou o timeout (TIMEOUT_COUNT) expire.
- Após a conclusão, verifica se ocorreu um erro (FLAG_ERROR_MASK).
- Inclui um delay extra (EXTRA_DELAY_COUNT) para sincronizar o HPS (800MHz) e o FPGA (50MHz).
> [!NOTE]
> Retorna 0 (sucesso), -1 (endereço inválido), ou -3 (erro de hardware/timeout).

#### 4. read_pixel.
Lê o valor de um pixel da VRAM.

- **Recebe o endereço do pixel (r0) e um valor de controle (r1).**
- Verifica a validade do endereço (r0).
- Monta a instrução (opcode LOAD_OPCODE + endereço + valor de controle) e a envia para PIO_INSTRUCT.
- Dispara a operação via PIO_ENABLE.
- Entra em um loop de espera (WAIT_LOOP_RD) pelo flag FLAG_DONE_MASK.
- Se for concluída sem erro, lê o valor do pixel do registrador de saída (PIO_DATA_OUT) e o retorna em r0.
  
> [!NOTE]
> Retorna o valor do pixel (sucesso), -1 (endereço inválido), ou -3 (erro de hardware/timeout).

---

### 3. Implementação do zoom em área específica 

Essa seção vai se aprofundar sobre como os requisitos da etapa 3 do projeto foram elaborados e aplicados no programa.

#### Zoom em área específica sendo aplicado.
![Image](https://github.com/user-attachments/assets/fc3f2450-f38e-4489-a271-ad2dbbe65e94)

#### 3.1. Double Buffering no Hardware (FPGA)
O uso de dois buffers na VGA se apresentou essencial para que houvesse uma transição suave da imagem original para a imagem com zoom na janela. O **write_pixel** simula o recorte da porção ampliada sobre a imagem original, utilizando o Buffer B como a fonte do zoom após o software ler a área selecionada. Enquanto a variável **current_view** armazena o estado atual da tela (imagem original + pixels de zoom). 

Após realizar o _zoom-in_ necessário para expandir a área o usuário pode optar por voltar ao tamanho original onde Buffer B (área de zoom) e a copia/mescla de volta no Buffer A (exibição).

Além disso o uso de dois buffers na VGA possibilita as seguintes propriedades no programa:
* Ele resolve o problema de restaurar o fundo do cursor (Modo 0), pois ele contém os pixels corretos, independentemente de serem da imagem original ou da região ampliada.
* Ele garante que o recorte da porção ampliada seja aplicado sobre o estado atual da tela, mantendo a integridade da imagem enquanto o mouse é movido e o zoom é ajustado.
* Essa arquitetura distribui a carga: a FPGA faz o trabalho pesado de processamento de imagem em massa (zoom em toda a tela), e a CPU faz o trabalho de gerenciamento de interface e recorte seletivo (cursor e janela de zoom).

#### 3.2. Fluxo de Zoom na Janela com o Coprocessador (FPGA)

| Etapa | Ação | Responsável | Detalhes |
| :--- | :--- | :--- | :--- |
| *1. Carregamento Inicial* | A imagem original é carregada para a memória de vídeo da VGA (Buffer A) e exibida. | CPU (via send_to_fpga) e Coprocessador | O original_image_data no software espelha o Buffer A. |
| *2. Seleção de Janela* | O usuário define a área de interesse (P1 e P2). | CPU/Software | Define as coordenadas da *região de origem* na imagem original. |
| *3. Processamento (Zoom)* | A imagem inteira no Buffer A é ampliada pelo fator $Z$ e o resultado é escrito no *Buffer B* da VGA. | *Coprocessador/FPGA* | O Coprocessador aplica o algoritmo de zoom (Vizinho Próximo, Replicação) em toda a imagem *de uma só vez*. |
| *4. Recorte e Exibição* | A aplicação CPU calcula as coordenadas da *região ampliada de interesse* no Buffer B e as copia de volta para a área da janela no *Buffer A*. | CPU/Software  draw_zoomed_region) | O Buffer A agora contém a imagem original (fundo) com a região da janela sobreposta pela versão ampliada. |
| *5. Atualização da Tela* | O Buffer A é exibido. | Coprocessador/FPGA | O usuário vê o zoom na janela sem que o restante da tela tenha sido processado pelo CPU. |


#### 3.3. Cursor do Software

O conceito de Cursor de Software é mantido, e ele depende fundamentalmente do Buffer de Visualização Atual do Software (current_view) para a restauração de pixels, interagindo diretamente com o Buffer A da VGA (que está sendo exibido).

* Necessidade do Buffer de Software (current_view): Como o Coprocessador está ocupado realizando o zoom, e a VGA está exibindo o Buffer A, a CPU precisa de uma cópia dos pixels sendo exibidos para gerenciar o cursor.

Processo de desenho:
* Apagar: Antes de mover o cursor, a área antiga deve ser restaurada usando os dados do current_view e a função write_pixel (Modo 0).
* Desenhar: O novo cursor é desenhado com CURSOR_COLOR usando write_pixel (Modo 1).
* O "send_refresh()" garante que as pequenas alterações de pixel (o cursor) feitas pelo write_pixel sejam rapidamente refletidas no display.

> [!NOTE]
> Se o cursor fosse movido durante a execução do zoom pelo FPGA (Etapa 3), a utilização do current_view garantiria que, mesmo que o FPGA estivesse lendo ou escrevendo em grandes blocos, o cursor seria gerenciado localmente pelo CPU.

---

### 4. MakeFile

Essa seção vai ser dedicada em explicar o funcionamento do MakeFile do projeto e suas características.

#### 4.1. Função do MakeFile

O propósito principal deste *Makefile* é simplificar o processo de construção do projeto. Simplificando o processo de execução do projeto para um usúario ao fazer uso de um único comando (make <alvo>). Sendo assim, o *Makefile* fica responsável por:
- Compilar o código em Assembly (lib.s) e C (main.c).
- Ligar (linking) os ficheiros objeto para criar um executável.
- Executar o programa resultante.
- Limpar os ficheiros temporários e o executável.

#### 4.2. Alvos do Makefile

O *Makefile* do projeto define três alvos para um uso mais direto e explicado do programa.

**1. Help** (Alvo Padrão Informativo)
 - Ação: Imprime uma lista dos comandos (alvos) disponíveis (run e clean) e uma breve descrição do que fazem.

**Comando:** make help


**2. Run** (Compilação, Execução e Limpeza)
 - Ação: Este é o alvo principal para construir e testar o programa. Ele executa uma sequência de quatro passos:
 - Montagem (Assembly): O ficheiro lib.s é processado pelo montador (as) para criar o ficheiro objeto lib.o.
 - Compilação e Ligação (C): O compilador C (gcc) compila o main.c e, em seguida, liga-o ao lib.o para criar o executável final chamado exe. As flags -z noexecstack, -std=c99, e -lm são usadas para configurar a compilação (segurança, padrão C e biblioteca matemática, respetivamente).
 - Execução: O programa é executado com ./exe.
 - Limpeza (Parcial): O executável exe e o ficheiro objeto lib.o são removidos para limpar o ambiente de trabalho.

**Comando:** make run

> [!WARNING]
> O usuário apenas conseguira executar corretamente o "make run" apeans após entrar no "Super Usuário" do sistema. Caso contrario, um erro de "*segment faul*" vai ser exibido.


**3. Clean** (Limpeza Completa)
 - Ação: Remove todos os ficheiros compilados que foram gerados.
 - Isto inclui o executável (exe) e todos os ficheiros objeto (*.o).

**Comando:** make clean


### 5. Erros Comuns e Mensagens de Alerta

O sistema foi desenhado para reportar problemas de forma clara:

| Categoria | Mensagem de Erro | Ocorrência Comum | Ação Recomendada |
| :--- | :--- | :--- | :--- |
| **Arquivos** | `❌ Erro ao abrir 'nome_do_arquivo'` | O arquivo BMP selecionado não está na pasta correta. | Verifique se a imagem está no mesmo diretório do programa e tente novamente. |
| | `❌ Arquivo não é BMP válido` | O arquivo selecionado não segue o formato BMP ou está corrompido. | Use apenas arquivos BMP válidos. |
| | `❌ Dimensão incorreta: DxH (esperado 320x240)` | A imagem não tem a resolução de **320x240 pixels** esperada. | Utilize apenas imagens BMP com a dimensão correta. |
| | `❌ Formato X bits não suportado` | O formato de cor da imagem é diferente do suportado. | Utilize imagens BMP com 8 bits por pixel. |
| **Sistema** | `❌ Erro ao enviar imagem para FPGA` | Falha de comunicação ao transferir os dados da imagem para o hardware. | Tente a operação novamente e, se o problema persistir, verifique a conexão do hardware. |
| | `❌ Hardware reportou erro!` | O coprocessador FPGA indicou uma falha interna. | Tente a operação novamente e/ou utilize a opção **[3] Reset do Sistema**. |
| | `❌ Operação não concluiu no tempo esperado TIMEOUT!` | O algoritmo de zoom não terminou no tempo limite (5 segundos). | Aumentar o tempo de espera pode ser necessário para operações complexas. |
| **Zoom** | `⚠️ Zoom máximo atingido (8x)` | Tentativa de aplicar zoom in (aumentar) após atingir o limite de 8x. | O zoom in só pode ser aplicado até 8x (2x -> 4x -> 8x). |
| | `⚠️ Zoom mínimo atingido (0.125x)` | Tentativa de aplicar zoom out (diminuir) após atingir o limite de 0.125x. | O zoom out só pode ser aplicado até 0.125x (0.5x -> 0.25x -> 0.125x). |

---
