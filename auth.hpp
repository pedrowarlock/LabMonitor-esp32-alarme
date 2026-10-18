#ifndef AUTH_HPP
#define AUTH_HPP

#include <Arduino.h>

bool login(const String& username, const String& password, const String& ip, String& token, String& message);

#endif
