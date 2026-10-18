#include "storage.hpp"

#include <LittleFS.h>
#include <Preferences.h>
#include <algorithm>
#include <math.h>
#include <string>
#include <vector>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <mbedtls/sha256.h>

namespace {
  const char* kNamespace = "sensorcfg";
  const char* kVisitorNamespace = "viscfg";
  constexpr size_t kPendingDashboardUnlinks = 4;
  constexpr char kHistoryPath[] = "/sensor-history.bin";
  constexpr char kHistoryTempPath[] = "/sensor-history.tmp";
  constexpr char kHistoryBackupPath[] = "/sensor-history.bak";
  constexpr uint32_t kHistoryRetentionSeconds = 30UL * 24UL * 60UL * 60UL;
  constexpr uint32_t kMinimumHistoryTimestamp = 1577836800UL;
  SemaphoreHandle_t gHistoryMutex = nullptr;
  bool gHistoryReady = false;
  bool gHistoryMountAttempted = false;

  struct __attribute__((packed)) HistoricoBinario {
    uint32_t timestamp;
    int16_t temperaturaCentesimos;
    uint16_t umidadeCentesimos;
    uint16_t gasAdc;
  };

  struct AcumuladorHistorico {
    uint64_t somaTimestamp = 0;
    int64_t somaTemperatura = 0;
    uint64_t somaUmidade = 0;
    uint64_t somaGas = 0;
    uint32_t quantidade = 0;
  };

  bool compactarHistorico(uint32_t timestampMinimo) {
    File source = LittleFS.open(kHistoryPath, FILE_READ);
    if (!source) return false;
    LittleFS.remove(kHistoryTempPath);
    File target = LittleFS.open(kHistoryTempPath, FILE_WRITE);
    if (!target) {
      source.close();
      return false;
    }

    HistoricoBinario registro;
    while (source.available() >= static_cast<int>(sizeof(registro))) {
      if (source.read(reinterpret_cast<uint8_t*>(&registro), sizeof(registro)) != sizeof(registro)) break;
      if (registro.timestamp >= timestampMinimo) {
        target.write(reinterpret_cast<const uint8_t*>(&registro), sizeof(registro));
      }
    }
    source.close();
    target.close();

    LittleFS.remove(kHistoryBackupPath);
    if (!LittleFS.rename(kHistoryPath, kHistoryBackupPath)) {
      LittleFS.remove(kHistoryTempPath);
      return false;
    }
    if (LittleFS.rename(kHistoryTempPath, kHistoryPath)) {
      LittleFS.remove(kHistoryBackupPath);
      return true;
    }
    LittleFS.rename(kHistoryBackupPath, kHistoryPath);
    LittleFS.remove(kHistoryTempPath);
    return false;
  }

  String pendingDashboardKey(const char* prefix, size_t slot) {
    String key(prefix);
    key += slot;
    return key;
  }
}

bool storageInicializarHistorico() {
  if (!gHistoryMutex) gHistoryMutex = xSemaphoreCreateMutex();
  if (!gHistoryMutex) return false;
  xSemaphoreTake(gHistoryMutex, portMAX_DELAY);
  if (!gHistoryMountAttempted) {
    gHistoryReady = LittleFS.begin(true);
    gHistoryMountAttempted = true;
  }
  const bool ready = gHistoryReady;
  xSemaphoreGive(gHistoryMutex);
  if (!ready) Serial.println("Aviso: LittleFS indisponível; histórico de sensores desativado.");
  return ready;
}

