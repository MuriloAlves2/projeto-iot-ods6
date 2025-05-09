#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// Credenciais Wi-Fi
const char* ssid = "SEU WIFI";      
const char* password = "SENHA DO SEU WIFI";

// Broker MQTT
const char* mqtt_server = "broker.hivemq.com"; 
const int mqtt_port = 1883;                      
const char* mqtt_topic = "topico/umidade"; 

// Definições de pinos
#define SENSOR A0         // Pino do sensor YL-69
#define BUZZER_PIN D1     // Pino do buzzer no ESP8266

WiFiClient espClient;
PubSubClient client(espClient);

// Variáveis para controle do tempo
unsigned long lastTime = 0;
unsigned long timerDelay = 1000;  // Intervalo de 1 segundo

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Conectando à rede Wi-Fi: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("Conectado ao Wi-Fi!");
  Serial.print("Endereço IP: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Tentando conexão MQTT...");
    String clientId = "ESP8266Client";
    if (client.connect(clientId.c_str())) {
      Serial.println("Conectado ao MQTT!");
    } else {
      Serial.print("Falha, rc=");
      Serial.print(client.state());
      Serial.println(" Tentando novamente em 5 segundos.");
      delay(5000);
    }
  }
}

void setup() {
  // Inicializa o monitor serial
  Serial.begin(115200);
  setup_wifi();  // Conecta ao Wi-Fi

  client.setServer(mqtt_server, mqtt_port);  // Define o servidor MQTT

  pinMode(SENSOR, INPUT);          // Configura o pino do sensor como entrada
  pinMode(BUZZER_PIN, OUTPUT);     // Configura o pino do buzzer como saída
  digitalWrite(BUZZER_PIN, LOW);   // Garante que o buzzer inicie desligado
}

void loop() {
  // Verifica se a conexão MQTT está ativa
  if (!client.connected()) {
    reconnect();
  }
  client.loop();  // Mantém a comunicação com o broker MQTT

  // Verifica o tempo passado para enviar os dados
  unsigned long currentTime = millis();
  if (currentTime - lastTime >= timerDelay) {
    lastTime = currentTime;  // Atualiza o tempo de referência

    // Leitura do sensor de umidade
    int leituraTotal = 0;
    int numLeituras = 10;

    // Faz 10 leituras para melhorar a precisão
    for (int i = 0; i < numLeituras; i++) {
      leituraTotal += analogRead(SENSOR);
      delay(10);  // Pequeno atraso entre as leituras
    }

    int umidade = leituraTotal / numLeituras; // Média das leituras
    int percentual = map(umidade, 1024, 300, 0, 100); // Converte em % para representar a umidade

    // Exibir status de umidade no Serial Monitor
    String status;
    if (percentual < 30) {
      status = "Solo Seco!";
      digitalWrite(BUZZER_PIN, HIGH);   // Liga o buzzer
    } else {
      status = "Solo Úmido ou Molhado!";
      digitalWrite(BUZZER_PIN, LOW);    // Desliga o buzzer
    }

    // Exibir as informações no Serial Monitor
    Serial.print("Umidade do Solo: ");
    Serial.print(percentual);
    Serial.print("% - ");
    Serial.println(status);

    // Publicar a umidade e status no broker MQTT
    String message = String("Umidade do solo: ") + String(percentual) + "% - " + status;
    if (client.publish(mqtt_topic, message.c_str())) {
      Serial.println("Mensagem publicada com sucesso!");
    } else {
      Serial.println("Falha ao publicar mensagem!");
    }
  }
}
