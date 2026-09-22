#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <ArduinoJson.h>
#include <esp_ota_ops.h>

const char* VERSAO = "1.0";
const char* WIFI_SSID = "Wokwi-GUEST";
const char* WIFI_PASSWORD = "";

const char* MANIFESTO_URL =
    "https://raw.githubusercontent.com/jalbino0/motiva-ota/main/version.json";

const int LED_R = 27;
const int LED_G = 26;
const int LED_B = 25;

const int NUM_LEITURAS = 5;
const unsigned long INTERVALO_LEITURA = 2000;
const unsigned long INTERVALO_SESSAO = 48000;

int leituras[NUM_LEITURAS];
int quantidadeLeituras = 0;
int sessoesConcluidas = 0;

unsigned long inicioSessao = 0;
bool primeiraSessao = true;
bool sessaoAtiva = false;
bool otaConsultada = false;

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
    Serial.println("Wi-Fi conectado!");
    Serial.print("Endereco IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("ERRO: sem conexao Wi-Fi.");
  }
}

// Compara versoes numericas no formato maior.menor.
bool lerVersao(const String& texto, int& maior, int& menor) {
  int ponto = texto.indexOf('.');

  if (ponto <= 0 || ponto == texto.length() - 1) {
    return false;
  }

  for (unsigned int i = 0; i < texto.length(); i++) {
    if (static_cast<int>(i) == ponto) {
      continue;
    }

    if (texto[i] < '0' || texto[i] > '9') {
      return false;
    }
  }

  maior = texto.substring(0, ponto).toInt();
  menor = texto.substring(ponto + 1).toInt();
  return true;
}

void mostrarDiagnosticoOTA() {
  Serial.println("===== DIAGNOSTICO DAS PARTICOES =====");

  const esp_partition_t* atual =
      esp_ota_get_running_partition();

  const esp_partition_t* proxima =
      esp_ota_get_boot_partition();

  if (atual != nullptr) {
    Serial.printf(
        "Particao atual: %s, endereco: 0x%lx\n",
        atual->label,
        static_cast<unsigned long>(atual->address)
    );
  } else {
    Serial.println("ERRO: particao atual nao encontrada.");
  }

  if (proxima != nullptr) {
    Serial.printf(
        "Particao selecionada: %s, endereco: 0x%lx\n",
        proxima->label,
        static_cast<unsigned long>(proxima->address)
    );

    esp_app_desc_t descricao;

    esp_err_t resultado =
        esp_ota_get_partition_description(proxima, &descricao);

    Serial.printf(
        "Verificacao da imagem: %s\n",
        esp_err_to_name(resultado)
    );
  } else {
    Serial.println("ERRO: particao de boot nao encontrada.");
  }

  Serial.println("====================================");
}

