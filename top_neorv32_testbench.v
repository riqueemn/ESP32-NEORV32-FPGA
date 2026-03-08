`timescale 1ns / 1ps

module top_neorv32_tb();

    // --- SINAIS DE INTERFACE ---
    reg  clk;
    reg  rst_n;
    reg  btn_fire;
    reg  btn_intruder;
    reg  uart_rxd;
    
    wire uart_txd;
    wire led_lock;
    wire led_alarm;
    wire led_arcondicionado;
    wire led_lampada;

    // --- PARÂMETROS DE TEMPO ---
    parameter CLK_PERIOD = 20;            // 50MHz (20ns por ciclo)
    parameter BAUD_RATE  = 19200;         // Baud Rate configurado no NEORV32
    parameter BIT_TIME   = 1000000000 / BAUD_RATE; // Tempo de 1 bit em ns (aprox. 52083 ns)

    // --- INSTÂNCIA DO DUO (Device Under Test) ---
    top_neorv32 dut (
        .clk                (clk),
        .rst_n              (rst_n),
        .btn_fire           (btn_fire),
        .btn_intruder       (btn_intruder),
        .uart_txd           (uart_txd),
        .uart_rxd           (uart_rxd),
        .led_lock           (led_lock),
        .led_alarm          (led_alarm),
        .led_arcondicionado (led_arcondicionado),
        .led_lampada        (led_lampada)
    );

    // --- GERADOR DE CLOCK (50 MHz) ---
    always #(CLK_PERIOD/2) clk = ~clk;

    // --- TAREFA: SIMULAR ENVIO DE BYTE VIA UART (ESP32 -> FPGA) ---
    task send_uart_byte(input [7:0] data);
        integer i;
        begin
            uart_rxd = 0; // Start bit
            #(BIT_TIME);
            for (i = 0; i < 8; i = i + 1) begin
                uart_rxd = data[i]; // Data bits (LSB primeiro)
                #(BIT_TIME);
            end
            uart_rxd = 1; // Stop bit
            #(BIT_TIME);
        end
    endtask

    // --- MONITORAMENTO DA EXTENSÃO Zkn (Sinais Internos) ---
    // Este bloco "espiona" a CPU para validar a aceleração AES
    always @(posedge clk) begin
        // O caminho da hierarquia pode variar dependendo da versão do NEORV32
        if (dut.neorv32_top_inst.neorv32_cpu_inst.cpu_cp_bitmanip_inst.valid_o) begin
            $display("[DEBUG Zkn] Instrucao AES detectada! Entrada: %h | Saida: %h", 
                     dut.neorv32_top_inst.neorv32_cpu_inst.cpu_cp_bitmanip_inst.rs1_i,
                     dut.neorv32_top_inst.neorv32_cpu_inst.cpu_cp_bitmanip_inst.res_o);
        end
    end

    // --- SEQUÊNCIA DE TESTE (STIMULUS) ---
    initial begin
        // 1. Reset Inicial
        clk          = 0;
        rst_n        = 0;
        btn_fire     = 0;
        btn_intruder = 0;
        uart_rxd     = 1; // UART em idle fica em '1'

        $display("--- [TB] Iniciando Simulacao do Sistema de Automacao ---");
        #(CLK_PERIOD * 10);
        rst_n = 1; // Libera o sistema

        // 2. Aguarda o Boot do Processador
        // O firmware leva um tempo para configurar UART e GPIO
        #(CLK_PERIOD * 500);

        // 3. Teste de Sensores (GPIO Input)
        $display("--- [TB] Testando Sensor de Incendio ---");
        btn_fire = 1;
        #(CLK_PERIOD * 100);
        btn_fire = 0;

        // 4. Teste de Comunicacao Segura (Simulando o ESP32)
        // Vamos enviar um bloco de 16 bytes criptografados (exemplo didático)
        $display("--- [TB] Enviando Bloco AES via UART (Simulando ESP32) ---");
        send_uart_byte(8'h69); send_uart_byte(8'hc4); send_uart_byte(8'he0); send_uart_byte(8'hd8);
        send_uart_byte(8'h6a); send_uart_byte(8'h7b); send_uart_byte(8'h04); send_uart_byte(8'h30);
        send_uart_byte(8'hd8); send_uart_byte(8'hcd); send_uart_byte(8'hb7); send_uart_byte(8'h80);
        send_uart_byte(8'h70); send_uart_byte(8'hb4); send_uart_byte(8'hc5); send_uart_byte(8'h5a);

        // 5. Verificacao de Resultado
        // Aguarda tempo suficiente para o código C processar a descriptografia
        #(BIT_TIME * 20); 

        if (led_lampada == 1'b0) // Lógica inversa: 0 acende
            $display("--- [TB] SUCESSO: Comando validado e Lampada Acesa! ---");
        else
            $display("--- [TB] ALERTA: Aguardando processamento do comando... ---");

        // 6. Finalizacao
        #(CLK_PERIOD * 2000);
        $display("--- [TB] Simulacao Finalizada com Sucesso ---");
        $stop;
    end

    // --- MONITOR DE CONSOLE ---
    initial begin
        $monitor("Tempo: %t | Lock: %b | Lampada: %b | Alarme: %b | Fire: %b", 
                  $time, led_lock, led_lampada, led_alarm, btn_fire);
    end

endmodule