void storageRegistrarLeituraHistorica(float temperatura, float umidade, uint16_t gasAdc, time_t timestamp) {
  if (timestamp < kMinimumHistoryTimestamp || !storageInicializarHistorico()) return;
  xSemaphoreTake(gHistoryMutex, portMAX_DELAY);

  uint32_t ultimoTimestamp = 0;
  File current = LittleFS.open(kHistoryPath, FILE_READ);
  if (current && current.size() >= sizeof(HistoricoBinario)) {
    HistoricoBinario ultimo;
    if (current.seek(current.size() - sizeof(ultimo)) &&
        current.read(reinterpret_cast<uint8_t*>(&ultimo), sizeof(ultimo)) == sizeof(ultimo)) {
      ultimoTimestamp = ultimo.timestamp;
    }
  }
  current.close();
  if (ultimoTimestamp && static_cast<uint32_t>(timestamp) - ultimoTimestamp < 60UL) {
    xSemaphoreGive(gHistoryMutex);
    return;
  }

  HistoricoBinario registro;
  registro.timestamp = static_cast<uint32_t>(timestamp);
  registro.temperaturaCentesimos = static_cast<int16_t>(lroundf(temperatura * 100.0f));
  registro.umidadeCentesimos = static_cast<uint16_t>(lroundf(umidade * 100.0f));
  registro.gasAdc = gasAdc;
  File history = LittleFS.open(kHistoryPath, FILE_APPEND);
  const bool written = history && history.write(reinterpret_cast<const uint8_t*>(&registro), sizeof(registro)) == sizeof(registro);
  if (history) history.close();
  if (written) {
    static uint32_t lastPruneAt = 0;
    const uint32_t now = static_cast<uint32_t>(timestamp);
    if (lastPruneAt == 0 || now - lastPruneAt >= 24UL * 60UL * 60UL) {
      if (compactarHistorico(now > kHistoryRetentionSeconds ? now - kHistoryRetentionSeconds : 0)) {
        lastPruneAt = now;
      }
    }
  }
  xSemaphoreGive(gHistoryMutex);
}

std::vector<PontoHistorico> storageCarregarHistorico(time_t inicio, time_t fim, size_t maxPontos) {
  std::vector<PontoHistorico> pontos;
  if (fim <= inicio || maxPontos == 0 || !storageInicializarHistorico()) return pontos;
  const uint64_t intervalo = static_cast<uint64_t>(fim - inicio);
  const size_t quantidadeBuckets = std::min(maxPontos, static_cast<size_t>(std::max<uint64_t>(1, intervalo / 60UL)));
  std::vector<AcumuladorHistorico> acumuladores(quantidadeBuckets);

  xSemaphoreTake(gHistoryMutex, portMAX_DELAY);
  File history = LittleFS.open(kHistoryPath, FILE_READ);
  if (history) {
    HistoricoBinario registro;
    while (history.available() >= static_cast<int>(sizeof(registro))) {
      if (history.read(reinterpret_cast<uint8_t*>(&registro), sizeof(registro)) != sizeof(registro)) break;
      const time_t recordedAt = static_cast<time_t>(registro.timestamp);
      if (recordedAt < inicio || recordedAt > fim) continue;
      size_t bucket = static_cast<size_t>((static_cast<uint64_t>(recordedAt - inicio) * quantidadeBuckets) / intervalo);
      if (bucket >= quantidadeBuckets) bucket = quantidadeBuckets - 1;
      AcumuladorHistorico& accumulator = acumuladores[bucket];
      accumulator.somaTimestamp += registro.timestamp;
      accumulator.somaTemperatura += registro.temperaturaCentesimos;
      accumulator.somaUmidade += registro.umidadeCentesimos;
      accumulator.somaGas += registro.gasAdc;
      ++accumulator.quantidade;
    }
    history.close();
  }
  xSemaphoreGive(gHistoryMutex);

  for (const AcumuladorHistorico& accumulator : acumuladores) {
    if (accumulator.quantidade == 0) continue;
    PontoHistorico ponto;
    ponto.timestamp = static_cast<time_t>(accumulator.somaTimestamp / accumulator.quantidade);
    ponto.temperatura = static_cast<float>(accumulator.somaTemperatura) / accumulator.quantidade / 100.0f;
    ponto.umidade = static_cast<float>(accumulator.somaUmidade) / accumulator.quantidade / 100.0f;
    ponto.gas_adc = static_cast<uint16_t>(accumulator.somaGas / accumulator.quantidade);
    pontos.push_back(ponto);
  }
  return pontos;
}

