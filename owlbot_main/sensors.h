#pragma once
#include <Arduino.h>
#include "events.h"

class Sensors {
public:
  void begin();
  // 每次 loop 调用一次，若产生事件则返回，否则 EVT_NONE
  Event poll(uint32_t now_ms);

private:
  uint32_t last_touch_ms_  = 0;
  uint32_t last_button_ms_ = 0;
  uint32_t last_sound_ms_  = 0;

  // ====== 关键：数字触摸/压力 D 口边沿检测 ======
  // 记录上一轮的触摸电平（用于“从未按->按下”的瞬间触发一次）
  bool last_touch_level_ = false;
};