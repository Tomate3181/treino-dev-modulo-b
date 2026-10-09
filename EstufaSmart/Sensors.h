#ifndef SENSORS_H
#define SENSORS_H

#include <Arduino.h>
#include <DHT.h>

// =============================================================================
// Mapeamento de Pinos de Hardware (Seção 3 da Especificação)
// =============================================================================
#define PIN_BUTTON      2   // Botão de Reconhecimento (INPUT_PULLUP)
#define PIN_DHT         3   // DHT11 (Pino de dados)
#define PIN_WINDOW      4   // Sensor de Janela (Reed/Tilt Switch com INPUT_PULLUP)
#define PIN_LED_GREEN   5   // LED Verde (Normal)
#define PIN_LED_YELLOW  6   // LED Amarelo (Atenção / Falha)
#define PIN_LED_RED     7   // LED Vermelho (Crítico / Falha)
#define PIN_BUZZER      8   // Buzzer piezoelétrico
#define PIN_RELAY       9   // Módulo Relé (Ventilação)
#define PIN_LDR         A0  // Sensor LDR (Leitura analógica)

#define DHT_TYPE        DHT11

// Definições de níveis lógicos (ajustáveis conforme circuito utilizado)
#define BUTTON_PRESSED_STATE  LOW   // Devido ao INPUT_PULLUP
#define WINDOW_OPEN_STATE     HIGH  // Aberto quando ímã afasta (desconecta do GND com pull-up)
#define RELAY_ACTIVE_STATE    HIGH  // Nível lógico para ligar ventilação

struct SensorReadings {
    float temperature;
    float humidity;
    bool windowOpen;
    int lightPercent;
    bool dhtValid;
    int consecutiveDhtErrors;
};

class Sensors {
public:
    Sensors();
    void begin();

    // Executa a leitura dos sensores ambientais
    SensorReadings readAll();

    // Detecção com debounce não-bloqueante do botão de reconhecimento
    bool checkButtonPressed();

    // Controle dos atuadores luminosos e de potência
    void setLeds(bool green, bool yellow, bool red);
    void setRelay(bool active);
    void setBuzzer(bool active);

    // Retorna a quantidade de falhas consecutivas do sensor DHT
    int getConsecutiveDhtErrors() const;

private:
    DHT _dht;
    int _consecutiveDhtErrors;
    
    // Variáveis de debounce do botão
    int _lastButtonState;
    unsigned long _lastDebounceTime;
    static const unsigned long DEBOUNCE_DELAY = 50; // 50 ms
};

#endif // SENSORS_H
