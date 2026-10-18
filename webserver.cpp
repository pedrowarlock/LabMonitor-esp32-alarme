#include "webserver.hpp"

#include <ArduinoJson.h>
#include <functional>
#include <new>
#include <utility>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <time.h>
#include "auth.hpp"
#include "dashboard_link.hpp"
#include "factory_reset.hpp"
#include "storage.hpp"
#include "wifi.hpp"
#include "web_assets.hpp"

namespace {
  WebServer* gServer = nullptr;
  DadosSistema gDadosAtuais;
  bool gReinicioPendente = false;
  unsigned long gReiniciarEm = 0;
  SemaphoreHandle_t gOperationMutex = nullptr;
  bool gOperationActive = false;
  bool gOperationDone = false;
  bool gOperationOk = false;
  bool gDashboardOperationActive = false;
  bool gTimeSyncRequested = false;
  uint32_t gOperationId = 0;
  String gOperationMessage;
  void enviarJson(WebServer& server, int codigo, const String& payload);

  struct AsyncOperationTask {
    uint32_t id;
    bool exposeResult;
    std::function<void(bool&, String&)> run;
  };

  void finalizarOperacao(uint32_t id, bool exposeResult, bool ok, const String& message) {
    xSemaphoreTake(gOperationMutex, portMAX_DELAY);
    gOperationActive = false;
    gDashboardOperationActive = false;
    if (exposeResult && gOperationId == id) {
      gOperationDone = true;
      gOperationOk = ok;
      gOperationMessage = message;
    }
    xSemaphoreGive(gOperationMutex);
  }

  void executarOperacaoAsync(void* argumento) {
    AsyncOperationTask* task = static_cast<AsyncOperationTask*>(argumento);
    bool ok = false;
    String message;
    vTaskDelay(pdMS_TO_TICKS(250));
    task->run(ok, message);
    finalizarOperacao(task->id, task->exposeResult, ok, message);
    delete task;
    vTaskDelete(nullptr);
  }

  bool iniciarOperacaoAsync(std::function<void(bool&, String&)> callback, uint32_t& id,
                            bool dashboardOperation = false, bool exposeResult = true) {
    if (!gOperationMutex) return false;
    xSemaphoreTake(gOperationMutex, portMAX_DELAY);
    if (gOperationActive) {
      xSemaphoreGive(gOperationMutex);
      return false;
    }
    id = 0;
    if (exposeResult) {
      id = ++gOperationId;
      if (id == 0) id = ++gOperationId;
      gOperationDone = false;
      gOperationOk = false;
      gOperationMessage = "Operação iniciada.";
    }
    gOperationActive = true;
    gDashboardOperationActive = dashboardOperation;
    xSemaphoreGive(gOperationMutex);

    AsyncOperationTask* task = new (std::nothrow) AsyncOperationTask{id, exposeResult, std::move(callback)};
    if (!task || xTaskCreate(executarOperacaoAsync, "device-operation", 16384, task, 1, nullptr) != pdPASS) {
      delete task;
      finalizarOperacao(id, exposeResult, false, "Não foi possível iniciar a operação em segundo plano.");
      return false;
    }
    return true;
  }

  void responderOperacaoIniciada(WebServer& server, uint32_t id, const String& message) {
    DynamicJsonDocument response(256);
    response["accepted"] = true;
    response["operationId"] = id;
    response["message"] = message;
    String payload;
    serializeJson(response, payload);
    enviarJson(server, 202, payload);
  }

  bool dashboardOperacaoAtiva() {
    if (!gOperationMutex) return false;
    xSemaphoreTake(gOperationMutex, portMAX_DELAY);
    const bool active = gDashboardOperationActive;
    xSemaphoreGive(gOperationMutex);
    return active;
  }

  struct Sessao {
    String token;
    String username;
    String role;
    unsigned long expiraEm;
    String ip;
  };

  std::vector<Sessao> gSessoes;

  String extrairToken(WebServer& server) {
    String token = server.header("Authorization");
    if (token.startsWith("Bearer ") || token.startsWith("bearer ")) {
      return token.substring(7);
    }
    return token;
  }

  bool validarSessao(const String& token, String& username, String& role) {
    if (token.isEmpty()) {
      return false;
    }

    unsigned long agora = millis();
    for (auto it = gSessoes.begin(); it != gSessoes.end();) {
      if (agora > it->expiraEm) {
        it = gSessoes.erase(it);
        continue;
      }

      if (it->token == token) {
        username = it->username;
        role = it->role;
        return true;
      }
      ++it;
    }
    return false;
  }

  bool temPermissao(const String& role, const String& nivel) {
    if (nivel == "overview") return true;
    if (nivel == "config") return (role == "admin" || role == "user");
    if (nivel == "users") return role == "admin";
    return false;
  }

  String obterNomeAparelho() {
    Config cfg = storageCarregarConfig();
    return cfg.nome_aparelho.isEmpty() ? "" : cfg.nome_aparelho;
  }

  String gerarDashboardHtml() {
    String html(reinterpret_cast<const char*>(WEB_INDEX));
    const String endereco = "http://" + wifiObterNomeHost() + ".local";
    html.replace("__SYSTEM_LINK__", endereco);
    html.replace("__SYSTEM_URL__", endereco);
    return html;
  }

  String gerarSetupHtml() {
    String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>Configurar Wi-Fi</title>";
    html += "<style>body{font-family:Arial;background:#0f172a;color:#e2e8f0;padding:24px} input,select{padding:8px;margin:6px 0;width:100%}</style></head><body>";
    html += "<h1>Configurar Wi-Fi</h1>";
    html += "<div><button onclick=\"scan()\">Escanear redes</button></div>";
    html += "<div id=networks></div>";
    html += "<form id=frm onsubmit=\"return submitCfg()\">";
    html += "<label>SSID</label><input id=ssid name=ssid required />";
    html += "<label>Senha (se necessária)</label><input id=password name=password />";
    html += "<label>Segurança</label><select id=security name=security><option>OPEN</option><option>WPA-PSK</option><option>WPA2-PSK</option></select>";
    html += "<div><button type=submit>Salvar e Conectar</button></div></form>";
    html += "<script>async function waitOperation(id){let end=Date.now()+90000;while(Date.now()<end){let r=await fetch('/api/operation?id='+id);let j=await r.json();if(j.done)return j;await new Promise(resolve=>setTimeout(resolve,500));}throw new Error('Tempo limite excedido.')}";
    html += "async function scan(){let r=await fetch('/api/networks');let j=await r.json();let end=Date.now()+20000;while(j.scanning&&Date.now()<end){await new Promise(resolve=>setTimeout(resolve,750));r=await fetch('/api/networks');j=await r.json();}let out=document.getElementById('networks');out.innerHTML='';(j.networks||[]).forEach(n=>{let b=document.createElement('button');b.textContent=n.ssid;b.onclick=()=>document.getElementById('ssid').value=n.ssid;out.appendChild(b);})}";
    html += "async function submitCfg(){let ssid=document.getElementById('ssid').value;let password=document.getElementById('password').value;let security=document.getElementById('security').value;try{let r=await fetch('/api/configure',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({ssid,password,security})});let j=await r.json();if(j.accepted)j=await waitOperation(j.operationId);alert(j.message||'Operação concluída.');if(j.ok)location.href='/';}catch(e){alert('Configuração iniciada. Se a conexão cair, conecte-se à rede configurada ou ao ponto de acesso.');}return false;} </script>";
    html += "</body></html>";
    return html;
  }
  void enviarJson(WebServer& server, int codigo, const String& payload) {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(codigo, "application/json; charset=utf-8", payload);
  }

  void responderHtml(WebServer& server) {
    server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate");
    server.send(200, "text/html; charset=utf-8", gerarDashboardHtml());
  }

  void responderSetup(WebServer& server) {
    server.send(200, "text/html; charset=utf-8", gerarSetupHtml());
  }

  void responderPortalCativo(WebServer& server) {
    if (!wifiModoConfiguracao) {
      server.send(404, "text/plain", "Not found");
      return;
    }

    String destino = "http://" + wifiObterNomeHost() + ".local/";
    server.sendHeader("Location", destino, true);
    server.send(302, "text/plain", "Redirecionando para o portal de configuração.");
  }
}

void webserverAtualizarDados(const DadosSistema& dados) {
  gDadosAtuais = dados;
}

bool webserverOperacaoAtiva() {
  if (!gOperationMutex) return false;
  xSemaphoreTake(gOperationMutex, portMAX_DELAY);
  const bool active = gOperationActive;
  xSemaphoreGive(gOperationMutex);
  return active;
}

