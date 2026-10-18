#include "alerts.hpp"

#include <time.h>
#include "storage.hpp"

bool verificarTemperatura(float temperatura, const Config& config) {
  if (!config.alarme_temperatura) {
    return false;
  }

  return (temperatura < config.temperatura_minima ||
          temperatura > config.temperatura_maxima);
}

bool verificarUmidade(float umidade, const Config& config) {
  if (!config.alarme_umidade) {
    return false;
  }

  return (umidade < config.umidade_minima ||
          umidade > config.umidade_maxima);
}

bool verificarGas(int gas_adc, const Config& config) {
  if (!config.alarme_gas) {
    return false;
  }

  return (gas_adc >= config.gas_adc_critico);
}

Alertas verificarAlertas(float temperatura, float umidade, int gas_adc, const Config& config) {
  Alertas alertas;
  alertas.alerta_temperatura = verificarTemperatura(temperatura, config);
  alertas.alerta_umidade = verificarUmidade(umidade, config);
  alertas.alerta_gas = verificarGas(gas_adc, config);
  return alertas;
}

bool existeAlerta(const Alertas& alertas) {
  return (alertas.alerta_temperatura ||
          alertas.alerta_umidade ||
          alertas.alerta_gas);
}

namespace {
  Alertas alertasAnteriores = {};

  void registrarMudanca(bool atual, bool anterior, const char* nome) {
    if (atual == anterior) return;

    RegistroAlerta registro;
    registro.timestamp = time(nullptr);
    registro.evento = String(atual ? "ENTRADA EM CRITICO: " : "RETORNO AO NORMAL: ") + nome;
    storageAdicionarLog(registro);
  }
}

bool registrarTransicoesAlertas(const Alertas& alertas) {
  const bool houveMudanca = alertas.alerta_temperatura != alertasAnteriores.alerta_temperatura
    || alertas.alerta_umidade != alertasAnteriores.alerta_umidade
    || alertas.alerta_gas != alertasAnteriores.alerta_gas;
  registrarMudanca(alertas.alerta_temperatura, alertasAnteriores.alerta_temperatura, "temperatura");
  registrarMudanca(alertas.alerta_umidade, alertasAnteriores.alerta_umidade, "umidade");
  registrarMudanca(alertas.alerta_gas, alertasAnteriores.alerta_gas, "gas");
  alertasAnteriores = alertas;
  return houveMudanca;
}
