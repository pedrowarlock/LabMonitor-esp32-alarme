#ifndef MAIN_H
#define MAIN_H

#include <Arduino.h>
#include <WebServer.h>
#include "config.hpp"

struct Config {
  bool alarme_temperatura;
  float temperatura_minima;
  float temperatura_maxima;

  bool alarme_umidade;
  float umidade_minima;
  float umidade_maxima;

  bool alarme_gas;
  int gas_adc_critico;

  bool led_alerta;
  bool buzzer;
  int intervalo_leitura;
  String wifi_ssid;
  String wifi_password;
  String wifi_security;
  String wifi_enterprise_identity;
  String wifi_enterprise_username;
  String wifi_enterprise_password;
  String wifi_enterprise_method;
  String wifi_enterprise_ttls_phase2;
  String nome_aparelho;
  bool permitir_visitante;
  bool usuario_pode_ver_temperatura;
  bool usuario_pode_ver_umidade;
  bool usuario_pode_ver_gas;
  bool usuario_pode_ver_wifi;
  bool usuario_pode_ver_rede;
  bool usuario_pode_ver_logs;
  bool usuario_pode_silenciar_alarme;
  bool visitante_pode_ver_temperatura;
  bool visitante_pode_ver_umidade;
  bool visitante_pode_ver_gas;
  bool visitante_pode_ver_wifi;
  bool visitante_pode_ver_rede;
  bool visitante_pode_ver_logs;
  bool visitante_pode_silenciar_alarme;
  bool sinc_hora_automatica;
  String data_hora_manual;
};

struct DadosSistema {
  float temperatura;
  float umidade;
  int gas_adc;
  bool alerta_temperatura;
  bool alerta_umidade;
  bool alerta_gas;
  bool buzzer_silenciado;
  bool alimentacao_externa;
  float bateria_tensao;
  uint8_t bateria_percentual;
  bool bateria_presente;
  time_t timestamp;
};

struct LeituraSensores {
  float temperatura;
  float umidade;
  int gas_adc;
  float bateria_tensao;
  uint8_t bateria_percentual;
  bool bateria_presente;
};

struct Alertas {
  bool alerta_temperatura;
  bool alerta_umidade;
  bool alerta_gas;
};

extern DadosSistema dados;
extern bool buzzerSilenciado;
extern volatile bool wifiModoConfiguracao;

void controlarLedWifi();
void monitorarReset();
void monitorar(bool forcarLeitura = false);
void iniciar();

// Declarações dos módulos que serão implementados nos outros arquivos do projeto
Config storageCarregarConfig();
LeituraSensores lerSensores();
Alertas verificarAlertas(float temperatura, float umidade, int gas_adc, const Config& config);
bool existeAlerta(const Alertas& alertas);
bool buttonFoiPressionado();
void buzzerControlar(bool ativo, bool silenciado);
void ledControlar(bool ativo);
void ledVerdeLigar();
void ledAzulControlarAp();
void ledAzulLigar();
bool factoryResetVerificar();
void wifiIniciar();
String wifiObterIp();
String wifiObterIpAp();
void captiveDnsServidor();
WebServer* webserverIniciar(int porta);
void webserverServidor(WebServer* server);

#endif
