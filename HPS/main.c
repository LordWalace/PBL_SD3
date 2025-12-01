//Ta com uns erros bestas na interface caso tenha tempo arruma depois. 

#define _DEFAULT_SOURCE 

#include "LibCoprocessador.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>
#include <linux/input.h>
#include <math.h>
#include <sys/select.h> 

// --- Definições ---
#define MAX_IMAGES 10
#define MAX_FILENAME 100
#define MOUSE_DEV "/dev/input/mice"

#ifndef IMG_WIDTH
#define IMG_WIDTH 320
#endif
#ifndef IMG_HEIGHT
#define IMG_HEIGHT 240
#endif

// Cor do Cursor: 0x00 = PRETO (Contraste máximo)
#define CURSOR_COLOR 0x00
#define CURSOR_SIZE 6 

// --- Estruturas ---
typedef struct {
    int x, y;
    int left_btn, right_btn;
} MouseState;

#pragma pack(push, 1)
typedef struct {
    uint16_t type;
    uint32_t size;
    uint16_t reserved1;
    uint16_t reserved2;
    uint32_t offset;
} BMPHeader;

typedef struct {
    uint32_t size;
    int32_t  width;
    int32_t  height;
    uint16_t planes;
    uint16_t bits;
    uint32_t compression;
    uint32_t imagesize;
    int32_t  xresolution;
    int32_t  yresolution;
    uint32_t ncolours;
    uint32_t importantcolours;
} BMPInfoHeader;
#pragma pack(pop)

// --- Funções de Sistema ---

void clear_screen() {
    printf("\033[2J\033[H");
}

void set_conio_terminal_mode() {
    struct termios new_termios;
    tcgetattr(0, &new_termios);
    new_termios.c_lflag &= ~ICANON;
    new_termios.c_lflag &= ~ECHO;
    new_termios.c_cc[VMIN] = 1;
    new_termios.c_cc[VTIME] = 0;
    tcsetattr(0, TCSANOW, &new_termios);
}

void set_nonblocking_mode() {
    struct termios new_termios;
    tcgetattr(0, &new_termios);
    new_termios.c_lflag &= ~ICANON;
    new_termios.c_lflag &= ~ECHO;
    new_termios.c_cc[VMIN] = 0;
    new_termios.c_cc[VTIME] = 0;
    tcsetattr(0, TCSANOW, &new_termios);
}

void reset_terminal_mode() {
    struct termios new_termios;
    tcgetattr(0, &new_termios);
    new_termios.c_lflag |= ICANON;
    new_termios.c_lflag |= ECHO;
    tcsetattr(0, TCSANOW, &new_termios);
}

int getch() {
    int r;
    unsigned char c;
    if ((r = read(0, &c, sizeof(c))) < 0) return r;
    else return c;
}

int kbhit() {
    struct timeval tv = { 0L, 0L };
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(0, &fds);
    return select(1, &fds, NULL, NULL, &tv) > 0;
}

int get_menu_choice() {
    set_conio_terminal_mode();
    char c = getch();
    reset_terminal_mode();
    return c - '0';
}

// --- Mouse ---

int init_mouse(int *fd) {
    *fd = open(MOUSE_DEV, O_RDONLY | O_NONBLOCK);
    if (*fd == -1) {
        printf("❌ Erro ao abrir mouse (%s). Use SUDO.\n", MOUSE_DEV);
        return -1;
    }
    return 0;
}

void update_mouse(int fd, MouseState *ms) {
    unsigned char data[3];
    int bytes = read(fd, data, sizeof(data));
    
    if (bytes > 0) {
        int dx = (signed char)data[1];
        int dy = (signed char)data[2]; 
        
        ms->left_btn = data[0] & 0x1;
        ms->right_btn = data[0] & 0x2;
        
        ms->x += dx;
        ms->y -= dy; 
        
        if (ms->x < 0) ms->x = 0;
        if (ms->x >= IMG_WIDTH) ms->x = IMG_WIDTH - 1;
        if (ms->y < 0) ms->y = 0;
        if (ms->y >= IMG_HEIGHT) ms->y = IMG_HEIGHT - 1;
    }
}

// --- Gráficos ---

uint8_t rgb_to_gray(uint8_t r, uint8_t g, uint8_t b) {
    return (uint8_t)((299 * r + 587 * g + 114 * b) / 1000);
}