void storageLimparHistorico() {
  if (!storageInicializarHistorico()) return;
  xSemaphoreTake(gHistoryMutex, portMAX_DELAY);
  LittleFS.remove(kHistoryPath);
  LittleFS.remove(kHistoryTempPath);
  LittleFS.remove(kHistoryBackupPath);
  xSemaphoreGive(gHistoryMutex);
}

Config storageCarregarConfig() {
  Preferences prefs;
  Config config = {};
  config.alarme_temperatura = true;
  config.temperatura_minima = 0.0f;
  config.temperatura_maxima = 45.0f;
  config.alarme_umidade = true;
  config.umidade_minima = 0.0f;
  config.umidade_maxima = 100.0f;
  config.alarme_gas = true;
  config.gas_adc_critico = 3800;
  config.led_alerta = true;
  config.buzzer = true;
  config.intervalo_leitura = 1;
  config.wifi_security = "OPEN";
  config.wifi_enterprise_method = "PEAP";
  config.wifi_enterprise_ttls_phase2 = "MSCHAPV2";
  config.nome_aparelho = "";
  config.permitir_visitante = true;
  config.usuario_pode_ver_temperatura = true;
  config.usuario_pode_ver_umidade = true;
  config.usuario_pode_ver_gas = true;
  config.usuario_pode_ver_wifi = true;
  config.usuario_pode_ver_rede = false;
  config.usuario_pode_ver_logs = true;
  config.usuario_pode_silenciar_alarme = true;
  config.visitante_pode_ver_temperatura = true;
  config.visitante_pode_ver_umidade = true;
  config.visitante_pode_ver_gas = true;
  config.visitante_pode_ver_wifi = true;
  config.visitante_pode_ver_rede = false;
  config.visitante_pode_ver_logs = true;
  config.visitante_pode_silenciar_alarme = true;

  if (!prefs.begin(kNamespace, true)) {
    return config;
  }

  config.alarme_temperatura = prefs.getBool("alarme_temp", true);
  config.temperatura_minima = prefs.getFloat("temp_min", 0.0f);
  config.temperatura_maxima = prefs.getFloat("temp_max", 35.0f);

  config.alarme_umidade = prefs.getBool("alarme_umid", true);
  config.umidade_minima = prefs.getFloat("umid_min", 00.0f);
  config.umidade_maxima = prefs.getFloat("umid_max", 100.0f);

  config.alarme_gas = prefs.getBool("alarme_gas", true);
  config.gas_adc_critico = prefs.getInt("gas_critico", 3800);

  config.buzzer = prefs.getBool("buzzer", true);
  config.led_alerta = prefs.getBool("led_alerta", true);
  config.intervalo_leitura = prefs.getInt("intervalo", 1);

  config.wifi_ssid = prefs.getString("wifi_ssid", "");
  config.wifi_password = prefs.getString("wifi_pass", "");
  config.wifi_security = prefs.getString("wifi_sec", "OPEN");
  config.wifi_enterprise_identity = prefs.getString("ent_identity", "");
  config.wifi_enterprise_username = prefs.getString("ent_user", "");
  config.wifi_enterprise_password = prefs.getString("ent_pass", "");
  config.wifi_enterprise_method = prefs.getString("ent_method", "PEAP");
  config.wifi_enterprise_ttls_phase2 = prefs.getString("ent_phase2", "MSCHAPV2");

  config.nome_aparelho = prefs.getString("nome_aparelho", "");
  config.usuario_pode_ver_temperatura = prefs.getBool("usr_ver_temp", true);
  config.usuario_pode_ver_umidade = prefs.getBool("usr_ver_umid", true);
  config.usuario_pode_ver_gas = prefs.getBool("usr_ver_gas", true);
  config.usuario_pode_ver_wifi = prefs.getBool("usr_ver_wifi", true);
  config.usuario_pode_ver_rede = prefs.getBool("usr_ver_rede", false);
  config.usuario_pode_ver_logs = prefs.getBool("usr_ver_logs", true);
  config.usuario_pode_silenciar_alarme = prefs.getBool("usr_silenciar", true);
  config.visitante_pode_ver_temperatura = prefs.getBool("vis_ver_temp", true);
  config.visitante_pode_ver_umidade = prefs.getBool("vis_ver_umid", true);
  config.visitante_pode_ver_gas = prefs.getBool("vis_ver_gas", true);
  config.visitante_pode_ver_wifi = prefs.getBool("vis_ver_wifi", true);
  config.visitante_pode_ver_rede = prefs.getBool("vis_ver_rede", false);
  config.visitante_pode_ver_logs = prefs.getBool("vis_ver_logs", true);
  config.visitante_pode_silenciar_alarme = prefs.getBool("vis_silenciar", true);
  config.sinc_hora_automatica = prefs.getBool("sinc_hora_auto", false);
  config.data_hora_manual = prefs.getString("data_hora_manual", "");

  prefs.end();
  config.permitir_visitante = storageCarregarAcessoVisitante();
  return config;
}

