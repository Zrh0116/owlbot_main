#pragma once
#include <Arduino.h>
#include <ESP32Servo.h>
#include "fsm.h"

class Actuators {
public:
  void begin();

  void start_head(uint32_t now_ms);
  void start_wings(uint32_t now_ms);
  void start_sound(uint32_t now_ms);

  void update(uint32_t now_ms);

  ActuatorFSM head;
  ActuatorFSM wings;
  ActuatorFSM sound;

private:
  // =========================
  // Head
  // =========================
  void init_head_servo_();
  void start_head_sequence_(uint32_t now_ms);
  void update_head_sequence_(uint32_t now_ms);
  void apply_head_motion();

  Servo head_servo_;
  bool head_servo_inited_ = false;

  struct HeadStep {
    int angle;
    uint32_t hold_ms;
  };

  static constexpr int HEAD_SEQ_MAX_LEN = 4;
  HeadStep head_seq_[HEAD_SEQ_MAX_LEN];

  int head_seq_len_ = 0;
  int head_step_index_ = 0;
  uint32_t head_step_start_ms_ = 0;
  bool head_sequence_running_ = false;

  // =========================
  // Wings
  // =========================
  void init_wing_servos_();
  void start_wing_sequence_(uint32_t now_ms);
  void update_wing_sequence_(uint32_t now_ms);
  void apply_wing_motion();

  Servo left_wing_servo_;
  Servo right_wing_servo_;
  bool wing_servos_inited_ = false;

  struct WingStep {
    int left_cmd;
    int right_cmd;
    uint32_t hold_ms;
  };

  static constexpr int WING_SEQ_MAX_LEN = 4;
  WingStep wing_seq_[WING_SEQ_MAX_LEN];

  int wing_seq_len_ = 0;
  int wing_step_index_ = 0;
  uint32_t wing_step_start_ms_ = 0;
  bool wing_sequence_running_ = false;

  // =========================
  // Sound
  // =========================
  void apply_sound_play();
  void audio_i2s_init_();
  static void sound_task_entry_(void* arg);
  void sound_task_run_();

  bool i2s_inited_ = false;

  volatile bool sound_done_ = true;
  volatile bool sound_playing_ = false;
  void* sound_task_handle_ = nullptr;
  float safe_gain_ = 1.0f;
};