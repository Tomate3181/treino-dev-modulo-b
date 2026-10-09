/*
  =============================================================================
  EstufaSmart - Firmware de Monitoramento e Controle Ambiental
  Dispositivo: Arduino UNO R4 WiFi
  
  Descrição:
  - Monitoramento contínuo de temperatura, umidade (DHT11), luminosidade (LDR)
    e status da janela (Reed Switch).
  - Máquina de Estados com 5 modos: NORMAL, ATENÇÃO, CRÍTICO, RECONHECIDO e FALHA_SENSOR.
  - Atuação nos periféricos (LEDs Verde/Amarelo/Vermelho, Buzzer e Relé de Ventilação).
  - Exibição de letras de status na Matriz de LED nativa (12x8) do UNO R4.
  - Fila circular (Ring Buffer) de tolerância a falhas de rede (mínimo 20 registros).
  - Telemetria HTTP POST em formato JSON para servidor Python (FastAPI).
  - Execução 100% não-bloqueante utilizando millis() (sem chamadas a delay() no loop).
  =============================================================================
*/

#include <Arduino.h>
#include "secrets.h"
#include "Sensors.h"
#include "StateMachine.h"
#include "DisplayMatrix.h"
#include "NetworkManager.h"

// =============================================================================
// Instâncias dos Módulos da Arquitetura
// =============================================================================
Sensors        sensors;
StateMachine   stateMachine;
DisplayMatrix  displayMatrix;
NetworkManager networkManager;

// =============================================================================
// Variáveis de Controle e Temporização Não-Bloqueante (RF-01)
// =============================================================================
const unsigned long SENSOR_CYCLE_INTERVAL_MS = 5000; // Ciclo de 5 segundos
unsigned long lastSensorCycleTime = 0;
unsigned long telemetrySequence = 0; // Contador sequencial (seq)

void setup() {
    // Inicialização da porta serial para depuração
    Serial.begin(115200);
    while (!Serial && millis() < 2500) {
        // Aguarda conexão da porta serial por no máximo 2.5s
    }
    Serial.println(F("\n=================================================="));
    Serial.println(F("    EstufaSmart - Arduino UNO R4 WiFi Iniciado    "));
    Serial.println(F("=================================================="));

    // Inicialização dos módulos do sistema
    sensors.begin();
    displayMatrix.begin();
    stateMachine.begin();
    networkManager.begin();

    // Primeira atualização visual da matriz de LEDs
    displayMatrix.displayState(stateMachine.getStateChar());

    Serial.println(F("[Setup] Todos os módulos foram inicializados com sucesso."));
    Serial.println(F("[Setup] Executando loop de controle não-bloqueante...\n"));
}

void loop() {
    unsigned long currentMillis = millis();

    // 1. Verificação contínua do botão de reconhecimento com debounce (D2)
    bool buttonPressed = sensors.checkButtonPressed();
    if (buttonPressed) {
        Serial.println(F("[Hardware] Botão de Reconhecimento pressionado!"));
    }

    // 2. Ciclo principal periódico de 5 segundos (RF-01, RF-02, RF-03 e RF-07)
    if (currentMillis - lastSensorCycleTime >= SENSOR_CYCLE_INTERVAL_MS) {
        lastSensorCycleTime = currentMillis;

        // Leitura e normalização de todos os sensores
        SensorReadings readings = sensors.readAll();

        // Atualização da lógica da Máquina de Estados (RF-04)
        stateMachine.update(readings, buttonPressed, currentMillis);

        // Incremento do contador de sequência
        telemetrySequence++;

        // Montagem do registro de telemetria
        TelemetryRecord record;
        record.seq = telemetrySequence;
        record.temp = readings.temperature;
        record.umid = readings.humidity;
        record.janela = readings.windowOpen;
        record.luz_pct = readings.lightPercent;
        strncpy(record.estado, stateMachine.getStateString(), sizeof(record.estado) - 1);
        record.estado[sizeof(record.estado) - 1] = '\0';

        // Log detalhado no monitor serial
        Serial.println(F("--------------------------------------------------"));
        Serial.print(F("Ciclo #")); Serial.println(record.seq);
        Serial.print(F("Temperatura: ")); Serial.print(record.temp, 1); Serial.print(F(" °C | Umidade: ")); Serial.print(record.umid, 1); Serial.println(F(" %"));
        Serial.print(F("Luminosidade: ")); Serial.print(record.luz_pct); Serial.print(F(" % | Janela: ")); Serial.println(record.janela ? F("ABERTA") : F("FECHADA"));
        Serial.print(F("Estado Atual: ")); Serial.print(record.estado); Serial.print(F(" [Display: '")); Serial.print(stateMachine.getStateChar()); Serial.println(F("']"));
        if (!readings.dhtValid) {
            Serial.print(F("[ALERTA] Falhas consecutivas no DHT11: "));
            Serial.println(readings.consecutiveDhtErrors);
        }

        // Enfileira telemetria no Buffer Circular (RF-08)
        networkManager.queueTelemetry(record);

        Serial.print(F("Buffer Pendente: "));
        Serial.print(networkManager.getPendingCount());
        Serial.print(F(" leituras | Wi-Fi: "));
        Serial.println(networkManager.isConnected() ? F("CONECTADO") : F("DESCONECTADO (Bufferizando)"));
        Serial.println(F("--------------------------------------------------"));
    } else {
        // Se o botão for pressionado fora do intervalo exato de 5s, repassa à FSM
        if (buttonPressed) {
            SensorReadings fastReadings = sensors.readAll();
            stateMachine.update(fastReadings, buttonPressed, currentMillis);
        }
    }

    // 3. Atualização contínua dos Atuadores e Display (Buzzer temporizado sem bloqueio)
    stateMachine.updateActuators(sensors, displayMatrix, currentMillis);

    // 4. Manutenção de Conexão Wi-Fi (reconexão não-bloqueante se desconectado)
    networkManager.maintainConnection(currentMillis);

    // 5. Descarregamento e retransmissão de registros do Ring Buffer via HTTP POST
    networkManager.processOutgoingQueue();
}
