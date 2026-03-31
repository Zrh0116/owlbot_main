#pragma once
#include <Arduino.h>

enum EventType : uint8_t {
  EVT_NONE = 0,
  EVT_TOUCH,
  EVT_SOUND,
  EVT_BUTTON,
  EVT_AUTO_HEAD,
  EVT_AUTO_WINGS,
  EVT_AUTO_SOUND
};

struct Event {
  EventType type;
  uint32_t  ts_ms;

  // 关键：加这个构造函数，保证 return {xxx, now_ms} 一定能用
  Event(EventType t = EVT_NONE, uint32_t ts = 0) : type(t), ts_ms(ts) {}
};
