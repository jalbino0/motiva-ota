#include <Arduino.h>
#include <WiFi.h>

const int LED_R = 27;
const int LED_G = 26;
const int LED_B = 25;

const char* WIFI_SSID = "Wokwi-GUEST";
const char* WIFI_PASSWORD = "";

const int NUM_LEITURAS = 5;
const unsigned long INTERVALO_LEITURA = 2000;
const unsigned long INTERVALO_SESSAO = 48000;

int leituras[NUM_LEITURAS];
int quantidadeLeituras = 0;

unsigned long inicioSessao = 0;
bool sessaoAtiva = false;
bool primeiraSessao = true;
bool estadoAlerta = false;

void atualizarLED() {
  digitalWrite(LED_R, estadoAlerta ? HIGH : LOW);
  digitalWrite(LED_G, estadoAlerta ? LOW : HIGH);
  digitalWrite(LED_B, LOW);
}

void conectarWiFi() {
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

void finalizarSessao() {
  int ordenadas[NUM_LEITURAS];
  int soma = 0;

  Serial.print("Ordem original: ");

  for (int i = 0; i < NUM_LEITURAS; i++) {
    ordenadas[i] = leituras[i];
    soma += leituras[i];

    Serial.print(leituras[i]);
    Serial.print(" ");
  }

  Serial.println();

  // Ordena uma copia, preservando o vetor original.
  for (int i = 0; i < NUM_LEITURAS - 1; i++) {
    for (int j = 0; j < NUM_LEITURAS - 1 - i; j++) {
      if (ordenadas[j] > ordenadas[j + 1]) {
        int temporario = ordenadas[j];
        ordenadas[j] = ordenadas[j + 1];
        ordenadas[j + 1] = temporario;
      }
    }
  }

  Serial.print("Ordem crescente: ");

  for (int i = 0; i < NUM_LEITURAS; i++) {
    Serial.print(ordenadas[i]);
    Serial.print(" ");
  }

  Serial.println();

  float media = soma / static_cast<float>(NUM_LEITURAS);
  int mediana = ordenadas[2];

  Serial.print("Media: ");
  Serial.print(media, 1);
  Serial.println(" cm");

  Serial.print("Mediana: ");
  Serial.print(mediana);
  Serial.println(" cm");

  // Histerese: na faixa intermediaria, preserva o estado.
  if (mediana >= 16) {
    estadoAlerta = true;
  } else if (mediana <= 14) {
    estadoAlerta = false;
  }

  atualizarLED();

  Serial.print("Estado: ");
  Serial.println(estadoAlerta ? "ALERTA" : "NORMAL");
  Serial.println("Proxima sessao: 48 segundos apos o inicio desta.");
  Serial.println();

  sessaoAtiva = false;
}

void setup() {
  Serial.begin(115200);

  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);

  atualizarLED();

  Serial.println("========================================");
  Serial.println("MONITORAMENTO DE VEGETACAO - FW 2.0");
  Serial.println("========================================");

  conectarWiFi();
}

void loop() {
  unsigned long agora = millis();

  if (!sessaoAtiva &&
      (primeiraSessao || agora - inicioSessao >= INTERVALO_SESSAO)) {

    if (primeiraSessao) {
      inicioSessao = agora;
      primeiraSessao = false;
    } else {
      inicioSessao += INTERVALO_SESSAO;
    }

    quantidadeLeituras = 0;
    sessaoAtiva = true;

    Serial.println("----- NOVA SESSAO -----");
    Serial.print("Inicio da sessao (ms): ");
    Serial.println(agora);
  }

  if (sessaoAtiva &&
      agora - inicioSessao >=
          quantidadeLeituras * INTERVALO_LEITURA) {

    leituras[quantidadeLeituras] = random(10, 21);

    Serial.print("Leitura ");
    Serial.print(quantidadeLeituras + 1);
    Serial.print(": ");
    Serial.print(leituras[quantidadeLeituras]);
    Serial.println(" cm");

    quantidadeLeituras++;

    if (quantidadeLeituras == NUM_LEITURAS) {
      finalizarSessao();
    }
  }
}