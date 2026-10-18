#include "main.h"

#include "alerts.hpp"
#include "button.hpp"
#include "buzzer.hpp"
#include "dashboard_link.hpp"
#include "factory_reset.hpp"
#include "led.hpp"
#include "led_azul.hpp"
#include "led_verde.hpp"
#include "sensors.hpp"
#include "storage.hpp"
#include "wifi.hpp"
#include "webserver.hpp"

DadosSistema dados = {};
bool buzzerSilenciado = false;

namespace {
  WebServer* servidorHttp = nullptr;
}

void controlarLedWifi() {
  if (wifiModoConfiguracao) {
    ledAzulControlarAp();
  } else {
    ledAzulLigar();
  }
}

void monitorarReset() {
  if (factoryResetVerificar()) {
    if (!webserverResetDeFabricaAsync()) {
      factoryResetExecutar();
    }
  }
}

void monitorar(bool forcarLeitura) {
  static unsigned long ultimoTempo = 0;
  static int ultimoIntervalo = 1;

  unsigned long agora = millis();
  if (!forcarLeitura && (agora - ultimoTempo) < (unsigned long)ultimoIntervalo * 1000UL) {
    return;
  }

  ultimoTempo = agora;

  Config config = storageCarregarConfig();
  LeituraSensores leitura = lerSensores();
  Alertas alertas = verificarAlertas(leitura.temperatura, leitura.umidade, leitura.gas_adc, config);
  const bool alertaMudou = registrarTransicoesAlertas(alertas);
  bool existe = existeAlerta(alertas);

  if (buttonFoiPressionado()) {
    if (existe) {
      buzzerSilenciado = true;
      Serial.println("Alarme silenciado.");
    }
  }

  if (!existe) {
    buzzerSilenciado = false;
  }

  if (config.led_alerta) {
    ledControlar(existe);
  } else {
    ledControlar(false);
  }

  if (config.buzzer) {
    buzzerControlar(existe, buzzerSilenciado);
  } else {
    buzzerControlar(false, true);
  }

  dados.temperatura = leitura.temperatura;
  dados.umidade = leitura.umidade;
  dados.gas_adc = leitura.gas_adc;
  dados.alerta_temperatura = alertas.alerta_temperatura;
  dados.alerta_umidade = alertas.alerta_umidade;
  dados.alerta_gas = alertas.alerta_gas;
  dados.buzzer_silenciado = buzzerSilenciado;
  dados.alimentacao_externa = digitalRead(config::POWER_SOURCE_PIN) == HIGH;
  dados.bateria_tensao = leitura.bateria_tensao;
  dados.bateria_percentual = leitura.bateria_percentual;
  dados.bateria_presente = leitura.bateria_presente;
  dados.timestamp = time(nullptr);

  webserverAtualizarDados(dados);
  webserverHistoricoRegistrarAsync(dados);
  webserverDashboardEnviarAsync(dados, config.nome_aparelho, alertaMudou, forcarLeitura);

  int intervalo = config.intervalo_leitura;
  if (intervalo < 1) {
    intervalo = 1;
  }
  ultimoIntervalo = intervalo;
}

void iniciar() {
  Serial.begin(115200);
  delay(500);

  Serial.println();
  Serial.println("==========================================");
  Serial.println("      SISTEMA DE MONITORAMENTO");
  Serial.println("==========================================");
  Serial.println();

  pinMode(config::FACTORY_RESET_PIN, INPUT_PULLUP);
  pinMode(config::POWER_SOURCE_PIN, INPUT);
  ledInit();
  ledVerdeInit();
  ledAzulInit();
  buttonInit();
  buzzerInit();
  sensorsInit();

  ledVerdeLigar();
  storageInicializarHistorico();
  wifiIniciar();
  servidorHttp = webserverIniciar(80);
  webserverConectarWifiConfiguradoAsync();

  // Serial.println();
  // Serial.println("Servidor iniciado.");
  // Serial.print("IP: ");
  // Serial.println(wifiObterIp());
  // Serial.println();
}

void setup() {
  iniciar();
}

void loop() {
  controlarLedWifi();
  if (!webserverOperacaoAtiva()) {
    monitorarReset();
  }
  const bool wifiReconectou = webserverOperacaoAtiva() ? false : dashboardLinkMonitorarConexao();
  monitorar(wifiReconectou);
  webserverServidor(servidorHttp);

  if (wifiModoConfiguracao && !webserverOperacaoAtiva()) {
    captiveDnsServidor();
  }
  webserverVerificarInternetAsync();

  delay(10);
}
