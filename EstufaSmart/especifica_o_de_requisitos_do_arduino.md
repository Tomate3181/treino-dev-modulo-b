# EstufaSmart - Especificação e Requisitos do Firmware (Arduino UNO R4 WiFi)

## 1. Visão Geral
O firmware do dispositivo EstufaSmart é responsável por monitorar continuamente os parâmetros ambientais de uma estufa (temperatura, umidade, luminosidade e estado da janela), gerenciar a máquina de estados local de forma independente da rede, atuar nos periféricos (LEDs, Buzzer e Relé), exibir status na matriz LED nativa e transmitir telemetria via Wi-Fi no formato JSON.

---

## 2. Requisitos Funcionais (RF)

### RF-01: Temporização Não-Bloqueante
- As leituras de sensores e atualização da lógica devem ocorrer em intervalos regulares de **5 segundos**.
- **Restrição Estrita:** É proibido o uso da função `delay()` no `loop()` principal. Todo o controle de tempo deve ser implementado via `millis()`.

### RF-02: Leitura do DHT11 e Tratamento de Falhas
- Ler temperatura (°C) e umidade (%).
- Validar as leituras contra retornos `NaN` ou fora de faixas válidas.
- Caso ocorram **3 leituras inválidas consecutivas**, o sistema deve transicionar imediatamente para o estado `FALHA_SENSOR`.

### RF-03: Leitura e Normalização dos Sensores Secundários
- **Luminosidade (LDR):** Ler o valor analógico no pino `A0` e normalizar a escala para um intervalo percentual de **0% a 100%**.
- **Sensor de Janela (Reed / Tilt):** Ler o estado digital no pino `D4` (Aberto/Fechado).

### RF-04: Gerenciamento da Máquina de Estados
A lógica do firmware deve responder estritamente às seguintes regras de transição e atuação:

| Estado | Condições de Entrada | Atuação nos Periféricos |
| :--- | :--- | :--- |
| **NORMAL** | Temperatura na faixa ideal ($20^\circ\text{C}$ a $28^\circ\text{C}$) **E** janela fechada. | LED Verde ligado. Demais LEDs, Buzzer e Relé desligados. |
| **ATENÇÃO** | Temperatura próxima do limite **OU** janela aberta $> 20\text{ s}$ **OU** luz muito alta. | LED Amarelo ligado; bipe sonoro (Buzzer) disparado a cada $10\text{ s}$. |
| **CRÍTICO** | Temperatura fora da faixa ($> 28^\circ\text{C}$ ou $< 20^\circ\text{C}$) $> 30\text{ s}$ **OU** janela aberta $> 60\text{ s}$. | LED Vermelho ligado; Buzzer acionado continuamente; Relé acionado (ventilação LIGADA). |
| **RECONHECIDO**| Botão acionado (pino `D2`) enquanto o sistema estiver no estado `CRÍTICO`. | Buzzer silenciado; Relé e LED mantêm estado necessário. Retorna automaticamente para `NORMAL` assim que as condições ambientais normalizarem. |
| **FALHA_SENSOR**| Ocorrência de 3 falhas consecutivas de leitura do sensor DHT11. | LEDs Amarelo e Vermelho ligados simultaneamente. |

### RF-05: Exibição na Matriz de LED (UNO R4 WiFi)
- O firmware deve utilizar a matriz de LED nativa ($12 \times 8$) do Arduino UNO R4 WiFi para exibir a letra correspondente ao estado atual:
  - `N`: NORMAL
  - `A`: ATENÇÃO
  - `C`: CRÍTICO
  - `R`: RECONHECIDO
  - `F`: FALHA_SENSOR

### RF-06: Conectividade e Credenciais Isoladas
- As credenciais de conexão Wi-Fi (`SSID` e `PASSWORD`) e o endereço IP/Porta do servidor central DEVEM ser mantidos em um arquivo de cabeçalho separado (`secrets.h`), evitando exposição direta no código fonte principal.

### RF-07: Telemetria e Payload JSON
- A cada ciclo de leitura (5s), o dispositivo deve emitir uma mensagem JSON via HTTP POST contendo a seguinte estrutura:

```json
{
  "device_id": "ESTUFA_SALA01",
  "placa": "UNO_R4_WIFI",
  "seq": 104,
  "temp": 25.4,
  "umid": 60.0,
  "janela": false,
  "luz_pct": 45,
  "estado": "NORMAL"
}
```

### RF-08: Buffer Offline e Resiliência de Rede
- A máquina de estados e o monitoramento local devem funcionar perfeitamente mesmo na ausência de conexão Wi-Fi.
- Em caso de falha de conexão ou queda da rede, o firmware deve armazenar as leituras não enviadas em uma fila de memória (Buffer Circular / Ring Buffer) com capacidade mínima de **20 leituras**.
- Assim que a conexão com o servidor for restabelecida, o dispositivo deve retransmitir os dados acumulados sequencialmente.

---

## 3. Mapeamento de Pinos (Pinout)

```
=====================================================
 Arduino UNO R4 WiFi   <--->   Periféricos / Sensores
=====================================================
 D2  ----------------------->  Botão de Reconhecimento (INPUT_PULLUP)
 D3  ----------------------->  DHT11 (Dados)
 D4  ----------------------->  Sensor de Janela (Reed/Tilt Switch)
 D5  ----------------------->  LED Verde (Resistor 220 Ω)
 D6  ----------------------->  LED Amarelo (Resistor 220 Ω)
 D7  ----------------------->  LED Vermelho (Resistor 220 Ω)
 D8  ----------------------->  Buzzer (Sonoro)
 D9  ----------------------->  Módulo Relé (Ventilação)
 A0  ----------------------->  Módulo LDR (Saída Analógica)
=====================================================
```

---

## 4. Arquitetura do Firmware (Estrutura de Arquivos)

- `EstufaSmart.ino`: Loop principal sem bloqueio, gerenciamento de timers `millis()`, chamadas de tarefas.
- `secrets.h`: Constantes do SSID, senha da rede Wi-Fi e IP do servidor.
- `StateMachine.h / StateMachine.cpp`: Enumeração dos estados (`NORMAL`, `ATENCAO`, `CRITICO`, `RECONHECIDO`, `FALHA_SENSOR`) e transições.
- `Sensors.h / Sensors.cpp`: Funções de leitura do DHT11, LDR e validações de leitura/falha.
- `DisplayMatrix.h / DisplayMatrix.cpp`: Desenho de frames de caracteres ($12 \times 8$) para a matriz nativa do UNO R4.
- `NetworkBuffer.h / NetworkBuffer.cpp`: Fila circular para até 20 instâncias da estrutura de dados de telemetria.