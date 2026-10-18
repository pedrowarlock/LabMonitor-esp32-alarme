#include "factory_reset.hpp"

#include <Preferences.h>

#include "config.hpp"
#include "dashboard_link.hpp"
#include "storage.hpp"

namespace {
  constexpr unsigned long kFactoryResetHoldMs = 5000UL;
  bool gFactoryResetButtonHeld = false;
  unsigned long gFactoryResetStartedAt = 0;
}

void factoryResetExecutar() {
  Serial.println();
  Serial.println("==========================================");
  Serial.println("       RESET DE FÁBRICA CONFIRMADO");
  Serial.println("==========================================");

  String unlinkMessage;
  if (!dashboardLinkRemover(unlinkMessage)) {
    storageRemoverVinculoDashboard();
  }
  storageLimparHistorico();

  Preferences prefs;

  if (prefs.begin("sensorcfg", false)) {
    prefs.clear();
    prefs.putString("nome_aparelho", "");
    prefs.end();
  }

  if (prefs.begin("viscfg", false)) {
    prefs.clear();
    prefs.end();
  }

  if (prefs.begin("sensorusr", false)) {
    prefs.clear();
    prefs.end();
  }

  if (prefs.begin("sensorlog", false)) {
    prefs.clear();
    prefs.end();
  }

  if (prefs.begin("lablink", false)) {
    prefs.clear();
    prefs.end();
  }

  std::vector<Usuario> usuariosPadrao;
  Usuario admin;
  admin.username = "admin";
  admin.passwordHash = storageHashSenha("admin");
  admin.role = "admin";
  usuariosPadrao.push_back(admin);

  Usuario visitante;
  visitante.username = "visitante";
  visitante.passwordHash = storageHashSenha("visitante");
  visitante.role = "visitor";
  usuariosPadrao.push_back(visitante);

  storageSalvarUsuarios(usuariosPadrao);

  Serial.println("Configurações, credenciais e logs apagados.");
  Serial.println("O aparelho iniciará em access point aberto: MONITOR-CONFIGURAR.");
  Serial.println();
  Serial.println("Reiniciando ESP32...");
  delay(2000);
  ESP.restart();
}

bool factoryResetVerificar() {
  if (digitalRead(config::FACTORY_RESET_PIN) == HIGH) {
    if (gFactoryResetButtonHeld) {
      Serial.println("Reset de fábrica cancelado.");
    }
    gFactoryResetButtonHeld = false;
    return false;
  }

  if (!gFactoryResetButtonHeld) {
    gFactoryResetButtonHeld = true;
    gFactoryResetStartedAt = millis();
    Serial.println();
    Serial.println("Botão de reset pressionado.");
    Serial.println("Segure por 5 segundos...");
  }

  if (millis() - gFactoryResetStartedAt >= kFactoryResetHoldMs) {
    gFactoryResetButtonHeld = false;
    return true;
  }

  return false;
}
