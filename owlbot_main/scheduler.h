#pragma once
#include <Arduino.h>
#include "events.h"

struct AutoTimers {
  uint32_t next_head_ms  = 0;
  uint32_t next_wings_ms = 0;
  uint32_t next_sound_ms = 0;
};

class Scheduler {
public:
  void begin(uint32_t now_ms);
  // 如果到了时间就生成一个 auto event，否则 EVT_NONE
  Event poll(uint32_t now_ms);

  // 某个动作执行完，重新抽样下一次触发时间
  void reschedule_head(uint32_t now_ms);
  void reschedule_wings(uint32_t now_ms);
  void reschedule_sound(uint32_t now_ms);

private:
  AutoTimers t_;
  uint32_t rand_interval_ms(uint32_t min_ms, uint32_t max_ms);
};
