// ------------------------------
//   BIBLIOTECAS DO PROJETO
// ------------------------------

// Permite conexão segura (criptografada) com o broker MQTT usando TLS/SSL
#include <WiFiClientSecure.h>

// Biblioteca que envia e recebe mensagens MQTT (publish/subscribe)
#include <PubSubClient.h>

// Biblioteca que conecta o ESP32 ao Wi-Fi
#include <WiFi.h>

// Controla servomotores usando o ESP32
#include <ESP32Servo.h>


// ------------------------------
//   DEFINIÇÃO DOS PINOS
// ------------------------------

// LED embutido da placa (para ligar e desligar)
#define PINO_LED 2

// Pinos do sensor ultrassônico HC-SR04
#define TRIG 26   // Envia o pulso
#define ECHO 25   // Recebe o eco

// Pinos dos servo motores
#define PINO_SERVO3 19
#define PINO_SERVO4 18

// Sensor PIR (detecta movimento/presença)
#define PINO_PRESENCA 14


// ------------------------------
//   OBJETOS DAS BIBLIOTECAS
// ------------------------------

// Cliente WiFi seguro (necessário para conexão TLS no HiveMQ Cloud)
WiFiClientSecure client;

// Cliente MQTT que envia/recebe mensagens do broker
PubSubClient mqtt(client);

// Objetos para controlar os servos
Servo servo3;
Servo servo4;


// ------------------------------
//   DADOS DO WI-FI
// ------------------------------
// Esses são o nome e a senha da rede Wi-Fi onde a placa vai se conectar
const char* SSID = "FIESC_IOT_EDU";
const char* PASS = "8120gv08";


// ------------------------------
//   CONFIGURAÇÕES DO BROKER MQTT
// ------------------------------

// Endereço do broker (servidor MQTT na nuvem)
const char* BROKER_URL  = "2941e5c6678a4be18375b50465ad0964.s1.eu.hivemq.cloud";

// Porta usada quando a conexão é segura (TLS)
const int   BROKER_PORT = 8883;

// Usuário e senha da conta MQTT
const char* BROKER_USER = "Placa-S3-Joao";
const char* BROKER_PASS = "123456abX";


// ------------------------------
//   TÓPICOS MQTT
// ------------------------------
// Tópicos onde a S3 publica informações
const char* TOPIC_PUBLISH_PRESENCA = "Projeto/S3/Presenca3";
const char* TOPIC_ULTRASSOM_S3     = "Projeto/S3/Ultrassom3";

// Tópicos que a S3 recebe comandos de outras placas
const char* TOPICO_SUBSCRIBE   = "S1/iluminacao";         // S1 controla LED
const char* TOPICO_S2_SENSOR1  = "Projeto/S2/Distancia1"; // S2 controla servo 3
const char* TOPICO_S2_SENSOR2  = "Projeto/S2/Distancia2"; // S2 controla servo 4


// ------------------------------
//   CONTROLE DE PUBLICAÇÃO
// ------------------------------

// Guarda o momento do último envio
unsigned long lastPublish = 0;

// Intervalo entre envios MQTT (3 segundos)
int publishInterval = 3000;


// ------------------------------
//   Função que mede a distância com o ultrassom
// ------------------------------
// Envia um pulso pelo TRIG, recebe o retorno no ECHO e converte para cm
long medirDistancia(int trigPin, int echoPin) {

  digitalWrite(trigPin, LOW);
  delayMicroseconds(3);

  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duracao = pulseIn(echoPin, HIGH, 30000); // Espera o eco voltar
  long distancia = (duracao * 0.034) / 2;       // Conversão para cm

  return distancia;
}


// ------------------------------
//   Callback MQTT
//   → Executada sempre que chega uma mensagem
// ------------------------------
void callback(char* topic, byte* payload, unsigned int length) {

  // Converte o payload (bytes) em texto legível
  String mensagem;
  for (int i = 0; i < length; i++) {
    mensagem += (char)payload[i];
  }

  Serial.print("Mensagem recebida em ");
  Serial.print(topic);
  Serial.print(": ");
  Serial.println(mensagem);


  // Controle do LED via tópico vindo da S1
  if (String(topic) == TOPICO_SUBSCRIBE) {
    if (mensagem == "acender") digitalWrite(PINO_LED, HIGH);
    if (mensagem == "apagar")  digitalWrite(PINO_LED, LOW);
  }

  // Sensor de distância da S2 controla posição do servo 3
  if (String(topic) == TOPICO_S2_SENSOR1) {
    if (mensagem == "objeto_proximo") servo3.write(90);
    if (mensagem == "objeto_longe")   servo3.write(45);
  }

  // Segundo sensor de distância da S2 controla servo 4
  if (String(topic) == TOPICO_S2_SENSOR2) {
    if (mensagem == "objeto_proximo") servo4.write(90);
    if (mensagem == "objeto_longe")   servo4.write(45);
  }
}


// ------------------------------
//   Conexão WiFi
// ------------------------------
// Tenta conectar ao Wi-Fi e só continua quando conectado
void conectarWiFi() {
  Serial.print("Conectando ao WiFi...");
  WiFi.begin(SSID, PASS);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println("\nWiFi conectado!");
}


// ------------------------------
//   Conexão ao Broker MQTT
// ------------------------------
void conectarMQTT() {
  mqtt.setServer(BROKER_URL, BROKER_PORT); // Define o servidor MQTT
  client.setInsecure();                    // Conexão TLS sem certificado
  mqtt.setCallback(callback);              // Função que trata mensagens recebidas

  while (!mqtt.connected()) {              // Tenta até conseguir
    Serial.print("Conectando ao broker... ");

    String clientId = "S3_" + String(random(0xffff), HEX); // ID aleatório

    if (mqtt.connect(clientId.c_str(), BROKER_USER, BROKER_PASS)) {
      Serial.println("Conectado!");

      // Inscreve nos tópicos necessários
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
//   SETUP DO SISTEMA
// ------------------------------
void setup() {
  Serial.begin(115200);

  // Configura os pinos
  pinMode(PINO_LED, OUTPUT);
  pinMode(PINO_PRESENCA, INPUT);
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);

  // Ativa os servos e define posição inicial
  servo3.attach(PINO_SERVO3);
  servo4.attach(PINO_SERVO4);

  servo3.write(0);
  servo4.write(0);

  conectarWiFi();
  conectarMQTT();
}


// ------------------------------
//   LOOP PRINCIPAL
// ------------------------------
void loop() {

  // Se cair do broker, tenta reconectar
  if (!mqtt.connected()) conectarMQTT();
  mqtt.loop(); // Mantém MQTT funcionando

  // Lê a distância do ultrassom
  long distancia = medirDistancia(TRIG, ECHO);
  Serial.println(distancia);

  // Envia informação se o objeto está perto ou longe
  if (distancia > 0 && distancia < 10) {
    mqtt.publish(TOPIC_ULTRASSOM_S3, "objeto_proximo");
  } else if (distancia > 10) {
    mqtt.publish(TOPIC_ULTRASSOM_S3, "objeto_longe");
  }

  // Publicação periódica de presença (a cada 3 segundos)
  unsigned long agora = millis();
  if (agora - lastPublish >= publishInterval) {

    lastPublish = agora;
    int presenca = digitalRead(PINO_PRESENCA);

    mqtt.publish(TOPIC_PUBLISH_PRESENCA, String(presenca).c_str());

    Serial.print("Presença publicada: ");
    Serial.println(presenca);
  }

  delay(20); // Delay pequeno apenas para estabilidade
}