int load_bmp(const char *filename, uint8_t *image_data) {
    FILE *file;
    BMPHeader header;
    BMPInfoHeader infoHeader;
    
    file = fopen(filename, "rb");
    if (!file) { printf("❌ Erro: %s\n", filename); return -1; }
    
    fread(&header, sizeof(BMPHeader), 1, file);
    if (header.type != 0x4D42) { fclose(file); return -1; }
    fread(&infoHeader, sizeof(BMPInfoHeader), 1, file);
    
    int height = abs(infoHeader.height);
    if (infoHeader.width != IMG_WIDTH || height != IMG_HEIGHT) {
        printf("❌ Dimensão incorreta: %dx%d\n", infoHeader.width, height);
        fclose(file); return -1;
    }
    
    fseek(file, header.offset, SEEK_SET);
    
    int bytes_per_pixel = infoHeader.bits / 8;
    int line_width_bytes = infoHeader.width * bytes_per_pixel;
    int padding = (4 - (line_width_bytes % 4)) % 4;
    
    uint8_t *row_buffer = (uint8_t*)malloc(line_width_bytes);
    if (!row_buffer) { fclose(file); return -1; }
    
    printf("Carregando");
    for (int y = 0; y < IMG_HEIGHT; y++) {
        fread(row_buffer, 1, line_width_bytes, file);
        for (int x = 0; x < IMG_WIDTH; x++) {
            uint8_t gray;
            int idx = x * bytes_per_pixel;
            if (infoHeader.bits == 32) gray = rgb_to_gray(row_buffer[idx+2], row_buffer[idx+1], row_buffer[idx+0]);
            else if (infoHeader.bits == 24) gray = rgb_to_gray(row_buffer[idx+2], row_buffer[idx+1], row_buffer[idx+0]);
            else if (infoHeader.bits == 8) gray = row_buffer[x];
            else { free(row_buffer); fclose(file); return -1; }
            
            int final_addr = (IMG_HEIGHT - 1 - y) * IMG_WIDTH + x;
            image_data[final_addr] = gray;
        }
        if (padding > 0) fseek(file, padding, SEEK_CUR);
        if (y % 60 == 0) { printf("."); fflush(stdout); }
    }
    free(row_buffer);
    printf(" OK!\n");
    fclose(file);
    return 0;
}

int send_to_fpga(uint8_t *image_data) {
    printf("Enviando para FPGA");
    for (int i = 0; i < IMG_WIDTH * IMG_HEIGHT; i++) {
        write_pixel(i, image_data[i]);
        if (i % ((IMG_WIDTH * IMG_HEIGHT) / 10) == 0) { printf("."); fflush(stdout); }
    }
    printf(" OK!\n");
    send_refresh();
    usleep(100000);
    return 0;
}

// --- FUNÇÕES DE CURSOR SOFTWARE ---

void draw_software_cursor(int x, int y, uint8_t *current_view, int mode) {
    for (int dy = 0; dy < CURSOR_SIZE; dy++) {
        for (int dx = 0; dx < CURSOR_SIZE; dx++) {
            int px = x + dx;
            int py = y + dy;
            if (px >= 0 && px < IMG_WIDTH && py >= 0 && py < IMG_HEIGHT) {
                int addr = py * IMG_WIDTH + px;
                if (mode == 1) write_pixel(addr, CURSOR_COLOR);
                else write_pixel(addr, current_view[addr]);
            }
        }
    }
}

