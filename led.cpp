#include "led.hpp"
#include "config.hpp"

namespace {
  bool estado = false;
  unsigned long ultimoPiscar = 0;
}

void ledInit() {
  pinMode(config::LED_PIN, OUTPUT);
  digitalWrite(config::LED_PIN, LOW);
}

void ledControlar(bool alerta) {
  unsigned long agora = millis();

  if (!alerta) {
    estado = false;
    digitalWrite(config::LED_PIN, LOW);
    return;
  }

  if ((agora - ultimoPiscar) >= 500UL) {
    estado = !estado;
    digitalWrite(config::LED_PIN, estado ? HIGH : LOW);
    ultimoPiscar = agora;
  }
}
