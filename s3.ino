#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <ESP32Servo.h>

#define PINO_LED 2
#define TRIG 26
#define ECHO 25
#define PINO_SERVO3 19
#define PINO_SERVO4 18
#define PINO_PRESENCA 14

WiFiClientSecure client;
PubSubClient mqtt(client);
Servo servo3;
Servo servo4;

const char* SSID = "FIESC_IOT_EDU";
const char* PASS = "8120gv08";

const char* BROKER_URL  = "7aecec580ecf4e5cbac2d52b35eb85b9.s1.eu.hivemq.cloud";
const int   BROKER_PORT = 8883;
const char* BROKER_USER = "Placa-3-Joao";
const char* BROKER_PASS = "123456abX";

const char* TOPIC_PUBLISH_PRESENCA   = "Projeto/S3/Presenca3";
const char* TOPIC_ULTRASSOM_S3       = "Projeto/S3/Ultrassom3";

const char* TOPICO_SUBSCRIBE   = "S1/iluminacao";
const char* TOPICO_S2_SENSOR1  = "Projeto/S2/Distancia1";
const char* TOPICO_S2_SENSOR2  = "Projeto/S2/Distancia2";

unsigned long lastPublish = 0;
int publishInterval = 3000;

// --- Função: medir distância ---
long medirDistancia(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(3);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duracao = pulseIn(echoPin, HIGH, 30000);
  long distancia = (duracao * 0.034) / 2;
  return distancia;
}

// --- Callback MQTT ---
void callback(char* topic, byte* payload, unsigned int length) {
  String mensagem;

  for (int i = 0; i < length; i++) {
    mensagem += (char)payload[i];
  }

  Serial.print("Mensagem recebida em ");
  Serial.print(topic);
  Serial.print(": ");
  Serial.println(mensagem);

  // Controle do LED vindo da S1
  if (String(topic) == TOPICO_SUBSCRIBE) {
    if (mensagem == "acender") digitalWrite(PINO_LED, HIGH);
    if (mensagem == "apagar")  digitalWrite(PINO_LED, LOW);
  }

  // Comando do Sensor 1 da S2 → Servo 3
  if (String(topic) == TOPICO_S2_SENSOR1) {
    if (mensagem == "objeto_proximo") servo3.write(90);
    if (mensagem == "objeto_longe")   servo3.write(45);
  }

  // Comando do Sensor 2 da S2 → Servo 4
  if (String(topic) == TOPICO_S2_SENSOR2) {
    if (mensagem == "objeto_proximo") servo4.write(90);
    if (mensagem == "objeto_longe")   servo4.write(45);
  }
}

// --- Conectar WiFi ---
void conectarWiFi() {
  Serial.print("Conectando ao WiFi...");
  WiFi.begin(SSID, PASS);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println("\nWiFi conectado!");
}

// --- Conectar MQTT ---
void conectarMQTT() {
  mqtt.setServer(BROKER_URL, BROKER_PORT);
  client.setInsecure();
  mqtt.setCallback(callback);

  while (!mqtt.connected()) {
    Serial.print("Conectando ao broker... ");

    String clientId = "S3_" + String(random(0xffff), HEX);

    if (mqtt.connect(clientId.c_str(), BROKER_USER, BROKER_PASS)) {
      Serial.println("Conectado!");

      mqtt.subscribe(TOPICO_SUBSCRIBE);
      mqtt.subscribe(TOPICO_S2_SENSOR1);
      mqtt.subscribe(TOPICO_S2_SENSOR2);

    } else {
      Serial.print("Falha. Código: ");
      Serial.println(mqtt.state());
      delay(1500);
    }
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(PINO_LED, OUTPUT);
  pinMode(PINO_PRESENCA, INPUT);
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);

  servo3.attach(PINO_SERVO3);
  servo4.attach(PINO_SERVO4);

  servo3.write(0);
  servo4.write(0);

  conectarWiFi();
  conectarMQTT();
}

// --- Loop principal ---
void loop() {
  if (!mqtt.connected()) conectarMQTT();
  mqtt.loop();

  long distancia = medirDistancia(TRIG, ECHO);
  Serial.println(distancia);

  if (distancia > 0 && distancia < 10) {
    mqtt.publish(TOPIC_ULTRASSOM_S3, "objeto_proximo");
  } else if (distancia > 10) {
    mqtt.publish(TOPIC_ULTRASSOM_S3, "objeto_longe");
  }

  unsigned long agora = millis();
  if (agora - lastPublish >= publishInterval) {
    lastPublish = agora;
    int presenca = digitalRead(PINO_PRESENCA);

    mqtt.publish(TOPIC_PUBLISH_PRESENCA, String(presenca).c_str());
    
    Serial.print("Presença publicada: ");
    Serial.println(presenca);
  }

  delay(20);
}
