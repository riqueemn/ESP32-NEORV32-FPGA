#include <neorv32.h>

// Endereço do seu periférico LED via XBUS
#define LED_XBUS_ADDR (*((volatile uint32_t*) (0x90000000)))

// Definição de pinos do GPIO (IO_GPIO_NUM(2))
#define PIN_BTN_FIRE    0  // gpio_i(0)
#define PIN_BTN_INTRUDER 1 // gpio_i(1)
#define PIN_ALARM_LED   1  // gpio_o(1)

typedef enum {
    ESTADO_NORMAL,
    ESTADO_INCENDIO,
    ESTADO_INTRUSAO
} t_sistema;

int main() {
    // Inicializa a UART0 para debug (19200 baud)
    neorv32_uart0_setup(19200, PARITY_NONE, FLOW_CONTROL_NONE);
    
    // Garante que o GPIO está pronto
    neorv32_gpio_setup();

    t_sistema status = ESTADO_NORMAL;
    
    neorv32_uart0_puts("\n--- Sistema de Automacao Residencial Iniciado ---\n");
    neorv32_uart0_puts("Comandos: [r] Resetar Alarme | [f] Abrir Forcado\n");

    while (1) {
        // 1. LEITURA DOS SENSORES (Botões da FPGA)
        // gpio_in[0] é o sensor de fogo, gpio_in[1] é o de intrusão
        if (neorv32_gpio_pin_get(PIN_BTN_FIRE)) {
            status = ESTADO_INCENDIO;
        } else if (neorv32_gpio_pin_get(PIN_BTN_INTRUDER) && status == ESTADO_NORMAL) {
            status = ESTADO_INTRUSAO;
        }

        // 2. LEITURA DE COMANDOS DA UART (Simulando o ESP32)
        if (neorv32_uart0_available()) {
            char cmd = neorv32_uart0_getc();
            if (cmd == 'r') {
                status = ESTADO_NORMAL;
                neorv32_uart0_puts("Sistema resetado para modo normal.\n");
            } else if (cmd == 'f') {
                neorv32_uart0_puts("Comando remoto: Abrindo porta...\n");
                LED_XBUS_ADDR = 1; // Liga LED via XBUS
                neorv32_cpu_delay_ms(3000); // Mantém aberta por 3s
                LED_XBUS_ADDR = 0;
            }
        }

        // 3. MÁQUINA DE ESTADOS (Ações dos Atuadores)
        switch (status) {
            case ESTADO_NORMAL:
                LED_XBUS_ADDR = 0;           // Fechadura trancada
                neorv32_gpio_pin_clr(PIN_ALARM_LED); // Alarme desligado
                break;

            case ESTADO_INCENDIO:
                LED_XBUS_ADDR = 1;           // Abre porta por segurança
                neorv32_gpio_pin_set(PIN_ALARM_LED); // Pisca alarme (seria uma sirene)
                neorv32_uart0_puts("!!! ALERTA DE INCENDIO !!!\n");
                neorv32_cpu_delay_ms(100);
                neorv32_gpio_pin_clr(PIN_ALARM_LED);
                neorv32_cpu_delay_ms(100);
                break;

            case ESTADO_INTRUSAO:
                neorv32_gpio_pin_set(PIN_ALARM_LED); // Toca alarme
                neorv32_uart0_puts("ALERTA: Intruso detectado!\n");
                neorv32_cpu_delay_ms(500);
                break;
        }
    }

    return 0;
}