void storageSalvarConfig(const Config& config) {
  Preferences prefs;
  if (!prefs.begin(kNamespace, false)) {
    return;
  }

  prefs.putBool("alarme_temp", config.alarme_temperatura);
  prefs.putFloat("temp_min", config.temperatura_minima);
  prefs.putFloat("temp_max", config.temperatura_maxima);

  prefs.putBool("alarme_umid", config.alarme_umidade);
  prefs.putFloat("umid_min", config.umidade_minima);
  prefs.putFloat("umid_max", config.umidade_maxima);

  prefs.putBool("alarme_gas", config.alarme_gas);
  prefs.putInt("gas_critico", config.gas_adc_critico);

  prefs.putBool("buzzer", config.buzzer);
  prefs.putBool("led_alerta", config.led_alerta);
  prefs.putInt("intervalo", config.intervalo_leitura);

  prefs.putString("wifi_ssid", config.wifi_ssid);
  prefs.putString("wifi_pass", config.wifi_password);
  prefs.putString("wifi_sec", config.wifi_security);
  prefs.putString("ent_identity", config.wifi_enterprise_identity);
  prefs.putString("ent_user", config.wifi_enterprise_username);
  prefs.putString("ent_pass", config.wifi_enterprise_password);
  prefs.putString("ent_method", config.wifi_enterprise_method);
  prefs.putString("ent_phase2", config.wifi_enterprise_ttls_phase2);

  prefs.putString("nome_aparelho", config.nome_aparelho);
  prefs.putBool("usr_ver_temp", config.usuario_pode_ver_temperatura);
  prefs.putBool("usr_ver_umid", config.usuario_pode_ver_umidade);
  prefs.putBool("usr_ver_gas", config.usuario_pode_ver_gas);
  prefs.putBool("usr_ver_wifi", config.usuario_pode_ver_wifi);
  prefs.putBool("usr_ver_rede", config.usuario_pode_ver_rede);
  prefs.putBool("usr_ver_logs", config.usuario_pode_ver_logs);
  prefs.putBool("usr_silenciar", config.usuario_pode_silenciar_alarme);
  prefs.putBool("vis_ver_temp", config.visitante_pode_ver_temperatura);
  prefs.putBool("vis_ver_umid", config.visitante_pode_ver_umidade);
  prefs.putBool("vis_ver_gas", config.visitante_pode_ver_gas);
  prefs.putBool("vis_ver_wifi", config.visitante_pode_ver_wifi);
  prefs.putBool("vis_ver_rede", config.visitante_pode_ver_rede);
  prefs.putBool("vis_ver_logs", config.visitante_pode_ver_logs);
  prefs.putBool("vis_silenciar", config.visitante_pode_silenciar_alarme);
  prefs.putBool("sinc_hora_auto", config.sinc_hora_automatica);
  prefs.putString("data_hora_manual", config.data_hora_manual);

  prefs.end();
  storageSalvarAcessoVisitante(config.permitir_visitante);
}

