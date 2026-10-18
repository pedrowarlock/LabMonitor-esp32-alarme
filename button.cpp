#include "button.hpp"
#include "config.hpp"

namespace {
  volatile bool botaoPressionado = false;
  volatile unsigned long ultimoToque = 0;
}

void IRAM_ATTR onButtonInterrupt() {
  unsigned long agora = millis();
  if ((agora - ultimoToque) > 150UL) {
    botaoPressionado = true;
    ultimoToque = agora;
  }
}

void buttonInit() {
  pinMode(config::BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(config::BUTTON_PIN), onButtonInterrupt, FALLING);
}

bool buttonFoiPressionado() {
  if (botaoPressionado) {
    botaoPressionado = false;
    return true;
  }
  return false;
}
