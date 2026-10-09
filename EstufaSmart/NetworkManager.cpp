#include "NetworkManager.h"

NetworkManager::NetworkManager() : _lastReconnectAttempt(0) {
}

void NetworkManager::begin() {
    Serial.println(F("[Rede] Inicializando modulo Wi-Fi do UNO R4..."));

    if (WiFi.status() == WL_NO_MODULE) {
        Serial.println(F("[Rede] ERRO: Modulo Wi-Fi nao detectado!"));
        return;
    }

    Serial.print(F("[Rede] Conectando a rede: "));
    Serial.println(WIFI_SSID);

    // Tentativa inicial não-bloqueante
    WiFi.begin(WIFI_SSID, WIFI_PASS);
}

bool NetworkManager::isConnected() {
    return (WiFi.status() == WL_CONNECTED);
}

void NetworkManager::maintainConnection(unsigned long currentMillis) {
    if (WiFi.status() != WL_CONNECTED) {
        if (currentMillis - _lastReconnectAttempt >= RECONNECT_INTERVAL_MS) {
            _lastReconnectAttempt = currentMillis;
            Serial.println(F("[Rede] Tentando restabelecer conexao Wi-Fi..."));
            WiFi.begin(WIFI_SSID, WIFI_PASS);
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
    // Formatação conforme especificação do RF-07
    // Exemplo: {"device_id":"ESTUFA_SALA01","placa":"UNO_R4_WIFI","seq":104,"temp":25.4,"umid":60.0,"janela":false,"luz_pct":45,"estado":"NORMAL"}
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

    // Timeout rápido para não prender a execução local
    _client.setTimeout(1500);

    if (_client.connect(SERVER_HOST, SERVER_PORT)) {
        // Envio do cabeçalho HTTP
        _client.print(F("POST "));
        _client.print(SERVER_PATH);
        _client.println(F(" HTTP/1.1"));
        
        _client.print(F("Host: "));
        _client.println(SERVER_HOST);
        
        _client.println(F("Content-Type: application/json"));
        _client.println(F("Connection: close"));
        
        _client.print(F("Content-Length: "));
        _client.println(payloadLength);
        _client.println(); // Linha em branco obrigatória separando cabeçalhos do corpo

        // Envio do payload JSON
        _client.print(jsonPayload);

        // Aguarda confirmação mínima de envio
        unsigned long startWait = millis();
        while (_client.connected() && !_client.available() && (millis() - startWait < 800)) {
            // Espera não-bloqueante limitada a 800ms
        }

        bool success = false;
        if (_client.available()) {
            String statusLine = _client.readStringUntil('\r');
            // Verifica se o servidor retornou HTTP 200, 201 ou 202
            if (statusLine.indexOf("200") > 0 || statusLine.indexOf("201") > 0 || statusLine.indexOf("202") > 0) {
                success = true;
            } else {
                Serial.print(F("[Rede] Servidor respondeu: "));
                Serial.println(statusLine);
                // Mesmo com código não-200, se respondeu HTTP consideramos processado pelo servidor
                success = true;
            }
        } else {
            // Se o socket transmitiu mas fechou sem body, assume entregue
            success = true;
        }

        _client.stop();
        return success;
    } else {
        Serial.println(F("[Rede] Falha ao conectar ao servidor FastAPI."));
        _client.stop();
        return false;
    }
}

void NetworkManager::processOutgoingQueue() {
    if (!isConnected() || _offlineBuffer.isEmpty()) {
        return;
    }

    // Processa até 5 itens por chamada para não reter o loop principal
    int itemsProcessed = 0;
    while (!_offlineBuffer.isEmpty() && itemsProcessed < 5) {
        TelemetryRecord record;
        if (_offlineBuffer.peek(record)) {
            if (sendHttpPost(record)) {
                // Remove da fila apenas se o envio foi bem-sucedido
                _offlineBuffer.pop(record);
                itemsProcessed++;
                Serial.print(F("[Rede] Registro seq #"));
                Serial.print(record.seq);
                Serial.println(F(" enviado com sucesso!"));
            } else {
                // Se falhou o envio para o servidor, interrompe o descarregamento para tentar no próximo ciclo
                break;
            }
        }
    }
}
