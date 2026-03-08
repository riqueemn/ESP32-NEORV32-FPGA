library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

library neorv32;
use neorv32.neorv32_package.all;

entity top_neorv32 is
  port (
    clk                : in  std_ulogic;
    rst_n              : in  std_ulogic;
    btn_fire           : in  std_ulogic; -- Sensor de Incêndio (GPIO 0)
    btn_intruder       : in  std_ulogic; -- Sensor de Presença (GPIO 1)
    uart_txd           : out std_ulogic;
    uart_rxd           : in  std_ulogic;
    led_lock           : out std_ulogic; -- Fechadura (XBUS 0x90000000)
    led_alarm          : out std_ulogic; -- Alarme (GPIO 1)
    led_arcondicionado : out std_ulogic; -- AC (GPIO 2)
    led_lampada        : out std_ulogic  -- Lâmpada (GPIO 3)
  );
end top_neorv32;

architecture rtl of top_neorv32 is

  -- --- Sinais Wishbone (XBUS) usando tipos do NEORV32 ---
  signal wb_adr       : std_ulogic_vector(31 downto 0);
  signal wb_dat_write : std_ulogic_vector(31 downto 0);
  signal wb_dat_read  : std_ulogic_vector(31 downto 0);
  signal wb_we        : std_ulogic;
  signal wb_sel       : std_ulogic_vector(3 downto 0);
  signal wb_stb       : std_ulogic;
  signal wb_cyc       : std_ulogic;
  signal wb_ack       : std_ulogic;

  -- --- Sinais GPIO ---
  signal gpio_out     : std_ulogic_vector(31 downto 0);
  signal gpio_in      : std_ulogic_vector(31 downto 0);

  -- --- Registradores e Sinais Internos ---
  signal led_reg      : std_ulogic := '0';

begin

  -- --- Mapeamento de Saídas (Lógica Inversa para a Placa) ---
  led_lock           <= not led_reg;
  led_alarm          <= not gpio_out(1);
  led_arcondicionado <= not gpio_out(2);
  led_lampada        <= not gpio_out(3);
  
  -- --- Mapeamento de Entradas GPIO ---
  gpio_in(0) <= btn_fire;     
  gpio_in(1) <= btn_intruder;
  gpio_in(31 downto 2) <= (others => '0');

  -- --- Instância do NEORV32 ---
  neorv32_top_inst: neorv32_top
  generic map (
    CLOCK_FREQUENCY   => 50000000,
    BOOT_MODE_SELECT  => 0,
    IMEM_EN           => true,
    IMEM_SIZE         => 8192,
    DMEM_EN           => true,
    DMEM_SIZE         => 8192,
    IO_UART0_EN       => true,
    IO_GPIO_NUM       => 4,
    RISCV_ISA_Zkne    => true, -- Necessário para AES
    RISCV_ISA_Zknd    => true, -- Habilita Decrypt
    XBUS_EN           => true
  )
  port map (
    clk_i       => clk,
    rstn_i      => rst_n,
    uart0_txd_o => uart_txd,
    uart0_rxd_i => uart_rxd,
    gpio_o      => gpio_out,
    gpio_i      => gpio_in,
    xbus_adr_o  => wb_adr,
    xbus_dat_o  => wb_dat_write,
    xbus_dat_i  => wb_dat_read,
    xbus_we_o   => wb_we,
    xbus_sel_o  => wb_sel,
    xbus_stb_o  => wb_stb,
    xbus_cyc_o  => wb_cyc,
    xbus_ack_i  => wb_ack,
    xbus_err_i  => '0'
  );

  -- --- Lógica XBUS (Periférico Customizado) ---
  wb_dat_read <= (others => '0'); -- Apenas escrita neste periférico básico

  xbus_logic: process(clk, rst_n)
  begin
    if rst_n = '0' then
      wb_ack  <= '0';
      led_reg <= '0';
    elsif rising_edge(clk) then
      wb_ack <= '0';
      -- Ciclo de escrita detectado
      if (wb_cyc = '1' and wb_stb = '1' and wb_ack = '0') then
        wb_ack <= '1';
        -- Endereço base (filtrando os 8 bits inferiores como no Verilog original)
        if (wb_we = '1' and wb_adr(7 downto 0) = x"00") then
          led_reg <= wb_dat_write(0);
        end if;
      end if;
    end if;
  end process;

end rtl;