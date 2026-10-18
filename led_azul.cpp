#include "led_azul.hpp"
#include "config.hpp"

namespace {
  bool estado = false;
  uint8_t etapa = 0;
  unsigned long ultimoTempo = 0;
}

void ledAzulInit() {
  pinMode(config::LED_AZUL_PIN, OUTPUT);
  digitalWrite(config::LED_AZUL_PIN, LOW);
}

void ledAzulLigar() {
  digitalWrite(config::LED_AZUL_PIN, HIGH);
  estado = true;
}

void ledAzulDesligar() {
  digitalWrite(config::LED_AZUL_PIN, LOW);
  estado = false;
}

void ledAzulInverter() {
  estado = !estado;
  digitalWrite(config::LED_AZUL_PIN, estado ? HIGH : LOW);
}

void ledAzulControlarAp() {
  unsigned long agora = millis();

  if (etapa < 6) {
    if ((agora - ultimoTempo) >= 150UL) {
      ledAzulInverter();
      ultimoTempo = agora;
      etapa++;
    }
  } else if (etapa == 6) {
    if ((agora - ultimoTempo) >= 500UL) {
      ledAzulLigar();
      ultimoTempo = agora;
      etapa = 7;
    }
  } else if (etapa == 7) {
    if ((agora - ultimoTempo) >= 1000UL) {
      ledAzulDesligar();
      ultimoTempo = agora;
      etapa = 0;
    }
  }
}

void ledAzulResetar() {
  estado = false;
  etapa = 0;
  ultimoTempo = millis();
  ledAzulDesligar();
}
