#include <Arduino.h>
#include <HardwareSerial.h>
#include <LittleFS.h>
#include "BluetoothSerial.h" // Biblioteca para Bluetooth Classic

#define FPGA_SERIAL Serial2 
HardwareSerial MySerial(2);
BluetoothSerial SerialBT; // Objeto Bluetooth

bool modoGateway = false; // Controla quando o Bluetooth assume o controle

void iniciarUploadAutomatico();

void setup() {
  Serial.begin(115200);
  MySerial.begin(19200, SERIAL_8N1, 16, 17);

  // Nome do dispositivo Bluetooth que aparecerá no celular
  if (!SerialBT.begin("ESP32_FPGA_Gateway")) {
    Serial.println("Erro ao iniciar Bluetooth!");
  } else {
    Serial.println("Bluetooth pronto! Nome: ESP32_FPGA_Gateway");
  }

  if (!LittleFS.begin(true)) {
    Serial.println("Erro ao montar o LittleFS!");
    return;
  }

  Serial.println("\n--- Sistema de Auto-Boot + Bluetooth ---");
  Serial.println("Envie 'G' (Serial) ou conecte via Bluetooth para iniciar.");
}



void iniciarUploadAutomatico() {
  File file = LittleFS.open("/neorv32_exe.bin", "r");
  if (!file || file.size() == 0) {
    Serial.println("Erro: Arquivo neorv32_exe.bin nao encontrado!");
    if (file) file.close();
    return;
  }

  Serial.println("\n[1/3] Solicitando modo Upload ('u')...");
  
  // Handshake
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

  // Envio do Binário
  Serial.printf("[2/3] Enviando %d bytes...\n", file.size());
  while (file.available()) {
    uint8_t buffer[256];
    size_t len = file.read(buffer, sizeof(buffer));
    MySerial.write(buffer, len);
    Serial.print("#");
    delay(5); 
  }
  file.close();

  // Execução
  Serial.println("\n[3/3] Aguardando OK final...");
  timeout = millis();
  while (millis() - timeout < 3000) {
    if (MySerial.available()) {
      String resp = MySerial.readString();
      if (resp.indexOf("OK") != -1) {
        Serial.println("Sucesso! Executando e ativando Bluetooth Gateway...");
        MySerial.write('e'); 
        modoGateway = true; // Agora a ponte no loop() está liberada
        return;
      }
    }
  }
  Serial.println("Upload concluído (sem confirmação de OK, mas Gateway ativo).");
  modoGateway = true;
}


void loop() {
  // 1. Comando de Upload via Serial USB
  if (Serial.available()) {
    char c = Serial.read();
    if (c == 'G' || c == 'g') iniciarUploadAutomatico();
  }

  // 2. Ponte de Dados (Após o Upload ou se o Gateway estiver ativo)
  // Repassa dados: Bluetooth -> FPGA
  if (SerialBT.available()) {
    char bData = SerialBT.read();
    MySerial.write(bData);
    // Opcional: ver no Serial Monitor o que está passando
    // Serial.print("BT->FPGA: "); Serial.println(bData);
  }

  // Repassa dados: FPGA -> Bluetooth (e Serial USB para debug)
  if (MySerial.available()) {
    char fData = MySerial.read();
    SerialBT.write(fData); // Envia para o Celular
    Serial.write(fData);   // Envia para o PC (Debug)
  }
}