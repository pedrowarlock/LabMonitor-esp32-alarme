#include "buzzer.hpp"
#include "config.hpp"

namespace {
  bool estado = false;
  bool ultimoAlerta = false;
  unsigned long ultimoTempo = 0;
}

void buzzerInit() {
  pinMode(config::BUZZER_PIN, OUTPUT);
  digitalWrite(config::BUZZER_PIN, LOW);
}

void buzzerLigar() {
  tone(config::BUZZER_PIN, 2500);
}

void buzzerDesligar() {
  noTone(config::BUZZER_PIN);
}

void buzzerControlar(bool alerta, bool silenciado) {
  if (!alerta) {
    estado = false;
    ultimoAlerta = false;
    buzzerDesligar();
    return;
  }

  if (silenciado) {
    estado = false;
    buzzerDesligar();
    return;
  }

  if (!ultimoAlerta) {
    estado = true;
    buzzerLigar();
    ultimoTempo = millis();
    ultimoAlerta = true;
    return;
  }

  unsigned long agora = millis();
  if ((agora - ultimoTempo) >= 250UL) {
    estado = !estado;
    if (estado) {
      buzzerLigar();
    } else {
      buzzerDesligar();
    }
    ultimoTempo = agora;
  }
}
