#pragma once
#include <Arduino.h>

#ifndef DBG_BAUD
#define DBG_BAUD 115200
#endif

inline void dbg_begin() {
  Serial.begin(DBG_BAUD);
  delay(200);
}

inline void dbg(const __FlashStringHelper* msg) {
  Serial.println(msg);
}

inline void dbg(const String& msg) {
  Serial.println(msg);
}