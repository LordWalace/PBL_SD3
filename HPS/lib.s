@ Finalizado
.equ PIO_INSTRUCT,      0x00
.equ PIO_ENABLE,        0x10
.equ PIO_DATA_OUT,      0x20
.equ PIO_FLAGS,         0x30

.equ VRAM_MAX_ADDR,     76800
.equ STORE_OPCODE,      0x02
.equ REFRESH_OPCODE,    0x00
.equ LOAD_OPCODE,       0x01
.equ VIZINHO_IN_OPCODE, 0x03
.equ REPLIC_OPCODE,     0x04
.equ ZOOM_OUT_OPCODE,   0x05
.equ MEDIA_OPCODE,      0x06
.equ RESET_OPCODE,      0x07

.equ FLAG_DONE_MASK,     0x01
.equ FLAG_ERROR_MASK,    0x02
.equ FLAG_ZOOM_Min_MASK, 0x04
.equ FLAG_ZOOM_Max_MASK, 0x08

.equ TIMEOUT_COUNT,     0x3000
.equ EXTRA_DELAY_COUNT, 0x1000   @ b Delay extra para sincronização

.section .rodata
.LC0:           .asciz "/dev/mem"
.LC1:           .asciz "ERROR: could not open '/dev/mem' ...\n"
.LC2:           .asciz "ERROR: mmap() failed ...\n"
.LC3:           .asciz "ERROR: munmap() failed ...\n"

.section .data
FPGA_ADRS:
    .space 4
FILE_DESCRIPTOR:
    .space 4
LW_SPAM:
    .word 0x1000
LW_BASE:
    .word 0xff200

.section .text

.global Lib
.type Lib, %function
Lib:
    push    {r4-r7, lr}
    ldr     r0, =.LC0
    ldr     r1, =4098
    mov     r2, #0
    mov     r7, #5
    svc     0
    mov     r4, r0
    ldr     r1, =FILE_DESCRIPTOR
    str     r4, [r1]
    cmp     r4, #-1
    bne     .NMAP_SETUP
    ldr     r0, =.LC1
    bl      puts
    mov     r0, #-1
    b       .RETURN_INIT

.NMAP_SETUP:
    mov     r0, #0
    ldr     r1, =LW_SPAM
    ldr     r1, [r1]
    mov     r2, #3
    mov     r3, #1
    ldr     r4, =FILE_DESCRIPTOR
    ldr     r4, [r4]
    ldr     r5, =LW_BASE
    ldr     r5, [r5]
    mov     r7, #192
    svc     0
    mov     r4, r0
    ldr     r1, =FPGA_ADRS
    str     r4, [r1]
    cmp     r4, #-1
    bne     .INIT_DONE
    ldr     r0, =.LC2
    bl      puts
    ldr     r0, =FILE_DESCRIPTOR
    ldr     r0, [r0]
    bl      close
    mov     r0, #-1
    b       .RETURN_INIT
.INIT_DONE:
    mov     r0, #0
.RETURN_INIT:
    pop     {r4-r7, pc}
.size Lib, .-Lib

.global encerraLib
.type encerraLib, %function
encerraLib:
    push    {r4-r7, lr}
    ldr     r0, =FPGA_ADRS
    ldr     r0, [r0]
    ldr     r1, =LW_SPAM
    ldr     r1, [r1]
    mov     r7, #91
    svc     0
    mov     r4, r0
    cmp     r4, #0
    beq     .CLOSE_FD
    ldr     r0, =.LC3
    bl      puts
    mov     r0, #-1
    b       .STATUS_BIB
.CLOSE_FD:
    ldr     r0, =FILE_DESCRIPTOR
    ldr     r0, [r0]
    mov     r7, #6
    svc     0
    mov     r0, #0
.STATUS_BIB:
    pop     {r4-r7, pc}
.size encerraLib, .-encerraLib

.global write_pixel
.type write_pixel, %function
write_pixel:
    push    {r4-r6, lr}
    ldr     r4, =FPGA_ADRS
    ldr     r4, [r4]
    cmp     r0, #VRAM_MAX_ADDR
    bhs     .ADDR_INVALID_WR
