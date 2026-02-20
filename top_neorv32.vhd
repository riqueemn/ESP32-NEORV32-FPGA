library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

library neorv32;
use neorv32.neorv32_package.all;

entity top_neorv32 is
  port (
    clk       : in  std_logic; 
    rst_n     : in  std_logic; 
    uart_txd  : out std_logic; 
    uart_rxd  : in  std_logic; 
    led_out   : out std_logic  
  );
end entity;

architecture rtl of top_neorv32 is

  -- Sinais do Barramento Wishbone (XBUS)
  signal wb_adr        : std_ulogic_vector(31 downto 0);
  signal wb_dat_read   : std_ulogic_vector(31 downto 0);
  signal wb_dat_write  : std_ulogic_vector(31 downto 0);
  signal wb_we         : std_ulogic;
  signal wb_sel        : std_ulogic_vector(3 downto 0);
  signal wb_stb        : std_ulogic;
  signal wb_cyc        : std_ulogic;
  signal wb_ack        : std_ulogic;

  -- Registrador para armazenar o estado do LED
  signal led_reg       : std_logic := '0';
  signal uart_tx_sig   : std_ulogic; 

begin

  -- CONEXÃO DO LED:
  -- Aplicando a lógica invertida (pino em '0' liga o LED)
  -- Agora o LED obedece ao registrador led_reg controlado pelo C
  led_out <= not led_reg; 
  
  -- Saída da UART
  uart_txd <= std_logic(uart_tx_sig);

  -- Instância do Processador NEORV32
  neorv32_top_inst: entity work.neorv32_top
  generic map (
    CLOCK_FREQUENCY   => 50000000,   -- 50 MHz
    BOOT_MODE_SELECT  => 0,          -- Modo Bootloader
    IMEM_EN           => true,
    IMEM_SIZE         => 16384,      -- 16 KB
    DMEM_EN           => true,
    DMEM_SIZE         => 8192,       -- 8 KB
    IO_UART0_EN       => true,
    XBUS_EN           => true        -- Ativa barramento externo
  )
  port map (
    clk_i          => std_ulogic(clk),
    rstn_i         => std_ulogic(rst_n),
    uart0_txd_o    => uart_tx_sig,
    uart0_rxd_i    => std_ulogic(uart_rxd),
    xbus_adr_o     => wb_adr,
    xbus_dat_o     => wb_dat_write,
    xbus_dat_i     => wb_dat_read,
    xbus_we_o      => wb_we,
    xbus_sel_o     => wb_sel,
    xbus_stb_o     => wb_stb,
    xbus_cyc_o     => wb_cyc,
    xbus_ack_i     => wb_ack,
    xbus_err_i     => '0'
  );

  -- Lógica do Periférico Customizado (LED no XBUS)
  -- Este processo escuta o barramento e salva o valor no led_reg
  process(clk, rst_n)
  begin
    if rst_n = '0' then
      wb_ack      <= '0';
      wb_dat_read <= (others => '0');
      led_reg     <= '0'; -- LED começa desligado
    elsif rising_edge(clk) then
      wb_ack <= '0'; -- Padrão: sem resposta
      
      -- Verifica se o processador está tentando acessar o barramento externo
      if (wb_cyc = '1' and wb_stb = '1') then
        wb_ack <= '1'; -- Responde que recebeu o comando
        
        -- Se for uma operação de ESCRITA (write enable)
        if (wb_we = '1') then
          -- Pegamos o bit 0 do que o processador enviou e salvamos no LED
          led_reg <= std_logic(wb_dat_write(0));
        end if;
        
        wb_dat_read <= (others => '0'); 
      end if;
    end if;
  end process;

end architecture;