#ifndef SECRETS_H
#define SECRETS_H

// =============================================================================
// EstufaSmart - Arquivo de Exemplo para Credenciais (secrets.example.h)
// Copie este arquivo para "secrets.h" e preencha com seus dados reais.
// O arquivo secrets.h é ignorado pelo Git para proteger suas senhas.
// =============================================================================

// Credenciais da Rede Wi-Fi (2.4 GHz)
const char WIFI_SSID[] = "SUA_REDE_WIFI";
const char WIFI_PASS[] = "SUA_SENHA_WIFI";

// Configurações do Servidor Python com FastAPI
// Altere para o IP local do computador executando o FastAPI (ex: "192.168.1.150")
const char SERVER_HOST[] = "192.168.1.100";
const int  SERVER_PORT   = 8000;
const char SERVER_PATH[] = "/api/telemetry";

// Identificadores do Dispositivo na Telemetria (RF-07)
const char DEVICE_ID[] = "ESTUFA_SALA01";
const char PLACA_ID[]  = "UNO_R4_WIFI";

#endif // SECRETS_H
