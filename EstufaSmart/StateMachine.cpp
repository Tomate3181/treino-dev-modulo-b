#include "StateMachine.h"

StateMachine::StateMachine()
    : _currentState(STATE_NORMAL),
      _tempOutOfRangeStartTime(0),
      _windowOpenStartTime(0),
      _lastBeepCycleTime(0),
      _beepPulseStartTime(0),
      _isBeeping(false) {
}

void StateMachine::begin() {
    _currentState = STATE_NORMAL;
    _tempOutOfRangeStartTime = 0;
    _windowOpenStartTime = 0;
    _lastBeepCycleTime = 0;
    _beepPulseStartTime = 0;
    _isBeeping = false;
}

bool StateMachine::isTempIdeal(float temp) const {
    return (temp >= TEMP_IDEAL_MIN && temp <= TEMP_IDEAL_MAX);
}

bool StateMachine::isTempNearLimit(float temp) const {
    return ((temp >= (TEMP_IDEAL_MIN - TEMP_MARGIN_ALERT) && temp < TEMP_IDEAL_MIN) ||
            (temp > TEMP_IDEAL_MAX && temp <= (TEMP_IDEAL_MAX + TEMP_MARGIN_ALERT)));
}

bool StateMachine::isTempOutOfRange(float temp) const {
    return (temp < TEMP_IDEAL_MIN || temp > TEMP_IDEAL_MAX);
}

void StateMachine::update(const SensorReadings& readings, bool buttonPressed, unsigned long currentMillis) {
    // -------------------------------------------------------------------------
    // 1. Prioridade Máxima: Falha de Leitura do DHT11 (RF-02 e RF-04)
    // -------------------------------------------------------------------------
    if (readings.consecutiveDhtErrors >= 3) {
        _currentState = STATE_FALHA_SENSOR;
        return;
    }

    // Se o sensor se recuperou de uma falha anterior com leituras válidas
    if (_currentState == STATE_FALHA_SENSOR && readings.dhtValid) {
        _currentState = STATE_NORMAL;
        _tempOutOfRangeStartTime = 0;
        _windowOpenStartTime = 0;
    }

    // -------------------------------------------------------------------------
    // 2. Rastreamento Temporal de Condições Anômalas
    // -------------------------------------------------------------------------
    // Janela Aberta
    if (readings.windowOpen) {
        if (_windowOpenStartTime == 0) {
            _windowOpenStartTime = currentMillis;
        }
    } else {
        _windowOpenStartTime = 0;
    }

    // Temperatura fora da faixa
    if (isTempOutOfRange(readings.temperature)) {
        if (_tempOutOfRangeStartTime == 0) {
            _tempOutOfRangeStartTime = currentMillis;
        }
    } else {
        _tempOutOfRangeStartTime = 0;
    }

    unsigned long windowOpenDuration = (_windowOpenStartTime > 0) ? (currentMillis - _windowOpenStartTime) : 0;
    unsigned long tempOutOfRangeDuration = (_tempOutOfRangeStartTime > 0) ? (currentMillis - _tempOutOfRangeStartTime) : 0;

    // -------------------------------------------------------------------------
    // 3. Avaliação de Condições de Estado
    // -------------------------------------------------------------------------
    bool isCriticalCondition = (tempOutOfRangeDuration >= DURATION_TEMP_CRITICAL_MS) ||
                               (windowOpenDuration >= DURATION_WINDOW_CRITICAL_MS);

    bool isAttentionCondition = isTempNearLimit(readings.temperature) ||
                                (windowOpenDuration >= DURATION_WINDOW_ALERT_MS && windowOpenDuration < DURATION_WINDOW_CRITICAL_MS) ||
                                (readings.lightPercent > LIGHT_HIGH_THRESHOLD) ||
                                (isTempOutOfRange(readings.temperature) && tempOutOfRangeDuration < DURATION_TEMP_CRITICAL_MS);

    // -------------------------------------------------------------------------
    // 4. Máquina de Estados e Transições Estritas (RF-04)
    // -------------------------------------------------------------------------
    switch (_currentState) {
        case STATE_NORMAL:
            if (isCriticalCondition) {
                _currentState = STATE_CRITICO;
            } else if (isAttentionCondition) {
                _currentState = STATE_ATENCAO;
            }
            break;

        case STATE_ATENCAO:
            if (isCriticalCondition) {
                _currentState = STATE_CRITICO;
            } else if (!isAttentionCondition && isTempIdeal(readings.temperature) && !readings.windowOpen) {
                _currentState = STATE_NORMAL;
            }
            break;

        case STATE_CRITICO:
            if (buttonPressed) {
                // Transição para RECONHECIDO se o botão D2 for pressionado
                _currentState = STATE_RECONHECIDO;
            } else if (!isCriticalCondition) {
                if (isAttentionCondition) {
                    _currentState = STATE_ATENCAO;
                } else if (isTempIdeal(readings.temperature) && !readings.windowOpen) {
                    _currentState = STATE_NORMAL;
                }
            }
            break;

        case STATE_RECONHECIDO:
            // Permanece reconhecido até que as condições ambientais normalizem completamente
            if (isTempIdeal(readings.temperature) && !readings.windowOpen) {
                _currentState = STATE_NORMAL;
            }
            break;

        case STATE_FALHA_SENSOR:
            // Já tratado no início do método
            break;
    }
}

