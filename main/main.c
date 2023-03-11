#include <stdbool.h>
#include <stdio.h>

#include "driver/timer.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#define TIMER_DIVIDER (16)  //  Hardware timer clock divider
#define TIMER_SCALE \
  (TIMER_BASE_CLK / TIMER_DIVIDER)  // convert counter value to seconds
typedef struct {
  unsigned char y;
  unsigned char x;
} point_t;

const static char* TAG = "HW-timer";

static xQueueHandle timer_queue;

bool callback(void* args) {
  // Here can not put any blocking API include printf and ESP_LOG* function
  BaseType_t high_task_awoken = pdFALSE;
  point_t* p = args;
  p->x = 2;
  // check x of piont is changed, here send x to main program task
  xQueueSendFromISR(timer_queue, args, &high_task_awoken);
  return high_task_awoken == pdTRUE;
}

void create_timer(int group, int timer, bool auto_reload,
                  int timer_interval_sec, void* args) {
  timer_config_t config = {
      .divider = TIMER_DIVIDER,
      .counter_dir = TIMER_COUNT_UP,
      .counter_en = TIMER_PAUSE,
      .alarm_en = TIMER_ALARM_EN,
      .auto_reload = auto_reload,
  };
  timer_init(group, timer, &config);
  timer_set_counter_value(group, timer, 0);
  timer_set_alarm_value(group, timer, timer_interval_sec * TIMER_SCALE);
  timer_enable_intr(group, timer);
  timer_isr_callback_add(group, timer, callback, args, 0);
  timer_start(group, timer);
}

void app_main(void) {
  timer_queue = xQueueCreate(10, sizeof(point_t));
  point_t point = {.x = 1, .y = 1};
  create_timer(TIMER_GROUP_0, TIMER_0, true, 1,
               &point);  // when auto_reload = true equal to
  // JavaScript setInterval function, otherwise setTimeout
  while (1) {
    point_t p;
    xQueueReceive(timer_queue, &p, portMAX_DELAY);
    ESP_LOGI(TAG, "point.x: %d point.y: %d", p.x, p.y);
  }
}