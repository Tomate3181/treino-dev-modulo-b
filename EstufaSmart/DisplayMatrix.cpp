#include "DisplayMatrix.h"

// Matrizes de pixels 8 linhas por 12 colunas para cada caractere

// Letra 'N' (NORMAL)
static const uint8_t FRAME_N[8][12] = {
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0},
    {0, 0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0},
    {0, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0, 0},
    {0, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 0},
    {0, 0, 0, 1, 0, 0, 0, 1, 1, 0, 0, 0},
    {0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
};

// Letra 'A' (ATENÇÃO)
static const uint8_t FRAME_A[8][12] = {
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0},
    {0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0},
    {0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0},
    {0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0},
    {0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0},
    {0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
};

// Letra 'C' (CRÍTICO)
static const uint8_t FRAME_C[8][12] = {
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0},
    {0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
};

// Letra 'R' (RECONHECIDO)
static const uint8_t FRAME_R[8][12] = {
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0},
    {0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0},
    {0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0},
    {0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0},
    {0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0},
    {0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
};

// Letra 'F' (FALHA_SENSOR)
static const uint8_t FRAME_F[8][12] = {
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0},
    {0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0},
    {0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
};

static const uint8_t FRAME_BLANK[8][12] = {0};

DisplayMatrix::DisplayMatrix() : _currentChar('\0') {
}

void DisplayMatrix::begin() {
    _matrix.begin();
    clear();
}

void DisplayMatrix::displayState(char stateChar) {
    if (_currentChar == stateChar) {
        return; // Evita re-renderizar o mesmo frame desnecessariamente
    }
    _currentChar = stateChar;

    switch (stateChar) {
        case 'N':
            _matrix.renderBitmap((uint8_t*)FRAME_N, 8, 12);
            break;
        case 'A':
            _matrix.renderBitmap((uint8_t*)FRAME_A, 8, 12);
            break;
        case 'C':
            _matrix.renderBitmap((uint8_t*)FRAME_C, 8, 12);
            break;
        case 'R':
            _matrix.renderBitmap((uint8_t*)FRAME_R, 8, 12);
            break;
        case 'F':
            _matrix.renderBitmap((uint8_t*)FRAME_F, 8, 12);
            break;
        default:
            _matrix.renderBitmap((uint8_t*)FRAME_BLANK, 8, 12);
            break;
    }
}

void DisplayMatrix::clear() {
    _currentChar = '\0';
    _matrix.renderBitmap((uint8_t*)FRAME_BLANK, 8, 12);
}
