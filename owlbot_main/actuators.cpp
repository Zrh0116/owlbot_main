#include "actuators.h"
#include "config.h"
#include "debug.h"

#include <driver/i2s.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "bird_pcm.h"

extern const int16_t bird_pcm[];
static constexpr int I2S_SAMPLE_RATE = 16000;
static const size_t BIRD_PCM_LEN = sizeof(bird_pcm) / sizeof(bird_pcm[0]);

static float find_peak_amplitude_() {
  int16_t peak = 0;
  for (size_t i = 0; i < BIRD_PCM_LEN; i++) {
    int16_t v = bird_pcm[i];
    int16_t a = (v < 0) ? (int16_t)(-v) : v;
    if (a > peak) peak = a;
  }
  return (float)peak / 32768.0f;
}

static inline int16_t linear_amplify_(int16_t sample, float gain) {
  float amplified = (float)sample * gain;
  if (amplified > 32767.0f) amplified = 32767.0f;
  if (amplified < -32768.0f) amplified = -32768.0f;
  return (int16_t)amplified;
}

// =====================================================
// I2S
// =====================================================
void Actuators::audio_i2s_init_() {
  if (i2s_inited_) return;

  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = I2S_SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S_MSB,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 16,
    .dma_buf_len = 256,
    .use_apll = false,
    .tx_desc_auto_clear = true,
    .fixed_mclk = 0
  };

  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_BCLK,
    .ws_io_num = I2S_LRC,
    .data_out_num = I2S_DOUT,
    .data_in_num = I2S_PIN_NO_CHANGE
  };

  i2s_driver_uninstall(I2S_NUM_0);
  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, nullptr);
  i2s_set_pin(I2S_NUM_0, &pin_config);
  i2s_zero_dma_buffer(I2S_NUM_0);

  float peak = find_peak_amplitude_();
  safe_gain_ = (peak < 1e-6f) ? 1.0f : (0.99f / peak);

  dbg(String("[I2S] init ok, safe_gain=") + String(safe_gain_, 2));
  i2s_inited_ = true;
}

// =====================================================
// Head servo
// =====================================================
void Actuators::init_head_servo_() {
  if (head_servo_inited_) return;

  head_servo_.setPeriodHertz(50);
  head_servo_.attach(PIN_SERVO_HEAD, SERVO_MIN_US, SERVO_MAX_US);
  head_servo_.write(90);

  head_servo_inited_ = true;
  dbg(F("[ACT] head servo init ok"));
}

// =====================================================
// Wing servos
// 保持你现成文件里的引脚：
// left  = 12
// right = 25
// =====================================================
void Actuators::init_wing_servos_() {
  if (wing_servos_inited_) return;

  left_wing_servo_.setPeriodHertz(50);
  right_wing_servo_.setPeriodHertz(50);

  left_wing_servo_.attach(12, SERVO_MIN_US, SERVO_MAX_US);
  right_wing_servo_.attach(25, SERVO_MIN_US, SERVO_MAX_US);

  left_wing_servo_.write(90);
  right_wing_servo_.write(90);

  wing_servos_inited_ = true;
  dbg(F("[ACT] wing servos init ok"));
}

// =====================================================
// Lifecycle
// =====================================================
void Actuators::begin() {
  randomSeed(micros());
  init_head_servo_();
  init_wing_servos_();
  audio_i2s_init_();
}

void Actuators::start_head(uint32_t now_ms) {
  if (head.state == ACTIVE) {
    dbg(F("[ACT] head already active, ignore"));
    return;
  }

  head.state = ACTIVE;
  start_head_sequence_(now_ms);
  apply_head_motion();
}

void Actuators::start_wings(uint32_t now_ms) {
  if (wings.state == ACTIVE || wings.state == COOLDOWN) {
    dbg(F("[ACT] wings busy, ignore"));
    return;
  }

  wings.state = ACTIVE;
  start_wing_sequence_(now_ms);
  apply_wing_motion();
}

void Actuators::start_sound(uint32_t now_ms) {
  sound.state = ACTIVE;
  sound.active_until_ms = now_ms + 900;
  apply_sound_play();
}

void Actuators::update(uint32_t now_ms) {
  if (head.state == ACTIVE) {
    update_head_sequence_(now_ms);
  }

  if (wings.state == ACTIVE) {
    update_wing_sequence_(now_ms);
  }
  if (wings.state == COOLDOWN && now_ms >= wings.cooldown_until_ms) {
    wings.state = IDLE;
    dbg(F("[ACT] wings cooldown done"));
  }

  if (sound.state == ACTIVE && sound_playing_ && sound_done_) {
    sound_playing_ = false;
    sound.state = IDLE;
  }
}

// =====================================================
// Head motion sequence
// 50% 两组动作：
// A: 左 -> 右 -> 左 -> 停
// B: 右 -> 左 -> 右 -> 停
// =====================================================
void Actuators::start_head_sequence_(uint32_t now_ms) {
  static constexpr int HEAD_LEFT_CMD  = 0;
  static constexpr int HEAD_STOP_CMD  = 90;
  static constexpr int HEAD_RIGHT_CMD = 180;

  bool group_a = (random(0, 2) == 0);

  if (group_a) {
    head_seq_[0] = {HEAD_LEFT_CMD,  300};
    head_seq_[1] = {HEAD_RIGHT_CMD, 500};
    head_seq_[2] = {HEAD_LEFT_CMD,  300};
    head_seq_[3] = {HEAD_STOP_CMD,  150};
    dbg(F("[ACT] head group A -> LEFT, RIGHT, LEFT, STOP"));
  } else {
    head_seq_[0] = {HEAD_RIGHT_CMD, 300};
    head_seq_[1] = {HEAD_LEFT_CMD,  500};
    head_seq_[2] = {HEAD_RIGHT_CMD, 300};
    head_seq_[3] = {HEAD_STOP_CMD,  150};
    dbg(F("[ACT] head group B -> RIGHT, LEFT, RIGHT, STOP"));
  }

  head_seq_len_ = 4;
  head_step_index_ = 0;
  head_step_start_ms_ = now_ms;
  head_sequence_running_ = true;

  head_servo_.write(head_seq_[0].angle);
  dbg(String("[ACT] head seq start, angle=") + head_seq_[0].angle);
}

