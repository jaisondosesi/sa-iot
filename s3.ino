// ------------------------------
//   BIBLIOTECAS DO PROJETO
// ------------------------------

// Biblioteca para comunicação MQTT usando conexão segura (TLS/SSL)
#include <WiFiClientSecure.h>

// Biblioteca MQTT (publicar/assinar tópicos)
#include <PubSubClient.h>

// Biblioteca Wi-Fi padrão do ESP32 (conectar à internet)
#include <WiFi.h>

// Biblioteca para controlar servomotores no ESP32
#include <ESP32Servo.h>


// ------------------------------
//   DEFINIÇÃO DOS PINOS
// ------------------------------

// LED da placa
#define PINO_LED 2

// Sensor ultrassônico HC-SR04
#define TRIG 26
#define ECHO 25

// Servomotores
#define PINO_SERVO3 19
#define PINO_SERVO4 18

// Sensor PIR de presença
#define PINO_PRESENCA 14


// ------------------------------
//   OBJETOS DAS BIBLIOTECAS
// ------------------------------
WiFiClientSecure client;   // Cliente WiFi seguro (necessário para MQTT com TLS)
PubSubClient mqtt(client); // Cliente MQTT usando conexão segura
Servo servo3;              // Objeto para controlar o servo 3
Servo servo4;              // Objeto para controlar o servo 4


// ------------------------------
//   DADOS DO WI-FI
// ------------------------------
const char* SSID = "FIESC_IOT_EDU";   // Nome da rede
const char* PASS = "8120gv08";         // Senha do WiFi


// ------------------------------
//   CONFIGURAÇÕES DO BROKER MQTT
// ------------------------------
const char* BROKER_URL  = "2941e5c6678a4be18375b50465ad0964.s1.eu.hivemq.cloud"; // Endereço do broker
const int   BROKER_PORT = 8883;        // Porta segura TLS
const char* BROKER_USER = "Placa-S3-Joao";  // Usuário MQTT
const char* BROKER_PASS = "123456abX";     // Senha MQTT


// ------------------------------
//   TÓPICOS MQTT
// ------------------------------

// Publicações da S3
const char* TOPIC_PUBLISH_PRESENCA = "Projeto/S3/Presenca3";
const char* TOPIC_ULTRASSOM_S3     = "Projeto/S3/Ultrassom3";

// Comandos recebidos
const char* TOPICO_SUBSCRIBE   = "S1/iluminacao";        // S1 controla o LED da S3
const char* TOPICO_S2_SENSOR1  = "Projeto/S2/Distancia1"; // S2 sensor 1 → servo 3
const char* TOPICO_S2_SENSOR2  = "Projeto/S2/Distancia2"; // S2 sensor 2 → servo 4


// ------------------------------
//   CONTROLE DE PUBLICAÇÃO
// ------------------------------
unsigned long lastPublish = 0;  // Guarda o tempo do último envio
int publishInterval = 3000;     // Intervalo de envio (3 segundos)


// ------------------------------
//   Função para medir distância com HC-SR04
// ------------------------------
long medirDistancia(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);               // Garante nível baixo antes do pulso
  delayMicroseconds(3);

  digitalWrite(trigPin, HIGH);              // Pulso de 10 microssegundos
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duracao = pulseIn(echoPin, HIGH, 30000); // Mede tempo do pulso (máx 30 ms)
  long distancia = (duracao * 0.034) / 2;       // Converte para cm
  return distancia;
}


// ------------------------------
//   Callback MQTT
//   → Executada sempre que uma mensagem chega
// ------------------------------
void callback(char* topic, byte* payload, unsigned int length) {
  String mensagem;

  // Converte o payload em texto
  for (int i = 0; i < length; i++) {
    mensagem += (char)payload[i];
  }

  Serial.print("Mensagem recebida em ");
  Serial.print(topic);
  Serial.print(": ");
  Serial.println(mensagem);


  // --- Controle do LED enviado pela S1 ---
  if (String(topic) == TOPICO_SUBSCRIBE) {
    if (mensagem == "acender") digitalWrite(PINO_LED, HIGH);
    if (mensagem == "apagar")  digitalWrite(PINO_LED, LOW);
  }

  // --- Sensor 1 da S2 controla o servo 3 ---
  if (String(topic) == TOPICO_S2_SENSOR1) {
    if (mensagem == "objeto_proximo") servo3.write(90);
    if (mensagem == "objeto_longe")   servo3.write(45);
  }

  // --- Sensor 2 da S2 controla o servo 4 ---
  if (String(topic) == TOPICO_S2_SENSOR2) {
    if (mensagem == "objeto_proximo") servo4.write(90);
    if (mensagem == "objeto_longe")   servo4.write(45);
  }
}


// ------------------------------
//   Conexão WiFi
// ------------------------------
void conectarWiFi() {
  Serial.print("Conectando ao WiFi...");
  WiFi.begin(SSID, PASS);  // Inicia conexão

  while (WiFi.status() != WL_CONNECTED) {  // Aguarda conexão
    Serial.print(".");
    delay(300);
  }
  Serial.println("\nWiFi conectado!");
}


// ------------------------------
//   Conexão MQTT
// ------------------------------
void conectarMQTT() {
  mqtt.setServer(BROKER_URL, BROKER_PORT); // Configura servidor MQTT
  client.setInsecure();                    // TLS sem certificado
  mqtt.setCallback(callback);              // Define função callback

  while (!mqtt.connected()) {              // Tenta conectar até conseguir
    Serial.print("Conectando ao broker... ");

    // Gera ID aleatório para o cliente
    String clientId = "S3_" + String(random(0xffff), HEX);

    if (mqtt.connect(clientId.c_str(), BROKER_USER, BROKER_PASS)) {
      Serial.println("Conectado!");

      // Inscreve nos tópicos
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


// ------------------------------
//   SETUP
// ------------------------------
void setup() {
  Serial.begin(115200);  // Inicializa monitor serial

  pinMode(PINO_LED, OUTPUT);
  pinMode(PINO_PRESENCA, INPUT);
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);

  // Liga servos
  servo3.attach(PINO_SERVO3);
  servo4.attach(PINO_SERVO4);

  servo3.write(0);  // Posição inicial
  servo4.write(0);

  conectarWiFi();
  conectarMQTT();
}


// ------------------------------
//   LOOP PRINCIPAL
// ------------------------------
void loop() {
  if (!mqtt.connected()) conectarMQTT(); // Reconecta caso caia
  mqtt.loop();                           // Mantém MQTT ativo

  // Lê distância do ultrassom
  long distancia = medirDistancia(TRIG, ECHO);
  Serial.println(distancia);


  // Publica situação do ultrassom
  if (distancia > 0 && distancia < 10) {
    mqtt.publish(TOPIC_ULTRASSOM_S3, "objeto_proximo");
  } else if (distancia > 10) {
    mqtt.publish(TOPIC_ULTRASSOM_S3, "objeto_longe");
  }


  // Envio periódico para o broker
  unsigned long agora = millis();
  if (agora - lastPublish >= publishInterval) {

    lastPublish = agora;
    int presenca = digitalRead(PINO_PRESENCA);

    mqtt.publish(TOPIC_PUBLISH_PRESENCA, String(presenca).c_str());

    Serial.print("Presença publicada: ");
    Serial.println(presenca);
  }

  delay(20); // Pequeno delay para estabilidade
}
