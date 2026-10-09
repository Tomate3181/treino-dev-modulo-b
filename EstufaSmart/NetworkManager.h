#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <Arduino.h>
#include <WiFiS3.h>
#include "secrets.h"
#include "NetworkBuffer.h"

class NetworkManager {
public:
    NetworkManager();
    void begin();

    // Mantém o estado da conexão Wi-Fi de forma não-bloqueante
    void maintainConnection(unsigned long currentMillis);

    // Enfileira um novo registro e tenta despachar imediatamente ou através do buffer
    bool queueTelemetry(const TelemetryRecord& record);

    // Processa a fila de retransmissão e envio de dados acumulados
    void processOutgoingQueue();

    // Retorna se o Wi-Fi está atualmente conectado
    bool isConnected();

    // Retorna a quantidade de itens aguardando envio na fila offline
    int getPendingCount() const;

private:
    WiFiClient _client;
    NetworkBuffer _offlineBuffer;
    
    unsigned long _lastReconnectAttempt;
    static const unsigned long RECONNECT_INTERVAL_MS = 10000; // Tenta reconectar a cada 10s se offline

    // Envia um único registro via HTTP POST para a API FastAPI
    bool sendHttpPost(const TelemetryRecord& record);
    
    // Constrói a string do payload JSON conforme RF-07
    void buildJsonPayload(const TelemetryRecord& record, char* buffer, size_t bufferSize);
};

#endif // NETWORK_MANAGER_H
