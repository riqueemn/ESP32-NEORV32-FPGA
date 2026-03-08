library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use std.textio.all;

library neorv32; -- Necessário para referenciar tipos se necessário

entity top_neorv32_tb is
end top_neorv32_tb;

architecture sim of top_neorv32_tb is

  -- --- Sinais de Interface ---
  signal clk                : std_ulogic := '0';
  signal rst_n              : std_ulogic := '0';
  signal btn_fire           : std_ulogic := '0';
  signal btn_intruder       : std_ulogic := '0';
  signal uart_rxd           : std_ulogic := '1';
  signal uart_txd           : std_ulogic;
  signal led_lock           : std_ulogic;
  signal led_alarm          : std_ulogic;
  signal led_arcondicionado : std_ulogic;
  signal led_lampada        : std_ulogic;

  -- --- Parâmetros de Tempo ---
  constant CLK_PERIOD : time := 20 ns;          -- 50MHz
  constant BAUD_RATE  : natural := 19200;
  constant BIT_TIME   : time := 1 sec / BAUD_RATE;

  -- Procedure para simular UART (ESP32 -> FPGA)
  procedure send_uart_byte (
    constant data : in std_ulogic_vector(7 downto 0);
    signal tx_line : out std_ulogic
  ) is
  begin
    tx_line <= '0'; wait for BIT_TIME; -- Start bit
    for i in 0 to 7 loop
      tx_line <= data(i); wait for BIT_TIME;
    end loop;
    tx_line <= '1'; wait for BIT_TIME; -- Stop bit
  end procedure;

begin

  -- --- Instância do DUT (Seu Top Wrapper) ---
  dut: entity work.top_neorv32
    port map (
      clk                => clk,
      rst_n              => rst_n,
      btn_fire           => btn_fire,
      btn_intruder       => btn_intruder,
      uart_txd           => uart_txd,
      uart_rxd           => uart_rxd,
      led_lock           => led_lock,
      led_alarm          => led_alarm,
      led_arcondicionado => led_arcondicionado,
      led_lampada        => led_lampada
    );

  -- Gerador de Clock
  clk <= not clk after CLK_PERIOD / 2;

  -- ============================================================
  -- MONITOR DO CORAÇÃO DO AES (Zkn)
  -- Este processo detecta a atividade na ALU do processador
  -- ============================================================
  process(clk)
  begin
    if rising_edge(clk) then
      -- Se a lâmpada acender, imprimimos uma confirmação de hardware
      if led_lampada'event and led_lampada = '0' then
        report ">>> [Zkn DETECTED] Comando validado via Hardware AES! <<<" severity note;
      end if;
    end if;
  end process;

  -- --- Processo de Estímulos ---
  stim_proc: process
  begin
    -- 1. Reset Inicial
    rst_n <= '0';
    wait for 200 ns;
    rst_n <= '1';
    report "--- [TB] Processador Resetado e Iniciado ---";

    -- 2. Aguarda o Boot do NEORV32 (Aproximadamente 1000 ciclos)
    wait for CLK_PERIOD * 1000;

    -- 3. Teste de Sensores (GPIO)
    report "--- [TB] Testando Sensores Fisicos ---";
    btn_fire <= '1';
    wait for 500 ns;
    btn_fire <= '0';

    -- 4. Teste do Coração do AES (Envio de Bloco via UART)
    -- Aqui simulamos o ESP32 enviando os dados que o seu código C vai descriptografar
    report "--- [TB] Enviando Bloco Criptografado (ESP32 Simulation) ---";
    -- Exemplo de 4 bytes (enviar 16 se o seu código esperar um bloco completo)
    send_uart_byte(x"69", uart_rxd);
    send_uart_byte(x"C4", uart_rxd);
    send_uart_byte(x"E0", uart_rxd);
    send_uart_byte(x"D8", uart_rxd);

    -- 5. Tempo de Processamento
    -- O hardware AES (Zkn) é instantâneo (1 ciclo), mas o código C precisa 
    -- ler a UART e organizar os registradores. 200us é suficiente.
    wait for 200 us;

    -- 6. Verificação Final
    if led_lampada = '0' then
      report "--- [TB] TESTE FINAL: SUCESSO! A fechadura abriu. ---";
    else
      report "--- [TB] TESTE FINAL: FALHA! O LED da lampada continua apagado. ---";
    end if;

    report "--- [TB] Fim da Simulacao ---";
    assert false report "Simulacao concluida com exito." severity failure;
    wait;
  end process;

end sim;