//Turma: 1ESPA
//Equipe: Giovanna Oliveira Ferreira Dias - RM: 566647
//Maria Laura Druzeic - RM: 566634
//Marianne Mukai Nishikawa - RM:568001

#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <math.h>

// Bibliotecas para WiFi, MQTT, I2C, MPU6050 e cálculos matemáticos

// =========================
// CONFIGURAÇÕES DO WIFI
// =========================
const char* WIFI_SSID = "Wokwi-GUEST";
const char* WIFI_PASSWORD = "";

// =========================
// CONFIGURAÇÕES DO FIWARE / MQTT
// =========================
const char* MQTT_BROKER = "34.121.132.55";
const int MQTT_PORT = 1883;

// ID do dispositivo cadastrado no FIWARE
const char* DEVICE_ID = "move001";

// Tópico usado para enviar dados ao FIWARE
String topicAttrs = "/TEF/" + String(DEVICE_ID) + "/attrs";

// =========================
// OBJETOS DO SENSOR E MQTT
// =========================
Adafruit_MPU6050 mpu;

WiFiClient espClient;
PubSubClient client(espClient);

// =========================
// VARIÁVEIS DO CONTADOR
// =========================
int passos = 0;

float magnitude = 0;
float movimento = 0;
float movimentoFiltrado = 0;

// Evita contar o mesmo passo mais de uma vez
bool esperandoVoltar = false;

// Controle de tempo dos passos e envios
unsigned long ultimoPasso = 0;
unsigned long ultimoEnvio = 0;

// Parâmetros de calibração da contagem
const float gravidade = 9.81;
const float limitePasso = 3.2;
const float limiteRetorno = 1.2;
const unsigned long intervaloMinimo = 450;

// Intervalo para envio ao FIWARE
const unsigned long intervaloEnvio = 5000;

// =========================
// CONECTA AO WIFI
// =========================
void conectarWiFi() {
  Serial.print("Conectando ao WiFi");

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi conectado!");
  Serial.print("IP do ESP32: ");
  Serial.println(WiFi.localIP());
}

// =========================
// CONECTA AO MQTT
// =========================
void conectarMQTT() {
  while (!client.connected()) {
    Serial.print("Conectando ao MQTT...");

    if (client.connect(DEVICE_ID)) {
      Serial.println(" conectado!");
    } else {
      Serial.print(" falhou. Código: ");
      Serial.println(client.state());
      delay(3000);
    }
  }
}

// =========================
// ENVIA OS PASSOS AO FIWARE
// =========================
void enviarPassosFiware() {
  // Formato enviado: atributo|valor
  String payload = "s|" + String(passos);

  client.publish(topicAttrs.c_str(), payload.c_str());

  Serial.print("Enviado para FIWARE -> ");
  Serial.print(topicAttrs);
  Serial.print(" : ");
  Serial.println(payload);
}

// =========================
// CONFIGURAÇÃO INICIAL
// =========================
void setup() {
  Serial.begin(115200);

  // Inicia comunicação I2C nos pinos do ESP32
  Wire.begin(21, 22);

  conectarWiFi();

  // Define o broker MQTT
  client.setServer(MQTT_BROKER, MQTT_PORT);

  Serial.println("Iniciando MPU6050...");

  // Verifica se o sensor foi encontrado
  if (!mpu.begin()) {
    Serial.println("Erro: MPU6050 não encontrado!");
    while (1) {
      delay(10);
    }
  }

  Serial.println("MPU6050 conectado!");

  // Configura sensibilidade do sensor
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  delay(1000);
}

// =========================
// EXECUÇÃO PRINCIPAL
// =========================
void loop() {
  // Garante conexão MQTT ativa
  if (!client.connected()) {
    conectarMQTT();
  }

  client.loop();

  // Lê os dados do MPU6050
  sensors_event_t aceleracao, giro, temperatura;
  mpu.getEvent(&aceleracao, &giro, &temperatura);

  float ax = aceleracao.acceleration.x;
  float ay = aceleracao.acceleration.y;
  float az = aceleracao.acceleration.z;

  // Calcula a aceleração total
  magnitude = sqrt(ax * ax + ay * ay + az * az);

  // Remove o efeito da gravidade
  movimento = abs(magnitude - gravidade);

  // Suaviza o movimento para evitar ruídos
  movimentoFiltrado = (0.80 * movimentoFiltrado) + (0.20 * movimento);

  unsigned long agora = millis();

  // Detecta um passo válido
  if (!esperandoVoltar &&
      movimentoFiltrado > limitePasso &&
      agora - ultimoPasso > intervaloMinimo) {

    passos++;
    ultimoPasso = agora;
    esperandoVoltar = true;

    Serial.print("PASSO DETECTADO! Total: ");
    Serial.println(passos);
  }

  // Libera a próxima contagem após o movimento diminuir
  if (esperandoVoltar && movimentoFiltrado < limiteRetorno) {
    esperandoVoltar = false;
  }

  // Envia os dados ao FIWARE a cada 5 segundos
  if (agora - ultimoEnvio >= intervaloEnvio) {
    enviarPassosFiware();
    ultimoEnvio = agora;
  }

  // Exibe os dados no monitor serial
  Serial.print("Movimento: ");
  Serial.print(movimentoFiltrado);
  Serial.print(" | Passos: ");
  Serial.println(passos);

  delay(120);
}