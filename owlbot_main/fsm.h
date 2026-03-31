#pragma once
#include <Arduino.h>

enum ActState : uint8_t { IDLE=0, ACTIVE=1, COOLDOWN=2 };

struct ActuatorFSM {
  ActState state = IDLE;
  uint32_t active_until_ms = 0;     // ACTIVE 结束时间
  uint32_t cooldown_until_ms = 0;   // COOLDOWN 结束时间
  bool pending = false;            // 合并 pending（只记一次）
};