.ASM_DATA:
    mov     r2, #STORE_OPCODE
    lsl     r3, r0, #3
    orr     r2, r2, r3
    mov     r3, #1
    lsl     r3, r3, #20
    orr     r2, r2, r3
    lsl     r3, r1, #21
    orr     r2, r2, r3
    str     r2, [r4, #PIO_INSTRUCT]
    dmb     sy
    mov     r2, #1
    str     r2, [r4, #PIO_ENABLE]
    mov     r2, #0
    str     r2, [r4, #PIO_ENABLE]
    mov     r5, #TIMEOUT_COUNT
.WAIT_LOOP_WR:
    ldr     r2, [r4, #PIO_FLAGS]
    tst     r2, #FLAG_DONE_MASK
    bne     .CHECK_ERROR_WR
    subs    r5, r5, #1
    bne     .WAIT_LOOP_WR
    mov     r0, #0
    b       .EXIT_WR
.CHECK_ERROR_WR:
    tst     r2, #FLAG_ERROR_MASK
    bne     .HW_ERROR_WR
    mov     r0, #0

    @ Delay extra para sincronizar hofps x fpga (comparar clocks 800Mhz vs 50Mhz)
    mov     r5, #EXTRA_DELAY_COUNT
.DELAY_LOOP:
    subs    r5, r5, #1
    bne     .DELAY_LOOP

    b       .EXIT_WR
.ADDR_INVALID_WR:
    mov     r0, #-1
    b       .EXIT_WR
.HW_ERROR_WR:
    mov     r0, #-3
.EXIT_WR:
    pop     {r4-r6, pc}
.size write_pixel, .-write_pixel


.global read_pixel
.type read_pixel, %function
read_pixel:
    push    {r4-r6, lr}
    ldr     r4, =FPGA_ADRS
    ldr     r4, [r4]
    cmp     r0, #VRAM_MAX_ADDR
    bhs     .ADDR_INVALID_RD
    mov     r2, #LOAD_OPCODE
    lsl     r3, r0, #3
    orr     r2, r2, r3
    mov     r3, r1
    lsl     r3, r3, #20
    orr     r2, r2, r3
    str     r2, [r4, #PIO_INSTRUCT]
    dmb     sy
    mov     r2, #1
    str     r2, [r4, #PIO_ENABLE]
    mov     r2, #0
    str     r2, [r4, #PIO_ENABLE]
    mov     r5, #TIMEOUT_COUNT
.WAIT_LOOP_RD:
    ldr     r2, [r4, #PIO_FLAGS]
    tst     r2, #FLAG_DONE_MASK
    bne     .CHECK_ERROR_RD
    subs    r5, r5, #1
    bne     .WAIT_LOOP_RD
    mov     r0, #0
    b       .EXIT_RD
.CHECK_ERROR_RD:
    tst     r2, #FLAG_ERROR_MASK
    bne     .HW_ERROR_RD
    ldr     r0, [r4, #PIO_DATA_OUT]
    b       .EXIT_RD
.ADDR_INVALID_RD:
    mov     r0, #-1
    b       .EXIT_RD
.HW_ERROR_RD:
    mov     r0, #-3
.EXIT_RD:
    pop     {r4-r6, pc}
.size read_pixel, .-read_pixel


.global send_refresh
.type send_refresh, %function
send_refresh:
    push    {r4, lr}
    ldr     r4, =FPGA_ADRS
    ldr     r4, [r4]
    mov     r2, #REFRESH_OPCODE
    str     r2, [r4, #PIO_INSTRUCT]
    mov     r2, #1
    str     r2, [r4, #PIO_ENABLE]
    mov     r2, #0
    str     r2, [r4, #PIO_ENABLE]
    pop     {r4, pc}
.size send_refresh, .-send_refresh


.global Vizinho_Prox
.type Vizinho_Prox, %function
Vizinho_Prox:
    push    {r7, lr}
    ldr     r3, =FPGA_ADRS
    ldr     r3, [r3]
    movs    r2, #VIZINHO_IN_OPCODE
    str     r2, [r3, #PIO_INSTRUCT]
    movs    r2, #1
    str     r2, [r3, #PIO_ENABLE]
    movs    r2, #0
    str     r2, [r3, #PIO_ENABLE]
    pop     {r7, pc}
.size Vizinho_Prox, .-Vizinho_Prox


.global Replicacao 
.type Replicacao, %function 
Replicacao:
    push    {r7, lr}
    ldr     r3, =FPGA_ADRS
    ldr     r3, [r3]
    movs    r2, #REPLIC_OPCODE
    str     r2, [r3, #PIO_INSTRUCT]
    movs    r2, #1
    str     r2, [r3, #PIO_ENABLE]
    movs    r2, #0
    str     r2, [r3, #PIO_ENABLE]
    pop     {r7, pc}
.size Replicacao, .-Replicacao


.global Decimacao 
.type Decimacao, %function 
Decimacao:
    push    {r7, lr}
    ldr     r3, =FPGA_ADRS
    ldr     r3, [r3]
    movs    r2, #ZOOM_OUT_OPCODE
    str     r2, [r3, #PIO_INSTRUCT]
    movs    r2, #1
    str     r2, [r3, #PIO_ENABLE]
    movs    r2, #0
    str     r2, [r3, #PIO_ENABLE]
    pop     {r7, pc}
.size Decimacao, .-Decimacao


.global Media 
.type Media, %function 
Media:
    push    {r7, lr}
    ldr     r3, =FPGA_ADRS
    ldr     r3, [r3]
    movs    r2, #MEDIA_OPCODE
    str     r2, [r3, #PIO_INSTRUCT]
    movs    r2, #1
    str     r2, [r3, #PIO_ENABLE]
    movs    r2, #0
    str     r2, [r3, #PIO_ENABLE]
    pop     {r7, pc}
.size Media, .-Media


.global Reset
.type Reset, %function
Reset:
    push    {r7, lr}
    ldr     r3, =FPGA_ADRS
    ldr     r3, [r3]
    movs    r2, #RESET_OPCODE
    str     r2, [r3, #PIO_INSTRUCT]
    movs    r2, #1
    str     r2, [r3, #PIO_ENABLE]
    movs    r2, #0
    str     r2, [r3, #PIO_ENABLE]
    pop     {r7, pc}
.size Reset, .-Reset


.global Flag_Done
.type Flag_Done, %function
Flag_Done:
    push    {r7, lr}
    ldr     r3, =FPGA_ADRS
    ldr     r3, [r3]
    ldr     r0, [r3, #PIO_FLAGS]
    and     r0, r0, #FLAG_DONE_MASK
    pop     {r7, pc}
.size Flag_Done, .-Flag_Done


.global Flag_Error
.type Flag_Error, %function
Flag_Error:
    push    {r7, lr}
    ldr     r3, =FPGA_ADRS
    ldr     r3, [r3]
    ldr     r0, [r3, #PIO_FLAGS]
    and     r0, r0, #FLAG_ERROR_MASK
    pop     {r7, pc}
.size Flag_Error, .-Flag_Error


.global Flag_Max
.type Flag_Max, %function
Flag_Max:
    push    {r7, lr}
    ldr     r3, =FPGA_ADRS
    ldr     r3, [r3]
    ldr     r0, [r3, #PIO_FLAGS]
    and     r0, r0, #FLAG_ZOOM_Max_MASK
    pop     {r7, pc}
.size Flag_Max, .-Flag_Max


.global Flag_Min
.type Flag_Min, %function
Flag_Min:
    push    {r7, lr}
    ldr     r3, =FPGA_ADRS
    ldr     r3, [r3]
    ldr     r0, [r3, #PIO_FLAGS]
    and     r0, r0, #FLAG_ZOOM_Min_MASK
    pop     {r7, pc}
.size Flag_Min, .-Flag_Min