bool storageCarregarAcessoVisitante() {
  Preferences prefs;
  if (!prefs.begin(kVisitorNamespace, true)) {
    return true;
  }

  bool permitido = prefs.getUChar("enabled", 1) != 0;
  prefs.end();
  return permitido;
}

bool storageSalvarAcessoVisitante(bool permitido) {
  Preferences prefs;
  if (!prefs.begin(kVisitorNamespace, false)) {
    storageLimparLogs();
    if (!prefs.begin(kVisitorNamespace, false)) {
      return false;
    }
  }

  prefs.putUChar("enabled", permitido ? 1 : 0);
  prefs.end();
  return storageCarregarAcessoVisitante() == permitido;
}

std::vector<Usuario> storageCarregarUsuarios() {
  Preferences prefs;
  std::vector<Usuario> usuarios;

  if (!prefs.begin("sensorusr", true)) {
    Usuario admin;
    admin.username = "admin";
    admin.passwordHash = storageHashSenha("admin");
    admin.role = "admin";
    usuarios.push_back(admin);

    Usuario visitante;
    visitante.username = "visitante";
    visitante.passwordHash = storageHashSenha("visitante");
    visitante.role = "visitor";
    usuarios.push_back(visitante);
    return usuarios;
  }

  int count = prefs.getInt("count", 0);
  for (int i = 0; i < count; ++i) {
    String key = "usr_" + String(i);
    Usuario usuario;
    usuario.username = prefs.getString((key + "_user").c_str(), "");
    usuario.passwordHash = prefs.getString((key + "_pass").c_str(), "");
    usuario.role = prefs.getString((key + "_role").c_str(), "user");
    if (!usuario.username.isEmpty()) {
      usuarios.push_back(usuario);
    }
  }

  if (usuarios.empty()) {
    Usuario admin;
    admin.username = "admin";
    admin.passwordHash = storageHashSenha("admin");
    admin.role = "admin";
    usuarios.push_back(admin);

    Usuario visitante;
    visitante.username = "visitante";
    visitante.passwordHash = storageHashSenha("visitante");
    visitante.role = "visitor";
    usuarios.push_back(visitante);
  }

  prefs.end();
  return usuarios;
}

void storageSalvarUsuarios(const std::vector<Usuario>& usuarios) {
  Preferences prefs;
  if (!prefs.begin("sensorusr", false)) {
    return;
  }

  prefs.putInt("count", usuarios.size());
  for (size_t i = 0; i < usuarios.size(); ++i) {
    String key = "usr_" + String(i);
    prefs.putString((key + "_user").c_str(), usuarios[i].username);
    prefs.putString((key + "_pass").c_str(), usuarios[i].passwordHash);
    prefs.putString((key + "_role").c_str(), usuarios[i].role);
  }

  prefs.end();
}

String storageHashSenha(const String& senha) {
  unsigned char digest[32];
  mbedtls_sha256_context ctx;

  mbedtls_sha256_init(&ctx);
  mbedtls_sha256_starts(&ctx, 0);
  mbedtls_sha256_update(&ctx, reinterpret_cast<const unsigned char*>(senha.c_str()), senha.length());
  mbedtls_sha256_finish(&ctx, digest);
  mbedtls_sha256_free(&ctx);

  char hexBuffer[65];
  for (int i = 0; i < 32; ++i) {
    sprintf(hexBuffer + (i * 2), "%02x", digest[i]);
  }
  hexBuffer[64] = '\0';

  return String(hexBuffer);
}

