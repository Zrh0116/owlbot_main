#include "sensors.h"
#include "config.h"

void Sensors::begin() {
  // 数字模式：你的压力/触摸模块 D 口接到 PIN_TOUCH
  // 如果你发现电平不稳，可以把 INPUT 改成 INPUT_PULLUP / INPUT_PULLDOWN（看你的模块输出）
  pinMode(PIN_TOUCH, INPUT);

  // 按钮：常见接法按下为 LOW
  pinMode(PIN_BUTTON, INPUT_PULLUP);

  // PIN_SOUND 是 ADC 输入，一般不需要 pinMode
}

Event Sensors::poll(uint32_t now_ms) {
  // =========================
  // Touch / Pressure (DIGITAL)
  // =========================
  // 默认假设：按下/触发时为 HIGH
  // 如果你实际测出来“按下变 LOW”，把下面这一行改成：
  // bool touch_level = (digitalRead(PIN_TOUCH) == LOW);
  bool touch_level = (digitalRead(PIN_TOUCH) == HIGH);

  // 边沿触发：只在 “上一轮没按 && 这一轮按下” 的瞬间触发一次
  if (touch_level && !last_touch_level_ && (now_ms - last_touch_ms_) >= TOUCH_DEBOUNCE_MS) {
    last_touch_ms_ = now_ms;
    last_touch_level_ = touch_level;
    return {EVT_TOUCH, now_ms};
  }
  last_touch_level_ = touch_level;

  // =========================
  // Button (按下为 LOW)
  // =========================
  int btn = digitalRead(PIN_BUTTON);
  if (btn == LOW && (now_ms - last_button_ms_) >= BUTTON_DEBOUNCE_MS) {
    last_button_ms_ = now_ms;
    return {EVT_BUTTON, now_ms};
  }

  // =========================
  // Sound (ADC阈值触发 + holdoff)
  // =========================
  int s = analogRead(PIN_SOUND);
  if (s >= SOUND_THRESHOLD && (now_ms - last_sound_ms_) >= SOUND_HOLDOFF_MS) {
    last_sound_ms_ = now_ms;
    return {EVT_SOUND, now_ms};
  }

  return {EVT_NONE, now_ms};
}