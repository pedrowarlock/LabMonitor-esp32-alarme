#ifndef BUZZER_HPP
#define BUZZER_HPP

#include <Arduino.h>

void buzzerInit();
void buzzerLigar();
void buzzerDesligar();
void buzzerControlar(bool alerta, bool silenciado);

#endif
