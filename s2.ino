#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>     

const char* SSID = "FIESC_IOT_EDU";
const char* PASS = "8120gv08";

const char* BROKER = "2941e5c6678a4be18375b50465ad0964.s1.eu.hivemq.cloud";
const int PORT = 8883; 

const char* BROKER_USER = "Placa-S2-Joao";
const char* BROKER_PASS = "SAdojoao10";

// Pinos utilizados no projeto
#define PINO_LED 2       // LED controlado via MQTT
#define PINO_SENSOR 4    // Sensor de presença (PIR)

// Tópicos MQTT organizados: Projeto / Placa(S2) / Dado
const char* TOPICO_PUBLISH = "Projeto/S2/Presenca";  // Envia presença (0/1)
const char* TOPICO_SUBSCRIBE = "Projeto/S2/LED";     // Recebe comandos do LED

// Objetos de conexão segura e MQTT
WiFiClientSecure espClient;
PubSubClient mqtt(espClient);

// Função chamada sempre que chega mensagem no tópico assinado
void callback(char* topic, byte* payload, unsigned int length) {
  String mensagem;

  // Monta a mensagem recebida byte a byte
  for (int i = 0; i < length; i++) {
    mensagem += (char)payload[i];
  }

  // Exibe o tópico e o conteúdo recebido
  Serial.print("Mensagem recebida em ");
  Serial.print(topic);
  Serial.print(": ");
  Serial.println(mensagem);

  // Comando para ligar o LED via MQTT
  if (mensagem == "ligar") {
    digitalWrite(PINO_LED, HIGH);
    Serial.println("LED ligado via MQTT");
  } 
  // Comando para desligar o LED
  else if (mensagem == "desligar") {
    digitalWrite(PINO_LED, LOW);
    Serial.println("LED desligado via MQTT");
  }
}

// Conecta no Wi-Fi
void conectaWiFi() {
  Serial.print("Conectando à rede Wi-Fi: ");
  Serial.println(SSID);
  WiFi.begin(SSID, PASS);

  // Aguarda até conectar
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWi-Fi conectado!");
  Serial.print("Endereço IP: ");
  Serial.println(WiFi.localIP());
}

// Conecta ao servidor MQTT
void conectaMQTT() {
  mqtt.setServer(BROKER, PORT);     
  mqtt.setCallback(callback);       
  espClient.setInsecure();          

  // Mantém tentando conectar até conseguir
  while (!mqtt.connected()) {
    String clientId = "S2_" + String(random(0xffff), HEX);  // ID único para cada conexão

    Serial.print("Conectando ao broker MQTT... ");

    // Tenta conectar com usuário e senha
    if (mqtt.connect(clientId.c_str(), BROKER_USER, BROKER_PASS)) {
      Serial.println("Conectado!");
      
      // Assina o tópico que controla o LED
      mqtt.subscribe(TOPICO_SUBSCRIBE);
      Serial.print("Inscrito no tópico: ");
      Serial.println(TOPICO_SUBSCRIBE);

    } else {
      // Caso falhe, exibe o erro e tenta de novo
      Serial.print("Falha (rc=");
      Serial.print(mqtt.state());
      Serial.println("). Tentando novamente...");
      delay(2000);
    }
  }
}

void setup() {
  Serial.begin(115200);        // Inicializa o monitor serial
  pinMode(PINO_LED, OUTPUT);   
  pinMode(PINO_SENSOR, INPUT); 

  conectaWiFi(); 
  conectaMQTT(); 
}

void loop() {
  // Se desconectar do MQTT, reconecta
  if (!mqtt.connected()) {
    conectaMQTT();
  }

  mqtt.loop();  // Mantém a comunicação MQTT ativa

  // Lê o sensor PIR (0 = sem presença / 1 = presença)
  int presenca = digitalRead(PINO_SENSOR);

  // Converte para String e envia para o broker
  String mensagem = String(presenca);
  mqtt.publish(TOPICO_PUBLISH, mensagem.c_str());

  // Mostra no monitor serial
  Serial.print("Detecção de presença: ");
  Serial.println(presenca);

  delay(2000); 
}
