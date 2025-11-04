#include <WiFi.h>

const char* ssid = "NOME_DA_REDE";
const char* password = "SENHA_DA_REDE";

void conectarWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Conectando-se à rede ");

  Serial.print(ssid);
  int tentativas = 0;

  while (WiFi.status() != WL_CONNECTED && tentativas < 20) {
    delay(500);
    Serial.print(".");
    tentativas++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ Conectado com sucesso!");
    Serial.print("Endereço IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n❌ Falha na conexão. Tentando novamente...");
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("Iniciando conexão Wi-Fi...");
  conectarWiFi();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    conectarWiFi();
  }
  delay(10000);
}
