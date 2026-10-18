#include "wifi.hpp"

#include <DNSServer.h>
#include <ESPmDNS.h>
#include <time.h>
#include "esp_eap_client.h"
#include <vector>
#include "config.hpp"
#include "led_azul.hpp"
#include "storage.hpp"

volatile bool wifiModoConfiguracao = false;

namespace {
  DNSServer dnsServidor;
  constexpr int kMaxConnectionAttempts = 2;
  constexpr unsigned long kConnectionTimeoutMs = 6000UL;
  volatile bool gInternetCheckResult = false;

  bool nomePadrao(const String& nome) {
    return nome.isEmpty() || nome.equalsIgnoreCase("");
  }

  String nomeSeguro(const String& nome, bool maiusculo) {
    String resultado;
    for (size_t i = 0; i < nome.length() && resultado.length() < 22; ++i) {
      char caractere = nome.charAt(i);
      if ((caractere >= 'a' && caractere <= 'z') || (caractere >= 'A' && caractere <= 'Z') ||
          (caractere >= '0' && caractere <= '9')) {
        if (maiusculo && caractere >= 'a' && caractere <= 'z') caractere -= ('a' - 'A');
        if (!maiusculo && caractere >= 'A' && caractere <= 'Z') caractere += ('a' - 'A');
        resultado += caractere;
      } else if (caractere == ' ' || caractere == '-' || caractere == '_') {
        if (!resultado.isEmpty() && !resultado.endsWith("-")) resultado += '-';
      }
    }
    while (resultado.endsWith("-")) resultado.remove(resultado.length() - 1);
    return resultado;
  }
}

void wifiIniciar() {
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
}

String nomeSeguranca(int numero) {
  if (numero == 5) return "WPA2-ENTERPRISE";
  switch (numero) {
    case WIFI_AUTH_OPEN: return "OPEN";
    case WIFI_AUTH_WEP: return "WEP";
    case WIFI_AUTH_WPA_PSK: return "WPA-PSK";
    case WIFI_AUTH_WPA2_PSK: return "WPA2-PSK";
    case WIFI_AUTH_WPA_WPA2_PSK: return "WPA/WPA2-PSK";
    default: return "UNKNOWN";
  }
}

bool escanearRedes(std::vector<RedeWifi>& redes) {
  static bool scanIniciado = false;
  int n = WiFi.scanComplete();
  if (n == WIFI_SCAN_RUNNING) {
    scanIniciado = true;
    return false;
  }
  if (n == WIFI_SCAN_FAILED) {
    if (scanIniciado) {
      scanIniciado = false;
      return true;
    }
    scanIniciado = true;
    n = WiFi.scanNetworks(true, true);
    if (n == WIFI_SCAN_RUNNING) return false;
    if (n < 0) {
      scanIniciado = false;
      return true;
    }
  }

  redes.clear();
  for (int i = 0; i < n; ++i) {
    RedeWifi rede;
    rede.ssid = WiFi.SSID(i);
    if (rede.ssid.isEmpty()) {
      continue;
    }
    rede.security = nomeSeguranca(WiFi.encryptionType(i));
    redes.push_back(rede);
  }
  WiFi.scanDelete();
  scanIniciado = false;
  return true;
}

bool wifiConectar(const String& ssid, const String& password, const String& security,
                  const String& enterpriseIdentity, const String& enterpriseUsername,
                  const String& enterprisePassword, const String& enterpriseMethod,
                  const String& enterpriseTtlsPhase2) {
  Serial.println();
  Serial.println("Conectando ao Wi-Fi:");
  Serial.println(ssid);

  MDNS.end();
  for (int tentativa = 1; tentativa <= kMaxConnectionAttempts; ++tentativa) {
    WiFi.disconnect(true, false);
    delay(500);
    WiFi.mode(WIFI_STA);
    ledAzulDesligar();

    esp_wifi_sta_enterprise_disable();
    if (security == "WPA2-ENTERPRISE" || security == "WPA-ENTERPRISE") {
      esp_eap_method_t eapMethod = ESP_EAP_TYPE_PEAP;
      if (enterpriseMethod == "TLS") eapMethod = ESP_EAP_TYPE_TLS;
      else if (enterpriseMethod == "TTLS") eapMethod = ESP_EAP_TYPE_TTLS;
      else if (enterpriseMethod == "FAST") eapMethod = ESP_EAP_TYPE_FAST;
      esp_eap_client_set_eap_methods(eapMethod);
      if (enterpriseMethod == "TTLS") {
        esp_eap_ttls_phase2_types phase2 = ESP_EAP_TTLS_PHASE2_MSCHAPV2;
        if (enterpriseTtlsPhase2 == "PAP") phase2 = ESP_EAP_TTLS_PHASE2_PAP;
        else if (enterpriseTtlsPhase2 == "CHAP") phase2 = ESP_EAP_TTLS_PHASE2_CHAP;
        else if (enterpriseTtlsPhase2 == "MSCHAP") phase2 = ESP_EAP_TTLS_PHASE2_MSCHAP;
        else if (enterpriseTtlsPhase2 == "EAP") phase2 = ESP_EAP_TTLS_PHASE2_EAP;
        esp_eap_client_set_ttls_phase2_method(phase2);
      }
      esp_eap_client_set_identity(
        reinterpret_cast<const uint8_t*>(enterpriseIdentity.c_str()), enterpriseIdentity.length());
      esp_eap_client_set_username(
        reinterpret_cast<const uint8_t*>(enterpriseUsername.c_str()), enterpriseUsername.length());
      esp_eap_client_set_password(
        reinterpret_cast<const uint8_t*>(enterprisePassword.c_str()), enterprisePassword.length());
      esp_wifi_sta_enterprise_enable();
      WiFi.begin(ssid.c_str());
    } else if (security == "OPEN") {
      WiFi.begin(ssid.c_str());
    } else {
      WiFi.begin(ssid.c_str(), password.c_str());
    }

    Serial.printf("Tentativa %d de %d...\n", tentativa, kMaxConnectionAttempts);
    unsigned long inicio = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - inicio) < kConnectionTimeoutMs) {
      delay(250);
      Serial.print(".");
    }
    if (WiFi.status() == WL_CONNECTED) {
      break;
    }
    Serial.println();
    WiFi.disconnect(true, false);
    delay(500);
  }

  if (WiFi.status() == WL_CONNECTED) {
    ledAzulLigar();
    wifiModoConfiguracao = false;
    desligarAp();
    Serial.println();
    Serial.println("Wi-Fi conectado!");
    Serial.print("IP: ");
    Serial.println(wifiObterIp());
    return true;
  }

  Serial.println();
  Serial.println("Falha ao conectar ao Wi-Fi.");
  return false;
}

