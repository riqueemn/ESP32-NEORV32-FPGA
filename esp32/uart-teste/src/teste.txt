#include <Arduino.h>
#include "BluetoothSerial.h"

constexpr int RXD2 = 16;
constexpr int TXD2 = 17;

BluetoothSerial SerialBT;

/* ===== Buffers ===== */
uint8_t txBuffer[16];
uint8_t rxBuffer[16];

uint8_t txIndex = 0;
uint8_t rxIndex = 0;

char hexPair[2];
uint8_t hexPairIndex = 0;

bool waitingResponse = false;

/* ===== Utils ===== */
uint8_t hexCharToValue(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return 0;
}

void flushBluetooth() {
    while (SerialBT.available()) {
        SerialBT.read();
    }
}

void flushUART() {
    while (Serial2.available()) {
        Serial2.read();
    }
}

void setup() {
    Serial.begin(115200);
    Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
    SerialBT.begin("ESP32_BT_UART");

    Serial.println("=================================");
    Serial.println("Envie 16 bytes em HEX (32 chars)");
    Serial.println("Ex: 00112233445566778899AABBCCDDEEFF");
    Serial.println("=================================");
}

void loop() {

    // ==============================
    // BLUETOOTH → UART
    // ==============================
    if (!waitingResponse) {

        while (SerialBT.available()) {

            char c = SerialBT.read();

            if (c == '\n' || c == '\r' || c == ' ')
                continue;

            hexPair[hexPairIndex++] = c;

            if (hexPairIndex == 2) {
                uint8_t byteValue =
                    (hexCharToValue(hexPair[0]) << 4) |
                     hexCharToValue(hexPair[1]);

                hexPairIndex = 0;
                txBuffer[txIndex++] = byteValue;

                Serial.print("BT RX: 0x");
                Serial.println(byteValue, HEX);

                if (txIndex == 16) {

                    Serial.println(">>> Enviando pacote para FPGA");

                    flushUART();

                    for (uint8_t i = 0; i < 16; i++){
                        Serial2.write(txBuffer[i]);
                        delay(10);
                    }
                    Serial2.flush();
                    txIndex = 0;
                    rxIndex = 0;
                    delay(50);
                    waitingResponse = true;
                    break;
                }
            }
        }
    }

    // ==============================
    // UART ← FPGA
    // ==============================
    if (waitingResponse) {

        while (Serial2.available()) {

            rxBuffer[rxIndex++] = Serial2.read();

            if (rxIndex == 16) {

                Serial.println("<<< Resposta AES da FPGA:");

                for (uint8_t i = 0; i < 16; i++) {
                    if (rxBuffer[i] < 0x10) Serial.print("0");
                    Serial.print(rxBuffer[i], HEX);
                    Serial.print(" ");
                }

                Serial.println();

                rxIndex = 0;
                waitingResponse = false;
                break;
            }
        }
    }
}
