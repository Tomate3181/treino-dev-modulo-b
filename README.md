# 🌱 EstufaSmart - Sistema Inteligente de Monitoramento e Controle Ambiental

> Sistema IoT integrado para monitoramento e automação de estufas agrícolas, unindo firmware não-bloqueante no **Arduino UNO R4 WiFi** e backend de alta performance em **Python (FastAPI + SQLite)** com dashboard web em tempo real.

---

## 📸 Demonstração do Protótipo em Funcionamento

<!-- ======================================================================= -->
<!-- SEÇÃO DE PLACEHOLDERS: Substitua os caminhos pelas fotos reais do seu protótipo -->
<!-- ======================================================================= -->

| 🌿 Montagem do Circuito e Sensores | 💡 Matriz de LED Nativa (UNO R4) |
| :---: | :---: |
| ![Circuito Físico do Protótipo](./Imagens/prototipo-fisico.png) | ![Matriz de LED em Operação](./Imagens/display-arduino.png) |
| *Foto da protoboard com DHT11, LDR, Reed Switch, Relé, LEDs e Buzzer.* | *Exibição da letra do estado atual ('N', 'A', 'C', 'R', 'F') na matriz 12x8.* |

| 📊 Dashboard Web em Tempo Real | 💻 Logs do Servidor e Monitor Serial |
| :---: | :---: |
| ![Dashboard Web](placeholder_dashboard_web.png) | ![Logs e Telemetria](placeholder_logs_telemetria.png) |
| *Interface gráfica moderna com gráficos temporais (Chart.js) e métricas.* | *Terminal exibindo os pacotes JSON recebidos e salvos no SQLite.* |

---

## 🚀 Funcionalidades do Projeto

### 🎛️ Firmware Embarcado (Arduino UNO R4 WiFi)
- **Temporização 100% Não-Bloqueante:** Todo o loop de leitura (5s) e temporizações do buzzer operam exclusivamente via `millis()`, sem uso da função `delay()`.
- **Monitoramento Multissensorial:**
  - Temperatura (°C) e Umidade Relativa do Ar (%) via **DHT11** com validação de leitura e tratamento de falhas.
  - Luminosidade normalizada de 0% a 100% via **LDR (A0)**.
  - Estado da Janela (Aberta/Fechada) via **Reed Switch / Sensor Magnético (D4)**.
- **Máquina de Estados Finita (FSM):**
  - `NORMAL`: Temperatura ideal (20°C a 28°C) e janela fechada. Apenas LED Verde ativo.
  - `ATENÇÃO`: Temperatura próxima do limite, janela aberta > 20s ou luz alta. LED Amarelo ativo e bipe sonoro de 200ms a cada 10s.
  - `CRÍTICO`: Temperatura fora da faixa por > 30s ou janela aberta > 60s. LED Vermelho ativo, alarme contínuo e relé de ventilação acionado.
  - `RECONHECIDO`: Botão de reconhecimento (D2) pressionado no estado crítico; alarme silenciado mantendo a ventilação e LED vermelho ligados até a normalização.
  - `FALHA_SENSOR`: 3 erros consecutivos de leitura do DHT11. LEDs Amarelo e Vermelho acesos simultaneamente.
- **Exibição na Matriz de LED Nativa ($12 \times 8$):** Renderização direta dos caracteres de estado (`N`, `A`, `C`, `R`, `F`).
- **Buffer Offline e Resiliência de Rede:** Fila circular FIFO para até 25 registros de telemetria em memória, mantendo a operação local em caso de queda de rede e retransmitindo tudo sequencialmente após a reconexão.

### 🌐 Backend e Dashboard Web (FastAPI + SQLite)
- **Ingestão HTTP REST:** Endpoint `POST /api/telemetry` para recepção contínua dos pacotes JSON.
- **Validação Rigorosa (Pydantic v2):** Validação estrita de limites físicos (temperatura entre -10°C e 60°C, porcentagens de 0 a 100% e estados válidos). Rejeição imediata de dados corrompidos com status `422`.
- **Persistência SQLite:** Banco de dados relacional leve em arquivo local (`estufasmart.db`) via SQLAlchemy.
- **Consultas de Histórico:** Endpoints `GET /api/telemetry/latest` e `GET /api/telemetry/history?limit=50`.
- **Dashboard Web Integrado (`GET /`):** Interface moderna com tema escuro (Dark Mode), cartões de métricas dinâmicos, cores por estado e gráfico temporal em tempo real com **Chart.js** via polling (3,5s).

---

## 🔌 Pinout de Hardware (Arduino UNO R4 WiFi)

