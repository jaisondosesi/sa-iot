#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ESP32Servo.h>

#define PINO_LED      2
#define PINO_TRIG     26
#define PINO_ECHO     25
#define PINO_SERVO_3  19
#define PINO_SERVO_4  18
#define PINO_PRESENCA 14

// ===== OBJETOS GLOBAIS =====
WiFiClientSecure espClient;
PubSubClient mqtt(espClient);
Servo servo3;
Servo servo4;

const char* WIFI_SSID = "FIESC_IOT_EDU";
const char* WIFI_PASS = "8120gv08";

const char* BROKER_URL  = "2941e5c6678a4be18375b50465ad0964.s1.eu.hivemq.cloud";
const int   BROKER_PORT = 8883;
const char* BROKER_USER = "Placa-S3-Jaison";
const char* BROKER_PASS = "SAdojoao10";

// Envio (Publicar)
const char* PUB_ULTRASSOM = "Projeto/S3/Ultrassom3";
const char* PUB_PRESENCA  = "Projeto/S3/Presenca3";

// Recebimento (Assinar)
const char* SUB_LED       = "S1/iluminacao";
const char* SUB_SERVO_3   = "Projeto/S2/Distancia1";
const char* SUB_SERVO_4   = "Projeto/S2/Distancia2";


// FUNÇÃO: MEDIR DISTÂNCIA (Ultrassom) =====
long medirDistancia() {
  // Envia pulso
  digitalWrite(PINO_TRIG, LOW);  delayMicroseconds(2);
  digitalWrite(PINO_TRIG, HIGH); delayMicroseconds(10);
  digitalWrite(PINO_TRIG, LOW);

  // Lê o retorno
  long duracao = pulseIn(PINO_ECHO, HIGH, 25000);
  
  if (duracao == 0) return 0; 
  return (duracao * 0.034) / 2; // Converte para cm
}


// CALLBACK MQTT (Ocorre ao receber mensagem) =====
void callback(char* topic, byte* payload, unsigned int length) {
  // Converte bytes recebidos para Texto
  String mensagem = "";
  for (int i = 0; i < length; i++) mensagem += (char)payload[i];
  String t = String(topic);

  Serial.println("Recebido [" + t + "]: " + mensagem);

  // 1. Comando S1 -> LED
  if (t == SUB_LED) {
    if (mensagem == "acender") digitalWrite(PINO_LED, HIGH);
    if (mensagem == "apagar")  digitalWrite(PINO_LED, LOW);
  }

  // 2. Comando S2 -> Servo 3
  if (t == SUB_SERVO_3) {
    if (mensagem == "objeto_proximo") servo3.write(90); // Abre
    else servo3.write(0);                               // Fecha
  }

  // 3. Comando S2 -> Servo 4
  if (t == SUB_SERVO_4) {
    if (mensagem == "objeto_proximo") servo4.write(90); // Abre
    else servo4.write(0);                               // Fecha
  }
}


// ===== CONEXÃO WI-FI =====
void conectarWiFi() {
  Serial.print("Conectando Wi-Fi...");
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  // Tenta conectar (max 20 tentativas)
  int tentativas = 0;
  while (WiFi.status() != WL_CONNECTED && tentativas < 20) {
    delay(500);
    Serial.print(".");
    tentativas++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(" OK!");
    espClient.setInsecure(); // OBRIGATÓRIO: Permite SSL no HiveMQ
  } else {
    Serial.println(" Falha.");
  }
}


// ===== CONEXÃO MQTT =====
void conectarMQTT() {
  mqtt.setServer(BROKER_URL, BROKER_PORT);
  mqtt.setCallback(callback); // Define função que "escuta"

  while (!mqtt.connected()) {
    Serial.print("Conectando MQTT...");
    
    // ID único para não cair a conexão
    String id = "S3_Jaison_" + String(random(0xffff), HEX);

    if (mqtt.connect(id.c_str(), BROKER_USER, BROKER_PASS)) {
      Serial.println(" OK!");
      
      // Volta a ouvir os tópicos
      mqtt.subscribe(SUB_LED);
      mqtt.subscribe(SUB_SERVO_3);
      mqtt.subscribe(SUB_SERVO_4);
    } else {
      Serial.print(" Erro. Tentando de novo...");
      delay(2000);
    }
  }
}


// SETUP (Configuração Inicial)
void setup() {
  Serial.begin(115200);

  // Configura Hardware
  pinMode(PINO_LED, OUTPUT);
  pinMode(PINO_TRIG, OUTPUT);
  pinMode(PINO_ECHO, INPUT);
  pinMode(PINO_PRESENCA, INPUT);

  // Configura Motores (Inicializa fechado)
  servo3.attach(PINO_SERVO_3);
  servo4.attach(PINO_SERVO_4);
  servo3.write(0);
  servo4.write(0);

  // Inicia Rede
  conectarWiFi();
  conectarMQTT();
}


// LOOP (Execução Contínua)
void loop() {
  // Mantém conexões ativas
  if (WiFi.status() != WL_CONNECTED) conectarWiFi();
  if (!mqtt.connected()) conectarMQTT();
  mqtt.loop();

  // 1. Lógica do Ultrassom
  long distancia = medirDistancia();
  
  if (distancia > 0) {
    if (distancia < 10) mqtt.publish(PUB_ULTRASSOM, "objeto_proximo");
    else                mqtt.publish(PUB_ULTRASSOM, "objeto_longe");
  }

  // 2. Lógica da Presença (A cada 3s)
  static unsigned long ultimoEnvio = 0;
  if (millis() - ultimoEnvio > 3000) {
    ultimoEnvio = millis(); // Atualiza timer
    
    int presenca = digitalRead(PINO_PRESENCA);
    mqtt.publish(PUB_PRESENCA, String(presenca).c_str());
    
    Serial.println("Presença enviada: " + String(presenca));
  }
  
  delay(100); // Estabilidade
}
