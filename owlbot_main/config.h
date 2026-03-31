#pragma once

// ===========================
// Build Mode (单独测试/整合)
// ===========================
enum BuildMode : uint8_t {
  MODE_FULL = 0,      // 最终整合主循环
  MODE_TEST_I2S = 1,  // 只测音频链路（I2S/MAX98357A）
  MODE_TEST_SERVOS = 2,
  MODE_TEST_SENSORS = 3,
};

static constexpr BuildMode BUILD_MODE = MODE_FULL;


static constexpr bool ENABLE_AUTO_HEAD  = false;
static constexpr bool ENABLE_AUTO_WINGS = false;
static constexpr bool ENABLE_AUTO_SOUND = false; // 先关掉自动叫声
// ===========================
// Pins (按你当前接线改)
// ===========================
static constexpr int PIN_TOUCH  = 27;   // 示例：触摸传感器
static constexpr int PIN_SOUND  = 34;   // 示例：声音传感器（ADC）
static constexpr int PIN_BUTTON = 26;   // 示例：模式按钮（数字输入）

static constexpr int PIN_SERVO_HEAD  = 13;
static constexpr int PIN_SERVO_WING_L = 12;
static constexpr int PIN_SERVO_WING_R = 25;

// I2S for MAX98357A
static constexpr int I2S_BCLK = 14;
static constexpr int I2S_LRC  = 15;
static constexpr int I2S_DOUT = 22;

// ===========================
// Timing / Debounce
// ===========================
static constexpr uint32_t TOUCH_DEBOUNCE_MS = 120;
static constexpr uint32_t BUTTON_DEBOUNCE_MS = 120;

// 声音传感器阈值（示例），你后面要根据实际ADC范围校准
static constexpr int SOUND_THRESHOLD = 1800;
static constexpr uint32_t SOUND_HOLDOFF_MS = 900;

// Cooldown（示例）
static constexpr uint32_t WING_COOLDOWN_MS = 1200;


// ===== Servo calibration =====
static constexpr int SERVO_MIN_US = 500;
static constexpr int SERVO_MAX_US = 2500;

// 头部舵机：中心与摆动幅度
static constexpr int HEAD_CENTER_DEG = 90;
static constexpr int HEAD_LEFT_DEG   = 90;
static constexpr int HEAD_RIGHT_DEG  = 180;

// 翅膀舵机：左右可能镜像，先给一组保守角度
static constexpr int WING_L_UP_DEG    = 60;
static constexpr int WING_L_DOWN_DEG  = 120;
static constexpr int WING_R_UP_DEG    = 120;
static constexpr int WING_R_DOWN_DEG  = 60;

// 动作时长（ms）：与 FSM 的 active_until_ms 对齐（你可按效果改）
static constexpr uint32_t HEAD_MOVE_MS  = 450;
static constexpr uint32_t WING_FLAP_MS  = 350;