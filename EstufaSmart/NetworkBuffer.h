#ifndef NETWORK_BUFFER_H
#define NETWORK_BUFFER_H

#include <Arduino.h>

// Capacidade do buffer circular (mínimo de 20 conforme RF-08)
#define BUFFER_CAPACITY 25

// Estrutura do registro de telemetria correspondente ao payload JSON (RF-07)
struct TelemetryRecord {
    unsigned long seq;
    float temp;
    float umid;
    bool janela;
    int luz_pct;
    char estado[16];
};

class NetworkBuffer {
public:
    NetworkBuffer();

    // Insere um registro no buffer. Se estiver cheio, descarta o mais antigo para manter os mais recentes.
    bool push(const TelemetryRecord& record);

    // Remove e retorna o registro mais antigo da fila (FIFO)
    bool pop(TelemetryRecord& record);

    // Consulta o registro mais antigo sem remover
    bool peek(TelemetryRecord& record) const;

    // Retorna se o buffer está vazio
    bool isEmpty() const;

    // Retorna se o buffer atingiu capacidade máxima
    bool isFull() const;

    // Quantidade atual de itens na fila
    int count() const;

    // Limpa todos os registros
    void clear();

private:
    TelemetryRecord _buffer[BUFFER_CAPACITY];
    int _head;  // Índice de inserção
    int _tail;  // Índice de remoção
    int _count; // Contador de elementos
};

#endif // NETWORK_BUFFER_H
