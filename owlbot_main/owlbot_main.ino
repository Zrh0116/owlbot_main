#include "config.h"
#include "debug.h"
#include "events.h"
#include "scheduler.h"
#include "sensors.h"
#include "actuators.h"

// 全局模块
Scheduler scheduler;
Sensors   sensors;
Actuators actuators;

// 一个简单的“系统静默”计时：用于你后面扩展 inactivity-conditioned cooldown
static uint32_t last_accepted_event_ms = 0;

// ============= Arbitration：按你写的思想做 =============
static void handle_event(const Event& e, uint32_t now_ms) {
  if (e.type == EVT_NONE) return;

  // 方便把成员函数当函数用
  auto start_head  = [&](uint32_t t){ actuators.start_head(t); };
  auto start_wings = [&](uint32_t t){ actuators.start_wings(t); };
  auto start_sound = [&](uint32_t t){ actuators.start_sound(t); };

  switch (e.type) {
    case EVT_TOUCH: {
      // 拍头：鸣叫 + 转头（分别仲裁，互不打架）
      if (actuators.sound.state == IDLE) start_sound(now_ms);
      else actuators.sound.pending = true;

      if (actuators.head.state == IDLE) start_head(now_ms);
      else actuators.head.pending = true;

      last_accepted_event_ms = now_ms; // 外界交互算“有效交互”
    } break;

    case EVT_SOUND: {
      // 环境声音：扇翅（建议有 cooldown，所以 pending 可以选择不用/或只留一次）
      if (actuators.wings.state == IDLE) {
        start_wings(now_ms);
        last_accepted_event_ms = now_ms;
      }
      // 忙就 drop（你也可以改为 pending=true）
    } break;

    case EVT_BUTTON: {
      dbg(F("[BTN] button pressed (stub: change mode)"));
      last_accepted_event_ms = now_ms;
    } break;

    case EVT_AUTO_HEAD: {
      if (!ENABLE_AUTO_HEAD) { 
        scheduler.reschedule_head(now_ms); 
        break; 
      }
      if (actuators.head.state == IDLE) {
        start_head(now_ms);
        scheduler.reschedule_head(now_ms);
      } else {
        scheduler.reschedule_head(now_ms); // drop 但也要把下一次时间推后
      }
    } break;

    case EVT_AUTO_WINGS: {
      if (!ENABLE_AUTO_WINGS) { 
        scheduler.reschedule_wings(now_ms); 
        break; 
      }
      if (actuators.wings.state == IDLE) {
        start_wings(now_ms);
        scheduler.reschedule_wings(now_ms);
      } else {
        scheduler.reschedule_wings(now_ms);
      }
    } break;

    case EVT_AUTO_SOUND: {
      if (!ENABLE_AUTO_SOUND) { 
        scheduler.reschedule_sound(now_ms); 
        break; 
      }
      if (actuators.sound.state == IDLE) {
        start_sound(now_ms);
      }
      scheduler.reschedule_sound(now_ms);
    } break;

    default: break;
  }
}

// ============= Pending 执行 =============
static void service_pending(uint32_t now_ms) {
  if (actuators.head.pending && actuators.head.state == IDLE) {
    actuators.head.pending = false;
    actuators.start_head(now_ms);
    last_accepted_event_ms = now_ms;
  }
  if (actuators.sound.pending && actuators.sound.state == IDLE) {
    actuators.sound.pending = false;
    actuators.start_sound(now_ms);
    last_accepted_event_ms = now_ms;
  }
  // wings pending 你可以先不做（避免噪声堆积），后续你要也行
}

void setup() {
  dbg_begin();
  dbg(F("OwlBot_Main boot"));

  sensors.begin();
  actuators.begin();

  uint32_t now_ms = millis();
  scheduler.begin(now_ms);
  last_accepted_event_ms = now_ms;
}

void loop() {
  uint32_t now_ms = millis();

  // ========== 分模式测试 ==========
  if (BUILD_MODE == MODE_TEST_SENSORS) {
    Event e = sensors.poll(now_ms);
    if (e.type != EVT_NONE) {
      Serial.printf("[SENS] event=%d at %lu\n", (int)e.type, (unsigned long)e.ts_ms);
    }
    delay(10);
    return;
  }

  if (BUILD_MODE == MODE_TEST_SERVOS) {
    // stub：循环触发动作，看 FSM 是否稳定
    static uint32_t t0 = 0;
    if (now_ms - t0 > 1500) {
      t0 = now_ms;
      actuators.start_head(now_ms);
      actuators.start_wings(now_ms);
    }
    actuators.update(now_ms);
    delay(5);
    return;
  }

  if (BUILD_MODE == MODE_TEST_I2S) {
    // ✅ 真 I2S 测试：每 3 秒触发一次 start_sound（不依赖传感器/仲裁）
    static uint32_t t0 = 0;
    if (now_ms - t0 > 10000) {
      t0 = now_ms;
      Serial.println("[I2S] start_sound()");
      actuators.start_sound(now_ms);
    }
    actuators.update(now_ms);
    delay(5);
    return;
  }

  // ========== MODE_FULL：整合主循环 ==========
  // 1) 采样外界事件
  Event e1 = sensors.poll(now_ms);
  handle_event(e1, now_ms);

  // 2) 生成内部随机事件（一次 poll 只吐一个，你也可以 while 吐完）
  Event e2 = scheduler.poll(now_ms);
  handle_event(e2, now_ms);

  // 3) 推进执行器状态机
  actuators.update(now_ms);

  // 4) 处理 pending（合并后的外界交互）
  service_pending(now_ms);

  delay(5); // 很小的节拍，后面也可以改成完全非阻塞
}




