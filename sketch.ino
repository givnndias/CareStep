//Turma: 1ESPA
//Equipe: Giovanna Oliveira Ferreira Dias - RM: 566647
//Maria Laura Druzeic - RM: 566634
//Marianne Mukai Nishikawa - RM:568001

/*
 * MONITOR DE PASSOS VESTÍVEL PARA TÊNIS - ESP32-C3 + GY-87 + FIWARE
 *
 * Possui:
 * - Leitura do acelerômetro MPU6050 do GY-87 por I2C direto
 * - Contador de passos
 * - Envio para FIWARE Orion por Wi-Fi usando WiFiClient
 * - LED verde somente para bateria baixa
 * - Leitura da bateria no GPIO 1
 *
 * PINOS:
 * GPIO 4 -> SDA GY-87
 * GPIO 5 -> SCL GY-87
 * GPIO 2 -> LED verde bateria
 * GPIO 1 -> ADC bateria
 */

#include <Wire.h>
#include <WiFi.h>

// ===================== PINOS =====================

#define PIN_SDA             4
#define PIN_SCL             5
#define PIN_LED_BATERIA     2
#define PIN_ADC_BATERIA     1

// ===================== WI-FI =====================

const char* WIFI_SSID  = "SEU_WIFI";
const char* WIFI_SENHA = "SUA_SENHA";

// ===================== FIWARE =====================

// Coloque aqui o IP do computador/servidor onde o Orion está rodando.
// Exemplo: 192.168.0.100
const char* ORION_HOST = "136.111.181.27";
const uint16_t ORION_PORT = 1026;

const char* FIWARE_SERVICE = "careplus";
const char* FIWARE_SERVICE_PATH = "/tenis";

const char* ENTITY_ID = "urn:ngsi-ld:StepMonitor:tenis001";
const char* ENTITY_TYPE = "StepMonitor";

const uint32_t INTERVALO_FIWARE_MS = 10000;

// ===================== MPU6050 / GY-87 =====================

const uint8_t MPU_ADDR = 0x68;
const float ACC_SCALE = 8192.0f;   // range +-4g
const float GRAVIDADE = 9.81f;

// ===================== PASSOS =====================

const float LIMIAR_PASSO = 3.5f;
const uint32_t INTERVALO_MIN_MS = 450;

uint32_t totalPassos = 0;
uint32_t ultimoPasso_ms = 0;
bool picoAtivo = false;

// ===================== BATERIA =====================

const uint16_t BATERIA_BAIXA_MV   = 3500;
const uint16_t BATERIA_CRITICA_MV = 3200;
const uint16_t BATERIA_VAZIA_MV   = 3000;
const uint32_t INTERVALO_BATERIA_MS = 30000;

uint16_t tensaoBateria_mV = 0;
uint32_t ultimaLeituraBat_ms = 0;
uint32_t ultimoPiscaBateria_ms = 0;
bool estadoLedBateria = false;

// ===================== ESTADO =====================

uint32_t ultimoEnvioFiware_ms = 0;
bool entidadeCriada = false;

float ultimoAx = 0;
float ultimoAy = 0;
float ultimoAz = 0;
float ultimaIntensidade = 0;

// ===================== PROTÓTIPOS =====================

bool iniciarMPU();
bool lerMPU(float &ax, float &ay, float &az, float &intensidade);
void detectarPasso(float intensidade);

void conectarWiFi();
bool enviarHttp(const char* metodo, const char* caminho, const String &body, int &codigoHttp);
String jsonCriacao();
String jsonAttrs();
bool criarEntidade();
void enviarFiware(bool forcar = false);

uint16_t lerBateria();
void monitorarBateria();
void atualizarLEDBateria(uint16_t mv);

// ===================== SETUP =====================

void setup() {
  Serial.begin(115200);
  delay(300);

  Serial.println();
  Serial.println("Monitor Passos ESP32-C3 - versao reduzida");

  pinMode(PIN_LED_BATERIA, OUTPUT);
  digitalWrite(PIN_LED_BATERIA, LOW);

  analogReadResolution(12);
  analogSetPinAttenuation(PIN_ADC_BATERIA, ADC_11db);

  Wire.begin(PIN_SDA, PIN_SCL);

  if (!iniciarMPU()) {
    Serial.println("ERRO: MPU6050/GY-87 nao encontrado");
    while (true) delay(1000);
  }

  tensaoBateria_mV = lerBateria();
  ultimaLeituraBat_ms = millis();
  atualizarLEDBateria(tensaoBateria_mV);

  Serial.printf("Bateria inicial: %u mV\n", tensaoBateria_mV);

  if (tensaoBateria_mV > 100 && tensaoBateria_mV < BATERIA_VAZIA_MV) {
    Serial.println("Bateria critica");
}

  conectarWiFi();
  enviarFiware(true);
}

// ===================== LOOP =====================