// --- Zoom de Janela (Atualiza buffer de fundo) ---
void draw_zoomed_region(uint8_t *current_view, uint8_t *original_data, int x1, int y1, int x2, int y2, int zoom_factor, int algorithm) {
    int x_start = (x1 < x2) ? x1 : x2;
    int x_end = (x1 < x2) ? x2 : x1;
    int y_start = (y1 < y2) ? y1 : y2;
    int y_end = (y1 < y2) ? y2 : y1;
    
    int width = x_end - x_start;
    int height = y_end - y_start;
    
    if (width <= 1 || height <= 1) return;

    if (zoom_factor == 1) {
        for (int y = y_start; y <= y_end; y++) {
            for (int x = x_start; x <= x_end; x++) {
                if (x >= 0 && x < IMG_WIDTH && y >= 0 && y < IMG_HEIGHT) {
                    int addr = y * IMG_WIDTH + x;
                    uint8_t val = original_data[addr];
                    write_pixel(addr, val);
                    current_view[addr] = val;
                }
            }
        }
        return; 
    }

    int src_width = width / zoom_factor;
    int src_height = height / zoom_factor;
    int src_x_start = x_start + (width - src_width) / 2;
    int src_y_start = y_start + (height - src_height) / 2;

    for (int dy = 0; dy < height; dy++) {
        for (int dx = 0; dx < width; dx++) {
            int out_x = x_start + dx;
            int out_y = y_start + dy;

            if (out_x >= 0 && out_x < IMG_WIDTH && out_y >= 0 && out_y < IMG_HEIGHT) {
                int addr = out_y * IMG_WIDTH + out_x;
                
                if (dx == 0 || dx == width - 1 || dy == 0 || dy == height - 1) {
                    write_pixel(addr, 255); // Borda Branca
                    current_view[addr] = 255;
                    continue;
                }

                int sx = src_x_start + (dx / zoom_factor);
                int sy = src_y_start + (dy / zoom_factor);
                
                uint8_t pixel_val = 0;
                if (sx >= 0 && sx < IMG_WIDTH && sy >= 0 && sy < IMG_HEIGHT) {
                    if (algorithm == 1) {
                        // Vizinho_Prox
                        pixel_val = original_data[sy * IMG_WIDTH + sx]; 
                    } else {
                        // Replicacao (Pixel Replication Simples)
                        pixel_val = original_data[sy * IMG_WIDTH + sx];
                    }
                }
                write_pixel(addr, pixel_val);
                current_view[addr] = pixel_val;
            }
        }
    }
}

// --- MODO INTERATIVO ---
void interactive_window_zoom(uint8_t *original_image_data) {
    uint8_t *current_view = (uint8_t*)malloc(IMG_WIDTH * IMG_HEIGHT);
    if (!current_view) return;
    
    memcpy(current_view, original_image_data, IMG_WIDTH * IMG_HEIGHT);

    clear_screen();
    printf("╔════════════════════════════════════════════╗\n");
    printf("║      MODO JANELA - ESCOLHA O ALGORITMO     ║\n");
    printf("╚════════════════════════════════════════════╝\n");
    printf("  [1] Vizinho_Prox\n");
    printf("  [2] Replicacao\n\nEscolha: ");
    
    int algorithm = get_menu_choice();
    printf("%d\n", algorithm);
    if (algorithm != 1 && algorithm != 2) algorithm = 1;

    int mouse_fd;
    if (init_mouse(&mouse_fd) != 0) { free(current_view); return; }
    
    set_nonblocking_mode();
    
    MouseState ms = {IMG_WIDTH/2, IMG_HEIGHT/2, 0, 0};
    int old_x = ms.x, old_y = ms.y;
    int state = 0; 
    int p1_x = 0, p1_y = 0, p2_x = 0, p2_y = 0;
    int zoom_level = 1;
    int running = 1;
    int btn_released = 1;

    clear_screen();
    printf("=== MODO JANELA (%s) ===\n", (algorithm==1) ? "Vizinho_Prox" : "Replicacao");
    printf("1. Selecione Ponto 1.\n");
    printf("2. Selecione Ponto 2.\n");
    printf("3. Use +/- para Zoom.\n");

    // Desenha cursor inicial
    draw_software_cursor(ms.x, ms.y, current_view, 1);
    send_refresh();

    while (running) {
        // Apaga cursor antigo
        draw_software_cursor(old_x, old_y, current_view, 0);
        // Atualiza Posição
        update_mouse(mouse_fd, &ms);
        // Desenha novo cursor
        draw_software_cursor(ms.x, ms.y, current_view, 1);
        // Atualiza Tela
        send_refresh();

        old_x = ms.x;
        old_y = ms.y;
        
        // --- INTERFACE DE TEXTO SOLICITADA ---
        printf("\r\033[K"); // Limpa a linha atual
        if (state == 0) {
            printf("Mouse: (%03d, %03d) | Clique para definir Ponto 1", ms.x, ms.y);
        } else if (state == 1) {
            printf("Ponto 1: (%03d, %03d) | Mouse: (%03d, %03d) | Clique para definir Ponto 2", p1_x, p1_y, ms.x, ms.y);
        } else if (state == 2) {
            printf("Ponto 1: (%03d, %03d) | Ponto 2: (%03d, %03d) | Zoom: %dx | Digite + ou -", p1_x, p1_y, p2_x, p2_y, zoom_level);
        }
        fflush(stdout);

        if (ms.left_btn && btn_released) {
            btn_released = 0;
            // Apaga cursor para não desenhar na imagem
            draw_software_cursor(ms.x, ms.y, current_view, 0);

            if (state == 0) {
                p1_x = ms.x; p1_y = ms.y; 
                state = 1;
            } else if (state == 1) {
                p2_x = ms.x; p2_y = ms.y; 
                state = 2; zoom_level = 1;
                draw_zoomed_region(current_view, original_image_data, p1_x, p1_y, p2_x, p2_y, 1, algorithm);
            } else if (state == 2) {
                draw_zoomed_region(current_view, original_image_data, p1_x, p1_y, p2_x, p2_y, 1, algorithm); // Reseta região
                state = 0;
            }
            draw_software_cursor(ms.x, ms.y, current_view, 1); // Redesenha
        }
        if (!ms.left_btn) btn_released = 1;

        if (kbhit()) {
            char key = getch();
            if (key == 27) running = 0; 
            else if (state == 2) {
                int old_zoom = zoom_level;
                if (key == '+' || key == '=') { if (zoom_level < 8) zoom_level *= 2; } 
                else if (key == '-' || key == '_') { if (zoom_level > 1) zoom_level /= 2; }
                
                if (old_zoom != zoom_level) {
                    draw_software_cursor(ms.x, ms.y, current_view, 0);
                    draw_zoomed_region(current_view, original_image_data, p1_x, p1_y, p2_x, p2_y, zoom_level, algorithm);
                    draw_software_cursor(ms.x, ms.y, current_view, 1);
                    send_refresh();
                }
            }
        }
        usleep(15000); 
    }
    
    send_to_fpga(original_image_data);
    free(current_view);
    reset_terminal_mode();
    close(mouse_fd);
}

