
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <WebServer.h>
#include <ArduinoJson.h>

const char* ssid = "ups";
const char* password = "camargo08";

// ========================================
// LOCALIZACAO: BARUERI - SP
// ========================================
const char* locationName = "Barueri";

const char* apiUrl =
    "https://api.open-meteo.com/v1/forecast"
    "?latitude=-23.5113&longitude=-46.8761"
    "&current_weather=true";

WebServer server(80);

// ========================================
// DADOS DO CLIMA
// ========================================
float currentTemp = 0.0;
float currentWind = 0.0;
float currentWindDirection = 0.0;
bool weatherAvailable = false;

// ========================================
// BUSCAR DADOS NA OPEN-METEO
// ========================================
void fetchWeatherData() {

  if (WiFi.status() != WL_CONNECTED) {
    weatherAvailable = false;
    Serial.println("Wi-Fi desconectado!");
    return;
  }

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.setTimeout(15000);

  Serial.println("Consultando Open-Meteo...");

  if (http.begin(client, apiUrl)) {

    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK) {

      String payload = http.getString();

      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, payload);

      if (!error && !doc["current_weather"].isNull()) {

        // DESAFIO 2: extrair dados do JSON
        currentTemp =
            doc["current_weather"]["temperature"] | 0.0f;

        currentWind =
            doc["current_weather"]["windspeed"] | 0.0f;

        currentWindDirection =
            doc["current_weather"]["winddirection"] | 0.0f;

        weatherAvailable = true;

        Serial.println("Dados recebidos com sucesso!");

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

        weatherAvailable = false;
        Serial.println("Erro ao interpretar o JSON.");

        if (error) {
          Serial.println(error.c_str());
        }
      }

    } else {

      weatherAvailable = false;

      Serial.printf(
        "Erro HTTP: %d\n",
        httpCode
      );
    }

    http.end();

  } else {

    weatherAvailable = false;
    Serial.println("Falha ao iniciar a requisicao HTTPS.");
  }
}

void handleRoot() {

  fetchWeatherData();

  String html = "<!DOCTYPE html><html lang='pt-BR'><head>";

  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<meta http-equiv='refresh' content='60'>";
  html += "<title>Painel Climatico - ESP32</title>";

  html += "<style>";

  html += "* { box-sizing: border-box; }";

  html += "body {"
          "font-family: Arial, sans-serif;"
          "background: #0f172a;"
          "color: #f8fafc;"
          "display: flex;"
          "justify-content: center;"
          "align-items: center;"
          "min-height: 100vh;"
          "margin: 0;"
          "padding: 16px;"
          "}";

  html += ".card {"
          "background: #1e293b;"
          "padding: 2rem;"
          "border-radius: 16px;"
          "text-align: center;"
          "max-width: 420px;"
          "width: 100%;"
          "border: 1px solid #334155;"
          "box-shadow: 0 10px 25px rgba(0,0,0,0.3);"
          "}";

  html += "h1 {"
          "color: #38bdf8;"
          "font-size: 1.3rem;"
          "margin-bottom: 1.5rem;"
          "}";

  html += ".metric {"
          "font-size: 2.8rem;"
          "font-weight: bold;"
          "color: #f1f5f9;"
          "margin: 16px 0;"
          "}";

  html += ".sub-metric {"
          "font-size: 1.05rem;"
          "color: #cbd5e1;"
          "margin: 14px 0;"
          "}";

  html += ".status {"
          "font-size: 0.9rem;"
          "color: #94a3b8;"
          "margin: 18px 0;"
          "}";

  html += ".btn {"
          "background: #38bdf8;"
          "color: #0f172a;"
          "border: none;"
          "padding: 12px 24px;"
          "font-weight: bold;"
          "border-radius: 8px;"
          "cursor: pointer;"
          "margin-top: 12px;"
          "}";

  html += ".btn:hover {"
          "background: #0284c7;"
          "color: white;"
          "}";

  html += "</style></head><body>";

  html += "<div class='card'>";

  // Nome da localizacao
  html += "<h1>Clima em Tempo Real - ";
  html += locationName;
  html += "</h1>";

  if (weatherAvailable) {

    // Temperatura
    html += "<div class='metric'>";
    html += String(currentTemp, 1);
    html += " &deg;C</div>";

    // Velocidade do vento
    html += "<div class='sub-metric'>Velocidade do vento: ";
    html += String(currentWind, 1);
    html += " km/h</div>";

    // Direcao do vento
    html += "<div class='sub-metric'>Direcao do vento: ";
    html += String(currentWindDirection, 0);
    html += "&deg;</div>";

    html += "<div class='status'>Dados obtidos da Open-Meteo</div>";

  } else {

    html += "<div class='sub-metric'>";
    html += "Nao foi possivel obter os dados do clima.";
    html += "</div>";

    html += "<div class='status'>";
    html += "Verifique a conexao Wi-Fi e tente novamente.";
    html += "</div>";
  }

  html += "<button class='btn' onclick='location.reload()'>";
  html += "Atualizar dados";
  html += "</button>";

  html += "</div></body></html>";

  server.send(200, "text/html", html);
}

void setup() {

  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("Dashboard Climatico ESP32");
  Serial.println("Conectando ao Wi-Fi...");

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  int tentativas = 0;

  while (WiFi.status() != WL_CONNECTED && tentativas < 60) {
    delay(500);
    Serial.print(".");
    tentativas++;
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {

    Serial.println("Wi-Fi conectado!");

    Serial.print("Endereco IP: http://");
    Serial.println(WiFi.localIP());

  } else {

    Serial.println("Nao foi possivel conectar ao Wi-Fi.");
    Serial.println("Verifique o nome, a senha e a rede de 2,4 GHz.");
    return;
  }

  server.on("/", handleRoot);
  server.begin();

  Serial.println("Servidor HTTP iniciado!");
  Serial.println("Acesse o IP acima pelo navegador.");
}

void loop() {

  if (WiFi.status() == WL_CONNECTED) {
    server.handleClient();
  }
}
