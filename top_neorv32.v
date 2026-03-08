module top_neorv32 (
    input  wire clk,
    input  wire rst_n,
    input  wire btn_fire,      // Sensor de Incêndio (GPIO 0)
    input  wire btn_intruder,  // Sensor de Presença (GPIO 1)
    output wire uart_txd,
    input  wire uart_rxd,
    output wire led_lock,      // Fechadura (XBUS 0x90000000)
    output wire led_alarm,     // Alarme (GPIO 1)
    output wire led_arcondicionado, // AC (GPIO 2)
    output wire led_lampada    // Lâmpada (GPIO 3)
);

    // --- SINAIS WISHBONE (XBUS) ---
    wire [31:0] wb_adr;
    wire [31:0] wb_dat_write;
    wire [31:0] wb_dat_read;
    wire        wb_we;
    wire [3:0]  wb_sel;
    wire        wb_stb;
    wire        wb_cyc;
    reg         wb_ack;

    // --- SINAIS GPIO ---
    wire [63:0] gpio_out;
    wire [63:0] gpio_in;

    // --- REGISTRADORES ---
    reg  led_reg = 1'b0;
    wire uart_tx_sig;

    assign uart_txd = uart_tx_sig;
    assign led_lock = ~led_reg; // Lógica inversa para a placa
    assign led_alarm = ~gpio_out[1];
    assign led_arcondicionado = ~gpio_out[2];
    assign led_lampada = ~gpio_out[3];
    
    assign gpio_in[0] = btn_fire;     
    assign gpio_in[1] = btn_intruder;
    assign gpio_in[63:2] = 62'b0;

    // --- INSTÂNCIA DO NEORV32 ---
    neorv32_top #(
        .CLOCK_FREQUENCY(50000000),
        .BOOT_MODE_SELECT(0),
        .IMEM_EN("true"),
        .IMEM_SIZE(8192),      
        .DMEM_EN("true"),
        .DMEM_SIZE(8192),      
        .IO_UART0_EN("true"),
        .IO_GPIO_NUM(4),
        .RISCV_ISA_Zkne("true"), 
        .RISCV_ISA_Zknd("true"),  
        .XBUS_EN("true")         
    ) neorv32_top_inst (
        .clk_i      (clk),
        .rstn_i     (rst_n),
        .uart0_txd_o(uart_tx_sig),
        .uart0_rxd_i(uart_rxd),
        .gpio_o     (gpio_out),
        .gpio_i     (gpio_in),
        .xbus_adr_o (wb_adr),
        .xbus_dat_o (wb_dat_write),
        .xbus_dat_i (wb_dat_read),
        .xbus_we_o  (wb_we),
        .xbus_sel_o (wb_sel),
        .xbus_stb_o (wb_stb),
        .xbus_cyc_o (wb_cyc),
        .xbus_ack_i (wb_ack),
        .xbus_err_i (1'b0)
    );

    // --- LÓGICA XBUS ---
    assign wb_dat_read = 32'b0;
    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            wb_ack  <= 1'b0;
            led_reg <= 1'b0;
        end else begin
            wb_ack <= 1'b0;
            if (wb_cyc && wb_stb && !wb_ack) begin
                wb_ack <= 1'b1;
                if (wb_we && wb_adr[7:0] == 8'h00) begin
                    led_reg <= wb_dat_write[0];
                end
            end
        end
    end

endmodule