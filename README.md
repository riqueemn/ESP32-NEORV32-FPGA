# ESP32-NEORV32 Bridge & Wireless Gateway 🚀

Este projeto consiste em um sistema de gerenciamento e comunicação para o processador **NEORV32 (RISC-V)** implementado em uma FPGA **Altera Cyclone IV**. O ESP32 atua como o "Service Processor", sendo responsável pelo armazenamento do firmware, execução do bootloader via UART e servindo como uma ponte (Bridge) de comunicação sem fio via Bluetooth.

## 📋 Funcionalidades

- **Auto-Bootloader via LittleFS**: Armazena o executável binário (`neorv32_exe.bin`) na memória Flash do ESP32 e realiza o upload automático para a RAM da FPGA.
- **Handshake de Inicialização**: Protocolo de comunicação robusto que sincroniza o estado da FPGA com o ESP32 (`u` para upload, `Awaiting` para pronto, `OK` para sucesso e `e` para execução).
- **Bluetooth Gateway**: Após o boot, o ESP32 estabelece uma ponte transparente entre o Bluetooth Classic e a UART2, permitindo o controle sem fio do processador RISC-V.
- **Ponte de Debug**: Interface de monitoramento duplo que repassa as mensagens da FPGA simultaneamente para o monitor Serial USB e para o dispositivo Bluetooth conectado.

## 🛠️ Arquitetura do Sistema



### Conexões de Hardware
- **UART0 (USB)**: Comunicação com o PC para monitoramento (115200 bps).
- **UART2 (GPIO 16/17)**: Interface de alta velocidade com a FPGA Cyclone IV (19200 bps).
- **Bluetooth Classic**: Canal de dados sem fio para controle via smartphone ou tablet.

## 💻 Tecnologias Utilizadas

- **Linguagem**: C++ (Framework Arduino)
- **Sistema de Arquivos**: LittleFS
- **Arquitetura Alvo**: NEORV32 (RISC-V) na Altera Cyclone IV
- **Ambiente de Desenvolvimento**: PlatformIO

## 📂 Estrutura do Repositório

- `/src`: Código fonte principal (`main.cpp`).
- `/data`: Pasta contendo o binário compilado do NEORV32 (`neorv32_exe.bin`).
- `platformio.ini`: Configurações de partição e sistema de arquivos.

## 🚀 Como Utilizar

1. **Configuração do Ambiente**:
   Certifique-se de que o PlatformIO está configurado para usar LittleFS no arquivo `platformio.ini`.

2. **Upload do Sistema de Arquivos**:
   - Coloque o seu arquivo `neorv32_exe.bin` dentro da pasta `data`.
   - No PlatformIO, execute a tarefa `Build Filesystem Image`.
   - Em seguida, execute `Upload Filesystem Image`.

3. **Upload do Firmware**:
   - Realize o upload normal do código para o ESP32.

4. **Operação**:
   - Conecte o dispositivo via Bluetooth (**ESP32_FPGA_Gateway**).
   - No monitor serial, envie o caractere `G` para disparar a rotina de boot.
   - O ESP32 fará o upload e ativará automaticamente o modo Gateway.

---

## 📄 Licença

Este projeto está sob a licença MIT - veja o arquivo [LICENSE](LICENSE) para detalhes.
