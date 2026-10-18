#include "led_verde.hpp"
#include "config.hpp"

void ledVerdeInit() {
  pinMode(config::LED_VERDE_PIN, OUTPUT);
  digitalWrite(config::LED_VERDE_PIN, LOW);
}

void ledVerdeLigar() {
  digitalWrite(config::LED_VERDE_PIN, HIGH);
}

void ledVerdeDesligar() {
  digitalWrite(config::LED_VERDE_PIN, LOW);
}
