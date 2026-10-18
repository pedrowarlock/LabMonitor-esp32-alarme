#include "dashboard_link.hpp"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <NetworkClientSecure.h>
#include <WiFi.h>
#include <time.h>
#include "storage.hpp"

namespace {
  constexpr unsigned long kUploadIntervalMs = 15000UL;
  unsigned long gLastUploadAt = 0;
  bool gWasWifiConnected = false;
  bool gHasSent = false;
  bool gLastSendOk = false;
  bool gClockSyncRequested = false;
  String gLastMessage = "Aguardando envio.";
  constexpr time_t kMinimumValidTime = 1735689600;
  constexpr time_t kMaximumValidTime = 2051222400;

  String normalizeServerUrl(String url) {
    url.trim();
    while (url.endsWith("/")) url.remove(url.length() - 1);
    return url;
  }

  String makeExternalId() {
    String id = WiFi.macAddress();
    id.toLowerCase();
    return id;
  }

  String makeDeviceName(const String& configuredName) {
    String name = configuredName;
    name.trim();
    if (!name.isEmpty()) return name;
    String mac = makeExternalId();
    return "Monitor LAB " + mac.substring(mac.length() > 5 ? mac.length() - 5 : 0);
  }

  bool ensureTrustedClock() {
    time_t now = time(nullptr);
    if (now >= kMinimumValidTime && now < kMaximumValidTime) return true;

    if (!gClockSyncRequested) {
      configTime(0, 0, "pool.ntp.org", "time.google.com");
      gClockSyncRequested = true;
    }
    return false;
  }

  template <typename Client>
  bool postJsonUsingClient(Client& client, const String& url, const String& token,
                           const String& body, int& statusCode, String& responseBody) {
    HTTPClient http;
    http.setConnectTimeout(3000);
    http.setTimeout(3000);
    if (!http.begin(client, url)) {
      statusCode = 0;
      responseBody = "Nao foi possivel abrir conexao com o servidor.";
      return false;
    }
    http.addHeader("Content-Type", "application/json");
    if (!token.isEmpty()) http.addHeader("Authorization", "Bearer " + token);
    statusCode = http.POST(body);
    responseBody = statusCode > 0 ? http.getString() : http.errorToString(statusCode);
    http.end();
    return statusCode >= 200 && statusCode < 300;
  }

  bool postJson(const String& url, const String& token, const String& body,
                int& statusCode, String& responseBody) {
    if (url.startsWith("https://")) {
      if (!ensureTrustedClock()) {
        statusCode = 0;
        responseBody = "Sem hora sincronizada; nao foi possivel validar o certificado TLS.";
        return false;
      }
      NetworkClientSecure client;
      client.useBuiltinCACertBundle();
      return postJsonUsingClient(client, url, token, body, statusCode, responseBody);
    }
    if (url.startsWith("http://")) {
      WiFiClient client;
      return postJsonUsingClient(client, url, token, body, statusCode, responseBody);
    }
    statusCode = 0;
    responseBody = "Use uma URL iniciada com https:// ou http://.";
    return false;
  }

  String responseMessage(const String& body, const String& fallback) {
    DynamicJsonDocument doc(512);
    if (!deserializeJson(doc, body)) {
      String message = doc["error"] | "";
      if (message.isEmpty()) message = doc["message"] | "";
      if (!message.isEmpty()) return message;
    }
    return fallback;
  }

  bool isRevokedDeviceLinkResponse(int statusCode, const String& body) {
    if (statusCode == 401) return true;
    DynamicJsonDocument doc(256);
    if (deserializeJson(doc, body)) return false;
    String error = doc["error"] | "";
    return error.indexOf("Chave de ingestao invalida") >= 0;
  }

  bool tentarConcluirRemocaoDashboard(const String& serverUrl = "") {
    VinculoDashboard pendente = storageCarregarRemocaoDashboardPendente();
    if (pendente.deviceToken.isEmpty()) return true;
    if (WiFi.status() != WL_CONNECTED) return false;
    if (!serverUrl.isEmpty() && normalizeServerUrl(pendente.serverUrl) != normalizeServerUrl(serverUrl)) return true;

    int statusCode = 0;
    String responseBody;
    const bool removido = postJson(pendente.serverUrl + "/api/device-links/logout",
                                   pendente.deviceToken, "{}", statusCode, responseBody);
    if (removido || statusCode == 401) {
      storageRemoverRemocaoDashboardPendente();
      return true;
    }
    return false;
  }
}

