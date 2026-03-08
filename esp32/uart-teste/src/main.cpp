#include <Arduino.h>
#include <HardwareSerial.h>
#include <LittleFS.h>
#include "BluetoothSerial.h"

#define FPGA_SERIAL Serial2 
HardwareSerial MySerial(2);
BluetoothSerial SerialBT;

/* ===== Comandos Cifrados Fixos (Conforme solicitado) ===== */
const uint8_t CMD_ALARME_ON[16]  = {0x04, 0x0c, 0xfe, 0xb7, 0xe9, 0x73, 0xa7, 0x64, 0x75, 0xf8, 0xce, 0x2a, 0x02, 0xbe, 0xd9, 0xf2};
const uint8_t CMD_ALARME_OFF[16] = {0xb0, 0x46, 0x27, 0xab, 0xd6, 0xe3, 0x7d, 0xc9, 0x00, 0x33, 0x8b, 0x28, 0xb5, 0xb1, 0x37, 0x99};
const uint8_t CMD_AC_ON[16]      = {0xed, 0x9f, 0x55, 0xf1, 0x9c, 0xeb, 0x26, 0xdf, 0x12, 0x18, 0x0d, 0x38, 0xd1, 0x86, 0x17, 0x07};
const uint8_t CMD_AC_OFF[16]     = {0x75, 0x66, 0x66, 0x44, 0x52, 0xad, 0x7f, 0xb8, 0xb4, 0x4d, 0x57, 0x8a, 0x58, 0x2b, 0x97, 0x2d};
const uint8_t CMD_LUZ_ON[16]     = {0xfb, 0x32, 0xe2, 0x12, 0xf2, 0xdc, 0x22, 0x88, 0x68, 0x42, 0x85, 0x46, 0xc8, 0xc4, 0x5d, 0xc7};
const uint8_t CMD_LUZ_OFF[16]    = {0x8d, 0x56, 0x26, 0xe5, 0x3f, 0xe6, 0x40, 0x75, 0x0a, 0x66, 0xe9, 0xfd, 0xde, 0x17, 0x84, 0x97};

/* ===== Buffers e Controle ===== */
uint8_t rxBuffer[16];
uint8_t rxIndex = 0;
bool waitingResponse = false;
bool modoGateway = false;

// Função para enviar o pacote de 16 bytes selecionado
void enviarComando(const uint8_t* cmd) {
    Serial.println(">>> Enviando pacote AES para FPGA...");
    // Limpeza de lixo na UART
    while(MySerial.available()) MySerial.read(); 

    for (uint8_t i = 0; i < 16; i++) {
        MySerial.write(cmd[i]);
        Serial.println(cmd[i], HEX);
        delay(5); 
    }
    waitingResponse = true;
    rxIndex = 0;
}

void setup() {
    Serial.begin(115200);
    MySerial.begin(19200, SERIAL_8N1, 16, 17);
    SerialBT.begin("ESP32_NEORV32_AES");

    if (!LittleFS.begin(true)) {
        Serial.println("Erro ao montar o LittleFS!");
    }

    Serial.println("\n--- ESP32 GATEWAY PRONTO ---");
    Serial.println("G: Upload Binário | 1-6: Comandos Automação");
}

/* ===== Sua Função de Upload (Mantida Intacta) ===== */
void iniciarUploadAutomatico() {
    File file = LittleFS.open("/neorv32_exe.bin", "r");
    if (!file || file.size() == 0) {
        Serial.println("Erro: Arquivo neorv32_exe.bin nao encontrado!");
        if (file) file.close();
        return;
    }

    Serial.println("\n[1/3] Solicitando modo Upload ('u')...");
    bool prontoParaEnviar = false;
    unsigned long timeout = millis();
    while (!prontoParaEnviar && (millis() - timeout < 5000)) {
        MySerial.write('u');
        delay(250);
        if (MySerial.available()) {
            String resposta = MySerial.readString();
            if (resposta.indexOf("Awaiting") != -1) prontoParaEnviar = true;
        }
    }

    if (!prontoParaEnviar) {
        Serial.println("Erro: FPGA nao respondeu.");
        file.close();
        return;
    }

    Serial.printf("[2/3] Enviando %d bytes...\n", file.size());
    while (file.available()) {
        uint8_t buffer[256];
        size_t len = file.read(buffer, sizeof(buffer));
        MySerial.write(buffer, len);
        delay(5); 
    }
    file.close();

    Serial.println("\n[3/3] Aguardando OK final...");
    timeout = millis();
    while (millis() - timeout < 3000) {
        if (MySerial.available()) {
            String resp = MySerial.readString();
            if (resp.indexOf("OK") != -1) {
                Serial.println("Sucesso! Executando e ativando Bluetooth Gateway...");
                MySerial.write('e'); 
                delay(500);
                while(MySerial.available()) MySerial.read(); 
                modoGateway = true;
                return;
            }
        }
    }
    modoGateway = true;
}

void loop() {
    // 1. Comando de Boot (USB)
    if (Serial.available()) {
        char c = Serial.read();
        if (c == 'G' || c == 'g') iniciarUploadAutomatico();
    }

    // 2. BLUETOOTH -> FPGA (Comandos 1 a 6)
    if (!waitingResponse && SerialBT.available()) {
        char c = SerialBT.read();
        
        // Se for um comando de 1 a 6, envia o array fixo correspondente
        if (c >= '1' && c <= '6') {
            switch(c) {
                case '1': Serial.println("BT: Alarme ON");  enviarComando(CMD_ALARME_ON);  break;
                case '2': Serial.println("BT: Alarme OFF"); enviarComando(CMD_ALARME_OFF); break;
                case '3': Serial.println("BT: AC ON");      enviarComando(CMD_AC_ON);      break;
                case '4': Serial.println("BT: AC OFF");     enviarComando(CMD_AC_OFF);     break;
                case '5': Serial.println("BT: Luz ON");     enviarComando(CMD_LUZ_ON);     break;
                case '6': Serial.println("BT: Luz OFF");    enviarComando(CMD_LUZ_OFF);    break;
            }
        }
    }

    // 3. RESPOSTA FPGA -> BLUETOOTH/USB
    if (waitingResponse) {
        if (MySerial.available()) {
            rxBuffer[rxIndex++] = MySerial.read();

            if (rxIndex == 16) {
                Serial.println("<<< Resposta recebida da FPGA (Status):");
                for (uint8_t i = 0; i < 16; i++) {
                    if (rxBuffer[i] < 0x10) Serial.print("0");
                    Serial.print(rxBuffer[i], HEX);
                    Serial.print(" ");
                    SerialBT.print(rxBuffer[i], HEX); // Opcional: envia o eco para o celular
                }
                Serial.println();
                SerialBT.println(" [COMANDO OK]");
                
                rxIndex = 0;
                waitingResponse = false;
            }
        }
    }
}