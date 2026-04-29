# 👟 CareStep - Monitoramento Inteligente de Passos

## 📌 Descrição do Projeto

O **CareStep** é um dispositivo IoT voltado para monitoramento de atividade física, capaz de identificar e registrar a quantidade de passos realizados pelo usuário.

O sistema foi projetado para ser versátil, podendo ser utilizado de duas formas:

* 🔑 Como **chaveiro inteligente**, facilitando o uso diário
* 👟 **Embutido no tênis**, permitindo uma coleta de dados mais precisa durante caminhadas e corridas

A solução utiliza um **ESP32** conectado ao sensor **MPU6050**, responsável por capturar dados de aceleração e movimento. Esses dados são processados localmente para detectar passos e enviados para a plataforma **FIWARE** via protocolo **MQTT**.

---

## 🧠 Arquitetura da Solução

A arquitetura segue um modelo em camadas, integrando hardware, comunicação e backend IoT.

---

## ⚙️ Tecnologias Utilizadas

* ESP32
* MPU6050 (acelerômetro)
* Wokwi (simulação)
* FIWARE
* MQTT (Mosquitto)
* Python / Node-RED (Dashboard)
* Postman

---

## 🔌 Dispositivo IoT

O CareStep pode ser utilizado em dois formatos:

### 🔑 Chaveiro Inteligente

* Portátil
* Uso contínuo no dia a dia
* Monitoramento básico de movimento

### 👟 Sensor Embutido no Tênis

* Maior precisão na detecção de passos
* Ideal para atividades físicas (corrida/caminhada)
* Melhor estabilidade dos dados

---

## 🔬 Funcionamento

O dispositivo realiza:

1. Leitura da aceleração nos eixos X, Y e Z
2. Cálculo da magnitude da aceleração
3. Remoção do efeito da gravidade
4. Aplicação de filtro para reduzir ruídos
5. Detecção de picos de movimento (passos)
6. Envio dos dados para o FIWARE via MQTT

---

## 📡 Comunicação MQTT

```txt
Tópico:
/TEF/move001/attrs

Payload:
s|10
```

Onde:

* `s` → atributo de passos
* `10` → quantidade de passos

---

## 🧪 Simulação no Wokwi

O projeto foi desenvolvido e testado utilizando o simulador online Wokwi, que permite a simulação de circuitos com ESP32 e sensores de forma prática e interativa.

🔗 **Acesse a simulação:**
https://wokwi.com/projects/461399361402004481

---

### 🔹 Componentes utilizados

- ESP32  
- Sensor MPU6050 (acelerômetro)

---

### 🔹 Funcionamento na simulação

No Wokwi, o sensor MPU6050 simula os movimentos do dispositivo. A variação dos valores de aceleração nos eixos X, Y e Z permite reproduzir o comportamento de uma pessoa em movimento.

Para simular passos:

1. Inicie a simulação  
2. Clique no sensor MPU6050  
3. Altere os valores de aceleração  
4. Observe o aumento da contagem de passos no Serial Monitor  

---

## ☁️ Configuração do FIWARE

### 🔹 Service Group

```json
{
  "services": [
    {
      "apikey": "123456",
      "cbroker": "http://orion:1026",
      "entity_type": "movement",
      "resource": "/iot/d"
    }
  ]
}
```

---

### 🔹 Device

```json
{
  "devices": [
    {
      "device_id": "move001",
      "entity_name": "urn:ngsi-id:move:001",
      "entity_type": "movement",
      "protocol": "PDI-IoTA-UltraLight",
      "transport": "MQTT",
      "attributes": [
        {
          "object_id": "s",
          "name": "steps",
          "type": "Integer"
        }
      ]
    }
  ]
}
```

---

### 🔹 Subscription (Histórico)

```json
{
  "description": "Salvar histórico de passos",
  "subject": {
    "entities": [
      {
        "id": "urn:ngsi-id:move:001",
        "type": "movement"
      }
    ],
    "condition": {
      "attrs": ["steps"]
    }
  },
  "notification": {
    "http": {
      "url": "http://sth-comet:8666/notify"
    },
    "attrs": ["steps"]
  }
}
```

---

## 📊 Consulta de Dados

### 🔹 Estado Atual (Orion)

```txt
GET /v2/entities
```

---

### 🔹 Histórico (STH-Comet)

```txt
GET /STH/v1/contextEntities/type/movement/id/urn:ngsi-id:move:001/attributes/steps?hLimit=100&hOffset=0
```

---

## 📦 Itens de Entrega

* 🔗 Link da simulação no Wokwi
* 💻 Repositório GitHub público
* 🎥 Vídeo explicativo (3 minutos)

---

## 🚀 Possíveis Melhorias

* Cálculo de distância percorrida
* Estimativa de calorias
* Integração com aplicativo mobile
* Dashboard com gráficos em tempo real

---

## 👥 Equipe

| Nome                          | RM     |
|------------------------------|--------|
| Giovanna Oliveira Ferreira Dias | 566647 |
| Maria Laura Druzeic            | 566634 |
| Marianne Mukai Nishikawa       | 568001 |

---

## 📄 Licença

Projeto desenvolvido para fins acadêmicos.
