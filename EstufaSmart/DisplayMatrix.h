#ifndef DISPLAY_MATRIX_H
#define DISPLAY_MATRIX_H

#include <Arduino.h>
#include <Arduino_LED_Matrix.h>

class DisplayMatrix {
public:
    DisplayMatrix();
    void begin();
    
    // Exibe um caractere de status ('N', 'A', 'C', 'R', 'F') na matriz 12x8
    void displayState(char stateChar);
    
    // Desliga todos os LEDs da matriz
    void clear();

private:
    ArduinoLEDMatrix _matrix;
    char _currentChar;
};

#endif // DISPLAY_MATRIX_H
