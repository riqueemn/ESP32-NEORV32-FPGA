#include <neorv32.h>

// Endereço do seu periférico LED via XBUS
#define LED_XBUS_ADDR (*((volatile uint32_t*) (0x90000000)))

// Definição de pinos do GPIO
#define PIN_BTN_FIRE     0
#define PIN_BTN_INTRUDER 1
#define PIN_ALARM_LED    1

typedef enum {
    ESTADO_NORMAL,
    ESTADO_INCENDIO,
    ESTADO_INTRUSAO
} t_sistema;

// Função de delay customizada usando a base de tempo do sistema
void delay_ms(uint32_t time_ms) {
    neorv32_aux_delay_ms(neorv32_sysinfo_get_clk(), time_ms);
}

int main() {
    
    neorv32_rte_setup();

    // UART: baudrate e irq_mask (0 = sem interrupções)
    neorv32_uart0_setup(19200, 0);
    
    t_sistema status = ESTADO_NORMAL;
    
    neorv32_uart0_puts("\n--- Sistema de Automacao Residencial Iniciado ---\n");
    neorv32_uart0_puts("Comandos: [r] Resetar Alarme | [f] Abrir Forcado\n");

    while (1) {
        // 1. LEITURA DOS SENSORES (Lógica Invertida: 0 = Ativado)
        if (neorv32_gpio_pin_get(PIN_BTN_FIRE) == 0) {
            status = ESTADO_INCENDIO;
            neorv32_uart0_puts("\n[ALERTA] Sensor de Fogo Ativado!\n");
        } 
        else if (neorv32_gpio_pin_get(PIN_BTN_INTRUDER) == 0 && status == ESTADO_NORMAL) {
            status = ESTADO_INTRUSAO;
            neorv32_uart0_puts("\n[ALERTA] Sensor de Intrusao Ativado!\n");
        }

        // 2. COMANDOS UART
        if (neorv32_uart0_available()) {
            char cmd = neorv32_uart0_getc();
			neorv32_uart0_puts(cmd+"");
			//char cmd = 'a';
			//char cmd = (char)neorv32_uart_getc(NEORV32_UART0);
            if (cmd == 'r') {
                status = ESTADO_NORMAL;
                neorv32_uart0_puts("Sistema resetado. Monitorando...\n");
            }
            else if (cmd == 'f') {
                neorv32_uart0_puts("Comando remoto: Abrindo porta...\n");
                LED_XBUS_ADDR = 1; 
                delay_ms(100); // Mantém aberta por 3s
                LED_XBUS_ADDR = 0;
            }
        }

        // 3. MÁQUINA DE ESTADOS (Atuadores)
        switch (status) {
            case ESTADO_NORMAL:
				neorv32_uart0_puts("\nESTADO_NORMAL\n");
                LED_XBUS_ADDR = 0;
                neorv32_gpio_pin_set(PIN_ALARM_LED, 0);
                break;
                
            case ESTADO_INCENDIO:
				neorv32_uart0_puts("\ESTADO_INCENDIO\n");
                //LED_XBUS_ADDR = 1; // Abre a porta
                neorv32_gpio_pin_set(PIN_ALARM_LED, 1); // Liga alarme
                delay_ms(100); // Pisca o alarme
                neorv32_gpio_pin_set(PIN_ALARM_LED, 0);
                delay_ms(100);
                break;

            case ESTADO_INTRUSAO:
				neorv32_uart0_puts("\ESTADO_INTRUSAO\n");
                neorv32_gpio_pin_set(PIN_ALARM_LED, 1); // Alarme fixo
                break;
        }

        // Pequeno delay global para evitar "spam" no terminal e debounce
        delay_ms(50); 
    }

    return 0; // O programa nunca chega aqui, mas é boa prática
}