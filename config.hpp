#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <Arduino.h>

namespace config {
  constexpr uint8_t DHT_PIN = 27;
  constexpr uint8_t GAS_PIN = 34;
  constexpr uint8_t BUZZER_PIN = 15;
  constexpr uint8_t BUTTON_PIN = 21;
  constexpr uint8_t FACTORY_RESET_PIN = 23;

  constexpr uint8_t LED_PIN = 17;
  constexpr uint8_t LED_VERDE_PIN = 5;
  constexpr uint8_t LED_AZUL_PIN = 18;
  constexpr uint8_t POWER_SOURCE_PIN = 35;
  constexpr uint8_t BATTERY_VOLTAGE_PIN = 32;
  constexpr float BATTERY_DIVIDER_MULTIPLIER = 28.0f / 18.0f;
  
  constexpr float TEMPERATURA_MINIMA = 0.0f;
  constexpr float TEMPERATURA_MAXIMA = 35.0f;

  constexpr float UMIDADE_MINIMA = 0.0f;
  constexpr float UMIDADE_MAXIMA = 100.0f;

  constexpr int GAS_ADC_CRITICO = 3800;
  constexpr int GAS_CRITICO = 2000;
  constexpr int GAS_ADC_MAX = 4095;

  constexpr const char* SSID = "";
  constexpr const char* PASSWORD = "";
  constexpr const char* WIFI_SECURITY = "OPEN";

  constexpr uint16_t SERVER_PORT = 80;
  constexpr const char* AP_SSID = "MONITOR-CONFIGURAR";
  constexpr const char* AP_PASSWORD = ""; 
  constexpr int INTERVALO_LEITURA = 1;
}

#endif