bool storageCriarUsuario(const Usuario& usuario) {
  std::vector<Usuario> usuarios = storageCarregarUsuarios();
  for (const auto& atual : usuarios) {
    if (atual.username.equalsIgnoreCase(usuario.username)) {
      return false;
    }
  }

  usuarios.push_back(usuario);
  storageSalvarUsuarios(usuarios);
  return true;
}

bool storageRemoverUsuario(const String& username) {
  std::vector<Usuario> usuarios = storageCarregarUsuarios();
  auto it = std::remove_if(usuarios.begin(), usuarios.end(),
    [&username](const Usuario& usuario) {
      return usuario.username.equalsIgnoreCase(username);
    });

  if (it == usuarios.begin()) {
    return false;
  }

  usuarios.erase(it, usuarios.end());
  storageSalvarUsuarios(usuarios);
  return true;
}

bool storageAtualizarSenha(const String& username, const String& senha) {
  std::vector<Usuario> usuarios = storageCarregarUsuarios();
  for (auto& usuario : usuarios) {
    if (usuario.username.equalsIgnoreCase(username)) {
      usuario.passwordHash = storageHashSenha(senha);
      storageSalvarUsuarios(usuarios);
      return true;
    }
  }
  return false;
}

std::vector<RegistroAlerta> storageCarregarLogs() {
  std::vector<RegistroAlerta> logs;
  Preferences prefs;

  if (!prefs.begin("sensorlog", true)) {
    return logs;
  }

  int count = prefs.getInt("count", 0);
  for (int i = 0; i < count; ++i) {
    RegistroAlerta log;
    log.timestamp = static_cast<time_t>(prefs.getLong(("log_ts_" + String(i)).c_str(), 0));
    log.evento = prefs.getString(("log_evt_" + String(i)).c_str(), "");
    logs.push_back(log);
  }

  prefs.end();
  return logs;
}

void storageAdicionarLog(const RegistroAlerta& registro) {
  std::vector<RegistroAlerta> logs = storageCarregarLogs();
  logs.push_back(registro);
  storageSalvarLogs(logs);
}

void storageSalvarLogs(const std::vector<RegistroAlerta>& logs) {
  Preferences prefs;
  if (!prefs.begin("sensorlog", false)) {
    return;
  }

  const size_t limite = 50;
  const size_t inicio = logs.size() > limite ? logs.size() - limite : 0;
  const size_t quantidade = logs.size() - inicio;

  prefs.clear();
  prefs.putInt("count", quantidade);
  for (size_t i = 0; i < quantidade; ++i) {
    const RegistroAlerta& log = logs[inicio + i];
    prefs.putLong(("log_ts_" + String(i)).c_str(), static_cast<long>(log.timestamp));
    prefs.putString(("log_evt_" + String(i)).c_str(), log.evento);
  }

  prefs.end();
}

void storageLimparLogs() {
  Preferences prefs;
  if (!prefs.begin("sensorlog", false)) {
    return;
  }

  prefs.clear();
  prefs.putInt("count", 0);
  prefs.end();
}

void storageRegistrarEvento(const String& evento) {
  RegistroAlerta reg;
  reg.timestamp = time(nullptr);
  reg.evento = evento;
  storageAdicionarLog(reg);
}

VinculoDashboard storageCarregarVinculoDashboard() {
  VinculoDashboard vinculo;
  Preferences prefs;
  if (!prefs.begin("lablink", true)) return vinculo;
  vinculo.serverUrl = prefs.getString("server", "");
  vinculo.deviceToken = prefs.getString("token", "");
  vinculo.externalId = prefs.getString("external_id", "");
  prefs.end();
  return vinculo;
}