void Actuators::update_head_sequence_(uint32_t now_ms) {
  if (!head_sequence_running_) return;

  if (now_ms - head_step_start_ms_ >= head_seq_[head_step_index_].hold_ms) {
    head_step_index_++;

    if (head_step_index_ >= head_seq_len_) {
      head_sequence_running_ = false;
      head_servo_.write(90);
      head.state = IDLE;
      dbg(F("[ACT] head seq done"));
      return;
    }

    head_step_start_ms_ = now_ms;
    head_servo_.write(head_seq_[head_step_index_].angle);

    dbg(String("[ACT] head seq step=") + head_step_index_ +
        " angle=" + head_seq_[head_step_index_].angle);
  }
}

void Actuators::apply_head_motion() {
  dbg(F("[ACT] head motion triggered"));
}

// =====================================================
// Wing motion sequence
// 左翅保持原命令
// 右翅反向命令
//
// 左翅: CW -> STOP -> CCW -> STOP
// 右翅: CCW -> STOP -> CW -> STOP
// =====================================================
void Actuators::start_wing_sequence_(uint32_t now_ms) {
  static constexpr int STOP_SPEED = 90;
  static constexpr int CW_SPEED   = 0;
  static constexpr int CCW_SPEED  = 180;

  wing_seq_[0] = {CW_SPEED,   CCW_SPEED,  200};
  wing_seq_[1] = {STOP_SPEED, STOP_SPEED, 1000};
  wing_seq_[2] = {CCW_SPEED,  CW_SPEED,   200};
  wing_seq_[3] = {STOP_SPEED, STOP_SPEED, 1000};

  wing_seq_len_ = 4;
  wing_step_index_ = 0;
  wing_step_start_ms_ = now_ms;
  wing_sequence_running_ = true;

  left_wing_servo_.write(wing_seq_[0].left_cmd);
  right_wing_servo_.write(wing_seq_[0].right_cmd);

  dbg(String("[ACT] wing seq start, L=") + wing_seq_[0].left_cmd +
      " R=" + wing_seq_[0].right_cmd);
}

void Actuators::update_wing_sequence_(uint32_t now_ms) {
  if (!wing_sequence_running_) return;

  if (now_ms - wing_step_start_ms_ >= wing_seq_[wing_step_index_].hold_ms) {
    wing_step_index_++;

    if (wing_step_index_ >= wing_seq_len_) {
      wing_sequence_running_ = false;

      left_wing_servo_.write(90);
      right_wing_servo_.write(90);

      wings.state = COOLDOWN;
      wings.cooldown_until_ms = now_ms + WING_COOLDOWN_MS;

      dbg(F("[ACT] wing seq done -> cooldown"));
      return;
    }

    wing_step_start_ms_ = now_ms;
    left_wing_servo_.write(wing_seq_[wing_step_index_].left_cmd);
    right_wing_servo_.write(wing_seq_[wing_step_index_].right_cmd);

    dbg(String("[ACT] wing seq step=") + wing_step_index_ +
        " L=" + wing_seq_[wing_step_index_].left_cmd +
        " R=" + wing_seq_[wing_step_index_].right_cmd);
  }
}

void Actuators::apply_wing_motion() {
  dbg(F("[ACT] wings motion triggered"));
}

// =====================================================
// Sound
// =====================================================
void Actuators::apply_sound_play() {
  audio_i2s_init_();

  if (sound_playing_ && !sound_done_) {
    dbg(F("[ACT] sound already playing, ignore"));
    return;
  }

  sound_done_ = false;
  sound_playing_ = true;

  TaskHandle_t handle = nullptr;
  BaseType_t ok = xTaskCreate(
    &Actuators::sound_task_entry_,
    "owl_sound",
    4096,
    this,
    2,
    &handle
  );

  if (ok != pdPASS) {
    dbg(F("[ACT] sound task create failed, fallback to immediate done"));
    sound_done_ = true;
    sound_playing_ = false;
    sound.state = IDLE;
    return;
  }

  sound_task_handle_ = (void*)handle;
}

void Actuators::sound_task_entry_(void* arg) {
  Actuators* self = (Actuators*)arg;
  self->sound_task_run_();
  vTaskDelete(nullptr);
}

void Actuators::sound_task_run_() {
  i2s_zero_dma_buffer(I2S_NUM_0);

  uint8_t buffer[2048];
  size_t bytes_written = 0;

  size_t offset = 0;
  while (offset < BIRD_PCM_LEN) {
    size_t cnt = std::min((size_t)(sizeof(buffer) / 2),
                          (size_t)(BIRD_PCM_LEN - offset));

    for (size_t i = 0; i < cnt; i++) {
      int16_t s = linear_amplify_(bird_pcm[offset + i], safe_gain_);
      buffer[i * 2]     = (uint8_t)(s & 0xFF);
      buffer[i * 2 + 1] = (uint8_t)((s >> 8) & 0xFF);
    }

    i2s_write(I2S_NUM_0, buffer, cnt * 2, &bytes_written, pdMS_TO_TICKS(500));
    offset += cnt;
  }

  vTaskDelay(pdMS_TO_TICKS(80));
  sound_done_ = true;
}