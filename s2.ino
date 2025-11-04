#include <WiFiClientSecure.h>
#include <PubSubClient.h>

WiFiClientSecure client;
PubSubClient mqtt(client);

#define LED 2

const char* SSID = "FIESC_IOT_EDU";
const char* PASS = "8120gv08";

const char* BROKER_URL = "7aecec580ecf4e5cbac2d52b35eb85b9.s1.eu.hivemq.cloud";
const int BROKER_PORT = 8883;
const char* BROKER_USER = "Placa-2-Jaison";
const char* BROKER_PASS = "123456abX";

const char* TOPIC_SEND = "Jaison";
const char* TOPIC_RECEIVE = "Joao";

void callback(char* topic, byte* payload, unsigned int length) {
  String msg = "";
  for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];
  Serial.print("Recebido de ");
  Serial.print(topic);
  Serial.print(": ");
  Serial.println(msg);
  if (msg == "Joao: Acender") {
    digitalWrite(LED, HIGH);
    Serial.println("LED ligado");
  } else if (msg == "Joao: Apagar") {
    digitalWrite(LED, LOW);
    Serial.println("LED desligado");
  }
}

void conectarWiFi() {
  Serial.print("Conectando ao Wi-Fi ");
  Serial.print(SSID);
  WiFi.begin(SSID, PASS);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println("\nWi-Fi conectado");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

void conectarMQTT() {
  Serial.print("Conectando ao broker MQTT");
  while (!mqtt.connected()) {
    String id = "ESP32_";
    id += String(random(0xffff), HEX);
    if (mqtt.connect(id.c_str(), BROKER_USER, BROKER_PASS)) {
      Serial.println("\nConectado ao broker");
      mqtt.subscribe(TOPIC_RECEIVE);
    } else {
      Serial.print(".");
      delay(500);
    }
  }
}

void setup() {
  pinMode(LED, OUTPUT);
  Serial.begin(115200);
  conectarWiFi();
  client.setInsecure();
  mqtt.setServer(BROKER_URL, BROKER_PORT);
  mqtt.setCallback(callback);
  conectarMQTT();
}

void loop() {
  if (!mqtt.connected()) conectarMQTT();
  if (Serial.available() > 0) {
    String msg = "Jaison: ";
    msg += Serial.readStringUntil('\n');
    mqtt.publish(TOPIC_SEND, msg.c_str());
    Serial.print("Enviado: ");
    Serial.println(msg);
  }
  mqtt.loop();
}
