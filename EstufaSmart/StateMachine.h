#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include <Arduino.h>
#include "Sensors.h"
#include "DisplayMatrix.h"

// Enumeração dos estados da máquina de estados (RF-04)
enum SystemState {
    STATE_NORMAL,
    STATE_ATENCAO,
    STATE_CRITICO,
    STATE_RECONHECIDO,
    STATE_FALHA_SENSOR
};

// Constantes de Limiares Ambientais
const float TEMP_IDEAL_MIN = 20.0f;     // Faixa ideal: 20°C a 28°C
const float TEMP_IDEAL_MAX = 28.0f;
const float TEMP_MARGIN_ALERT = 1.5f;   // Margem para "próxima do limite" (18.5°C a 20°C ou 28°C a 29.5°C)
const int   LIGHT_HIGH_THRESHOLD = 80;  // Luminosidade muito alta (> 80%)

// Constantes de Temporização (milissegundos)
const unsigned long DURATION_TEMP_CRITICAL_MS   = 30000; // > 30 segundos fora da faixa
const unsigned long DURATION_WINDOW_ALERT_MS    = 20000; // Janela aberta > 20 segundos
const unsigned long DURATION_WINDOW_CRITICAL_MS = 60000; // Janela aberta > 60 segundos
const unsigned long BEEP_INTERVAL_ALERT_MS      = 10000; // Bipe a cada 10 segundos no estado ATENÇÃO
const unsigned long BEEP_DURATION_ALERT_MS      = 200;   // Duração do pulso do bipe sonoro

class StateMachine {
public:
    StateMachine();
    void begin();

    // Atualiza a lógica de transição de estados baseado nos sensores e temporizadores
    void update(const SensorReadings& readings, bool buttonPressed, unsigned long currentMillis);

    // Atualiza os atuadores físicos e a matriz LED de acordo com o estado corrente
    void updateActuators(Sensors& sensors, DisplayMatrix& matrix, unsigned long currentMillis);

    // Getters de estado
    SystemState getState() const;
    const char* getStateString() const;
    char getStateChar() const;

private:
    SystemState _currentState;

    // Rastreamento temporal de condições anômalas
    unsigned long _tempOutOfRangeStartTime;
    unsigned long _windowOpenStartTime;

    // Temporização do bipe de atenção (não-bloqueante)
    unsigned long _lastBeepCycleTime;
    unsigned long _beepPulseStartTime;
    bool _isBeeping;

    // Avaliação de condições auxiliares
    bool isTempIdeal(float temp) const;
    bool isTempNearLimit(float temp) const;
    bool isTempOutOfRange(float temp) const;
};

#endif // STATE_MACHINE_H
