module top_neorv32 (
    input  wire clk,
    input  wire rst_n,
    input  wire btn_fire,     // Simulação: Sensor de Incêndio (Botão 0)
    input  wire btn_intruder, // Simulação: Sensor de Presença (Botão 1)
    output wire uart_txd,
    input  wire uart_rxd,
    output wire led_lock,     // LED representando a Fechadura
    output wire led_alarm,     // LED representando o Alarme
	 output wire led_arcondicionado,     // LED representando a Fechadura
    output wire led_lampada    // LED representando o Alarme
);

    // Sinais do Barramento Wishbone (XBUS)
    wire [31:0] wb_adr;
    wire [31:0] wb_dat_write;
    wire [31:0] wb_dat_read;
    wire        wb_we;
    wire [3:0]  wb_sel;
    wire        wb_stb;
    wire        wb_cyc;
    reg         wb_ack;

    // Sinais de GPIO
    wire [63:0] gpio_out;
    wire [63:0] gpio_in;

    // Registrador para o LED via XBUS (como você já tinha)
    reg         led_reg = 1'b0;
    wire        uart_tx_sig;

    // --- CONEXÕES FÍSICAS ---
    assign uart_txd  = uart_tx_sig;
    
    // O LED da fechadura pode ser controlado pelo seu antigo periférico XBUS 
    // OU pelo GPIO. Vamos manter o XBUS para o LED 0 por enquanto.
    assign led_lock  = ~led_reg;
	 
    
    // O Alarme vamos conectar ao GPIO 1 para você testar essa interface
    assign led_alarm = ~gpio_out[1];
	 assign led_arcondicionado  = ~gpio_out[2];
	 assign led_lampada  = ~gpio_out[3];

    // Entrada dos Sensores (Botões) via GPIO
    // Invertemos (~) se o seu botão na placa for "0" quando apertado (Active Low)
    assign gpio_in[0] = btn_fire;     
    assign gpio_in[1] = btn_intruder;

    // --- INSTÂNCIA DO PROCESSADOR NEORV32 ---
    neorv32_top #(
        .CLOCK_FREQUENCY(50000000),
        .BOOT_MODE_SELECT(0),
        .IMEM_EN("true"),
        .IMEM_SIZE(16384),
        .DMEM_EN("true"),
        .DMEM_SIZE(8192),
        .IO_UART0_EN("true"),
        .IO_GPIO_NUM(4),    // <--- Habilitando GPIO para os botões
        .XBUS_EN("true")
    ) neorv32_top_inst (
        .clk_i      (clk),
        .rstn_i     (rst_n),
        .uart0_txd_o(uart_tx_sig),
        .uart0_rxd_i(uart_rxd),
        // Conexão GPIO
        .gpio_o     (gpio_out),
        .gpio_i     (gpio_in),
        // Conexão XBUS
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

    // --- LÓGICA DO PERIFÉRICO CUSTOMIZADO (LED VIA XBUS) ---
    assign wb_dat_read = 32'b0; // Sempre retorna 0 na leitura

    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            wb_ack  <= 1'b0;
            led_reg <= 1'b0;
        end else begin
            wb_ack <= 1'b0;
            if (wb_cyc && wb_stb) begin
                wb_ack <= 1'b1;
                if (wb_we) begin
                    led_reg <= wb_dat_write[0];
                end
            end
        end
    end

endmodule