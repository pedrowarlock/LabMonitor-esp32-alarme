#include "sensors.hpp"
#include "config.hpp"
#include "storage.hpp"

#include <DHT.h>

namespace {
  DHT dhtSensor(config::DHT_PIN, DHT11);

  uint8_t estimarPercentualBateria(float tensao) {
    constexpr float tensoes[] = {3.0f, 3.3f, 3.5f, 3.6f, 3.7f, 3.75f, 3.8f, 3.9f, 4.0f};
    constexpr uint8_t percentuais[] = {0, 5, 10, 20, 40, 50, 60, 75, 100};
    constexpr size_t quantidade = sizeof(tensoes) / sizeof(tensoes[0]);

    if (tensao <= tensoes[0]) return percentuais[0];
    for (size_t indice = 1; indice < quantidade; ++indice) {
      if (tensao <= tensoes[indice]) {
        const float fracao = (tensao - tensoes[indice - 1]) / (tensoes[indice] - tensoes[indice - 1]);
        const float percentual = percentuais[indice - 1] + fracao * (percentuais[indice] - percentuais[indice - 1]);
        return static_cast<uint8_t>(percentual + 0.5f);
      }
    }
    return 100;
  }
}

LeituraSensores lerSensores() {
  static bool temperaturaInvalida = false;
  static bool umidadeInvalida = false;
  LeituraSensores leitura;
  leitura.temperatura = dhtSensor.readTemperature();
  leitura.umidade = dhtSensor.readHumidity();
  long somaGas = 0;
  for (int amostra = 0; amostra < 8; ++amostra) {
    somaGas += analogRead(config::GAS_PIN);
  }
  leitura.gas_adc = static_cast<int>(somaGas / 8L);
  const int gasMv = analogReadMilliVolts(config::GAS_PIN);
  Serial.printf("MQ-2 GPIO %u: ADC=%d, tensao=%d mV\n", config::GAS_PIN, leitura.gas_adc, gasMv);

  long somaBateriaMv = 0;
  for (int amostra = 0; amostra < 8; ++amostra) {
    somaBateriaMv += analogReadMilliVolts(config::BATTERY_VOLTAGE_PIN);
  }
  const float tensaoNoPino = static_cast<float>(somaBateriaMv) / 8000.0f;
  leitura.bateria_tensao = tensaoNoPino * config::BATTERY_DIVIDER_MULTIPLIER;
  leitura.bateria_presente = leitura.bateria_tensao >= 2.5f;
  leitura.bateria_percentual = leitura.bateria_presente ? estimarPercentualBateria(leitura.bateria_tensao) : 0;

  if (isnan(leitura.temperatura)) {
    if (!temperaturaInvalida) storageRegistrarEvento("FALHA DE SISTEMA: leitura de temperatura invalida");
    temperaturaInvalida = true;
    leitura.temperatura = 0.0f;
  } else {
    temperaturaInvalida = false;
  }

  if (isnan(leitura.umidade)) {
    if (!umidadeInvalida) storageRegistrarEvento("FALHA DE SISTEMA: leitura de umidade invalida");
    umidadeInvalida = true;
    leitura.umidade = 0.0f;
  } else {
    umidadeInvalida = false;
  }

  return leitura;
}

void sensorsInit() {
  dhtSensor.begin();
  pinMode(config::GAS_PIN, INPUT);
  pinMode(config::BATTERY_VOLTAGE_PIN, INPUT);
  analogReadResolution(12);
  analogSetPinAttenuation(config::GAS_PIN, ADC_11db);
  analogSetPinAttenuation(config::BATTERY_VOLTAGE_PIN, ADC_11db);
  Serial.printf("Entrada analogica do MQ-2 iniciada no GPIO %u\n", config::GAS_PIN);
}
