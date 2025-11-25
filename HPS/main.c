// Finalizado
#include <ncurses.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include "LibCoprocessador.h"

// stb_image
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize.h"

#define IMG_PATH "./imagens/"
#define MAX_FILENAME 100
#define MAX_IMAGES 50

// Estrutura do cabeçalho BMP
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

// Zoom janela
typedef struct {
    int x1, y1, x2, y2;
    int definida;
} JanelaZoom;

uint8_t image_data[IMG_WIDTH * IMG_HEIGHT];
JanelaZoom janela = {0, 0, 0, 0, 0};
char imagens_bmp[MAX_IMAGES][MAX_FILENAME];
int num_imagens = 0;
char current_image[MAX_FILENAME] = "";

int sistema_inicializado = 0;
int imagem_carregada = 0;
int modo_janela = 0; // 0 = imagem inteira, 1 = janela

uint8_t rgb_to_gray(uint8_t r, uint8_t g, uint8_t b) {
    return (uint8_t)((299*r + 587*g + 114*b)/1000);
}

// Salva BMP 8-bit grayscale usando cabeçalho
int salvar_bmp_gray(const char* out_bmp, uint8_t* data, int width, int height) {
    FILE* f = fopen(out_bmp, "wb");
    if (!f) return -1;
    BMPHeader head;
    BMPInfoHeader info;
    head.type = 0x4D42; //"BM"
    head.size = 54 + 1024 + width*height;
    head.reserved1 = 0;
    head.reserved2 = 0;
    head.offset = 54 + 1024;
    info.size = 40;
    info.width = width;
    info.height = height;
    info.planes = 1;
    info.bits = 8;
    info.compression = 0;
    info.imagesize = width*height;
    info.xresolution = 2835;
    info.yresolution = 2835;
    info.ncolours = 256;
    info.importantcolours = 256;
    fwrite(&head, sizeof(head), 1, f);
    fwrite(&info, sizeof(info), 1, f);

    // Paleta 8-bit gray
    unsigned char pal[1024];
    for(int i=0; i<256; i++) {
        pal[i*4]=pal[i*4+1]=pal[i*4+2]=i; pal[i*4+3]=0;
    }
    fwrite(pal,1,1024,f);

    for(int y=height-1;y>=0;y--) // BMP bottom-up
        fwrite(&data[y*width], 1, width, f);
    fclose(f);
    return 0;
}

// Converte JPG/PNG para BMP 320x240 grayscale usando stb_image
int convert_to_bmp_320x240_gray(const char* input_path, const char* output_bmp) {
    int w,h,comp;
    unsigned char* img = stbi_load(input_path, &w, &h, &comp, 3);
    if(!img) return -1;
    unsigned char* resized = malloc(320*240*3);
    stbir_resize_uint8(img, w, h, 0, resized, 320, 240, 0, 3);
    uint8_t* gray = malloc(320*240);
    for(int i=0;i<320*240;i++) {
        uint8_t r = resized[i*3+0], g = resized[i*3+1], b = resized[i*3+2];
        gray[i] = rgb_to_gray(r,g,b);
    }
    free(resized); stbi_image_free(img);
    int ok = salvar_bmp_gray(output_bmp, gray, 320, 240);
    free(gray);
    return ok;
}

// Lista BMPs válidos na pasta
void listar_bmps() {
    DIR *d; struct dirent *dir;
    num_imagens = 0;
    d = opendir(IMG_PATH);
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            size_t len=strlen(dir->d_name);
            if (len>4 && strcasecmp(dir->d_name+len-4,".bmp")==0) {
                char fullpath[MAX_FILENAME*2];
                snprintf(fullpath,sizeof(fullpath),"%s%s",IMG_PATH,dir->d_name);
                FILE* f = fopen(fullpath,"rb");
                if(f) {
                    BMPHeader head; fread(&head,sizeof(head),1,f);
                    if(head.type==0x4D42) {
                        strncpy(imagens_bmp[num_imagens],dir->d_name,MAX_FILENAME-1);
                        imagens_bmp[num_imagens][MAX_FILENAME-1]=0;
                        num_imagens++; if(num_imagens>=MAX_IMAGES) break;
                    }
                    fclose(f);
                }
            }
        }
        closedir(d);
    }
}