bool storageSalvarVinculoDashboard(const VinculoDashboard& vinculo) {
  Preferences prefs;
  if (!prefs.begin("lablink", false)) return false;
  prefs.putString("server", vinculo.serverUrl);
  prefs.putString("token", vinculo.deviceToken);
  prefs.putString("external_id", vinculo.externalId);
  prefs.end();
  return storageCarregarVinculoDashboard().deviceToken == vinculo.deviceToken;
}

bool storageRemoverVinculoDashboard() {
  Preferences prefs;
  if (!prefs.begin("lablink", false)) return false;
  prefs.remove("token");
  prefs.remove("external_id");
  prefs.end();
  return storageCarregarVinculoDashboard().deviceToken.isEmpty();
}

VinculoDashboard storageCarregarRemocaoDashboardPendente() {
  VinculoDashboard vinculo;
  Preferences prefs;
  if (!prefs.begin("lablink", true)) return vinculo;
  for (size_t slot = 0; slot < kPendingDashboardUnlinks; ++slot) {
    String tokenKey = pendingDashboardKey("pend_tok", slot);
    vinculo.deviceToken = prefs.getString(tokenKey.c_str(), "");
    if (vinculo.deviceToken.isEmpty()) continue;
    String serverKey = pendingDashboardKey("pend_srv", slot);
    vinculo.serverUrl = prefs.getString(serverKey.c_str(), "");
    break;
  }
  prefs.end();
  return vinculo;
}

bool storageSalvarRemocaoDashboardPendente(const VinculoDashboard& vinculo) {
  Preferences prefs;
  if (!prefs.begin("lablink", false)) return false;
  size_t freeSlot = kPendingDashboardUnlinks;
  for (size_t slot = 0; slot < kPendingDashboardUnlinks; ++slot) {
    String tokenKey = pendingDashboardKey("pend_tok", slot);
    String pendingToken = prefs.getString(tokenKey.c_str(), "");
    if (pendingToken == vinculo.deviceToken) {
      String serverKey = pendingDashboardKey("pend_srv", slot);
      bool salvo = prefs.getString(serverKey.c_str(), "") == vinculo.serverUrl;
      prefs.end();
      return salvo;
    }
    if (pendingToken.isEmpty() && freeSlot == kPendingDashboardUnlinks) freeSlot = slot;
  }
  if (freeSlot == kPendingDashboardUnlinks) {
    prefs.end();
    return false;
  }
  String serverKey = pendingDashboardKey("pend_srv", freeSlot);
  String tokenKey = pendingDashboardKey("pend_tok", freeSlot);
  prefs.putString(serverKey.c_str(), vinculo.serverUrl);
  prefs.putString(tokenKey.c_str(), vinculo.deviceToken);
  bool salvo = prefs.getString(serverKey.c_str(), "") == vinculo.serverUrl
      && prefs.getString(tokenKey.c_str(), "") == vinculo.deviceToken;
  prefs.end();
  return salvo;
}

void storageRemoverRemocaoDashboardPendente() {
  Preferences prefs;
  if (!prefs.begin("lablink", false)) return;
  for (size_t slot = 0; slot + 1 < kPendingDashboardUnlinks; ++slot) {
    String nextServerKey = pendingDashboardKey("pend_srv", slot + 1);
    String nextTokenKey = pendingDashboardKey("pend_tok", slot + 1);
    String serverKey = pendingDashboardKey("pend_srv", slot);
    String tokenKey = pendingDashboardKey("pend_tok", slot);
    prefs.putString(serverKey.c_str(), prefs.getString(nextServerKey.c_str(), ""));
    prefs.putString(tokenKey.c_str(), prefs.getString(nextTokenKey.c_str(), ""));
  }
  String lastServerKey = pendingDashboardKey("pend_srv", kPendingDashboardUnlinks - 1);
  String lastTokenKey = pendingDashboardKey("pend_tok", kPendingDashboardUnlinks - 1);
  prefs.remove(lastServerKey.c_str());
  prefs.remove(lastTokenKey.c_str());
  prefs.end();
}