// --- Menus ---

void print_main_menu() {
    clear_screen();
    printf("╔════════════════════════════════════════════╗\n");
    printf("║   SISTEMA DE ZOOM - COPROCESSADOR FPGA    ║\n");
    printf("╚════════════════════════════════════════════╝\n\n");
    printf("  [1] Carregar Imagem\n");
    printf("  [2] Zoom Janela (Mouse/Software)\n");
    printf("  [3] Reset\n");
    printf("  [4] Status\n");
    printf("  [0] Sair\n\n");
    printf("Escolha: ");
}

int select_image_menu(char *filename) {
    char images[MAX_IMAGES][MAX_FILENAME] = {
        "Xadrez.bmp", "Hornet.bmp", "imagem2.bmp", "imagem3.bmp"
    };
    clear_screen();
    printf("Selecione:\n");
    for(int i=0; i<4; i++) printf("  [%d] %s\n", i+1, images[i]);
    printf("  [0] Voltar\n\n> ");
    int c = get_menu_choice();
    printf("%d\n", c);
    if(c < 1 || c > 4) return -1;
    strcpy(filename, images[c-1]);
    return 0;
}

void show_status() {
    clear_screen();
    printf("STATUS HARDWARE: Done=%d | Err=%d | Max=%d | Min=%d\n", 
            Flag_Done(), Flag_Error(), Flag_Max(), Flag_Min());
    printf("\nPressione qualquer tecla para voltar...");
    set_conio_terminal_mode(); getch(); reset_terminal_mode();
}

int main() {
    uint8_t *image_data = (uint8_t*)malloc(IMG_WIDTH * IMG_HEIGHT);
    if (!image_data) { printf("Erro fatal de memoria\n"); return 1; }
    
    int system_initialized = 0;
    int image_loaded = 0;

    while (1) {
        print_main_menu();
        int choice = get_menu_choice();
        printf("%d\n", choice);

        switch (choice) {
            case 1: {
                char fname[MAX_FILENAME];
                if (select_image_menu(fname) == 0) {
                    if (!system_initialized) { Lib(); Reset(); system_initialized=1; }
                    if (load_bmp(fname, image_data) == 0) {
                        if (send_to_fpga(image_data) == 0) {
                            image_loaded = 1;
                            printf("\nImagem carregada!");
                            sleep(1);
                        }
                    }
                }
                break;
            }
            case 2:
                if (image_loaded) interactive_window_zoom(image_data);
                else { printf("\nCarregue imagem antes!"); sleep(1); }
                break;
            case 3:
                Reset(); image_loaded = 0; printf("\nResetado!"); sleep(1); break;
            case 4:
                show_status(); break;
            case 0:
                free(image_data); printf("Saindo...\n"); return 0;
            default:
                printf("\nOpção inválida"); sleep(1);
        }
    }
}