// Carrega BMP da pasta usando cabeçalho
int load_bmp(const char* filename, uint8_t* image_data) {
    FILE *file; BMPHeader head; BMPInfoHeader info;
    file = fopen(filename, "rb");
    if (!file) return -1;
    fread(&head, sizeof(BMPHeader), 1, file);
    if(head.type != 0x4D42) { fclose(file); return -2; }
    fread(&info, sizeof(BMPInfoHeader), 1, file);
    if (info.width != IMG_WIDTH || abs(info.height) != IMG_HEIGHT) {
        fclose(file); return -3;
    }
    fseek(file, head.offset, SEEK_SET);
    int bytes_per_pixel = info.bits / 8;
    int row_size = ((info.width * bytes_per_pixel + 3) / 4) * 4;
    uint8_t *row_data = (uint8_t*)malloc(row_size);
    for (int y = 0; y < IMG_HEIGHT; y++) {
        fread(row_data, 1, row_size, file);
        for (int x = 0; x < IMG_WIDTH; x++) {
            uint8_t gray;
            if (info.bits == 32) {
                uint8_t b = row_data[x*4+0];
                uint8_t g = row_data[x*4+1];
                uint8_t r = row_data[x*4+2];
                gray = rgb_to_gray(r,g,b);
            } else if (info.bits == 24) {
                uint8_t b = row_data[x*3+0];
                uint8_t g = row_data[x*3+1];
                uint8_t r = row_data[x*3+2];
                gray = rgb_to_gray(r,g,b);
            } else if (info.bits == 8) {
                gray = row_data[x];
            } else {
                free(row_data); fclose(file); return -4;
            }
            int addr = (IMG_HEIGHT-1-y)*IMG_WIDTH + x;
            image_data[addr] = gray;
        }
    }
    free(row_data); fclose(file);
    return 0;
}

int envia_fpga() {
    for (int i = 0; i < IMG_WIDTH * IMG_HEIGHT; i++) {
        if (write_pixel(i, image_data[i]) < 0) return -1;
    }
    send_refresh();
    usleep(110000);
    return 0;
}

void zoom_in() {
    Vizinho_Prox();
    while(!Flag_Done()) usleep(1000);
    send_refresh();
    mvprintw(25, 0, "Zoom IN aplicado.");
}

void zoom_out() {
    Decimacao();
    while(!Flag_Done()) usleep(1000);
    send_refresh();
    mvprintw(25, 0, "Zoom OUT aplicado.");
}

void desenha_interface() {
    clear();
    mvprintw(0,0,"");
    mvprintw(1,0,"      SISTEMA DE ZOOM - COPROCESSADOR FPGA - MI Sistemas Digitais      ");
    mvprintw(2,0,"");
    mvprintw(4,0,"[I] Escolher imagem  [A] Adicionar nova imagem  [Q] Sair");
    mvprintw(5,0,"[+] Zoom in  [-] Zoom out  [W] Modo imagem/janela  [R] Reset");
    mvprintw(6,0,"Clique ESQ/Dir define janela de zoom.  ");
    mvprintw(8,0,"Imagem: %s", imagem_carregada ? current_image : "[Nenhuma]");
    mvprintw(9,0,"Modo: %s", modo_janela ? "JANELA" : "IMAGEM COMPLETA");
    if(janela.definida) mvprintw(10,0,"Janela: (%d,%d) a (%d,%d)", janela.x1,janela.y1,janela.x2,janela.y2);
    else mvprintw(10,0,"Janela: [Não definida]");
    mvprintw(12,0,"─────────────────────────────────────────────────────────────");
}

// Menu para escolher imagem por número
void menu_escolher_imagem() {
    listar_bmps();
    int sel = 0;
    while(1) {
        clear();
        mvprintw(2,0,"ESCOLHA UMA IMAGEM BMP:");
        for(int i=0;i<num_imagens;i++)
            mvprintw(4+i,0,"[%d] %s %s", i+1, imagens_bmp[i], (strcmp(current_image,imagens_bmp[i])==0?"(Atual)":""));
        mvprintw(6+num_imagens,0,"[0] Cancelar");
        mvprintw(7+num_imagens,0,"Número: ");
        refresh(); echo(); curs_set(1);
        char buf[10]={0}; getnstr(buf,9);
        noecho(); curs_set(0); sel=atoi(buf);
        if(sel==0) break;
        if(sel>0 && sel<=num_imagens) {
            char fullpath[MAX_FILENAME*2];
            snprintf(fullpath,sizeof(fullpath),"%s%s",IMG_PATH,imagens_bmp[sel-1]);
            int ret=load_bmp(fullpath,image_data);
            if(ret==0 && envia_fpga()==0){
                strcpy(current_image,imagens_bmp[sel-1]);
                imagem_carregada=1;
                desenha_interface();
                mvprintw(20,0,"✓ '%s' carregada com sucesso.",imagens_bmp[sel-1]);
                getch(); break;
            }else{
                desenha_interface();
                mvprintw(20,0,"Erro ao carregar/enviar '%s'!",imagens_bmp[sel-1]);
                getch(); break;
            }
        } else {
            mvprintw(10+num_imagens,0,"Opção inválida!");
            getch();
        }
    }
}