void webserverConectarWifiConfiguradoAsync() {
  Config config = storageCarregarConfig();
  if (config.wifi_ssid.isEmpty()) {
    iniciarModoConfiguracao();
    return;
  }

  uint32_t operationId;
  const bool started = iniciarOperacaoAsync([config](bool& ok, String& message) {
    ok = wifiConectar(config.wifi_ssid, config.wifi_password, config.wifi_security,
                      config.wifi_enterprise_identity, config.wifi_enterprise_username,
                      config.wifi_enterprise_password, config.wifi_enterprise_method,
                      config.wifi_enterprise_ttls_phase2);
    if (ok) {
      wifiIniciarNomeRede();
      message = "Conectado à rede Wi-Fi salva.";
    } else {
      iniciarModoConfiguracao();
      message = "Falha ao conectar; ponto de acesso de configuração iniciado.";
    }
  }, operationId, false, false);
  if (!started) iniciarModoConfiguracao();
}

bool webserverResetDeFabricaAsync() {
  uint32_t operationId;
  return iniciarOperacaoAsync([](bool& ok, String& message) {
    factoryResetExecutar();
    ok = true;
    message = "Reset de fábrica concluído.";
  }, operationId, true, false);
}

void webserverDashboardEnviarAsync(const DadosSistema& dados, const String& configuredName,
                                   bool alertaMudou, bool forcarEnvio) {
  static unsigned long lastUploadQueuedAt = 0;
  static unsigned long lastUnlinkQueuedAt = 0;
  static bool alertaPendente = false;
  static bool envioForcadoPendente = false;
  static bool alimentacaoInicializada = false;
  static bool alimentacaoExternaAnterior = false;
  static bool mudancaAlimentacaoPendente = false;
  constexpr unsigned long kUploadIntervalMs = 15000UL;
  constexpr unsigned long kUnlinkRetryIntervalMs = 60000UL;
  alertaPendente = alertaPendente || alertaMudou;
  envioForcadoPendente = envioForcadoPendente || forcarEnvio;
  if (alimentacaoInicializada && dados.alimentacao_externa != alimentacaoExternaAnterior) {
    mudancaAlimentacaoPendente = true;
  }
  alimentacaoInicializada = true;
  alimentacaoExternaAnterior = dados.alimentacao_externa;
  if (webserverOperacaoAtiva() || WiFi.status() != WL_CONNECTED) return;

  const unsigned long now = millis();
  const DashboardLinkInfo info = dashboardLinkObterInfo();
  const bool uploadDue = info.linked && (envioForcadoPendente || alertaPendente || mudancaAlimentacaoPendente || lastUploadQueuedAt == 0 ||
                                         now - lastUploadQueuedAt >= kUploadIntervalMs);
  const bool unlinkDue = lastUnlinkQueuedAt == 0 || now - lastUnlinkQueuedAt >= kUnlinkRetryIntervalMs;
  if (!uploadDue && !unlinkDue) return;

  uint32_t operationId;
  if (!iniciarOperacaoAsync([dados, configuredName, alertaPendente, envioForcadoPendente, mudancaAlimentacaoPendente,
                             uploadDue, unlinkDue](bool& ok, String& message) {
    if (unlinkDue) dashboardLinkProcessarPendencias();
    if (uploadDue) dashboardLinkEnviar(dados, configuredName, alertaPendente || mudancaAlimentacaoPendente, envioForcadoPendente);
    ok = true;
    message = "Comunicação com o dashboard concluída.";
  }, operationId, true, false)) {
    return;
  }
  if (uploadDue) {
    lastUploadQueuedAt = now;
    alertaPendente = false;
    envioForcadoPendente = false;
    mudancaAlimentacaoPendente = false;
  }
  if (unlinkDue) lastUnlinkQueuedAt = now;
}

void webserverVerificarInternetAsync() {
  static unsigned long lastCheckQueuedAt = 0;
  constexpr unsigned long kInternetCheckIntervalMs = 30000UL;
  if (webserverOperacaoAtiva() || WiFi.status() != WL_CONNECTED) return;
  const unsigned long now = millis();
  if (lastCheckQueuedAt != 0 && now - lastCheckQueuedAt < kInternetCheckIntervalMs) return;

  uint32_t operationId;
  if (iniciarOperacaoAsync([](bool& ok, String& message) {
    wifiInternetVerificar();
    ok = true;
    message = "Verificação de internet concluída.";
  }, operationId, false, false)) {
    lastCheckQueuedAt = now;
  }
}

void webserverHistoricoRegistrarAsync(const DadosSistema& dados) {
  static time_t lastQueuedAt = 0;
  if (dados.timestamp < 1577836800 || webserverOperacaoAtiva() ||
      (lastQueuedAt > 0 && dados.timestamp - lastQueuedAt < 60)) return;

  uint32_t operationId;
  if (iniciarOperacaoAsync([dados](bool& ok, String& message) {
    storageRegistrarLeituraHistorica(dados.temperatura, dados.umidade,
                                     static_cast<uint16_t>(dados.gas_adc), dados.timestamp);
    ok = true;
    message = "Leitura histórica registrada.";
  }, operationId, false, false)) {
    lastQueuedAt = dados.timestamp;
  }
}

