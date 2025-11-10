#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

const char* SSID = "FIESC_IOT_EDU";
const char* PASS = "8120gv08";

const char* BROKER = "2941e5c6678a4be18375b50465ad0964.s1.eu.hivemq.cloud";
const int PORT = 8883;

const char* BROKER_USER = "Placa-S2-Jaison";
const char* BROKER_PASS = "SAdojoao10";

#define PINO_LED 2
#define PINO_SENSOR 4

const char* TOPICO_PUBLISH = "Projeto/S2/Presenca";
const char* TOPICO_SUBSCRIBE = "Projeto/S2/LED";

WiFiClientSecure espClient;
PubSubClient mqtt(espClient);

void callback(char* topic, byte* payload, unsigned int length) {
  String mensagem;
  for (int i = 0; i < length; i++) {
    mensagem += (char)payload[i];
  }

  Serial.print("Mensagem recebida em ");
  Serial.print(topic);
  Serial.print(": ");
  Serial.println(mensagem);

  if (mensagem == "ligar") {
    digitalWrite(PINO_LED, HIGH);
    Serial.println("LED ligado via MQTT");
  } 
  else if (mensagem == "desligar") {
    digitalWrite(PINO_LED, LOW);
    Serial.println("LED desligado via MQTT");
  }
}

void conectaWiFi() {
  Serial.print("Conectando à rede Wi-Fi: ");
  Serial.println(SSID);
  WiFi.begin(SSID, PASS);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWi-Fi conectado com sucesso!");
  Serial.print("Endereço IP: ");
  Serial.println(WiFi.localIP());
}

void conectaMQTT() {
  mqtt.setServer(BROKER, PORT);
  mqtt.setCallback(callback);
  espClient.setInsecure();

  while (!mqtt.connected()) {
    String clientId = "S2_" + String(random(0xffff), HEX);
    Serial.print("Conectando ao broker MQTT... ");
    if (mqtt.connect(clientId.c_str(), BROKER_USER, BROKER_PASS)) {
      Serial.println("Conectado com sucesso!");
      mqtt.subscribe(TOPICO_SUBSCRIBE);
      Serial.print("Inscrito no tópico: ");
      Serial.println(TOPICO_SUBSCRIBE);
    } else {
      Serial.print("Falha (rc=");
      Serial.print(mqtt.state());
      Serial.println("). Tentando novamente...");
      delay(2000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(PINO_LED, OUTPUT);
  pinMode(PINO_SENSOR, INPUT);

  conectaWiFi();
  conectaMQTT();
}

void loop() {
  if (!mqtt.connected()) {
    conectaMQTT();
  }
  mqtt.loop();

  int presenca = digitalRead(PINO_SENSOR);
  String mensagem = String(presenca);
  mqtt.publish(TOPICO_PUBLISH, mensagem.c_str());

  Serial.print("Detecção de presença: ");
  Serial.println(presenca);

  delay(2000);
}