void iniciarModoConfiguracao() {
  MDNS.end();
  WiFi.mode(WIFI_AP);
  String nomeAp = wifiObterNomeAccessPoint();
  bool ok = WiFi.softAP(nomeAp.c_str());
  wifiModoConfiguracao = ok;

  if (ok) {
    dnsServidor.start(53, "*", WiFi.softAPIP());
    wifiIniciarNomeRede();
  }

  Serial.println();
  Serial.println(ok ? "Modo de configuração ativado." : "Falha ao iniciar AP.");
  Serial.print("AP: ");
  Serial.println(nomeAp);
  Serial.print("Senha AP: ");
  Serial.println(config::AP_PASSWORD);
  Serial.print("IP AP: ");
  Serial.println(wifiObterIpAp());
}

void wifiAtualizarNomeAccessPoint() {
  if (!wifiModoConfiguracao) {
    return;
  }

  dnsServidor.stop();
  WiFi.softAPdisconnect(true);
  delay(100);
  String nomeAp = wifiObterNomeAccessPoint();
  if (WiFi.softAP(nomeAp.c_str())) {
    dnsServidor.start(53, "*", WiFi.softAPIP());
    wifiIniciarNomeRede();
    Serial.print("Access point atualizado: ");
    Serial.println(nomeAp);
  }
}

void wifiIniciarNomeRede() {
  MDNS.end();
  if (WiFi.status() == WL_CONNECTED) {
    configTime(0, 0, "pool.ntp.org", "time.google.com");
  }
  String host = wifiObterNomeHost();
  if (MDNS.begin(host.c_str())) {
    MDNS.addService("http", "tcp", 80);
    Serial.print("Acesso por nome: http://");
    Serial.print(host);
    Serial.println(".local");
  } else {
    Serial.println("Não foi possível iniciar o mDNS.");
  }
}

String wifiObterNomeAccessPoint() {
  Config config = storageCarregarConfig();
  if (nomePadrao(config.nome_aparelho)) return String(config::AP_SSID);
  String sufixo = nomeSeguro(config.nome_aparelho, true);
  return sufixo.isEmpty() ? String(config::AP_SSID) : "MONITOR-" + sufixo;
}

String wifiObterNomeHost() {
  Config config = storageCarregarConfig();
  if (nomePadrao(config.nome_aparelho)) return "monitor-configurar";
  String sufixo = nomeSeguro(config.nome_aparelho, false);
  return sufixo.isEmpty() ? "monitor-configurar" : "monitor-" + sufixo;
}

void desligarAp() {
  dnsServidor.stop();
  WiFi.softAPdisconnect(true);
  wifiModoConfiguracao = false;
}

String wifiObterIp() {
  return WiFi.localIP().toString();
}

String wifiObterIpAp() {
  return WiFi.softAPIP().toString();
}

bool wifiInternetDisponivel() {
  return WiFi.status() == WL_CONNECTED && gInternetCheckResult;
}

void wifiInternetVerificar() {
  if (WiFi.status() != WL_CONNECTED) {
    gInternetCheckResult = false;
    return;
  }
  IPAddress endereco;
  gInternetCheckResult = WiFi.hostByName("connectivitycheck.gstatic.com", endereco) == 1;
}

void captiveDnsServidor() {
  if (wifiModoConfiguracao) {
    dnsServidor.processNextRequest();
  }
}

bool wifiConfigurado() {
  Config config = storageCarregarConfig();
  return !config.wifi_ssid.isEmpty();
}
