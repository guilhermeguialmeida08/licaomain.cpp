
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <WebServer.h>
#include <ArduinoJson.h>

// Credenciais do Wi-Fi
const char* ssid = "IPhonegui";
const char* password = "gui1234567";

WebServer server(80);

// Variáveis do clima
float currentTemp = 0.0;
float currentWind = 0.0;
float currentWindDirection = 0.0;

bool dadosValidos = false;
bool servidorIniciado = false;

unsigned long ultimaTentativa = 0;

// ==========================================
// DIAGNÓSTICO DE CONEXÃO WI-FI
// ==========================================

void onWiFiDisconnected(
  WiFiEvent_t event,
  WiFiEventInfo_t info
) {
  Serial.println("\nWi-Fi desconectado!");

  Serial.print("Motivo detalhado da desconexao: ");
  Serial.println(info.wifi_sta_disconnected.reason);
}

// ==========================================
// BUSCAR DADOS DA API OPEN-METEO
// ==========================================

void fetchWeatherData() {

  dadosValidos = false;

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Sem conexao Wi-Fi!");
    return;
  }

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;

  http.setFollowRedirects(
    HTTPC_STRICT_FOLLOW_REDIRECTS
  );

  // Coordenadas de Barueri - SP
  String apiUrl =
    "https://api.open-meteo.com/v1/forecast?"
    "latitude=-23.5112"
    "&longitude=-46.8765"
    "&current=temperature_2m,wind_speed_10m,wind_direction_10m";

  if (http.begin(client, apiUrl)) {

    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK) {

      String payload = http.getString();

      JsonDocument doc;

      DeserializationError error =
        deserializeJson(doc, payload);

      if (!error && !doc["current"].isNull()) {

        currentTemp =
          doc["current"]["temperature_2m"].as<float>();

        currentWind =
          doc["current"]["wind_speed_10m"].as<float>();

        currentWindDirection =
          doc["current"]["wind_direction_10m"].as<float>();

        dadosValidos = true;

        Serial.println("\nDados climaticos recebidos!");

        Serial.printf(
          "Temperatura: %.1f C\n",
          currentTemp
        );

        Serial.printf(
          "Velocidade do vento: %.1f km/h\n",
          currentWind
        );

        Serial.printf(
          "Direcao do vento: %.0f graus\n",
          currentWindDirection
        );

      } else {
        Serial.println("Erro ao processar JSON!");
      }

    } else {
      Serial.print("Erro HTTP: ");
      Serial.println(httpCode);
    }

    http.end();

  } else {
    Serial.println("Falha ao iniciar HTTP!");
  }
}

// ==========================================
// PÁGINA WEB
// ==========================================

void handleRoot() {

  fetchWeatherData();

  String html = "<!DOCTYPE html><html lang='pt-BR'><head>";

  html += "<meta charset='UTF-8'>";

  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";

  html += "<title>Painel Climatico - ESP32</title>";

  html += "<style>";

  html += "body { font-family: 'Segoe UI', Tahoma, sans-serif; background: #0f172a; color: #f8fafc; display: flex; justify-content: center; align-items: center; min-height: 100vh; margin: 0; }";

  html += ".card { background: #1e293b; padding: 2rem; border-radius: 12px; text-align: center; max-width: 400px; width: 90%; border: 1px solid #334155; box-shadow: 0 10px 25px rgba(0,0,0,0.3); }";

  html += "h1 { color: #38bdf8; font-size: 1.2rem; text-transform: uppercase; letter-spacing: 1px; margin-bottom: 1.5rem; }";

  html += ".metric { font-size: 2.5rem; font-weight: bold; color: #f1f5f9; margin: 10px 0; }";

  html += ".sub-metric { font-size: 1.1rem; color: #94a3b8; margin-bottom: 20px; }";

  html += ".btn { background: #38bdf8; color: #0f172a; border: none; padding: 12px 24px; font-weight: bold; border-radius: 6px; cursor: pointer; }";

  html += ".btn:hover { background: #0284c7; color: #ffffff; }";

  html += "</style></head><body>";

  html += "<div class='card'>";

  html += "<h1>Clima em Tempo Real (Barueri)</h1>";

  if (dadosValidos) {

    // Temperatura
    html += "<div class='metric'>" +
      String(currentTemp, 1) + " °C</div>";

    // Velocidade do vento
    html += "<div class='sub-metric'>Vento: " +
      String(currentWind, 1) + " km/h</div>";

    // Direção do vento
    html += "<div class='sub-metric'>Direção do vento: " +
      String(currentWindDirection, 0) + "°</div>";

  } else {

    html += "<div class='sub-metric'>Não foi possível carregar os dados climáticos.</div>";

  }

  // Botão de atualização
  html += "<button class='btn' onclick='location.reload()'>Atualizar Dados</button>";

  html += "</div></body></html>";

  server.send(200, "text/html", html);
}

// ==========================================
// INICIAR SERVIDOR
// ==========================================

void iniciarServidor() {

  if (!servidorIniciado) {

    server.on("/", handleRoot);
    server.begin();

    servidorIniciado = true;

    Serial.println("Servidor HTTP iniciado!");
  }
}

// ==========================================
// SETUP
// ==========================================

void setup() {

  Serial.begin(115200);
  delay(1000);

  Serial.println("\nIniciando ESP32...");

  WiFi.mode(WIFI_STA);

  WiFi.setSleep(false);

  // Registra o diagnóstico de desconexão
  WiFi.onEvent(
    onWiFiDisconnected,
    ARDUINO_EVENT_WIFI_STA_DISCONNECTED
  );

  // Procura redes disponíveis
  Serial.println("Procurando redes Wi-Fi...");

  int redes = WiFi.scanNetworks();

  bool encontrou = false;

  for (int i = 0; i < redes; i++) {

    Serial.print("Rede encontrada: ");
    Serial.println(WiFi.SSID(i));

    if (WiFi.SSID(i) == ssid) {

      encontrou = true;

      Serial.println("Rede desejada localizada!");

      Serial.print("Sinal: ");
      Serial.print(WiFi.RSSI(i));
      Serial.println(" dBm");
    }
  }

  WiFi.scanDelete();

  if (!encontrou) {

    Serial.println("ATENCAO: Rede Wi-Fi nao encontrada!");

  }

  // Conexão Wi-Fi
  Serial.println("\nTentando conectar ao Wi-Fi...");

  WiFi.begin(ssid, password);

  unsigned long inicio = millis();

  while (
    WiFi.status() != WL_CONNECTED &&
    millis() - inicio < 20000
  ) {

    delay(500);
    Serial.print(".");
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {

    Serial.println("\nWi-Fi conectado!");

    Serial.print("Endereco IP: http://");
    Serial.println(WiFi.localIP());

    iniciarServidor();

  } else {

    Serial.println("\nFalha ao conectar ao Wi-Fi!");

    Serial.print("Codigo do erro: ");
    Serial.println(WiFi.status());

    Serial.println("Verifique o motivo detalhado acima.");
  }

  ultimaTentativa = millis();
}

// ==========================================
// LOOP
// ==========================================

void loop() {

  if (WiFi.status() == WL_CONNECTED) {

    iniciarServidor();

    server.handleClient();

  } else {

    // Tenta reconectar a cada 15 segundos
    if (millis() - ultimaTentativa >= 15000) {

      ultimaTentativa = millis();

      Serial.println("\nTentando reconectar ao Wi-Fi...");

      WiFi.reconnect();
    }
  }

  delay(10);
}
