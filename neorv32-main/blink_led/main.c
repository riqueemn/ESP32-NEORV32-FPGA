#include <neorv32.h>

// Definimos o endereço que criamos no VHDL
// 0x90000000 é o início do espaço para dispositivos externos (XBUS)
#define LED_ADDR (*((volatile uint32_t*) (0x90000000)))

int main() {

  // 1. Inicializa a UART0 para mensagens de debug
  // O bootloader já faz isso, mas é boa prática garantir a config:
  // baudrate padrão, sem paridade, 8 bits de dados
  neorv32_uart0_setup(19200, PARITY_NONE, FLOW_CONTROL_NONE);

  // 2. Envia uma mensagem de boas-vindas
  neorv32_uart0_puts("\n--- Projeto TRIARE / Casa Inteligente Ativo ---\n");
  neorv32_uart0_puts("Iniciando o blink do LED via barramento XBUS...\n");

  while (1) {
    // Liga o LED (escreve 1 no bit 0 do endereço 0x90000000)
    LED_ADDR = 0x00000001;
    neorv32_uart0_puts("LED Ligado\n");
    neorv32_cpu_delay_ms(500); // Espera 500ms

    // Desliga o LED (escreve 0)
    LED_ADDR = 0x00000000;
    neorv32_uart0_puts("LED Desligado\n");
    neorv32_cpu_delay_ms(500);
  }

  return 0; // O programa nunca chega aqui
}