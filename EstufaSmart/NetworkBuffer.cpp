#include "NetworkBuffer.h"

NetworkBuffer::NetworkBuffer() : _head(0), _tail(0), _count(0) {
}

bool NetworkBuffer::push(const TelemetryRecord& record) {
    if (isFull()) {
        // Se o buffer estiver cheio, avança o _tail para sobrescrever o dado mais antigo (ring buffer circular)
        _tail = (_tail + 1) % BUFFER_CAPACITY;
        _count--;
    }

    _buffer[_head] = record;
    _head = (_head + 1) % BUFFER_CAPACITY;
    _count++;
    return true;
}

bool NetworkBuffer::pop(TelemetryRecord& record) {
    if (isEmpty()) {
        return false;
    }

    record = _buffer[_tail];
    _tail = (_tail + 1) % BUFFER_CAPACITY;
    _count--;
    return true;
}

bool NetworkBuffer::peek(TelemetryRecord& record) const {
    if (isEmpty()) {
        return false;
    }

    record = _buffer[_tail];
    return true;
}

bool NetworkBuffer::isEmpty() const {
    return (_count == 0);
}

bool NetworkBuffer::isFull() const {
    return (_count >= BUFFER_CAPACITY);
}

int NetworkBuffer::count() const {
    return _count;
}

void NetworkBuffer::clear() {
    _head = 0;
    _tail = 0;
    _count = 0;
}
