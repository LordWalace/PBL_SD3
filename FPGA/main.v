module main(
    // Portas de Entrada
    CLOCK_50, INSTRUCTION, DATA_IN, MEM_ADDR, SEL_MEM, ENABLE,
    // Portas de Saída
    DATA_OUT, FLAG_DONE, FLAG_ERROR, FLAG_ZOOM_MAX, FLAG_ZOOM_MIN,
    VGA_R, VGA_B, VGA_G, VGA_BLANK_N, VGA_H_SYNC_N, VGA_V_SYNC_N, VGA_CLK, VGA_SYNC
);

    input CLOCK_50;
    input [2:0] INSTRUCTION;
    input [7:0] DATA_IN;
    input [16:0] MEM_ADDR;
    input SEL_MEM;
    input ENABLE;

    output reg [7:0] DATA_OUT;
    output reg FLAG_DONE;
    output reg FLAG_ERROR;
    output FLAG_ZOOM_MAX;
    output FLAG_ZOOM_MIN;
    
    // Saídas VGA (8 bits por canal)
    output [7:0] VGA_R;
    output [7:0] VGA_B; 
    output [7:0] VGA_G;
    output VGA_BLANK_N;
    output VGA_H_SYNC_N; 
    output VGA_V_SYNC_N; 
    output VGA_CLK;
    output VGA_SYNC;

    // --- 1. Definições ---
    wire clk_100, clk_25_vga;
    pll pll0( .refclk(CLOCK_50), .rst(1'b0), .outclk_0(clk_100), .outclk_1(clk_25_vga) );

    localparam REFRESH_SCREEN = 3'b000, LOAD = 3'b001, STORE = 3'b010, NHI_ALG = 3'b011; 
    localparam PR_ALG = 3'b100, BA_ALG = 3'b101, NH_ALG = 3'b110, RESET_INST = 3'b111; 
    localparam IDLE = 3'b00, READ_AND_WRITE = 3'b001, ALGORITHM = 3'b010, RESET = 3'b011, COPY_READ = 3'b100, COPY_WRITE = 3'b101, WAIT_WR_OR_RD = 3'b111;

    reg [2:0] uc_state;
    reg [2:0] last_instruction;
    reg enable_ff;
    wire enable_pulse;
    always @(posedge clk_100) enable_ff <= !ENABLE;
    assign enable_pulse = !ENABLE && !enable_ff;
    
    // VGA Sinais
    wire [9:0] next_x, next_y;
    reg [16:0] addr_from_vga;
    reg        inside_box;
    
    // Wires para evitar latch na memória
    reg [16:0] counter_address;
    wire [16:0] addr_for_copy;
    wire [16:0] addr_mem3_read;
    assign addr_for_copy = counter_address; 
    assign addr_mem3_read = counter_address; 

    // Registradores do Algoritmo
    reg [16:0] addr_for_read;
    reg [16:0] addr_for_write;
    reg [7:0]  data_to_write;
    reg [1:0]  counter_rd_wr;

    // --- 2. Memórias ---
    wire [16:0] addr_mem1, addr_mem2;
    reg [7:0]  data_in_mem1, data_in_mem2;
    reg        wren_mem1, wren_mem2, wren_mem3;
    wire [7:0] data_out_mem1, data_out_mem2, data_out_mem3;
    reg [16:0] addr_wr_mem1, addr_wr_mem2;
     
    // Mem1: Imagem Original
    mem1 memory1(.rdaddress(addr_mem1), .wraddress(addr_wr_mem1), .clock(clk_100), .data(data_in_mem1), .wren(wren_mem1), .q(data_out_mem1));
    // Mem2: VGA Display
    mem1 memory2(.rdaddress(addr_mem2), .wraddress(addr_wr_mem2), .clock(clk_100), .data(data_in_mem2), .wren(wren_mem2), .q(data_out_mem2));
    // Mem3: Buffer de Algoritmo
    mem1 memory3(.rdaddress(addr_mem3_read), .wraddress(addr_for_write), .clock(clk_100), .data(data_to_write), .wren(wren_mem3), .q(data_out_mem3));

    // MUX DE ENDEREÇOS (Direto)
    assign addr_mem1 = (uc_state == ALGORITHM || uc_state == WAIT_WR_OR_RD) ? addr_for_read : addr_for_copy;
    assign addr_mem2 = addr_from_vga;

    // --- 3. Lógica VGA (Endereçamento Otimizado) ---
    always @(posedge clk_25_vga) begin
        localparam X_START=159, Y_START=119, X_END=X_START+320, Y_END=Y_START+240;
        
        // Verifica Área da Imagem
        if (next_x >= X_START && next_x < X_END && next_y >= Y_START && next_y < Y_END) begin
            inside_box <= 1'b1;
            // Cálculo Otimizado: (Y * 320) + X usando Shifts
            addr_from_vga <= ( (next_y - Y_START) << 8 ) + ( (next_y - Y_START) << 6 ) + (next_x - X_START);
        end else begin
            inside_box <= 1'b0;
            addr_from_vga <= 17'd0;
        end
    end
    
    // Pipeline de dados de vídeo
    reg [7:0] data_to_vga_pipe;
    always @(posedge clk_100) begin
        if (inside_box)
            data_to_vga_pipe <= data_out_mem2;
        else
            data_to_vga_pipe <= 8'h00;
    end 

    // --- 4. Variáveis de Algoritmo ---
    reg [2:0] next_zoom;
    reg [2:0] current_zoom;
    reg has_alg_on_exec;
    reg [9:0] new_x, new_y, old_x, old_y;
    reg [16:0] needed_steps, current_step;
    reg [3:0] op_step;
    reg [31:0] data_to_avg;

    assign FLAG_ZOOM_MAX = (current_zoom == 3'b111);
    assign FLAG_ZOOM_MIN = (current_zoom == 3'b001);
    
    // --- 5. FSM ---
    always @(posedge clk_100) begin
        case (uc_state) 
            IDLE: begin 
                has_alg_on_exec <= 1'b0;
                FLAG_DONE <= 1'b1;
                wren_mem1 <= 1'b0; wren_mem2 <= 1'b0; wren_mem3 <= 1'b0;

                if (enable_pulse) begin
                    counter_address <= 17'd0;
                    counter_rd_wr <= 2'b0;
                    if (INSTRUCTION == LOAD || INSTRUCTION == STORE) begin
                        uc_state <= READ_AND_WRITE;
                        last_instruction <= INSTRUCTION;
                    end else if (INSTRUCTION >= NHI_ALG && INSTRUCTION <= NH_ALG) begin
                        case (INSTRUCTION)
                            NH_ALG: begin 
                                    if (FLAG_ZOOM_MIN) begin FLAG_DONE <= 1'b1; uc_state <= IDLE; end 
                                    else begin next_zoom <= current_zoom - 1'b1; uc_state <= (current_zoom == 3'b101) ? COPY_READ : ALGORITHM; last_instruction <= (current_zoom == 3'b101) ? RESET_INST : NH_ALG; end
                                    end
                            NHI_ALG: begin 
                                    if (FLAG_ZOOM_MAX) begin FLAG_DONE <= 1'b1; uc_state <= IDLE; end 
                                    else begin next_zoom <= current_zoom + 1'b1; uc_state <= (current_zoom == 3'b011) ? COPY_READ : ALGORITHM; last_instruction <= (current_zoom == 3'b011) ? RESET_INST : NHI_ALG; end
                                    end
                            BA_ALG: begin uc_state <= ALGORITHM; last_instruction <= BA_ALG; end
                            PR_ALG: begin uc_state <= ALGORITHM; last_instruction <= PR_ALG; end
                        endcase
                    end else if (INSTRUCTION == RESET_INST) begin
                        last_instruction <= 3'b111; uc_state <= RESET; counter_address <= 17'd0;
                    end else if (INSTRUCTION == REFRESH_SCREEN) begin
                        last_instruction <= 3'b111; uc_state <= COPY_READ; counter_address <= 17'b0;
                    end
                end
            end
            
            READ_AND_WRITE: begin
                FLAG_DONE <= 1'b0;
                if (last_instruction == STORE) begin
                    if (MEM_ADDR > 17'd76799) begin
                        FLAG_ERROR <= 1'b1; 
                    end
                    addr_wr_mem1 <= MEM_ADDR;
                    data_in_mem1 <= DATA_IN;
                    wren_mem1 <= 1'b1;
                    uc_state <= WAIT_WR_OR_RD;
                    counter_rd_wr <= 2'b00;
                end else begin // LOAD
                    wren_mem1 <= 1'b0;
                    wren_mem3 <= 1'b0;
                    counter_rd_wr <= 2'b0;
                    uc_state <= WAIT_WR_OR_RD;
                end
            end

            // --- Lógica dos Algoritmos ---
            ALGORITHM: begin
                wren_mem1 <= 1'b0;
                FLAG_DONE <= 1'b0;
                case (last_instruction)
                    PR_ALG: begin
                        if (!has_alg_on_exec) begin
                            current_step <= 19'd0; has_alg_on_exec <= 1'b1; needed_steps <= 19'd19199; op_step <= 3'b0; new_x <= 10'b0; new_y <= 10'b0;
                            if (next_zoom == 3'b101) begin old_x <= 10'd80; old_y <= 10'd60; end 
                            else if (next_zoom == 3'b110) begin old_x <= 10'd120; old_y <= 10'd90; end 
                            else if (next_zoom == 3'b111) begin old_x <= 10'd140; old_y <= 10'd105; end 
                            else begin old_x <= 10'd0; old_y <= 10'd0; end
                        end else begin
                            if (current_step >= needed_steps) begin
                                counter_address <= 17'd0; counter_rd_wr <= 2'b0; has_alg_on_exec <= 1'b0; wren_mem3 <= 1'b0; uc_state <= COPY_READ;
                            end else begin
                                if (op_step == 3'b000) begin
                                    addr_for_read <= old_x + (old_y*10'd320); counter_rd_wr <= 2'b0; op_step <= 3'b001; wren_mem3 <= 1'b0; uc_state <= WAIT_WR_OR_RD;
                                end else if (op_step == 3'b001) begin
                                    data_to_write <= data_out_mem1; counter_rd_wr <= 2'b0; addr_for_write <= new_x + (new_y*10'd320); wren_mem3 <= 1'b1; op_step <= 3'b010; uc_state <= WAIT_WR_OR_RD; new_x <= new_x + 1'b1;
                                end else if (op_step == 3'b010) begin
                                    data_to_write <= data_out_mem1; counter_rd_wr <= 2'b0; addr_for_write <= new_x + (new_y*10'd320); wren_mem3 <= 1'b1; op_step <= 3'b011; uc_state <= WAIT_WR_OR_RD; new_x <= new_x - 1'b1; new_y <= new_y + 1'b1;
                                end else if (op_step == 3'b011) begin
                                    data_to_write <= data_out_mem1; counter_rd_wr <= 2'b0; addr_for_write <= new_x + (new_y*10'd320); wren_mem3 <= 1'b1; op_step <= 3'b100; uc_state <= WAIT_WR_OR_RD; new_x <= new_x + 1'b1;
                                end else if (op_step == 3'b100) begin
                                    data_to_write <= data_out_mem1; counter_rd_wr <= 2'b0; addr_for_write <= new_x + (new_y*10'd320); wren_mem3 <= 1'b1; op_step <= 3'b000; uc_state <= WAIT_WR_OR_RD;
                                    if (new_x >= 10'd319) begin
                                        new_x <= 10'd0; new_y <= new_y + 1'b1;
                                        if (next_zoom == 3'b101) begin old_x <= 10'd80; old_y <= (new_y >> 1'b1) + 10'd60; end 
                                        else if (next_zoom == 3'b110) begin old_x <= 10'd120; old_y <= (new_y >> 2'd2) + 10'd90; end 
                                        else  if (next_zoom == 3'b111) begin old_x <= 10'd140; old_y <= (new_y>>2'd3) + 10'd105; end 
                                        else begin old_x <= new_x; old_y <= new_y; end
                                    end else begin
                                        new_x <= new_x + 1'b1; new_y <= new_y - 1'b1;
                                        if (next_zoom == 3'b101) begin old_x <= (new_x >> 1'b1) + 10'd80; end 
                                        else if(next_zoom == 3'b110) begin old_x <= (new_x >> 2'd2) + 10'd120; end 
                                        else if (next_zoom == 3'b111) begin old_x <= (new_x >> 2'd3) + 10'd140; end 
                                        else begin old_x <= new_x; end
                                        current_step <= current_step + 1;
                                    end
                                end
                            end
                        end
                    end
                    NHI_ALG: begin
                        if (!has_alg_on_exec) begin
                            has_alg_on_exec <= 1'b1; current_step <= 19'd0; needed_steps <= 19'd76799; op_step <= 3'b0; new_x <= 10'b0; new_y <= 10'b0;
                            if (next_zoom == 3'b101) begin old_x <= 10'd80; old_y <= 10'd60; end 
                            else if (next_zoom == 3'b110) begin old_x <= 10'd120; old_y <= 10'd90; end 
                            else if (next_zoom == 3'b111) begin old_x <= 10'd140; old_y <= 10'd105; end 
                            else begin old_x <= 10'd0; old_y <= 10'd0; end
                        end else begin
                            if (current_step >= needed_steps) begin
                                counter_address <= 17'd0; counter_rd_wr <= 2'b0; has_alg_on_exec <= 1'b0; wren_mem3 <= 1'b0; uc_state <= COPY_READ;
                            end else begin
                                if ((((new_x < 10'd80 || new_x > 10'd239 ) || (new_y < 10'd60 ||  new_y > 10'd179)) && next_zoom == 3'b011) || 
                                    (((new_x < 10'd120 || new_x > 10'd199 ) || (new_y < 10'd90 ||  new_y > 10'd149)) && next_zoom == 3'b010) || 
                                    (((new_x < 10'd140 || new_x > 10'd179 ) || (new_y < 10'd105 ||  new_y > 10'd134)) && next_zoom == 3'b001)) begin
                                    current_step <= current_step + 1'b1; data_to_write <= 8'b0; counter_rd_wr <= 2'b0; wren_mem3 <= 1'b1; addr_for_write <= new_x + (new_y*10'd320); op_step <= 3'b000;
                                    if(new_x >= 10'd319) begin new_x <= 10'd0; new_y <= new_y + 1'b1; end else begin new_x <= new_x + 1'b1; end
                                    uc_state <= WAIT_WR_OR_RD;
                                end else begin
                                    if (op_step == 3'b000) begin
                                        if (next_zoom == 3'b011) addr_for_read <= (old_x<<1) + ((old_y<<1)*10'd320); 
                                        else if (next_zoom == 3'b010) addr_for_read <= (old_x<<2) + ((old_y<<2)*10'd320);
                                        else if (next_zoom == 3'b001) addr_for_read <= (old_x<<3) + ((old_y<<3)*10'd320);
                                        counter_rd_wr <= 2'b0; wren_mem3 <= 1'b0; uc_state <= WAIT_WR_OR_RD;
                                        if (next_zoom == 3'b011) begin if (old_x >= 10'd159) begin old_x <= 10'd0; old_y <= old_y + 2'd1; end else begin old_x <= old_x + 2'd1; end end 
                                        else if (next_zoom == 3'b010) begin if (old_x >= 10'd79) begin old_x <= 10'd0; old_y <= old_y + 2'd1; end else begin old_x <= old_x + 2'd1; end end 
                                        else if (next_zoom == 3'b001) begin if (old_x >= 10'd39) begin old_x <= 10'd0; old_y <= old_y + 2'd1; end else begin old_x <= old_x + 2'd1; end end 
                                        else begin old_x <= new_x; old_y <= new_y; end
                                        op_step <= 3'b001;
                                    end else if (op_step == 3'b001) begin
                                        current_step <= current_step + 1'b1; data_to_write <= data_out_mem1; counter_rd_wr <= 2'b0; addr_for_write <= new_x + (new_y*10'd320); wren_mem3 <= 1'b1; op_step <= 3'b000; uc_state <= WAIT_WR_OR_RD;
                                        if (new_x >= 10'd319) begin new_x <= 10'd0; new_y <= new_y + 1'b1; end 
                                        else begin new_x <= new_x + 1'b1; end
                                    end
                                end
                            end
                        end
                    end
                    default: begin has_alg_on_exec <= 1'b0; uc_state <= IDLE; end
                endcase
            end

            RESET: begin
                FLAG_DONE <= 1'b0; next_zoom <= 3'b100; FLAG_ERROR <= 1'b0; last_instruction <= RESET_INST; counter_address <= 17'd0; counter_rd_wr <= 2'b0; uc_state <= COPY_READ;
            end

            COPY_READ: begin
                if(counter_rd_wr == 2'b10) begin wren_mem2 <= 1'b0; counter_rd_wr <= 2'b00; uc_state <= COPY_WRITE; end 
                else counter_rd_wr <= counter_rd_wr + 1;
            end

            COPY_WRITE: begin
                if (last_instruction == RESET_INST || last_instruction == STORE) data_in_mem2 <= data_out_mem1;
                else data_in_mem2 <= data_out_mem3;
                addr_wr_mem2 <= counter_address; wren_mem2 <= 1'b1;
                if (counter_rd_wr == 2'b10) begin
                    counter_rd_wr <= 2'b00;
                    if (counter_address == 17'd76799) begin
                        current_zoom <= next_zoom; FLAG_DONE <= 1'b1; uc_state <= IDLE;
                    end else begin
                        counter_address <= counter_address + 1'b1; uc_state <= COPY_READ;
                    end
                end else counter_rd_wr <= counter_rd_wr + 1;
            end

            WAIT_WR_OR_RD: begin
                if (counter_rd_wr == 2'b10) begin
                    counter_rd_wr <= 2'b00;
                    if (last_instruction == LOAD) begin
                        uc_state <= IDLE;
                        if (SEL_MEM) DATA_OUT <= data_out_mem3; else DATA_OUT <= data_out_mem1;
                        FLAG_DONE <= 1'b1;
                    end else if (last_instruction == STORE) begin
                        uc_state <= IDLE; wren_mem1 <= 1'b0; counter_address <= 17'd0;
                    end else begin
                        wren_mem3 <= 1'b0; uc_state <= ALGORITHM;
                    end
                end else counter_rd_wr <= counter_rd_wr + 1;
            end
            
            default: uc_state <= IDLE;
        endcase
    end

    // Instância VGA (Deixando portas de cor desconectadas)
    vga_module vga_out(
        .clock(clk_25_vga), .reset(1'b0), .color_in(data_to_vga_pipe), 
        .next_x(next_x), .next_y(next_y), 
        .hsync(VGA_H_SYNC_N), .vsync(VGA_V_SYNC_N), 
        .red(), .green(), .blue(), // Desconectados, controlados abaixo
        .sync(VGA_SYNC), .clk(VGA_CLK), .blank(VGA_BLANK_N)
    );

    // --- ATRIBUIÇÃO FINAL PARA GRAYSCALE ---
    // Se o pixel vale 100, R=100, G=100, B=100 -> Cinza
    assign VGA_R = data_to_vga_pipe;
    assign VGA_G = data_to_vga_pipe;
    assign VGA_B = data_to_vga_pipe;

endmodule