DashboardLinkInfo dashboardLinkObterInfo() {
  VinculoDashboard vinculo = storageCarregarVinculoDashboard();
  DashboardLinkInfo info;
  info.linked = !vinculo.deviceToken.isEmpty();
  info.serverUrl = vinculo.serverUrl;
  info.externalId = vinculo.externalId;
  info.hasSent = gHasSent;
  info.lastSendOk = gLastSendOk;
  info.lastMessage = gLastMessage;
  return info;
}

bool dashboardLinkVincular(const String& serverUrl, const String& email, String& password,
                           const String& name, const String& location, bool allowInsecureHttp,
                           String& message) {
  String baseUrl = normalizeServerUrl(serverUrl);
  const bool secureUrl = baseUrl.startsWith("https://");
  const bool insecureUrl = baseUrl.startsWith("http://");
  if ((!secureUrl && !insecureUrl) || baseUrl.length() > 200 || (insecureUrl && !allowInsecureHttp)) {
    password = "";
    message = insecureUrl
      ? "HTTP nao criptografa senha nem token. Confirme o uso de HTTP antes de vincular."
      : "Informe uma URL HTTPS (recomendada) ou HTTP com confirmacao explicita.";
    return false;
  }
  if (WiFi.status() != WL_CONNECTED) {
    password = "";
    message = "Conecte o aparelho ao Wi-Fi com acesso ao servidor antes de vincular.";
    return false;
  }
  if (email.isEmpty() || password.isEmpty()) {
    password = "";
    message = "Informe o e-mail e a senha da conta LAB/MONITOR.";
    return false;
  }

  if (!tentarConcluirRemocaoDashboard(baseUrl)) {
    password = "";
    message = "Nao foi possivel confirmar a remocao do vinculo anterior no servidor. Tente novamente.";
    return false;
  }

  String externalId = makeExternalId();
  DynamicJsonDocument request(768);
  request["email"] = email;
  request["password"] = password;
  request["externalId"] = externalId;
  request["name"] = makeDeviceName(name);
  if (!location.isEmpty()) request["location"] = location;
  String body;
  serializeJson(request, body);
  password = "";

  int statusCode = 0;
  String responseBody;
  if (!postJson(baseUrl + "/api/device-links/login", "", body, statusCode, responseBody)) {
    message = responseMessage(responseBody, statusCode > 0
      ? "O servidor recusou o vinculo (HTTP " + String(statusCode) + ")."
      : responseBody);
    return false;
  }

  DynamicJsonDocument response(768);
  if (deserializeJson(response, responseBody)) {
    message = "Resposta invalida do servidor LAB/MONITOR.";
    return false;
  }
  String deviceToken = response["deviceToken"] | "";
  if (deviceToken.isEmpty()) {
    message = "O servidor nao retornou o token do aparelho.";
    return false;
  }

  VinculoDashboard vinculo;
  vinculo.serverUrl = baseUrl;
  vinculo.deviceToken = deviceToken;
  vinculo.externalId = externalId;
  if (!storageSalvarVinculoDashboard(vinculo)) {
    message = "Nao foi possivel salvar o vinculo na memoria do aparelho.";
    return false;
  }

  gHasSent = false;
  gLastSendOk = false;
  gLastMessage = "Vinculo ativo; aguardando envio de dados.";
  message = "Aparelho vinculado ao LAB/MONITOR com sucesso.";
  return true;
}