void consultarOTA() {
  Serial.println();
  Serial.println("===== CONSULTA OTA APOS 3 SESSOES =====");

  if (WiFi.status() != WL_CONNECTED) {
    conectarWiFi();
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("OTA cancelada: sem conexao Wi-Fi.");
    return;
  }

  String versaoRemota;
  String firmwareURL;

  {
    WiFiClientSecure cliente;

    // Simplificacao para esta demonstracao no simulador:
    // usa HTTPS sem validar o certificado do servidor.
    cliente.setInsecure();

    HTTPClient http;
    http.setTimeout(15000);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

    if (!http.begin(cliente, MANIFESTO_URL)) {
      Serial.println("ERRO: nao foi possivel iniciar a consulta.");
      return;
    }

    int codigo = http.GET();

    if (codigo != HTTP_CODE_OK) {
      Serial.print("ERRO ao acessar manifesto. Codigo: ");
      Serial.println(codigo);
      http.end();
      return;
    }

    String conteudo = http.getString();
    http.end();

    JsonDocument documento;

    DeserializationError erro =
        deserializeJson(documento, conteudo);

    if (erro) {
      Serial.print("ERRO: manifesto JSON invalido: ");
      Serial.println(erro.c_str());
      return;
    }

    if (!documento["version"].is<const char*>() ||
        !documento["url"].is<const char*>()) {
      Serial.println("ERRO: manifesto sem version ou url validos.");
      return;
    }

    versaoRemota = documento["version"].as<String>();
    firmwareURL = documento["url"].as<String>();
  }

  int maiorLocal, menorLocal;
  int maiorRemoto, menorRemoto;

  if (!lerVersao(VERSAO, maiorLocal, menorLocal) ||
      !lerVersao(versaoRemota, maiorRemoto, menorRemoto)) {
    Serial.println("ERRO: use versoes no formato 1.0 ou 2.0.");
    return;
  }

  Serial.print("Versao instalada: ");
  Serial.println(VERSAO);

  Serial.print("Versao disponivel: ");
  Serial.println(versaoRemota);

  bool maisNova =
      maiorRemoto > maiorLocal ||
      (maiorRemoto == maiorLocal && menorRemoto > menorLocal);

  if (!maisNova) {
    Serial.println(
        "Firmware ja atualizado: nenhuma versao mais nova."
    );
    return;
  }

  if (!firmwareURL.startsWith(
          "https://raw.githubusercontent.com/jalbino0/motiva-ota/")) {
    Serial.println("ERRO: URL fora do repositorio esperado.");
    return;
  }

  Serial.println("Atualizacao disponivel!");
  Serial.print("Baixando: ");
  Serial.println(firmwareURL);

  WiFiClientSecure clienteFirmware;
  clienteFirmware.setInsecure();

  HTTPUpdate atualizador(15000);
  atualizador.rebootOnUpdate(false);
  atualizador.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

  t_httpUpdate_return resultado =
      atualizador.update(clienteFirmware, firmwareURL, VERSAO);

  if (resultado == HTTP_UPDATE_OK) {
    Serial.println("OTA concluida com sucesso!");

    mostrarDiagnosticoOTA();

    Serial.println("Reiniciando para executar o novo firmware...");
    Serial.flush();
    delay(500);
    ESP.restart();

  } else if (resultado == HTTP_UPDATE_NO_UPDATES) {
    Serial.println("Servidor informou que nao ha atualizacao.");

  } else {
    Serial.print("ERRO no download ou na gravacao OTA: ");
    Serial.print(atualizador.getLastError());
    Serial.print(" - ");
    Serial.println(atualizador.getLastErrorString());
  }
}

void finalizarSessao() {
  int soma = 0;

  for (int i = 0; i < NUM_LEITURAS; i++) {
    soma += leituras[i];
  }

  float media = soma / static_cast<float>(NUM_LEITURAS);

  Serial.print("Media da sessao: ");
  Serial.print(media, 1);
  Serial.println(" cm");

  sessoesConcluidas++;

  Serial.print("Sessoes concluidas: ");
  Serial.println(sessoesConcluidas);
  Serial.println();

  sessaoAtiva = false;
}

void setup() {
  Serial.begin(115200);

  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);

  digitalWrite(LED_R, LOW);
  digitalWrite(LED_G, LOW);
  digitalWrite(LED_B, HIGH);

  Serial.println("========================================");
  Serial.println("MONITORAMENTO DE VEGETACAO - FW 1.0");
  Serial.println("OTA habilitada apos 3 sessoes completas");
  Serial.println("========================================");

  conectarWiFi();
}

void loop() {
  unsigned long agora = millis();

  if (!sessaoAtiva &&
      (primeiraSessao ||
       agora - inicioSessao >= INTERVALO_SESSAO)) {

    if (primeiraSessao) {
      inicioSessao = agora;
      primeiraSessao = false;

    } else {
      // Preserva a cadencia normal. Se uma tentativa OTA
      // demorar demais, evita acumular sessoes atrasadas.
      inicioSessao += INTERVALO_SESSAO;

      if (agora - inicioSessao >= INTERVALO_SESSAO) {
        inicioSessao = agora;
      }
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

  if (!sessaoAtiva &&
      sessoesConcluidas >= 3 &&
      !otaConsultada) {

    otaConsultada = true;
    consultarOTA();
  }
}