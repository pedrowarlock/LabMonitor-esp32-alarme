#ifndef ALERTS_HPP
#define ALERTS_HPP

// Use shared types from main.h to avoid duplicate definitions
#include "main.h"

bool verificarTemperatura(float temperatura, const Config& config);
bool verificarUmidade(float umidade, const Config& config);
bool verificarGas(int gas_adc, const Config& config);
Alertas verificarAlertas(float temperatura, float umidade, int gas_adc, const Config& config);
bool existeAlerta(const Alertas& alertas);
bool registrarTransicoesAlertas(const Alertas& alertas);

#endif
