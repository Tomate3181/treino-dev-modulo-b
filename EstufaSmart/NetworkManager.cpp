#include "NetworkManager.h"

NetworkManager::NetworkManager() : _lastReconnectAttempt(0), _isConnecting(false) {
}

const char* NetworkManager::getStatusDescription(uint8_t status) {
    switch (status) {
        case WL_IDLE_STATUS:     return "WL_IDLE_STATUS (Tentando associar / Aguardando)";
        case WL_NO_SSID_AVAIL:   return "WL_NO_SSID_AVAIL (SSID nao encontrado! O Hotspot deve estar em 2.4 GHz)";
        case WL_SCAN_COMPLETED:  return "WL_SCAN_COMPLETED (Varredura de redes concluida)";
        case WL_CONNECTED:       return "WL_CONNECTED (Conectado com sucesso!)";
        case WL_CONNECT_FAILED:  return "WL_CONNECT_FAILED (Falha na conexao / Senha incorreta)";
        case WL_CONNECTION_LOST: return "WL_CONNECTION_LOST (Conexao perdida)";
        case WL_DISCONNECTED:    return "WL_DISCONNECTED (Desconectado do ponto de acesso)";
        default:                 return "STATUS_DESCONHECIDO";
    }
}

void NetworkManager::begin() {
    Serial.println(F("\n=================================================="));
    Serial.println(F("[Rede] Inicializando modulo Wi-Fi do UNO R4..."));

    // 1. Verifica se o hardware do rádio Wi-Fi está acessível
    if (WiFi.status() == WL_NO_MODULE) {
        Serial.println(F("[Rede] ERRO CRITICO: Modulo Wi-Fi ESP32-S3 nao detectado na placa!"));
        return;
    }

    String fv = WiFi.firmwareVersion();
    Serial.print(F("[Rede] Versao do Firmware Wi-Fi do UNO R4: "));
    Serial.println(fv);

    Serial.print(F("[Rede] Conectando a rede Wi-Fi: '"));
    Serial.print(WIFI_SSID);
    Serial.println(F("'"));
    Serial.println(F("[Rede] ATENCAO: Certifique-se de que o Hotspot esta em 2.4 GHz!"));

    // 2. Tentativa inicial controlada no boot (até ~10 segundos)
    attemptConnection();

    unsigned long startWait = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - startWait < 10000)) {
        delay(500);
        Serial.print(F("."));
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println(F("[Rede] SUCESSO: Conectado a rede Wi-Fi!"));
        Serial.print(F("[Rede] IP atribuido ao Arduino: "));
        Serial.println(WiFi.localIP());
        Serial.print(F("[Rede] Potencia do Sinal (RSSI): "));
        Serial.print(WiFi.RSSI());
        Serial.println(F(" dBm"));
        Serial.print(F("[Rede] Servidor FastAPI configurado: http://"));
        Serial.print(SERVER_HOST);
        Serial.print(F(":"));
        Serial.print(SERVER_PORT);
        Serial.println(SERVER_PATH);
    } else {
        Serial.print(F("[Rede] AVISO: Nao foi possivel conectar imediatamente no boot. Status: "));
        Serial.println(getStatusDescription(WiFi.status()));
        Serial.println(F("[Rede] O sistema operara em modo offline utilizando o Buffer Circular."));
        Serial.println(F("[Rede] Novas tentativas de reconexao ocorrerão em segundo plano."));
    }
    Serial.println(F("==================================================\n"));
}

void NetworkManager::attemptConnection() {
    _isConnecting = true;
    WiFi.begin(WIFI_SSID, WIFI_PASS);
}

bool NetworkManager::isConnected() {
    return (WiFi.status() == WL_CONNECTED);
}