bool dashboardLinkRemover(String& message) {
  VinculoDashboard vinculo = storageCarregarVinculoDashboard();
  if (vinculo.deviceToken.isEmpty()) {
    message = "Este aparelho nao esta vinculado.";
    return false;
  }

  const bool notificacaoPendenteSalva = storageSalvarRemocaoDashboardPendente(vinculo);
  if (!storageRemoverVinculoDashboard()) {
    message = "Nao foi possivel remover o vinculo da memoria do aparelho.";
    return false;
  }
  gHasSent = false;
  gLastSendOk = false;
  gLastMessage = "Vinculo removido.";
  message = notificacaoPendenteSalva
      ? "Vinculo removido deste aparelho. O servidor sera notificado automaticamente quando houver comunicacao."
      : "Vinculo removido deste aparelho, mas nao foi possivel salvar a notificacao pendente ao servidor.";
  return true;
}

bool dashboardLinkMonitorarConexao() {
  const bool wifiConnected = WiFi.status() == WL_CONNECTED;
  const bool wifiJustConnected = wifiConnected && !gWasWifiConnected;
  gWasWifiConnected = wifiConnected;
  return wifiJustConnected;
}

void dashboardLinkProcessarPendencias() {
  if (WiFi.status() == WL_CONNECTED) {
    tentarConcluirRemocaoDashboard();
  }
}

void dashboardLinkEnviar(const DadosSistema& dados, const String& configuredName,
                         bool alertaMudou, bool forcarEnvio) {
  VinculoDashboard vinculo = storageCarregarVinculoDashboard();
  if (vinculo.deviceToken.isEmpty() || WiFi.status() != WL_CONNECTED) return;
  const unsigned long now = millis();
  if (gLastUploadAt != 0 && now - gLastUploadAt < kUploadIntervalMs && !alertaMudou && !forcarEnvio) return;
  gLastUploadAt = now;

  DynamicJsonDocument bodyDoc(1024);
  bodyDoc["externalId"] = vinculo.externalId;
  bodyDoc["name"] = makeDeviceName(configuredName);
  bodyDoc["status"] = (dados.alerta_temperatura || dados.alerta_umidade || dados.alerta_gas) ? "warning" : "online";
  JsonObject power = bodyDoc.createNestedObject("power");
  power["externalPower"] = dados.alimentacao_externa;
  power["batteryPresent"] = dados.bateria_presente;
  power["batteryVoltage"] = dados.bateria_tensao;
  power["batteryPercent"] = dados.bateria_percentual;
  JsonArray readings = bodyDoc.createNestedArray("readings");
  const char* types[] = {"temperatura", "umidade", "gas_adc"};
  const float values[] = {dados.temperatura, dados.umidade, static_cast<float>(dados.gas_adc)};
  const char* units[] = {"C", "%", "ADC"};
  const bool alerts[] = {dados.alerta_temperatura, dados.alerta_umidade, dados.alerta_gas};
  char recordedAt[25] = {};
  if (dados.timestamp > 1735689600) {
    struct tm utcTime;
    if (gmtime_r(&dados.timestamp, &utcTime)) strftime(recordedAt, sizeof(recordedAt), "%Y-%m-%dT%H:%M:%SZ", &utcTime);
  }
  for (size_t index = 0; index < 3; ++index) {
    JsonObject reading = readings.createNestedObject();
    reading["type"] = types[index];
    reading["value"] = values[index];
    reading["unit"] = units[index];
    reading["alert"] = alerts[index];
    if (recordedAt[0] != '\0') reading["recordedAt"] = recordedAt;
  }

  String body;
  serializeJson(bodyDoc, body);
  int statusCode = 0;
  String responseBody;
  gHasSent = true;
  gLastSendOk = postJson(vinculo.serverUrl + "/api/ingest/devices", vinculo.deviceToken, body, statusCode, responseBody);
  if (isRevokedDeviceLinkResponse(statusCode, responseBody)) {
    gLastSendOk = false;
    gLastMessage = storageRemoverVinculoDashboard()
        ? "Vinculo removido pelo servidor. Faca login novamente para vincular o aparelho."
        : "Servidor confirmou a remocao do vinculo, mas nao foi possivel apagar o token da memoria do aparelho.";
    Serial.println(gLastMessage);
    return;
  }
  gLastMessage = gLastSendOk ? "Dados enviados ao LAB/MONITOR." : responseMessage(responseBody,
    statusCode > 0 ? "Falha de envio (HTTP " + String(statusCode) + ")." : responseBody);
  Serial.println(gLastMessage);
}