WebServer* webserverIniciar(int porta) {
  if (!gOperationMutex) {
    gOperationMutex = xSemaphoreCreateMutex();
  }
  if (!gServer) {
    gServer = new WebServer(porta);
    const char* headerKeys[] = {"Authorization"};
    gServer->collectHeaders(headerKeys, 1);
  }

  gServer->on("/generate_204", HTTP_GET, []() { responderPortalCativo(*gServer); });
  gServer->on("/gen_204", HTTP_GET, []() { responderPortalCativo(*gServer); });
  gServer->on("/hotspot-detect.html", HTTP_GET, []() { responderPortalCativo(*gServer); });
  gServer->on("/connecttest.txt", HTTP_GET, []() { responderPortalCativo(*gServer); });
  gServer->on("/ncsi.txt", HTTP_GET, []() { responderPortalCativo(*gServer); });
  gServer->on("/redirect", HTTP_GET, []() { responderPortalCativo(*gServer); });
  gServer->onNotFound([]() { responderPortalCativo(*gServer); });

  gServer->on("/api/operation", HTTP_GET, []() {
    if (!gOperationMutex) {
      enviarJson(*gServer, 503, "{\"ok\":false,\"message\":\"Estado assíncrono indisponível.\"}");
      return;
    }
    const uint32_t requestedId = static_cast<uint32_t>(gServer->arg("id").toInt());
    bool active;
    bool done;
    bool ok;
    uint32_t id;
    String message;
    xSemaphoreTake(gOperationMutex, portMAX_DELAY);
    active = gOperationActive;
    done = gOperationDone;
    ok = gOperationOk;
    id = gOperationId;
    message = gOperationMessage;
    xSemaphoreGive(gOperationMutex);
    if (requestedId == 0 || requestedId != id) {
      enviarJson(*gServer, 404, "{\"ok\":false,\"message\":\"Operação não encontrada.\"}");
      return;
    }
    DynamicJsonDocument response(256);
    response["operationId"] = id;
    response["active"] = active;
    response["done"] = done;
    response["ok"] = ok;
    response["message"] = message;
    String payload;
    serializeJson(response, payload);
    enviarJson(*gServer, 200, payload);
  });

  gServer->on("/", []() {
    responderHtml(*gServer);
  });
  gServer->on("/setup", []() {
    responderSetup(*gServer);
  });
  gServer->on("/style.css", HTTP_GET, []() {
    gServer->sendHeader("Cache-Control", "no-store, no-cache, must-revalidate");
    gServer->send_P(200, "text/css; charset=utf-8", reinterpret_cast<const char*>(WEB_STYLE), WEB_STYLE_LEN);
  });
  gServer->on("/script.js", HTTP_GET, []() {
    gServer->sendHeader("Cache-Control", "no-store, no-cache, must-revalidate");
    gServer->send_P(200, "application/javascript; charset=utf-8", reinterpret_cast<const char*>(WEB_SCRIPT), WEB_SCRIPT_LEN);
  });
  gServer->on("/logo.jpg", HTTP_GET, []() {
    gServer->send_P(200, "image/jpeg", reinterpret_cast<const char*>(WEB_LOGO), WEB_LOGO_LEN);
  });
  gServer->on("/api/status", HTTP_GET, []() {
    DynamicJsonDocument doc(768);
    doc["temperatura"] = gDadosAtuais.temperatura;
    doc["umidade"] = gDadosAtuais.umidade;
    doc["gas_adc"] = gDadosAtuais.gas_adc;
    doc["alerta_temperatura"] = gDadosAtuais.alerta_temperatura;
    doc["alerta_umidade"] = gDadosAtuais.alerta_umidade;
    doc["alerta_gas"] = gDadosAtuais.alerta_gas;
    doc["buzzer_silenciado"] = gDadosAtuais.buzzer_silenciado;
    doc["alimentacao_externa"] = gDadosAtuais.alimentacao_externa;
    doc["bateria_tensao"] = gDadosAtuais.bateria_tensao;
    doc["bateria_percentual"] = gDadosAtuais.bateria_percentual;
    doc["bateria_presente"] = gDadosAtuais.bateria_presente;
    doc["rede_nome"] = WiFi.status() == WL_CONNECTED ? WiFi.SSID() : "";
    Config config = storageCarregarConfig();
    const bool conectado = WiFi.status() == WL_CONNECTED;
    doc["ip_acesso"] = wifiModoConfiguracao ? wifiObterIpAp() : wifiObterIp();
    doc["endereco_acesso"] = "http://" + wifiObterNomeHost() + ".local";
    doc["tipo_conexao"] = conectado ? config.wifi_security : (wifiModoConfiguracao ? "PONTO DE ACESSO" : "DESCONECTADO");
    if (conectado && (config.wifi_security == "WPA2-ENTERPRISE" || config.wifi_security == "WPA-ENTERPRISE")) {
      doc["metodo_enterprise"] = config.wifi_enterprise_method;
    }
    doc["internet_ok"] = wifiInternetDisponivel();

    String payload;
    serializeJson(doc, payload);
    enviarJson(*gServer, 200, payload);
  });

  gServer->on("/api/history", HTTP_GET, []() {
    String token = extrairToken(*gServer);
    String username;
    String role;
    if (!validarSessao(token, username, role)) {
      enviarJson(*gServer, 403, "{\"ok\":false,\"message\":\"Sessão inválida.\"}");
      return;
    }

    const int days = gServer->arg("days").toInt();
    if (days != 1 && days != 7 && days != 15 && days != 30) {
      enviarJson(*gServer, 400, "{\"ok\":false,\"message\":\"Selecione 1, 7, 15 ou 30 dias.\"}");
      return;
    }

    Config config = storageCarregarConfig();
    const bool admin = role == "admin";
    const bool canSeeTemp = admin || (role == "visitor" ? config.visitante_pode_ver_temperatura : config.usuario_pode_ver_temperatura);
    const bool canSeeHumidity = admin || (role == "visitor" ? config.visitante_pode_ver_umidade : config.usuario_pode_ver_umidade);
    const bool canSeeGas = admin || (role == "visitor" ? config.visitante_pode_ver_gas : config.usuario_pode_ver_gas);
    const time_t now = time(nullptr);
    DynamicJsonDocument response(32768);
    response["days"] = days;
    response["canSeeTemperature"] = canSeeTemp;
    response["canSeeHumidity"] = canSeeHumidity;
    response["canSeeGas"] = canSeeGas;
    const bool historyAvailable = storageInicializarHistorico();
    response["available"] = historyAvailable;
    JsonArray points = response.createNestedArray("points");
    if (now >= 1577836800 && historyAvailable) {
      const time_t start = now - static_cast<time_t>(days) * 24 * 60 * 60;
      const std::vector<PontoHistorico> history = storageCarregarHistorico(start, now, 240);
      for (const PontoHistorico& item : history) {
        JsonArray point = points.createNestedArray();
        point.add(static_cast<long>(item.timestamp));
        if (canSeeTemp) point.add(item.temperatura); else point.add(nullptr);
        if (canSeeHumidity) point.add(item.umidade); else point.add(nullptr);
        if (canSeeGas) point.add(item.gas_adc); else point.add(nullptr);
      }
    } else if (now < 1577836800) {
      response["timeValid"] = false;
    }
    String payload;
    serializeJson(response, payload);
    enviarJson(*gServer, 200, payload);
  });

  gServer->on("/api/status", HTTP_POST, []() {
    String token = extrairToken(*gServer);
    String username;
    String role;
    if (!validarSessao(token, username, role)) {
      enviarJson(*gServer, 403, "{\"ok\":false,\"message\":\"Acesso restrito.\"}");
      return;
    }

    Config config = storageCarregarConfig();
    const bool podeSilenciar = role == "admin" ||
      (role == "user" && config.usuario_pode_silenciar_alarme) ||
      (role == "visitor" && config.visitante_pode_silenciar_alarme);
    if (!podeSilenciar) {
      enviarJson(*gServer, 403, "{\"ok\":false,\"message\":\"Você não tem permissão para desativar o alarme sonoro.\"}");
      return;
    }

    String corpo = gServer->arg("plain");
    DynamicJsonDocument doc(64);
    DeserializationError err = deserializeJson(doc, corpo);
    if (err) {
      DynamicJsonDocument response(256);
      response["ok"] = false;
      response["message"] = "JSON inválido.";
      String payload;
      serializeJson(response, payload);
      enviarJson(*gServer, 400, payload);
      return;
    }

    if (doc.containsKey("buzzer_silenciado")) {
      buzzerSilenciado = doc["buzzer_silenciado"].as<bool>();
      gDadosAtuais.buzzer_silenciado = buzzerSilenciado;
    }

    DynamicJsonDocument response(256);
    response["ok"] = true;
    response["message"] = "Estado atualizado.";
    String payload;
    serializeJson(response, payload);
    enviarJson(*gServer, 200, payload);
  });

  gServer->on("/api/time", HTTP_GET, []() {
    String token = extrairToken(*gServer);
    String username;
    String role;
    if (!validarSessao(token, username, role) || role != "admin") {
      DynamicJsonDocument response(256);
      response["ok"] = false;
      response["message"] = "Acesso restrito ao administrador.";
      String payload;
      serializeJson(response, payload);
      enviarJson(*gServer, 403, payload);
      return;
    }

    struct tm hora = {};
    if (!getLocalTime(&hora, 1) || hora.tm_year < (2020 - 1900)) {
      if (!gTimeSyncRequested) {
        configTime(0, 0, "time.windows.com", "pool.ntp.org");
        gTimeSyncRequested = true;
      }
      enviarJson(*gServer, 202, "{\"ok\":false,\"pending\":true,\"message\":\"Sincronizando o horário pela rede.\"}");
      return;
    }

    char datetime[17];
    strftime(datetime, sizeof(datetime), "%Y-%m-%dT%H:%M", &hora);
    DynamicJsonDocument response(256);
    response["ok"] = true;
    response["datetime"] = datetime;
    response["source"] = "time.windows.com (fallback: pool.ntp.org)";
    String payload;
    serializeJson(response, payload);
    enviarJson(*gServer, 200, payload);
  });

  gServer->on("/api/visitor", HTTP_POST, []() {
    Config config = storageCarregarConfig();
    if (!config.permitir_visitante) {
      DynamicJsonDocument response(256);
      response["ok"] = false;
      response["message"] = "O acesso de visitante está desativado.";
      String payload;
      serializeJson(response, payload);
      enviarJson(*gServer, 403, payload);
      return;
    }

    Sessao sessao;
    sessao.token = "lab_visitor_" + String(millis(), HEX) + "_" + String(gSessoes.size());
    sessao.username = "visitante";
    sessao.role = "visitor";
    sessao.expiraEm = millis() + 60UL * 60UL * 1000UL;
    sessao.ip = gServer->client().remoteIP().toString();
    gSessoes.push_back(sessao);

    DynamicJsonDocument response(256);
    response["ok"] = true;
    response["token"] = sessao.token;
    response["username"] = "Visitante";
    response["role"] = "visitor";
    String payload;
    serializeJson(response, payload);
    enviarJson(*gServer, 200, payload);
  });

  gServer->on("/api/visitor-access", HTTP_POST, []() {
    String token = extrairToken(*gServer);
    String username;
    String role;
    if (!validarSessao(token, username, role) || role != "admin") {
      DynamicJsonDocument response(256);
      response["ok"] = false;
      response["message"] = "Acesso restrito ao administrador.";
      String payload;
      serializeJson(response, payload);
      enviarJson(*gServer, 403, payload);
      return;
    }

    DynamicJsonDocument doc(128);
    DeserializationError err = deserializeJson(doc, gServer->arg("plain"));
    if (err || !doc.containsKey("permitir_visitante")) {
      DynamicJsonDocument response(256);
      response["ok"] = false;
      response["message"] = "Valor de acesso visitante inválido.";
      String payload;
      serializeJson(response, payload);
      enviarJson(*gServer, 400, payload);
      return;
    }

    bool permitido = doc["permitir_visitante"].as<bool>();
    if (!storageSalvarAcessoVisitante(permitido)) {
      DynamicJsonDocument response(256);
      response["ok"] = false;
      response["message"] = "Não foi possível gravar o acesso visitante no armazenamento.";
      String payload;
      serializeJson(response, payload);
      enviarJson(*gServer, 500, payload);
      return;
    }

    bool salvo = storageCarregarAcessoVisitante();
    DynamicJsonDocument response(256);
    response["ok"] = salvo == permitido;
    response["permitir_visitante"] = salvo;
    response["message"] = salvo ? "Acesso visitante ativado." : "Acesso visitante desativado.";
    String payload;
    serializeJson(response, payload);
    enviarJson(*gServer, salvo == permitido ? 200 : 500, payload);
  });

  gServer->on("/api/dashboard-link", HTTP_GET, []() {
    String token = extrairToken(*gServer);
    String username;
    String role;
    if (!validarSessao(token, username, role) || role != "admin") {
      enviarJson(*gServer, 403, "{\"ok\":false,\"message\":\"Acesso restrito ao administrador.\"}");
      return;
    }
    if (dashboardOperacaoAtiva()) {
      enviarJson(*gServer, 200, "{\"busy\":true}");
      return;
    }
    DashboardLinkInfo info = dashboardLinkObterInfo();
    DynamicJsonDocument response(768);
    response["linked"] = info.linked;
    response["serverUrl"] = info.serverUrl;
    response["externalId"] = info.externalId;
    response["hasSent"] = info.hasSent;
    response["lastSendOk"] = info.lastSendOk;
    response["lastMessage"] = info.lastMessage;
    String payload;
    serializeJson(response, payload);
    enviarJson(*gServer, 200, payload);
  });

  gServer->on("/api/dashboard-link/login", HTTP_POST, []() {
    String token = extrairToken(*gServer);
    String username;
    String role;
    if (!validarSessao(token, username, role) || role != "admin") {
      enviarJson(*gServer, 403, "{\"ok\":false,\"message\":\"Acesso restrito ao administrador.\"}");
      return;
    }
    DynamicJsonDocument request(1024);
    if (deserializeJson(request, gServer->arg("plain"))) {
      enviarJson(*gServer, 400, "{\"ok\":false,\"message\":\"JSON invalido.\"}");
      return;
    }
    String serverUrl = request["serverUrl"] | "";
    String email = request["email"] | "";
    String password = request["password"] | "";
    String location = request["location"] | "";
    bool allowInsecureHttp = request["allowInsecureHttp"] | false;
    Config config = storageCarregarConfig();
    uint32_t operationId;
    const String deviceName = config.nome_aparelho;
    if (!iniciarOperacaoAsync([serverUrl, email, password, location, allowInsecureHttp,
                               deviceName](bool& ok, String& message) mutable {
      ok = dashboardLinkVincular(serverUrl, email, password, deviceName, location,
                                 allowInsecureHttp, message);
    }, operationId, true)) {
      enviarJson(*gServer, 409, "{\"ok\":false,\"message\":\"Outra operação está em andamento.\"}");
      return;
    }
    responderOperacaoIniciada(*gServer, operationId, "Validando a conta e vinculando o aparelho.");
  });

  gServer->on("/api/dashboard-link/remove", HTTP_POST, []() {
    String token = extrairToken(*gServer);
    String username;
    String role;
    if (!validarSessao(token, username, role) || role != "admin") {
      enviarJson(*gServer, 403, "{\"ok\":false,\"message\":\"Acesso restrito ao administrador.\"}");
      return;
    }
    uint32_t operationId;
    if (!iniciarOperacaoAsync([](bool& ok, String& message) {
      ok = dashboardLinkRemover(message);
    }, operationId, true)) {
      enviarJson(*gServer, 409, "{\"ok\":false,\"message\":\"Outra operação está em andamento.\"}");
      return;
    }
    responderOperacaoIniciada(*gServer, operationId, "Removendo o vínculo do aparelho.");
  });

  gServer->on("/api/networks", HTTP_GET, []() {
    std::vector<RedeWifi> redes;
    const bool scanConcluido = escanearRedes(redes);
    DynamicJsonDocument doc(4096);
    doc["scanning"] = !scanConcluido;
    JsonArray arr = doc.createNestedArray("networks");
    for (auto &s : redes) {
      JsonObject item = arr.createNestedObject();
      item["ssid"] = s.ssid;
      item["security"] = s.security;
    }
    String payload;
    serializeJson(doc, payload);
    enviarJson(*gServer, 200, payload);
  });

  gServer->on("/api/configure", HTTP_POST, []() {
    String corpo = gServer->arg("plain");
    DynamicJsonDocument doc(768);
    DeserializationError err = deserializeJson(doc, corpo);
    DynamicJsonDocument resp(256);
    if (err) {
      resp["ok"] = false; resp["message"] = "JSON inválido";
      String payload; serializeJson(resp, payload); enviarJson(*gServer, 400, payload); return;
    }
    String ssid = doc["ssid"] | "";
    String password = doc["password"] | "";
    String security = doc["security"] | "OPEN";

    Config cfg = storageCarregarConfig();
    Config anterior = cfg;
    cfg.wifi_ssid = ssid;
    cfg.wifi_password = password;
    cfg.wifi_security = security;
    cfg.wifi_enterprise_identity = doc["enterprise_identity"] | "";
    cfg.wifi_enterprise_username = doc["enterprise_username"] | "";
    cfg.wifi_enterprise_password = doc["enterprise_password"] | "";
    cfg.wifi_enterprise_method = doc["enterprise_method"] | "PEAP";
    cfg.wifi_enterprise_ttls_phase2 = doc["enterprise_ttls_phase2"] | "MSCHAPV2";
    uint32_t operationId;
    if (!iniciarOperacaoAsync([cfg, anterior](bool& conectado, String& message) {
      conectado = wifiConectar(cfg.wifi_ssid, cfg.wifi_password, cfg.wifi_security,
                               cfg.wifi_enterprise_identity, cfg.wifi_enterprise_username,
                               cfg.wifi_enterprise_password, cfg.wifi_enterprise_method,
                               cfg.wifi_enterprise_ttls_phase2);
      if (conectado) {
        storageSalvarConfig(cfg);
        wifiIniciarNomeRede();
        message = "Conectado com sucesso.";
      } else if (!anterior.wifi_ssid.isEmpty()) {
        const bool restaurada = wifiConectar(anterior.wifi_ssid, anterior.wifi_password,
            anterior.wifi_security, anterior.wifi_enterprise_identity,
            anterior.wifi_enterprise_username, anterior.wifi_enterprise_password,
            anterior.wifi_enterprise_method, anterior.wifi_enterprise_ttls_phase2);
        if (!restaurada) iniciarModoConfiguracao();
        message = "Não foi possível conectar após 2 tentativas. A rede anterior foi restaurada.";
      } else {
        iniciarModoConfiguracao();
        message = "Não foi possível conectar após 2 tentativas. O access point foi reaberto.";
      }
    }, operationId)) {
      enviarJson(*gServer, 409, "{\"ok\":false,\"message\":\"Outra operação está em andamento.\"}");
      return;
    }
    responderOperacaoIniciada(*gServer, operationId, "Conectando à rede Wi-Fi.");
  });

  gServer->on("/api/config", HTTP_GET, []() {
    Config config = storageCarregarConfig();
    DynamicJsonDocument doc(768);
    doc["temperatura_minima"] = config.temperatura_minima;
    doc["temperatura_maxima"] = config.temperatura_maxima;
    doc["umidade_minima"] = config.umidade_minima;
    doc["umidade_maxima"] = config.umidade_maxima;
    doc["gas_adc_critico"] = config.gas_adc_critico;
    doc["alarme_temperatura"] = config.alarme_temperatura;
    doc["alarme_umidade"] = config.alarme_umidade;
    doc["alarme_gas"] = config.alarme_gas;
    doc["buzzer"] = config.buzzer;
    doc["led_alerta"] = config.led_alerta;
    doc["intervalo_leitura"] = config.intervalo_leitura;
    doc["nome_aparelho"] = config.nome_aparelho;
    doc["permitir_visitante"] = config.permitir_visitante;
    doc["usuario_pode_ver_temperatura"] = config.usuario_pode_ver_temperatura;
    doc["usuario_pode_ver_umidade"] = config.usuario_pode_ver_umidade;
    doc["usuario_pode_ver_gas"] = config.usuario_pode_ver_gas;
    doc["usuario_pode_ver_wifi"] = config.usuario_pode_ver_wifi;
    doc["usuario_pode_ver_rede"] = config.usuario_pode_ver_rede;
    doc["usuario_pode_ver_logs"] = config.usuario_pode_ver_logs;
    doc["usuario_pode_silenciar_alarme"] = config.usuario_pode_silenciar_alarme;
    doc["visitante_pode_ver_temperatura"] = config.visitante_pode_ver_temperatura;
    doc["visitante_pode_ver_umidade"] = config.visitante_pode_ver_umidade;
    doc["visitante_pode_ver_gas"] = config.visitante_pode_ver_gas;
    doc["visitante_pode_ver_wifi"] = config.visitante_pode_ver_wifi;
    doc["visitante_pode_ver_rede"] = config.visitante_pode_ver_rede;
    doc["visitante_pode_ver_logs"] = config.visitante_pode_ver_logs;
    doc["visitante_pode_silenciar_alarme"] = config.visitante_pode_silenciar_alarme;
    doc["sinc_hora_automatica"] = config.sinc_hora_automatica;
    doc["data_hora_manual"] = config.data_hora_manual;
    doc["wifi"]["ssid"] = config.wifi_ssid;
    doc["wifi"]["security"] = config.wifi_security;
    doc["wifi"]["enterprise_identity"] = config.wifi_enterprise_identity;
    doc["wifi"]["enterprise_username"] = config.wifi_enterprise_username;
    doc["wifi"]["enterprise_method"] = config.wifi_enterprise_method;
    doc["wifi"]["enterprise_ttls_phase2"] = config.wifi_enterprise_ttls_phase2;

    String payload;
    serializeJson(doc, payload);
    enviarJson(*gServer, 200, payload);
  });

  gServer->on("/api/config", HTTP_POST, []() {
    String token = extrairToken(*gServer);
    String username;
    String role;
    if (!validarSessao(token, username, role) || !temPermissao(role, "config")) {
      DynamicJsonDocument response(256);
      response["ok"] = false;
      response["message"] = "Acesso restrito.";
      String payload;
      serializeJson(response, payload);
      enviarJson(*gServer, 403, payload);
      return;
    }

    String corpo = gServer->arg("plain");
    DynamicJsonDocument doc(8192);
    DeserializationError err = deserializeJson(doc, corpo);
    if (err) {
      DynamicJsonDocument response(256);
      response["ok"] = false;
      response["message"] = String("Não foi possível ler as configurações: ") + err.c_str();
      String payload;
      serializeJson(response, payload);
      enviarJson(*gServer, 400, payload);
      return;
    }

    Config config = storageCarregarConfig();
    const String nomeAnterior = config.nome_aparelho;
    String nomeNovo = config.nome_aparelho;
    if (doc.containsKey("nome_aparelho")) {
      nomeNovo = doc["nome_aparelho"].as<String>();
    }
    nomeNovo.trim();
    config.temperatura_minima = doc["temperatura_minima"] | config.temperatura_minima;
    config.temperatura_maxima = doc["temperatura_maxima"] | config.temperatura_maxima;
    config.umidade_minima = doc["umidade_minima"] | config.umidade_minima;
    config.umidade_maxima = doc["umidade_maxima"] | config.umidade_maxima;
    config.gas_adc_critico = doc["gas_adc_critico"] | config.gas_adc_critico;
    config.alarme_temperatura = doc["alarme_temperatura"] | config.alarme_temperatura;
    config.alarme_umidade = doc["alarme_umidade"] | config.alarme_umidade;
    config.alarme_gas = doc["alarme_gas"] | config.alarme_gas;
    config.led_alerta = doc["led_alerta"] | config.led_alerta;
    config.buzzer = doc["buzzer"] | config.buzzer;
    config.intervalo_leitura = doc["intervalo_leitura"] | config.intervalo_leitura;
    config.nome_aparelho = nomeNovo;
    if (doc.containsKey("permitir_visitante")) {
      config.permitir_visitante = doc["permitir_visitante"].as<bool>();
    }
    config.usuario_pode_ver_temperatura = doc["usuario_pode_ver_temperatura"] | config.usuario_pode_ver_temperatura;
    config.usuario_pode_ver_umidade = doc["usuario_pode_ver_umidade"] | config.usuario_pode_ver_umidade;
    config.usuario_pode_ver_gas = doc["usuario_pode_ver_gas"] | config.usuario_pode_ver_gas;
    config.usuario_pode_ver_wifi = doc["usuario_pode_ver_wifi"] | config.usuario_pode_ver_wifi;
    config.usuario_pode_ver_rede = doc["usuario_pode_ver_rede"] | config.usuario_pode_ver_rede;
    config.usuario_pode_ver_logs = doc["usuario_pode_ver_logs"] | config.usuario_pode_ver_logs;
    config.usuario_pode_silenciar_alarme = doc["usuario_pode_silenciar_alarme"] | config.usuario_pode_silenciar_alarme;
    config.visitante_pode_ver_temperatura = doc["visitante_pode_ver_temperatura"] | config.visitante_pode_ver_temperatura;
    config.visitante_pode_ver_umidade = doc["visitante_pode_ver_umidade"] | config.visitante_pode_ver_umidade;
    config.visitante_pode_ver_gas = doc["visitante_pode_ver_gas"] | config.visitante_pode_ver_gas;
    config.visitante_pode_ver_wifi = doc["visitante_pode_ver_wifi"] | config.visitante_pode_ver_wifi;
    config.visitante_pode_ver_rede = doc["visitante_pode_ver_rede"] | config.visitante_pode_ver_rede;
    config.visitante_pode_ver_logs = doc["visitante_pode_ver_logs"] | config.visitante_pode_ver_logs;
    config.visitante_pode_silenciar_alarme = doc["visitante_pode_silenciar_alarme"] | config.visitante_pode_silenciar_alarme;
    config.sinc_hora_automatica = doc["sinc_hora_automatica"] | config.sinc_hora_automatica;
    config.data_hora_manual = doc["data_hora_manual"] | config.data_hora_manual;
    storageSalvarConfig(config);
    wifiIniciarNomeRede();
    wifiAtualizarNomeAccessPoint();

    const bool nomeAlterado = nomeAnterior != nomeNovo;
    const String redirectUrl = "http://" + wifiObterNomeHost() + ".local/";

    DynamicJsonDocument response(512);
    response["ok"] = true;
    response["restart_required"] = nomeAlterado;
    response["redirect_url"] = redirectUrl;
    response["message"] = nomeAlterado ? "Nome do aparelho alterado. Reiniciando..." : "Configuração salva.";
    String payload;
    serializeJson(response, payload);
    enviarJson(*gServer, 200, payload);

    if (nomeAlterado) {
      gReinicioPendente = true;
      gReiniciarEm = millis() + 300;
    }
  });

  gServer->on("/api/backup", HTTP_GET, []() {
    String token = extrairToken(*gServer);
    String username;
    String role;
    if (!validarSessao(token, username, role) || role != "admin") {
      enviarJson(*gServer, 403, "{\"ok\":false,\"message\":\"Acesso restrito ao administrador.\"}");
      return;
    }

    Config config = storageCarregarConfig();
    DynamicJsonDocument backup(4096);
    backup["versao"] = 1;
    backup["temperatura_minima"] = config.temperatura_minima;
    backup["temperatura_maxima"] = config.temperatura_maxima;
    backup["umidade_minima"] = config.umidade_minima;
    backup["umidade_maxima"] = config.umidade_maxima;
    backup["gas_adc_critico"] = config.gas_adc_critico;
    backup["alarme_temperatura"] = config.alarme_temperatura;
    backup["alarme_umidade"] = config.alarme_umidade;
    backup["alarme_gas"] = config.alarme_gas;
    backup["led_alerta"] = config.led_alerta;
    backup["buzzer"] = config.buzzer;
    backup["intervalo_leitura"] = config.intervalo_leitura;
    backup["nome_aparelho"] = config.nome_aparelho;
    backup["permitir_visitante"] = config.permitir_visitante;
    backup["usuario_pode_ver_temperatura"] = config.usuario_pode_ver_temperatura;
    backup["usuario_pode_ver_umidade"] = config.usuario_pode_ver_umidade;
    backup["usuario_pode_ver_gas"] = config.usuario_pode_ver_gas;
    backup["usuario_pode_ver_wifi"] = config.usuario_pode_ver_wifi;
    backup["usuario_pode_ver_rede"] = config.usuario_pode_ver_rede;
    backup["usuario_pode_ver_logs"] = config.usuario_pode_ver_logs;
    backup["usuario_pode_silenciar_alarme"] = config.usuario_pode_silenciar_alarme;
    backup["visitante_pode_ver_temperatura"] = config.visitante_pode_ver_temperatura;
    backup["visitante_pode_ver_umidade"] = config.visitante_pode_ver_umidade;
    backup["visitante_pode_ver_gas"] = config.visitante_pode_ver_gas;
    backup["visitante_pode_ver_wifi"] = config.visitante_pode_ver_wifi;
    backup["visitante_pode_ver_rede"] = config.visitante_pode_ver_rede;
    backup["visitante_pode_ver_logs"] = config.visitante_pode_ver_logs;
    backup["visitante_pode_silenciar_alarme"] = config.visitante_pode_silenciar_alarme;
    backup["sinc_hora_automatica"] = config.sinc_hora_automatica;
    backup["data_hora_manual"] = config.data_hora_manual;
    backup["wifi_ssid"] = config.wifi_ssid;
    backup["wifi_password"] = config.wifi_password;
    backup["wifi_security"] = config.wifi_security;
    backup["wifi_enterprise_identity"] = config.wifi_enterprise_identity;
    backup["wifi_enterprise_username"] = config.wifi_enterprise_username;
    backup["wifi_enterprise_password"] = config.wifi_enterprise_password;
    String payload;
    serializeJson(backup, payload);
    enviarJson(*gServer, 200, payload);
  });

  gServer->on("/api/backup/restore", HTTP_POST, []() {
    String token = extrairToken(*gServer);
    String username;
    String role;
    if (!validarSessao(token, username, role) || role != "admin") {
      enviarJson(*gServer, 403, "{\"ok\":false,\"message\":\"Acesso restrito ao administrador.\"}");
      return;
    }

    DynamicJsonDocument doc(4096);
    if (deserializeJson(doc, gServer->arg("plain"))) {
      enviarJson(*gServer, 400, "{\"ok\":false,\"message\":\"Arquivo de backup inválido.\"}");
      return;
    }

    Config restaurada = storageCarregarConfig();
    restaurada.temperatura_minima = doc["temperatura_minima"] | restaurada.temperatura_minima;
    restaurada.temperatura_maxima = doc["temperatura_maxima"] | restaurada.temperatura_maxima;
    restaurada.umidade_minima = doc["umidade_minima"] | restaurada.umidade_minima;
    restaurada.umidade_maxima = doc["umidade_maxima"] | restaurada.umidade_maxima;
    restaurada.gas_adc_critico = doc["gas_adc_critico"] | restaurada.gas_adc_critico;
    restaurada.alarme_temperatura = doc["alarme_temperatura"] | restaurada.alarme_temperatura;
    restaurada.alarme_umidade = doc["alarme_umidade"] | restaurada.alarme_umidade;
    restaurada.alarme_gas = doc["alarme_gas"] | restaurada.alarme_gas;
    restaurada.led_alerta = doc["led_alerta"] | restaurada.led_alerta;
    restaurada.buzzer = doc["buzzer"] | restaurada.buzzer;
    restaurada.intervalo_leitura = doc["intervalo_leitura"] | restaurada.intervalo_leitura;
    restaurada.nome_aparelho = doc["nome_aparelho"] | restaurada.nome_aparelho;
    restaurada.permitir_visitante = doc["permitir_visitante"] | restaurada.permitir_visitante;
    restaurada.usuario_pode_ver_temperatura = doc["usuario_pode_ver_temperatura"] | restaurada.usuario_pode_ver_temperatura;
    restaurada.usuario_pode_ver_umidade = doc["usuario_pode_ver_umidade"] | restaurada.usuario_pode_ver_umidade;
    restaurada.usuario_pode_ver_gas = doc["usuario_pode_ver_gas"] | restaurada.usuario_pode_ver_gas;
    restaurada.usuario_pode_ver_wifi = doc["usuario_pode_ver_wifi"] | restaurada.usuario_pode_ver_wifi;
    restaurada.usuario_pode_ver_rede = doc["usuario_pode_ver_rede"] | restaurada.usuario_pode_ver_rede;
    restaurada.usuario_pode_ver_logs = doc["usuario_pode_ver_logs"] | restaurada.usuario_pode_ver_logs;
    restaurada.usuario_pode_silenciar_alarme = doc["usuario_pode_silenciar_alarme"] | restaurada.usuario_pode_silenciar_alarme;
    restaurada.visitante_pode_ver_temperatura = doc["visitante_pode_ver_temperatura"] | restaurada.visitante_pode_ver_temperatura;
    restaurada.visitante_pode_ver_umidade = doc["visitante_pode_ver_umidade"] | restaurada.visitante_pode_ver_umidade;
    restaurada.visitante_pode_ver_gas = doc["visitante_pode_ver_gas"] | restaurada.visitante_pode_ver_gas;
    restaurada.visitante_pode_ver_wifi = doc["visitante_pode_ver_wifi"] | restaurada.visitante_pode_ver_wifi;
    restaurada.visitante_pode_ver_rede = doc["visitante_pode_ver_rede"] | restaurada.visitante_pode_ver_rede;
    restaurada.visitante_pode_ver_logs = doc["visitante_pode_ver_logs"] | restaurada.visitante_pode_ver_logs;
    restaurada.visitante_pode_silenciar_alarme = doc["visitante_pode_silenciar_alarme"] | restaurada.visitante_pode_silenciar_alarme;
    restaurada.sinc_hora_automatica = doc["sinc_hora_automatica"] | restaurada.sinc_hora_automatica;
    restaurada.data_hora_manual = doc["data_hora_manual"] | restaurada.data_hora_manual;
    restaurada.wifi_ssid = doc["wifi_ssid"] | "";
    restaurada.wifi_password = doc["wifi_password"] | "";
    restaurada.wifi_security = doc["wifi_security"] | "OPEN";
    restaurada.wifi_enterprise_identity = doc["wifi_enterprise_identity"] | "";
    restaurada.wifi_enterprise_username = doc["wifi_enterprise_username"] | "";
    restaurada.wifi_enterprise_password = doc["wifi_enterprise_password"] | "";
    restaurada.wifi_enterprise_method = doc["wifi_enterprise_method"] | "PEAP";
    restaurada.wifi_enterprise_ttls_phase2 = doc["wifi_enterprise_ttls_phase2"] | "MSCHAPV2";

    uint32_t operationId;
    if (!iniciarOperacaoAsync([restaurada](bool& ok, String& message) {
      if (!restaurada.wifi_ssid.isEmpty() &&
          !wifiConectar(restaurada.wifi_ssid, restaurada.wifi_password, restaurada.wifi_security,
                        restaurada.wifi_enterprise_identity, restaurada.wifi_enterprise_username,
                        restaurada.wifi_enterprise_password, restaurada.wifi_enterprise_method,
                        restaurada.wifi_enterprise_ttls_phase2)) {
        message = "Backup rejeitado: não foi possível conectar à rede salva.";
        return;
      }
      storageSalvarConfig(restaurada);
      wifiIniciarNomeRede();
      ok = true;
      message = "Backup restaurado com sucesso.";
    }, operationId)) {
      enviarJson(*gServer, 409, "{\"ok\":false,\"message\":\"Outra operação está em andamento.\"}");
      return;
    }
    responderOperacaoIniciada(*gServer, operationId, "Validando a rede e restaurando o backup.");
  });

  gServer->on("/api/factory-reset", HTTP_POST, []() {
    String token = extrairToken(*gServer);
    String username;
    String role;
    if (!validarSessao(token, username, role) || role != "admin") {
      enviarJson(*gServer, 403, "{\"ok\":false,\"message\":\"Acesso restrito ao administrador.\"}");
      return;
    }
    uint32_t operationId;
    if (!iniciarOperacaoAsync([](bool& ok, String& message) {
      factoryResetExecutar();
      ok = true;
      message = "Reset de fábrica concluído.";
    }, operationId, true)) {
      enviarJson(*gServer, 409, "{\"ok\":false,\"message\":\"Outra operação está em andamento.\"}");
      return;
    }
    responderOperacaoIniciada(*gServer, operationId, "Reset de fábrica iniciado.");
  });

  gServer->on("/api/network-config", HTTP_POST, []() {
    String token = extrairToken(*gServer);
    String username;
    String role;
    if (!validarSessao(token, username, role) || role != "admin") {
      DynamicJsonDocument response(256);
      response["ok"] = false;
      response["message"] = "Acesso restrito ao administrador.";
      String payload;
      serializeJson(response, payload);
      enviarJson(*gServer, 403, payload);
      return;
    }

    DynamicJsonDocument doc(768);
    DeserializationError err = deserializeJson(doc, gServer->arg("plain"));
    String ssid = doc["ssid"] | "";
    String security = doc["security"] | "OPEN";
    if (err || ssid.isEmpty()) {
      DynamicJsonDocument response(256);
      response["ok"] = false;
      response["message"] = "Informe uma rede válida.";
      String payload;
      serializeJson(response, payload);
      enviarJson(*gServer, 400, payload);
      return;
    }

    const bool enterprise = security == "WPA2-ENTERPRISE" || security == "WPA-ENTERPRISE";
    const String enterpriseMethod = doc["enterprise_method"] | "PEAP";
    if (enterprise && (String(doc["enterprise_username"] | "").isEmpty() ||
               String(doc["enterprise_password"] | "").isEmpty() ||
               (enterpriseMethod != "PEAP" && enterpriseMethod != "TTLS" && enterpriseMethod != "FAST"))) {
      DynamicJsonDocument response(256);
      response["ok"] = false;
      response["message"] = "Preencha login e senha da rede Enterprise.";
      String payload;
      serializeJson(response, payload);
      enviarJson(*gServer, 400, payload);
      return;
    }

    Config configAnterior = storageCarregarConfig();
    Config novaConfig = configAnterior;
    novaConfig.wifi_ssid = ssid;
    novaConfig.wifi_password = doc["password"] | "";
    novaConfig.wifi_security = security;
    novaConfig.wifi_enterprise_username = doc["enterprise_username"] | "";
    novaConfig.wifi_enterprise_password = doc["enterprise_password"] | "";
    novaConfig.wifi_enterprise_identity = doc["enterprise_identity"] | "";
    if (novaConfig.wifi_enterprise_identity.isEmpty()) {
      novaConfig.wifi_enterprise_identity = novaConfig.wifi_enterprise_username;
    }
    novaConfig.wifi_enterprise_method = doc["enterprise_method"] | "PEAP";
    novaConfig.wifi_enterprise_ttls_phase2 = doc["enterprise_ttls_phase2"] | "MSCHAPV2";

    uint32_t operationId;
    if (!iniciarOperacaoAsync([novaConfig, configAnterior](bool& conectado, String& message) {
      conectado = wifiConectar(novaConfig.wifi_ssid, novaConfig.wifi_password, novaConfig.wifi_security,
                               novaConfig.wifi_enterprise_identity, novaConfig.wifi_enterprise_username,
                               novaConfig.wifi_enterprise_password, novaConfig.wifi_enterprise_method,
                               novaConfig.wifi_enterprise_ttls_phase2);
      if (conectado) {
        storageSalvarConfig(novaConfig);
        wifiIniciarNomeRede();
        message = "Rede conectada e salva para as próximas conexões.";
      } else if (!configAnterior.wifi_ssid.isEmpty()) {
        const bool restaurada = wifiConectar(configAnterior.wifi_ssid, configAnterior.wifi_password,
            configAnterior.wifi_security, configAnterior.wifi_enterprise_identity,
            configAnterior.wifi_enterprise_username, configAnterior.wifi_enterprise_password,
            configAnterior.wifi_enterprise_method, configAnterior.wifi_enterprise_ttls_phase2);
        if (!restaurada) iniciarModoConfiguracao();
        message = restaurada ? "Não foi possível conectar. A rede anterior foi restaurada."
                             : "Não foi possível conectar nem restaurar a rede anterior; access point iniciado.";
      } else {
        iniciarModoConfiguracao();
        message = "Não foi possível conectar. O access point foi reaberto.";
      }
    }, operationId)) {
      enviarJson(*gServer, 409, "{\"ok\":false,\"message\":\"Outra operação está em andamento.\"}");
      return;
    }
    responderOperacaoIniciada(*gServer, operationId, "Conectando à rede Wi-Fi.");
  });

  gServer->on("/api/network-config/delete", HTTP_POST, []() {
    String token = extrairToken(*gServer);
    String username;
    String role;
    if (!validarSessao(token, username, role) || role != "admin") {
      enviarJson(*gServer, 403, "{\"ok\":false,\"message\":\"Acesso restrito ao administrador.\"}");
      return;
    }

    uint32_t operationId;
    if (!iniciarOperacaoAsync([](bool& ok, String& message) {
      Config config = storageCarregarConfig();
      config.wifi_ssid = "";
      config.wifi_password = "";
      config.wifi_security = "OPEN";
      config.wifi_enterprise_identity = "";
      config.wifi_enterprise_username = "";
      config.wifi_enterprise_password = "";
      config.wifi_enterprise_method = "PEAP";
      config.wifi_enterprise_ttls_phase2 = "MSCHAPV2";
      storageSalvarConfig(config);
      iniciarModoConfiguracao();
      ok = true;
      message = "Conexão excluída. O access point está ativo.";
    }, operationId)) {
      enviarJson(*gServer, 409, "{\"ok\":false,\"message\":\"Outra operação está em andamento.\"}");
      return;
    }
    responderOperacaoIniciada(*gServer, operationId, "Excluindo a rede salva e iniciando o access point.");
  });

  gServer->on("/api/login", HTTP_POST, []() {
    String corpo = gServer->arg("plain");
    DynamicJsonDocument doc(512);
    DeserializationError err = deserializeJson(doc, corpo);

    if (err) {
      DynamicJsonDocument response(256);
      response["ok"] = false;
      response["message"] = "JSON inválido.";
      String payload;
      serializeJson(response, payload);
      enviarJson(*gServer, 400, payload);
      return;
    }

    String username = doc["username"] | "";
    String password = doc["password"] | "";
    String ip = gServer->client().remoteIP().toString();

    String token;
    String message;
    bool ok = login(username, password, ip, token, message);
    DynamicJsonDocument response(256);
    response["ok"] = ok;
    response["message"] = message;
    if (ok) {
      response["token"] = token;
      response["username"] = username;

      std::vector<Usuario> usuarios = storageCarregarUsuarios();
      for (const auto& usuario : usuarios) {
        if (usuario.username.equalsIgnoreCase(username)) {
          response["role"] = usuario.role;
          break;
        }
      }

      Sessao sessao;
      sessao.token = token;
      sessao.username = username;
      sessao.role = response["role"] | "user";
      sessao.expiraEm = millis() + 60UL * 60UL * 1000UL;
      sessao.ip = ip;
      gSessoes.push_back(sessao);
    }

    String payload;
    serializeJson(response, payload);
    enviarJson(*gServer, ok ? 200 : 401, payload);
  });

  gServer->on("/api/session", HTTP_GET, []() {
    String token = extrairToken(*gServer);
    String username;
    String role;
    if (!validarSessao(token, username, role)) {
      DynamicJsonDocument response(256);
      response["ok"] = false;
      response["message"] = "Sessão inválida.";
      String payload;
      serializeJson(response, payload);
      enviarJson(*gServer, 401, payload);
      return;
    }

    DynamicJsonDocument response(256);
    response["ok"] = true;
    response["username"] = username;
    response["role"] = role;
    String payload;
    serializeJson(response, payload);
    enviarJson(*gServer, 200, payload);
  });

  gServer->on("/api/users", HTTP_GET, []() {
    String token = extrairToken(*gServer);
    String username;
    String role;
    if (!validarSessao(token, username, role) || !temPermissao(role, "users")) {
      DynamicJsonDocument response(256);
      response["ok"] = false;
      response["message"] = "Acesso restrito ao administrador.";
      String payload;
      serializeJson(response, payload);
      enviarJson(*gServer, 403, payload);
      return;
    }

    std::vector<Usuario> usuarios = storageCarregarUsuarios();
    DynamicJsonDocument response(512);
    JsonArray arr = response.createNestedArray("users");
    for (const auto& usuario : usuarios) {
      JsonObject item = arr.createNestedObject();
      item["username"] = usuario.username;
      item["role"] = usuario.role;
    }
    String payload;
    serializeJson(response, payload);
    enviarJson(*gServer, 200, payload);
  });

  gServer->on("/api/users", HTTP_POST, []() {
    String token = extrairToken(*gServer);
    String username;
    String role;
    if (!validarSessao(token, username, role) || !temPermissao(role, "users")) {
      DynamicJsonDocument response(256);
      response["ok"] = false;
      response["message"] = "Acesso restrito ao administrador.";
      String payload;
      serializeJson(response, payload);
      enviarJson(*gServer, 403, payload);
      return;
    }

    String corpo = gServer->arg("plain");
    DynamicJsonDocument doc(512);
    DeserializationError err = deserializeJson(doc, corpo);
    if (err) {
      DynamicJsonDocument response(256);
      response["ok"] = false;
      response["message"] = "JSON inválido.";
      String payload;
      serializeJson(response, payload);
      enviarJson(*gServer, 400, payload);
      return;
    }

    String novoUsuario = doc["username"] | "";
    String senha = doc["password"] | "";
    String confirmacaoSenha = doc["password_confirm"] | "";
    String papel = doc["role"] | "user";

    if (novoUsuario.isEmpty() || senha.isEmpty()) {
      DynamicJsonDocument response(256);
      response["ok"] = false;
      response["message"] = "Usuário e senha são obrigatórios.";
      String payload;
      serializeJson(response, payload);
      enviarJson(*gServer, 400, payload);
      return;
    }

    if (senha != confirmacaoSenha) {
      DynamicJsonDocument response(256);
      response["ok"] = false;
      response["message"] = "As senhas não conferem.";
      String payload;
      serializeJson(response, payload);
      enviarJson(*gServer, 400, payload);
      return;
    }

    Usuario usuario;
    usuario.username = novoUsuario;
    usuario.passwordHash = storageHashSenha(senha);
    usuario.role = papel;

    bool ok = storageCriarUsuario(usuario);
    DynamicJsonDocument response(256);
    response["ok"] = ok;
    response["message"] = ok ? "Usuário criado com sucesso." : "Usuário já existe.";
    String payload;
    serializeJson(response, payload);
    enviarJson(*gServer, ok ? 200 : 409, payload);
  });

  gServer->on("/api/users/password", HTTP_POST, []() {
    String token = extrairToken(*gServer);
    String username;
    String role;
    if (!validarSessao(token, username, role) || role != "admin") {
      DynamicJsonDocument response(256);
      response["ok"] = false;
      response["message"] = "Acesso restrito ao administrador.";
      String payload;
      serializeJson(response, payload);
      enviarJson(*gServer, 403, payload);
      return;
    }

    DynamicJsonDocument doc(512);
    if (deserializeJson(doc, gServer->arg("plain"))) {
      enviarJson(*gServer, 400, "{\"ok\":false,\"message\":\"JSON inválido.\"}");
      return;
    }

    String alvo = doc["username"] | "";
    String senha = doc["password"] | "";
    String confirmacao = doc["password_confirm"] | "";
    if (alvo.isEmpty() || senha.isEmpty()) {
      enviarJson(*gServer, 400, "{\"ok\":false,\"message\":\"Usuário e nova senha são obrigatórios.\"}");
      return;
    }
    if (senha != confirmacao) {
      enviarJson(*gServer, 400, "{\"ok\":false,\"message\":\"As senhas não conferem.\"}");
      return;
    }

    bool ok = storageAtualizarSenha(alvo, senha);
    DynamicJsonDocument response(256);
    response["ok"] = ok;
    response["message"] = ok ? "Senha alterada com sucesso." : "Usuário não encontrado.";
    String payload;
    serializeJson(response, payload);
    enviarJson(*gServer, ok ? 200 : 404, payload);
  });

  gServer->on("/api/users", HTTP_DELETE, []() {
    String token = extrairToken(*gServer);
    String username;
    String role;
    if (!validarSessao(token, username, role) || !temPermissao(role, "users")) {
      DynamicJsonDocument response(256);
      response["ok"] = false;
      response["message"] = "Acesso restrito ao administrador.";
      String payload;
      serializeJson(response, payload);
      enviarJson(*gServer, 403, payload);
      return;
    }

    String corpo = gServer->arg("plain");
    DynamicJsonDocument doc(256);
    DeserializationError err = deserializeJson(doc, corpo);
    if (err) {
      DynamicJsonDocument response(256);
      response["ok"] = false;
      response["message"] = "JSON inválido.";
      String payload;
      serializeJson(response, payload);
      enviarJson(*gServer, 400, payload);
      return;
    }

    String alvo = doc["username"] | "";
    if (alvo.isEmpty() || alvo.equalsIgnoreCase("admin")) {
      DynamicJsonDocument response(256);
      response["ok"] = false;
      response["message"] = "Não foi possível remover este usuário.";
      String payload;
      serializeJson(response, payload);
      enviarJson(*gServer, 400, payload);
      return;
    }

    bool ok = storageRemoverUsuario(alvo);
    DynamicJsonDocument response(256);
    response["ok"] = ok;
    response["message"] = ok ? "Usuário removido." : "Usuário não encontrado.";
    String payload;
    serializeJson(response, payload);
    enviarJson(*gServer, ok ? 200 : 404, payload);
  });

  gServer->on("/api/logs", HTTP_GET, []() {
    String token = extrairToken(*gServer);
    String username;
    String role;
    if (!validarSessao(token, username, role)) {
      DynamicJsonDocument response(256);
      response["ok"] = false;
      response["message"] = "Acesso restrito.";
      String payload;
      serializeJson(response, payload);
      enviarJson(*gServer, 403, payload);
      return;
    }

    Config config = storageCarregarConfig();
    const bool podeVerLogs = role == "admin" ||
      (role == "user" && config.usuario_pode_ver_logs) ||
      (role == "visitor" && config.visitante_pode_ver_logs);
    if (!podeVerLogs) {
      DynamicJsonDocument response(256);
      response["ok"] = false;
      response["message"] = "Visualização de eventos não autorizada.";
      String payload;
      serializeJson(response, payload);
      enviarJson(*gServer, 403, payload);
      return;
    }

    std::vector<RegistroAlerta> logs = storageCarregarLogs();
    DynamicJsonDocument response(1024);
    JsonArray arr = response.createNestedArray("logs");
    for (const auto& item : logs) {
      JsonObject obj = arr.createNestedObject();
      obj["timestamp"] = static_cast<long>(item.timestamp);
      obj["evento"] = item.evento;
    }
    String payload;
    serializeJson(response, payload);
    enviarJson(*gServer, 200, payload);
  });

  gServer->on("/api/logs", HTTP_DELETE, []() {
    String token = extrairToken(*gServer);
    String username;
    String role;
    if (!validarSessao(token, username, role) || role != "admin") {
      DynamicJsonDocument response(256);
      response["ok"] = false;
      response["message"] = "Acesso restrito ao administrador.";
      String payload;
      serializeJson(response, payload);
      enviarJson(*gServer, 403, payload);
      return;
    }

    storageLimparLogs();
    if (!storageCarregarLogs().empty()) {
      DynamicJsonDocument response(256);
      response["ok"] = false;
      response["message"] = "Os logs não puderam ser removidos do armazenamento.";
      String payload;
      serializeJson(response, payload);
      enviarJson(*gServer, 500, payload);
      return;
    }
    DynamicJsonDocument response(256);
    response["ok"] = true;
    response["message"] = "Logs apagados.";
    String payload;
    serializeJson(response, payload);
    enviarJson(*gServer, 200, payload);
  });

  gServer->on("/api/logs/clear", HTTP_POST, []() {
    String token = extrairToken(*gServer);
    String username;
    String role;
    if (!validarSessao(token, username, role) || role != "admin") {
      DynamicJsonDocument response(256);
      response["ok"] = false;
      response["message"] = "Acesso restrito ao administrador.";
      String payload;
      serializeJson(response, payload);
      enviarJson(*gServer, 403, payload);
      return;
    }

    storageLimparLogs();
    if (!storageCarregarLogs().empty()) {
      DynamicJsonDocument response(256);
      response["ok"] = false;
      response["message"] = "Os logs não puderam ser removidos do armazenamento.";
      String payload;
      serializeJson(response, payload);
      enviarJson(*gServer, 500, payload);
      return;
    }
    DynamicJsonDocument response(256);
    response["ok"] = true;
    response["message"] = "Logs apagados.";
    String payload;
    serializeJson(response, payload);
    enviarJson(*gServer, 200, payload);
  });

  gServer->begin();
  Serial.println("Servidor HTTP iniciado.");
  return gServer;
}

void webserverServidor(WebServer* server) {
  if (server != nullptr) {
    server->handleClient();
  }
  if (gReinicioPendente && millis() >= gReiniciarEm) {
    gReinicioPendente = false;
    ESP.restart();
  }
}
