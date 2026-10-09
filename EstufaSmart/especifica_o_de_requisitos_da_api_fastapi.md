# EstufaSmart - Especificação e Requisitos do Servidor Backend (FastAPI + SQLite)

## 1. Visão Geral

O servidor backend do projeto EstufaSmart é desenvolvido em Python utilizando a biblioteca **FastAPI**. Suas principais responsabilidades consistem em disponibilizar endpoints HTTP para a recepção continuada de telemetria IoT enviada pelo Arduino, efetuar a validação rigorosa dos dados ambientais recebidos, armazenar o histórico em um banco de dados **SQLite** e disponibilizar uma interface web (Painel/Dashboard) em tempo real com gráfico e indicadores de estado.

---

## 2. Requisitos Funcionais (RF)

### RF-01: Ingestão de Telemetria
- Disponibilizar um endpoint HTTP no formato `POST /api/telemetry` para receber os pacotes de dados no formato JSON emitidos pelo Arduino UNO R4 WiFi.

### RF-02: Validação Estrita de Dados (Pydantic)
- Todos os pacotes recebidos devem ser submetidos à validação antes de serem salvos no banco de dados.
- **Regras de Validação:**
  - `temp` (Temperatura): Deve estar rigorosamente na faixa de $-10.0^\circ\text{C}$ a $60.0^\circ\text{C}$.
  - `umid` (Umidade): Deve estar na faixa de $0.0\%$ a $100.0\%$.
  - `luz_pct` (Luminosidade): Deve estar na faixa de $0\%$ a $100\%$.
  - `janela` (Status da Janela): Deve ser um valor booleano (`true` / `false`).
  - `estado` (Estado do Sistema): Deve pertencer estritamente ao conjunto de valores permitidos: `["NORMAL", "ATENÇÃO", "CRÍTICO", "RECONHECIDO", "FALHA_SENSOR"]`.
- **Tratamento de Exceções:** Dados que não atendam a essas condições devem ser descarte/rejeitados com retorno de erro HTTP `422 Unprocessable Entity` ou `400 Bad Request`.

### RF-03: Persistência de Dados em Banco SQLite
- Criar e gerenciar um banco de dados relacional leve em arquivo local chamado `estufasmart.db` utilizando SQLite.
- Registrar os pacotes de telemetria válidos acompanhados do carimbo de data e hora (*timestamp*) do recebimento no servidor.

### RF-04: Endpoint para Leitura de Histórico e Estado Atual
- `GET /api/telemetry/latest`: Retorna o último registro de telemetria recebido (temperatura, umidade, estado atual e janela).
- `GET /api/telemetry/history`: Retorna o histórico das últimas $N$ leituras (padrão: 50 leituras) ordenadas cronologicamente para exibição gráfica.

### RF-05: Interface Web / Dashboard Integrado
- Servir uma página web responsiva na rota raiz (`GET /`).
- **Elementos do Dashboard:**
  - **Indicador Principal de Temperatura:** Exibição numérica destacada do valor mais recente da temperatura em $^\circ\text{C}$.
  - **Status do Sistema:** Badge/Cartão de estado atual (`NORMAL`, `ATENÇÃO`, `CRÍTICO`, etc.) com alteração dinâmica de cor.
  - **Gráfico Temporal:** Gráfico interativo (utilizando *Chart.js* ou biblioteca equivalente) atualizado automaticamente mostrando a variação da temperatura e umidade ao longo do tempo.
  - **Atualização Automática:** Atualização periódica da interface via polling (ex: requisição `fetch` a cada 3 a 5 segundos).

---

## 3. Modelo do Banco de Dados (Schema - Table `telemetry`)

| Coluna | Tipo SQL | Restrições | Descrição |
| :--- | :--- | :--- | :--- |
| `id` | `INTEGER` | PRIMARY KEY, AUTOINCREMENT | Identificador único do registro. |
| `device_id` | `TEXT` | NOT NULL | Identificador do dispositivo (ex: `"ESTUFA_SALA01"`). |
| `placa` | `TEXT` | NOT NULL | Modelo do hardware (ex: `"UNO_R4_WIFI"`). |
| `seq` | `INTEGER` | NOT NULL | Número sequencial da leitura enviada pelo dispositivo. |
| `temp` | `REAL` | NOT NULL | Valor de temperatura gravado (°C). |
| `umid` | `REAL` | NOT NULL | Valor de umidade relativa do ar (%). |
| `janela` | `BOOLEAN` | NOT NULL | Status do sensor de janela (`1` = Aberta, `0` = Fechada). |
| `luz_pct` | `INTEGER` | NOT NULL | Porcentagem de luminosidade ($0$ a $100\%$). |
| `estado` | `TEXT` | NOT NULL | Estado operacional da máquina de estados. |
| `created_at` | `DATETIME` | DEFAULT CURRENT_TIMESTAMP | Horário do registro no servidor. |

---

## 4. Endpoints da API

### `POST /api/telemetry`
- **Descrição:** Recebe e valida a telemetria do Arduino.
- **Payload de Exemplo:**
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
- **Respostas Esperadas:**
  - `201 Created`: Dado validado e salvo com sucesso.
  - `422 Unprocessable Entity`: Falha na validação dos campos.

### `GET /api/telemetry/latest`
- **Descrição:** Retorna a telemetria mais recente.
- **Resposta Esperada (`200 OK`):**
```json
{
  "id": 105,
  "device_id": "ESTUFA_SALA01",
  "placa": "UNO_R4_WIFI",
  "seq": 104,
  "temp": 25.4,
  "umid": 60.0,
  "janela": false,
  "luz_pct": 45,
  "estado": "NORMAL",
  "created_at": "2026-10-09T14:30:00"
}
```

### `GET /api/telemetry/history?limit=50`
- **Descrição:** Retorna a lista dos últimos $N$ registros em ordem cronológica.

---

## 5. Arquitetura de Arquivos do Servidor (Python)

```
/backend
│── main.py            # Instância principal do FastAPI e definição de rotas
│── database.py        # Configuração da conexão SQLite e SQLAlchemy
│── models.py          # Modelos de tabelas do banco de dados (ORM)
│── schemas.py         # Schemas de validação de dados (Pydantic)
│── requirements.txt   # Dependências do projeto (fastapi, uvicorn, sqlalchemy, pydantic)
│── /static            # Arquivos estáticos (CSS, JS, Chart.js)
└── /templates         # Arquivos HTML da interface do usuário (index.html)
```