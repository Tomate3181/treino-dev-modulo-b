#include "Sensors.h"

Sensors::Sensors() 
    : _dht(PIN_DHT, DHT_TYPE), 
      _consecutiveDhtErrors(0),
      _lastButtonState(HIGH),
      _lastDebounceTime(0) {
}

void Sensors::begin() {
    // Configuração dos pinos de entrada
    pinMode(PIN_BUTTON, INPUT_PULLUP);
    pinMode(PIN_WINDOW, INPUT_PULLUP);

    // Configuração dos pinos de saída
    pinMode(PIN_LED_GREEN, OUTPUT);
    pinMode(PIN_LED_YELLOW, OUTPUT);
    pinMode(PIN_LED_RED, OUTPUT);
    pinMode(PIN_BUZZER, OUTPUT);
    pinMode(PIN_RELAY, OUTPUT);

    // Estado inicial: tudo desligado
    setLeds(false, false, false);
    setBuzzer(false);
    setRelay(false);

    // Inicialização do sensor DHT
    _dht.begin();
}

SensorReadings Sensors::readAll() {
    SensorReadings data;

    // Leitura do DHT11 (RF-02)
    float t = _dht.readTemperature();
    float h = _dht.readHumidity();

    // Validação de integridade da leitura do DHT11
    if (isnan(t) || isnan(h) || t < 0.0f || t > 60.0f || h < 10.0f || h > 100.0f) {
        _consecutiveDhtErrors++;
        data.dhtValid = false;
        data.temperature = (isnan(t) ? 0.0f : t);
        data.humidity    = (isnan(h) ? 0.0f : h);
    } else {
        _consecutiveDhtErrors = 0;
        data.dhtValid = true;
        data.temperature = t;
        data.humidity = h;
    }
    data.consecutiveDhtErrors = _consecutiveDhtErrors;

    // Leitura e normalização do LDR (RF-03): A0 (0 a 1023) -> 0% a 100%
    int rawLdr = analogRead(PIN_LDR);
    int pct = map(rawLdr, 0, 1023, 0, 100);
    if (pct < 0) pct = 0;
    if (pct > 100) pct = 100;
    data.lightPercent = pct;

    // Leitura do sensor de janela (RF-03)
    int windowRaw = digitalRead(PIN_WINDOW);
    data.windowOpen = (windowRaw == WINDOW_OPEN_STATE);

    return data;
}

bool Sensors::checkButtonPressed() {
    int reading = digitalRead(PIN_BUTTON);
    bool pressedEvent = false;

    if (reading != _lastButtonState) {
        if ((millis() - _lastDebounceTime) > DEBOUNCE_DELAY) {
            _lastDebounceTime = millis();
            _lastButtonState = reading;
            if (reading == BUTTON_PRESSED_STATE) {
                pressedEvent = true;
            }
        }
    }

    return pressedEvent;
}

void Sensors::setLeds(bool green, bool yellow, bool red) {
    digitalWrite(PIN_LED_GREEN, green ? HIGH : LOW);
    digitalWrite(PIN_LED_YELLOW, yellow ? HIGH : LOW);
    digitalWrite(PIN_LED_RED, red ? HIGH : LOW);
}

void Sensors::setRelay(bool active) {
    digitalWrite(PIN_RELAY, active ? RELAY_ACTIVE_STATE : !RELAY_ACTIVE_STATE);
}

void Sensors::setBuzzer(bool active) {
    digitalWrite(PIN_BUZZER, active ? HIGH : LOW);
}

int Sensors::getConsecutiveDhtErrors() const {
    return _consecutiveDhtErrors;
}
