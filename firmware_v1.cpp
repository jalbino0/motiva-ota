#include <Arduino.h>
#include <WiFi.h>

// PROJETO MOTIVA - FIRMWARE 1.0
// Monitoramento de vegetacao

const int LED_R = 27;
const int LED_G = 26;
const int LED_B = 25;

const char* WIFI_SSID = "Wokwi-GUEST";
const char* WIFI_PASSWORD = "";

const int NUM_LEITURAS = 5;
const unsigned long INTERVALO_LEITURA = 2000;
const unsigned long INTERVALO_SESSAO = 48000;

int leituras[NUM_LEITURAS];

unsigned long inicioUltimaSessao = 0;
bool primeiraSessao = true;

// Declaracoes das funcoes
void conectarWiFi();
void realizarSessao();

void setup() {
  Serial.begin(115200);

  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);

  // Firmware 1.0: LED azul
  digitalWrite(LED_R, LOW);
  digitalWrite(LED_G, LOW);
  digitalWrite(LED_B, HIGH);

  Serial.println("========================================");
  Serial.println("MONITORAMENTO DE VEGETACAO - FW 1.0");
  Serial.println("========================================");

  conectarWiFi();
}

void conectarWiFi() {
  Serial.println();
  Serial.println("Conectando ao Wi-Fi...");

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int tentativas = 0;

  while (WiFi.status() != WL_CONNECTED && tentativas < 20) {
    delay(500);
    Serial.print(".");
    tentativas++;
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Wi-Fi conectado com sucesso!");
    Serial.print("Endereco IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("ERRO: Nao foi possivel conectar ao Wi-Fi.");
  }
}

void realizarSessao() {
  for (int i = 0; i < NUM_LEITURAS; i++) {
    // Alturas simuladas entre 10 e 20 cm
    leituras[i] = random(10, 21);

    Serial.print("Leitura ");
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.print(leituras[i]);
    Serial.println(" cm");

    delay(INTERVALO_LEITURA);
  }

  float soma = 0;

  for (int i = 0; i < NUM_LEITURAS; i++) {
    soma += leituras[i];
  }

  float media = soma / NUM_LEITURAS;

  Serial.print("Media da sessao: ");
  Serial.print(media, 1);
  Serial.println(" cm");
}

void loop() {
  unsigned long tempoAtual = millis();

  // Intervalo de 48 segundos entre os inicios das sessoes
  if (
    primeiraSessao ||
    tempoAtual - inicioUltimaSessao >= INTERVALO_SESSAO
  ) {
    inicioUltimaSessao = tempoAtual;
    primeiraSessao = false;

    Serial.println();
    Serial.println("----- NOVA SESSAO -----");

    realizarSessao();

    Serial.println("Proxima sessao: 48 segundos apos o inicio desta.");
    Serial.println();
  }
}