// Menu para adicionar nova imagem (converter JPG/PNG)
void menu_adicionar_nova_imagem() {
    listar_bmps();
    DIR *d = opendir(IMG_PATH);
    struct dirent *dir;
    char novas[MAX_IMAGES][MAX_FILENAME];
    int qnt_novas=0;
    if(d){
        while((dir=readdir(d))!=NULL){
            size_t len=strlen(dir->d_name);
            if(len>4 &&
               (strcasecmp(dir->d_name+len-4,".jpg")==0 ||
                strcasecmp(dir->d_name+len-4,".png")==0)){
                char bmp_equiv[MAX_FILENAME];
                strncpy(bmp_equiv,dir->d_name,len-4);
                bmp_equiv[len-4]=0;
                strcat(bmp_equiv,".bmp");
                int found=0;
                for(int i=0;i<num_imagens;i++)
                    if(strcasecmp(imagens_bmp[i],bmp_equiv)==0) found=1;
                if(!found){
                    strncpy(novas[qnt_novas],dir->d_name,MAX_FILENAME-1);
                    novas[qnt_novas][MAX_FILENAME-1]=0;
                    qnt_novas++; if(qnt_novas>=MAX_IMAGES) break;
                }
            }
        } closedir(d);
    }
    if(qnt_novas==0){
        mvprintw(5,0,"Não há novas imagens JPG/PNG não convertidas na pasta.");
        mvprintw(6,0,"Pressione qualquer tecla..."); getch(); return;
    }
    mvprintw(2,0,"NOVAS IMAGENS ENCONTRADAS PARA CONVERTER:");
    for(int i=0;i<qnt_novas;i++)
        mvprintw(4+i,0,"[%d] %s",i+1,novas[i]);
    mvprintw(6+qnt_novas,0,"[0] Cancelar");
    mvprintw(7+qnt_novas,0,"Escolha: "); refresh(); echo(); curs_set(1);
    char buf[10]; getnstr(buf,9); noecho(); curs_set(0);
    int sel=atoi(buf);
    if(sel<1||sel>qnt_novas) return;
    char bmp_final[MAX_FILENAME*2], input_path[MAX_FILENAME*2], output_path[MAX_FILENAME*2];
    strncpy(bmp_final,novas[sel-1],MAX_FILENAME-1); bmp_final[MAX_FILENAME-1]=0;
    size_t l=strlen(bmp_final); bmp_final[l-3]='b'; bmp_final[l-2]='m'; bmp_final[l-1]='p'; bmp_final[l]='\0';
    snprintf(input_path,sizeof(input_path),"%s%s",IMG_PATH,novas[sel-1]);
    snprintf(output_path,sizeof(output_path),"%s%s",IMG_PATH,bmp_final);
    int ret=convert_to_bmp_320x240_gray(input_path,output_path);
    if(ret==0){
        mvprintw(15,0,"Imagem '%s' convertida e salva como '%s'.",novas[sel-1],bmp_final);
    }else{
        mvprintw(15,0,"Erro ao converter '%s'. Formato não suportado?",novas[sel-1]);
    }
    listar_bmps(); getch();
}

int main() {
    int ch; MEVENT event;
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);
    mousemask(BUTTON1_CLICKED | BUTTON3_CLICKED, NULL);
    curs_set(0);
    Lib(); Reset(); sistema_inicializado=1;
    desenha_interface();
    while(1){
        ch=getch();
        if(ch==KEY_MOUSE){
            if(getmouse(&event)==OK){
                if(event.bstate & BUTTON1_CLICKED){
                    janela.x1=event.x; janela.y1=event.y; janela.definida=(janela.x2!=0||janela.y2!=0);
                }
                if(event.bstate & BUTTON3_CLICKED){
                    janela.x2=event.x; janela.y2=event.y; janela.definida=(janela.x1!=0||janela.y1!=0);
                }
                desenha_interface();
            }
        } else if(ch=='i'||ch=='I') {
            menu_escolher_imagem(); desenha_interface();
        } else if(ch=='a'||ch=='A') {
            menu_adicionar_nova_imagem(); desenha_interface();
        } else if(ch=='+'||ch=='=') {
            if(imagem_carregada) zoom_in();
        } else if(ch=='-'||ch=='_') {
            if(imagem_carregada) zoom_out();
        } else if(ch=='w'||ch=='W') {
            modo_janela=!modo_janela; desenha_interface();
        } else if(ch=='r'||ch=='R') {
            Reset(); usleep(120000); imagem_carregada=0;
            janela.definida=0; janela.x1=janela.y1=janela.x2=janela.y2=0;
            current_image[0]='\0'; desenha_interface();
        } else if(ch=='q'||ch=='Q') {
            break;
        }
        refresh(); usleep(35000);
    }
    if(sistema_inicializado) encerrarLib();
    endwin();
    return 0;
}