void loop() {


  float ax, ay, az, intensidade;

  if (lerMPU(ax, ay, az, intensidade)) {
    ultimoAx = ax;
    ultimoAy = ay;
    ultimoAz = az;
    ultimaIntensidade = intensidade;

    detectarPasso(intensidade);
  }

  monitorarBateria();
  enviarFiware(false);

  static uint32_t ultimoSerial = 0;
  if (millis() - ultimoSerial >= 1000) {
    ultimoSerial = millis();
    Serial.printf("Passos:%u Bat:%u Int:%.2f\n", totalPassos, tensaoBateria_mV, ultimaIntensidade);
  }

  delay(20);
}

// ===================== MPU6050 DIRETO =====================

bool iniciarMPU() {
  Wire.beginTransmission(MPU_ADDR);
  if (Wire.endTransmission() != 0) return false;

  // Acorda MPU6050: PWR_MGMT_1 = 0
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B);
  Wire.write(0x00);
  Wire.endTransmission();

  delay(100);

  // Configura acelerômetro para +-4g: ACCEL_CONFIG = 0x08
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x1C);
  Wire.write(0x08);
  Wire.endTransmission();

  // Filtro passa-baixa aproximado 10 Hz: CONFIG = 0x05
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x1A);
  Wire.write(0x05);
  Wire.endTransmission();

  Serial.println("MPU6050 OK");
  return true;
}

int16_t ler16(uint8_t reg) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(reg);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, (uint8_t)2);

  if (Wire.available() < 2) return 0;

  int16_t valor = (Wire.read() << 8) | Wire.read();
  return valor;
}

bool lerMPU(float &ax, float &ay, float &az, float &intensidade) {
  int16_t rawX = ler16(0x3B);
  int16_t rawY = ler16(0x3D);
  int16_t rawZ = ler16(0x3F);

  ax = ((float)rawX / ACC_SCALE) * GRAVIDADE;
  ay = ((float)rawY / ACC_SCALE) * GRAVIDADE;
  az = ((float)rawZ / ACC_SCALE) * GRAVIDADE;

  float mag = sqrt(ax * ax + ay * ay + az * az);
  intensidade = fabs(mag - GRAVIDADE);

  return true;
}

void detectarPasso(float intensidade) {

  const float LIMIAR_SUBIDA = 3.5f;
  const float LIMIAR_DESCIDA = 1.5f;

  uint32_t agora = millis();

  if (!picoAtivo) {

    if (intensidade > LIMIAR_SUBIDA) {
      picoAtivo = true;
    }

  } else {

    if (intensidade < LIMIAR_DESCIDA) {

      picoAtivo = false;

      if (agora - ultimoPasso_ms >= 450) {

        ultimoPasso_ms = agora;
        totalPassos++;

        Serial.printf("PASSO %u\n", totalPassos);

      }
    }
  }
}

// ===================== WI-FI =====================

