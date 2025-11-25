#ifndef LIB_H
#define LIB_H

#include <stdint.h>

// ====================================================================
// BIBLIOTECA DO COPROCESSADOR DE ZOOM
// Interface C para controlar a FPGA via PIOs
// ====================================================================

// Inicialização e Finalização
void Lib(void);                    // Inicializa mmap para acessar os PIOs
void encerrarLib(void);            // Finaliza e libera o mmap

// Operações de Leitura/Escrita
int write_pixel(int addr, int value);  // Escreve um pixel na RAM
                                       // addr: 0-76799 (endereço do pixel)
                                       // value: 0-255 (valor do pixel em escala de cinza)
                                       // Retorna: 0 (sucesso), -1 (erro endereço), -3 (erro HW)

int read_pixel(int addr, int sel_mem); // Lê um pixel da RAM
                                       // addr: 0-76799 (endereço do pixel)
                                       // sel_mem: 0 (memória A), 1 (memória B)
                                       // Retorna: valor do pixel (0-255)

void send_refresh(void);           // Envia comando REFRESH para atualizar a VGA

// Operações de Zoom - Zoom IN (2x, 4x, 8x)
void Vizinho_Prox(void);           // Zoom IN via Vizinho Mais Próximo
void Replicacao(void);             // Zoom IN via Replicação de Pixel

// Operações de Zoom - Zoom OUT (0.5x, 0.25x, 0.125x)
void Decimacao(void);              // Zoom OUT via Vizinho Mais Próximo
void Media(void);                  // Zoom OUT via Média de Blocos

// Controle Geral
void Reset(void);                  // Reseta o coprocessador

// Leitura de Flags
int Flag_Done(void);               // Retorna 1 se operação concluída, 0 caso contrário
int Flag_Error(void);              // Retorna 1 se houve erro, 0 caso contrário
int Flag_Max(void);                // Retorna 1 se atingiu zoom máximo (8x), 0 caso contrário
int Flag_Min(void);                // Retorna 1 se atingiu zoom mínimo (0.125x), 0 caso contrário

// ====================================================================
// Constantes e Definições
// ====================================================================

#define VRAM_MAX_ADDR      76800   // Máximo endereço de memória (320x240)
#define IMG_WIDTH          320
#define IMG_HEIGHT         240

// Opcodes das instruções (não precisa usar diretamente)
#define OPCODE_REFRESH     0x00
#define OPCODE_LOAD        0x01
#define OPCODE_STORE       0x02
#define OPCODE_VIZINHO_IN  0x03
#define OPCODE_REPLIC      0x04
#define OPCODE_ZOOM_OUT    0x05
#define OPCODE_MEDIA       0x06
#define OPCODE_RESET       0x07

// Máscaras de Flags
#define FLAG_DONE_MASK     0x01
#define FLAG_ERROR_MASK    0x02
#define FLAG_ZOOM_MIN_MASK 0x04
#define FLAG_ZOOM_MAX_MASK 0x08

 
// Exemplo de uso da biblioteca

/*
    // Inicializa a biblioteca
    Lib();

    // Escreve um pixel
    write_pixel(38400, 128);  // Pixel no centro com valor 128

    // Atualiza a VGA
    send_refresh();

    // Faz zoom in
    Vizinho_Prox();
    
    // Aguarda conclusão
    while (!Flag_Done());

    // Verifica se chegou ao zoom máximo
    if (Flag_Max()) {
        printf("Zoom máximo atingido!\n");
    }

    // Finaliza a biblioteca
    encerrarLib();
*/

#endif // LIB_H
