#include "UsbPcInterface.h"
#include "driver/gptimer.h"
#include "esp_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "globalVariables.h"
#include <stdio.h>

// Timer handle
static gptimer_handle_t gptimer = NULL;

// Callback function for MEASURE timer tick
static bool tickMeasure(gptimer_handle_t timer,
                        const gptimer_alarm_event_data_t *edata,
                        void *user_ctx) {
  // Prefer resuming the loop which is currently active. Tunnel and Measure
  // both use the same timer; resume the tunnel loop when `tunnelIsActive`
  // is true, otherwise resume the measure loop when `measureIsActive`
  // is true. This prevents the timer from always resuming the measure
  // loop when its task handle exists but the tunnel was requested.
  if (handleTunnelLoop != NULL && tunnelIsActive) {
    vTaskResume(handleTunnelLoop);
    return true;
  }
  if (handleMeasureLoop != NULL && measureIsActive) {
    vTaskResume(handleMeasureLoop);
    return true;
  }
  return true;
}

// Start the timer
extern "C" void timer_start() { ESP_ERROR_CHECK(gptimer_start(gptimer)); }

// Stop the timer
extern "C" void timer_stop() { ESP_ERROR_CHECK(gptimer_stop(gptimer)); }

extern "C" void timer_initialize() {
  // Timer configuration
  gptimer_config_t timer_config = {
      .clk_src = GPTIMER_CLK_SRC_DEFAULT,
      .direction = GPTIMER_COUNT_UP,
      .resolution_hz = 1 * 1000 * 1000, // 1 MHz, 1 tick = 1 us
      .intr_priority = 0,
      .flags = {.intr_shared = true, .backup_before_sleep = false},
  };

  // Create a new timer
  ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &gptimer));

  // Alarm configuration
  gptimer_alarm_config_t alarm_config = {};
  alarm_config.reload_count = 0;
  alarm_config.flags.auto_reload_on_alarm = true;
  alarm_config.alarm_count = 1260 * measureMs; // 1260 us

  // Set the alarm action
  ESP_ERROR_CHECK(gptimer_set_alarm_action(gptimer, &alarm_config));

  // Event callbacks
  gptimer_event_callbacks_t cbs = {};

  cbs.on_alarm = tickMeasure;

  // Register the event callbacks and enable the timer
  ESP_ERROR_CHECK(gptimer_register_event_callbacks(gptimer, &cbs, NULL));
  ESP_ERROR_CHECK(gptimer_enable(gptimer));
  ESP_ERROR_CHECK(gptimer_start(gptimer));
}