```text
=====================================================
 Arduino UNO R4 WiFi   <--->   Periféricos / Sensores
=====================================================
 D2  ----------------------->  Botão de Reconhecimento (INPUT_PULLUP)
 D3  ----------------------->  DHT11 (Pino de Sinal/Dados)
 D4  ----------------------->  Sensor de Janela (Reed Switch / INPUT_PULLUP)
 D5  ----------------------->  LED Verde (Normal) + Resistor 220 Ω
 D6  ----------------------->  LED Amarelo (Atenção/Falha) + Resistor 220 Ω
 D7  ----------------------->  LED Vermelho (Crítico/Falha) + Resistor 220 Ω
 D8  ----------------------->  Buzzer Piezoelétrico (Sonoro)
 D9  ----------------------->  Módulo Relé (Ventilação Forçada)
 A0  ----------------------->  Módulo LDR (Leitura Analógica de Luz)
=====================================================
```

---

## 📚 Bibliotecas Necessárias

### 1. No Arduino IDE (C++)
As bibliotecas de conectividade e matriz de LED já vêm inclusas no pacote oficial de suporte à placa **Arduino UNO R4 WiFi**:
- **`Arduino_LED_Matrix.h`** (Inclusa nativamente no Core do R4 WiFi)
- **`WiFiS3.h`** (Inclusa nativamente no Core do R4 WiFi)
- **`DHT sensor library`** por *Adafruit* (Instalar via Gerenciador de Bibliotecas da Arduino IDE)
  - *(O Arduino IDE solicitará a instalação automática da biblioteca dependente `Adafruit Unified Sensor`)*

### 2. No Python (Backend)
Instaladas facilmente via `pip` a partir do arquivo [requirements.txt](file:///c:/Users/51909778893/Desktop/arduino-dev/EstufaSmart/backend/requirements.txt):
- **`fastapi`** ($\ge 0.100.0$) - Framework web de alta performance.
- **`uvicorn`** ($\ge 0.22.0$) - Servidor ASGI ultrarrápido.
- **`sqlalchemy`** ($\ge 2.0.0$) - ORM para gerenciar o SQLite.
- **`pydantic`** ($\ge 2.0.0$) - Validação de schemas e integridade dos dados.
- **`jinja2`** ($\ge 3.1.0$) - Mecanismo de templates HTML.

---

## 🛠️ Passo a Passo para Rodar o Projeto

### Passo 1: Preparar o Ambiente Python e Iniciar o Servidor
1. Abra um terminal (PowerShell ou Bash) e navegue até a pasta do backend:
   ```bash
   cd EstufaSmart/backend
   ```
2. Instale as dependências:
   ```bash
   pip install -r requirements.txt
   ```
3. Execute o servidor:
   ```bash
   python main.py
   ```
4. O servidor iniciará escutando em todas as interfaces de rede na porta **8000**:
   - 🌐 **Dashboard Web:** [http://localhost:8000](http://localhost:8000)
   - 📑 **Swagger / Documentação da API:** [http://localhost:8000/docs](http://localhost:8000/docs)

---

### Passo 2: Configurar o Firmware do Arduino
1. Abra a pasta `EstufaSmart` na **Arduino IDE** (ou abra o arquivo `EstufaSmart.ino`).
2. Abra o arquivo `secrets.h` e configure:
   ```cpp
   // Nome e senha da rede Wi-Fi (2.4 GHz)
   const char WIFI_SSID[] = "SEU_WIFI_OU_HOTSPOT";
   const char WIFI_PASS[] = "SUA_SENHA";

   // Endereço IP do computador que está rodando o FastAPI
   // (Descubra digitando 'ipconfig' no terminal do Windows)
   const char SERVER_HOST[] = "10.134.234.201"; 
   const int  SERVER_PORT   = 8000;
   const char SERVER_PATH[] = "/api/telemetry";
   ```
3. No menu **Ferramentas > Placa**, selecione **Arduino UNO R4 WiFi**.
4. Conecte a placa via cabo USB-C e selecione a porta serial correspondente (**Porta COM**).
5. Clique no botão **Carregar (Upload)**.
6. Abra o **Monitor Serial** configurado em **115200 baud**.

---

### Passo 3: Visualizar a Operação Integrada
1. O Arduino conectará à rede Wi-Fi e começará a emitir leituras a cada 5 segundos.
2. A matriz de LED do Arduino exibirá a letra `'N'` indicando estado normal.
3. No terminal do Python, você verá os registros sendo salvos no SQLite em tempo real.
4. Abra seu navegador em `http://localhost:8000` para acompanhar as curvas de temperatura e umidade, o estado da janela e a indicação de status!