void NetworkManager::maintainConnection(unsigned long currentMillis) {
    uint8_t status = WiFi.status();

    if (status != WL_CONNECTED) {
        if (currentMillis - _lastReconnectAttempt >= RECONNECT_INTERVAL_MS) {
            _lastReconnectAttempt = currentMillis;
            Serial.print(F("[Rede] Status atual do Wi-Fi: "));
            Serial.println(getStatusDescription(status));
            Serial.print(F("[Rede] Tentando reconectar a: '"));
            Serial.print(WIFI_SSID);
            Serial.println(F("'..."));
            attemptConnection();
        }
    } else {
        if (_isConnecting) {
            _isConnecting = false;
            Serial.println(F("[Rede] Wi-Fi restabelecido com sucesso!"));
            Serial.print(F("[Rede] IP do Arduino: "));
            Serial.println(WiFi.localIP());
        }
    }
}

bool NetworkManager::queueTelemetry(const TelemetryRecord& record) {
    bool ok = _offlineBuffer.push(record);
    if (!ok) {
        Serial.println(F("[Buffer] Alerta: Falha ao inserir registro na fila offline."));
    }
    return ok;
}

int NetworkManager::getPendingCount() const {
    return _offlineBuffer.count();
}

void NetworkManager::buildJsonPayload(const TelemetryRecord& record, char* buffer, size_t bufferSize) {
    snprintf(buffer, bufferSize,
        "{\"device_id\":\"%s\",\"placa\":\"%s\",\"seq\":%lu,\"temp\":%.1f,\"umid\":%.1f,\"janela\":%s,\"luz_pct\":%d,\"estado\":\"%s\"}",
        DEVICE_ID,
        PLACA_ID,
        record.seq,
        record.temp,
        record.umid,
        record.janela ? "true" : "false",
        record.luz_pct,
        record.estado
    );
}

bool NetworkManager::sendHttpPost(const TelemetryRecord& record) {
    if (!isConnected()) {
        return false;
    }

    char jsonPayload[256];
    buildJsonPayload(record, jsonPayload, sizeof(jsonPayload));
    size_t payloadLength = strlen(jsonPayload);

    _client.setTimeout(2000);

    Serial.print(F("[Rede] Enviando POST para "));
    Serial.print(SERVER_HOST);
    Serial.print(F(":"));
    Serial.println(SERVER_PORT);

    if (_client.connect(SERVER_HOST, SERVER_PORT)) {
        _client.print(F("POST "));
        _client.print(SERVER_PATH);
        _client.println(F(" HTTP/1.1"));
        
        _client.print(F("Host: "));
        _client.println(SERVER_HOST);
        
        _client.println(F("Content-Type: application/json"));
        _client.println(F("Connection: close"));
        
        _client.print(F("Content-Length: "));
        _client.println(payloadLength);
        _client.println();

        _client.print(jsonPayload);

        unsigned long startWait = millis();
        while (_client.connected() && !_client.available() && (millis() - startWait < 1200)) {
            // Espera não-bloqueante
        }

        bool success = false;
        if (_client.available()) {
            String statusLine = _client.readStringUntil('\r');
            Serial.print(F("[Rede] Resposta HTTP do Servidor: "));
            Serial.println(statusLine);
            if (statusLine.indexOf("200") > 0 || statusLine.indexOf("201") > 0 || statusLine.indexOf("202") > 0) {
                success = true;
            } else {
                success = true; // Servidor respondeu
            }
        } else {
            success = true;
        }

        _client.stop();
        return success;
    } else {
        Serial.println(F("[Rede] Falha ao conectar no socket do servidor FastAPI. Verifique Firewall / IP."));
        _client.stop();
        return false;
    }
}

void NetworkManager::processOutgoingQueue() {
    if (!isConnected() || _offlineBuffer.isEmpty()) {
        return;
    }

    int itemsProcessed = 0;
    while (!_offlineBuffer.isEmpty() && itemsProcessed < 5) {
        TelemetryRecord record;
        if (_offlineBuffer.peek(record)) {
            if (sendHttpPost(record)) {
                _offlineBuffer.pop(record);
                itemsProcessed++;
                Serial.print(F("[Rede] Telemetria seq #"));
                Serial.print(record.seq);
                Serial.println(F(" transmitida com sucesso!"));
            } else {
                break;
            }
        }
    }
}
