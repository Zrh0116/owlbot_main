#include "scheduler.h"
#include <esp_system.h> // for esp_random()

void Scheduler::begin(uint32_t now_ms) {
  randomSeed((uint32_t)esp_random());
  t_.next_head_ms  = now_ms + rand_interval_ms(800, 2500);
  t_.next_wings_ms = now_ms + rand_interval_ms(1200, 4000);
  t_.next_sound_ms = now_ms + rand_interval_ms(1500, 4500);
}

uint32_t Scheduler::rand_interval_ms(uint32_t min_ms, uint32_t max_ms) {
  if (max_ms <= min_ms) return min_ms;
  return (uint32_t)random((long)min_ms, (long)max_ms + 1);
}

Event Scheduler::poll(uint32_t now_ms) {
  if (now_ms >= t_.next_head_ms)  return {EVT_AUTO_HEAD,  now_ms};
  if (now_ms >= t_.next_wings_ms) return {EVT_AUTO_WINGS, now_ms};
  if (now_ms >= t_.next_sound_ms) return {EVT_AUTO_SOUND, now_ms};
  return {EVT_NONE, now_ms};
}

void Scheduler::reschedule_head(uint32_t now_ms) {
  t_.next_head_ms = now_ms + rand_interval_ms(800, 2500);
}

void Scheduler::reschedule_wings(uint32_t now_ms) {
  t_.next_wings_ms = now_ms + rand_interval_ms(1200, 4000);
}

void Scheduler::reschedule_sound(uint32_t now_ms) {
  t_.next_sound_ms = now_ms + rand_interval_ms(1500, 4500);
}