void StateMachine::updateActuators(Sensors& sensors, DisplayMatrix& matrix, unsigned long currentMillis) {
    // 1. Atualização do Display Matriz de LED (RF-05)
    matrix.displayState(getStateChar());

    // 2. Atuação nos Periféricos (RF-04)
    switch (_currentState) {
        case STATE_NORMAL:
            sensors.setLeds(true, false, false); // Apenas LED Verde ligado
            sensors.setRelay(false);             // Relé desligado
            sensors.setBuzzer(false);            // Buzzer desligado
            _isBeeping = false;
            break;

        case STATE_ATENCAO:
            sensors.setLeds(false, true, false); // Apenas LED Amarelo ligado
            sensors.setRelay(false);             // Relé desligado

            // Bipe sonoro não-bloqueante a cada 10 segundos
            if (!_isBeeping) {
                if (currentMillis - _lastBeepCycleTime >= BEEP_INTERVAL_ALERT_MS) {
                    _isBeeping = true;
                    _beepPulseStartTime = currentMillis;
                    _lastBeepCycleTime = currentMillis;
                    sensors.setBuzzer(true);
                }
            } else {
                if (currentMillis - _beepPulseStartTime >= BEEP_DURATION_ALERT_MS) {
                    _isBeeping = false;
                    sensors.setBuzzer(false);
                }
            }
            break;

        case STATE_CRITICO:
            sensors.setLeds(false, false, true); // LED Vermelho ligado
            sensors.setRelay(true);              // Relé acionado (ventilação LIGADA)
            sensors.setBuzzer(true);             // Buzzer contínuo
            _isBeeping = false;
            break;

        case STATE_RECONHECIDO:
            sensors.setLeds(false, false, true); // Mantém LED Vermelho ligado
            sensors.setRelay(true);              // Mantém Relé ligado
            sensors.setBuzzer(false);            // Buzzer silenciado
            _isBeeping = false;
            break;

        case STATE_FALHA_SENSOR:
            sensors.setLeds(false, true, true);  // LEDs Amarelo e Vermelho ligados simultaneamente
            sensors.setRelay(false);             // Relé desligado
            sensors.setBuzzer(false);            // Buzzer desligado
            _isBeeping = false;
            break;
    }
}

SystemState StateMachine::getState() const {
    return _currentState;
}

const char* StateMachine::getStateString() const {
    switch (_currentState) {
        case STATE_NORMAL:       return "NORMAL";
        case STATE_ATENCAO:      return "ATENÇÃO";
        case STATE_CRITICO:      return "CRÍTICO";
        case STATE_RECONHECIDO:  return "RECONHECIDO";
        case STATE_FALHA_SENSOR: return "FALHA_SENSOR";
        default:                 return "DESCONHECIDO";
    }
}

char StateMachine::getStateChar() const {
    switch (_currentState) {
        case STATE_NORMAL:       return 'N';
        case STATE_ATENCAO:      return 'A';
        case STATE_CRITICO:      return 'C';
        case STATE_RECONHECIDO:  return 'R';
        case STATE_FALHA_SENSOR: return 'F';
        default:                 return ' ';
    }
}