void conectarWiFi() {
  if (WiFi.status() == WL_CONNECTED) {
    return;
  }

  Serial.println("[WiFi] Reiniciando conexao...");

  WiFi.disconnect(true);
  delay(1000);

  WiFi.mode(WIFI_STA);
  delay(500);

  Serial.print("[WiFi] Conectando em: ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_SENHA);

  uint32_t inicio = millis();

  while (WiFi.status() != WL_CONNECTED && millis() - inicio < 20000) {
    Serial.print(".");
    delay(500);
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.print("[WiFi] Conectado! IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println();
    Serial.print("[WiFi] Falhou. Status: ");
    Serial.println(WiFi.status());
  }
}

// ===================== FIWARE HTTP SEM HTTPCLIENT =====================

bool enviarHttp(const char* metodo, const char* caminho, const String &body, int &codigoHttp) {
  codigoHttp = -1;

  WiFiClient client;

  if (!client.connect(ORION_HOST, ORION_PORT)) {
    Serial.println("Falha conexao Orion");
    return false;
  }

  client.print(String(metodo) + " " + caminho + " HTTP/1.1\r\n");
  client.print(String("Host: ") + ORION_HOST + ":" + ORION_PORT + "\r\n");
  client.print("Connection: close\r\n");
  client.print("Content-Type: application/json\r\n");
  client.print(String("Fiware-Service: ") + FIWARE_SERVICE + "\r\n");
  client.print(String("Fiware-ServicePath: ") + FIWARE_SERVICE_PATH + "\r\n");
  client.print(String("Content-Length: ") + body.length() + "\r\n");
  client.print("\r\n");
  client.print(body);

  uint32_t inicio = millis();

  while (!client.available() && millis() - inicio < 5000) {
    delay(10);
  }

  if (!client.available()) {
    client.stop();
    Serial.println("Sem resposta Orion");
    return false;
  }

  String statusLine = client.readStringUntil('\n');
  statusLine.trim();

  int p1 = statusLine.indexOf(' ');
  int p2 = statusLine.indexOf(' ', p1 + 1);

  if (p1 > 0 && p2 > p1) {
    codigoHttp = statusLine.substring(p1 + 1, p2).toInt();
  }

  client.stop();
  return true;
}

String jsonCriacao() {
  String j;
  j.reserve(360);

  j += "{\"id\":\"";
  j += ENTITY_ID;
  j += "\",\"type\":\"";
  j += ENTITY_TYPE;
  j += "\",";
  j += "\"steps\":{\"value\":";
  j += totalPassos;
  j += ",\"type\":\"Number\"},";
  j += "\"battery\":{\"value\":";
  j += tensaoBateria_mV;
  j += ",\"type\":\"Number\"},";
  j += "\"accelX\":{\"value\":";
  j += String(ultimoAx, 2);
  j += ",\"type\":\"Number\"},";
  j += "\"accelY\":{\"value\":";
  j += String(ultimoAy, 2);
  j += ",\"type\":\"Number\"},";
  j += "\"accelZ\":{\"value\":";
  j += String(ultimoAz, 2);
  j += ",\"type\":\"Number\"},";
  j += "\"intensity\":{\"value\":";
  j += String(ultimaIntensidade, 2);
  j += ",\"type\":\"Number\"},";
  j += "\"status\":{\"value\":\"active\",\"type\":\"Text\"}}";

  return j;
}

String jsonAttrs() {
  String j;
  j.reserve(320);

  j += "{\"steps\":{\"value\":";
  j += totalPassos;
  j += ",\"type\":\"Number\"},";
  j += "\"battery\":{\"value\":";
  j += tensaoBateria_mV;
  j += ",\"type\":\"Number\"},";
  j += "\"accelX\":{\"value\":";
  j += String(ultimoAx, 2);
  j += ",\"type\":\"Number\"},";
  j += "\"accelY\":{\"value\":";
  j += String(ultimoAy, 2);
  j += ",\"type\":\"Number\"},";
  j += "\"accelZ\":{\"value\":";
  j += String(ultimoAz, 2);
  j += ",\"type\":\"Number\"},";
  j += "\"intensity\":{\"value\":";
  j += String(ultimaIntensidade, 2);
  j += ",\"type\":\"Number\"},";
  j += "\"status\":{\"value\":\"active\",\"type\":\"Text\"}}";

  return j;
}

bool criarEntidade() {
  if (entidadeCriada) return true;
  if (WiFi.status() != WL_CONNECTED) return false;

  int code = -1;
  bool ok = enviarHttp("POST", "/v2/entities", jsonCriacao(), code);

  if (ok && (code == 201 || code == 422)) {
    entidadeCriada = true;
    Serial.printf("Entidade OK HTTP %d\n", code);
    return true;
  }

  Serial.printf("Erro criar entidade HTTP %d\n", code);
  return false;
}

void enviarFiware(bool forcar) {
  uint32_t agora = millis();

  if (!forcar && agora - ultimoEnvioFiware_ms < INTERVALO_FIWARE_MS) {
    return;
  }

  ultimoEnvioFiware_ms = agora;

  if (WiFi.status() != WL_CONNECTED) {
    conectarWiFi();
    if (WiFi.status() != WL_CONNECTED) return;
  }

  if (!criarEntidade()) return;

  String caminho = String("/v2/entities/") + ENTITY_ID + "/attrs";

  int code = -1;
  bool ok = enviarHttp("PATCH", caminho.c_str(), jsonAttrs(), code);

  if (ok && code >= 200 && code < 300) {
    Serial.println("FIWARE enviado");
  } else if (code == 404) {
    entidadeCriada = false;
    Serial.println("Entidade sumiu");
  } else {
    Serial.printf("Erro FIWARE HTTP %d\n", code);
  }
}

// ===================== BATERIA =====================

uint16_t lerBateria() {
  uint32_t soma = 0;

  for (uint8_t i = 0; i < 16; i++) {
    soma += analogRead(PIN_ADC_BATERIA);
    delay(2);
  }

  uint16_t media = soma / 16;

  if (media < 50) return 0;

  uint16_t vadc = (uint32_t)media * 3300 / 4095;

  // Divisor 100k + 100k
  return vadc * 2;
}

void monitorarBateria() {
  atualizarLEDBateria(tensaoBateria_mV);

  uint32_t agora = millis();

  if (agora - ultimaLeituraBat_ms < INTERVALO_BATERIA_MS) return;

  ultimaLeituraBat_ms = agora;
  tensaoBateria_mV = lerBateria();

  Serial.printf("BAT %u mV\n", tensaoBateria_mV);

  if (tensaoBateria_mV > 100 && tensaoBateria_mV < BATERIA_VAZIA_MV) {
    Serial.println("Bateria critica");
  }
}

void atualizarLEDBateria(uint16_t mv) {
  if (mv == 0 || mv >= BATERIA_BAIXA_MV) {
    digitalWrite(PIN_LED_BATERIA, LOW);
    return;
  }

  if (mv >= BATERIA_CRITICA_MV) {
    digitalWrite(PIN_LED_BATERIA, HIGH);
    return;
  }

  if (millis() - ultimoPiscaBateria_ms >= 300) {
    ultimoPiscaBateria_ms = millis();
    estadoLedBateria = !estadoLedBateria;
    digitalWrite(PIN_LED_BATERIA, estadoLedBateria ? HIGH : LOW);
  }
}



