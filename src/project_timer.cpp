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
// Track timer state to avoid calling stop/disable on the wrong timer
static bool gptimer_created = false;
static bool gptimer_enabled = false;
static bool gptimer_running = false;

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

// Start the timer (sets running flag)
extern "C" void timer_start() {
  if (gptimer == NULL)
    return;
  esp_err_t err = gptimer_start(gptimer);
  if (err == ESP_OK) {
    gptimer_running = true;
  } else {
    ESP_LOGW("timer_start", "gptimer_start returned %d", err);
  }
}

// Stop the timer (only if running)
extern "C" void timer_stop() {
  if (gptimer == NULL || !gptimer_running)
    return;
  esp_err_t err = gptimer_stop(gptimer);
  if (err == ESP_OK || err == ESP_ERR_INVALID_STATE) {
    gptimer_running = false;
  } else {
    ESP_LOGW("timer_stop", "gptimer_stop returned %d", err);
  }
}

// Deinitialize and free the GPTIMER resource
extern "C" void timer_deinitialize() {
  if (gptimer == NULL) {
    return;
  }

  // Stop only if we believe the timer is running
  if (gptimer_running) {
    esp_err_t err = gptimer_stop(gptimer);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE &&
        err != ESP_ERR_INVALID_ARG) {
      ESP_LOGW("timer_deinitialize", "gptimer_stop returned %d", err);
    }
    gptimer_running = false;
  }

  // Disable only if we enabled it before
  if (gptimer_enabled) {
    esp_err_t err = gptimer_disable(gptimer);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE &&
        err != ESP_ERR_INVALID_ARG) {
      ESP_LOGW("timer_deinitialize", "gptimer_disable returned %d", err);
    }
    gptimer_enabled = false;
  }

  // Do NOT delete the timer handle here. Keeping the handle allocated
  // avoids races with the low-level driver when stopping/deleting while
  // interrupts or callbacks may be running on another core. The handle
  // will be reused for subsequent runs.
}

extern "C" void timer_initialize() {
  // Timer configuration
  gptimer_config_t timer_config = {
      .clk_src = GPTIMER_CLK_SRC_DEFAULT,
      .direction = GPTIMER_COUNT_UP,
      .resolution_hz = 1 * 1000 * 1000, // 1 MHz, 1 tick = 1 us
      .intr_priority = 0,
      .flags = {.intr_shared = true, .backup_before_sleep = false},
  };

  // Prevent double initialization: if a timer handle already exists,
  // If a timer handle already exists, reconfigure/enable/start it rather
  // than skipping entirely. This allows restarting the timer after a
  // previous run disabled it without deleting the handle.
  if (gptimer != NULL) {
    // Alarm configuration
    gptimer_alarm_config_t alarm_config = {};
    alarm_config.reload_count = 0;
    alarm_config.flags.auto_reload_on_alarm = true;
    alarm_config.alarm_count = 1260 * measureMs; // 1260 us

    ESP_ERROR_CHECK(gptimer_set_alarm_action(gptimer, &alarm_config));

    gptimer_event_callbacks_t cbs = {};
    cbs.on_alarm = tickMeasure;
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(gptimer, &cbs, NULL));

    if (!gptimer_enabled) {
      ESP_ERROR_CHECK(gptimer_enable(gptimer));
      gptimer_enabled = true;
    }
    if (!gptimer_running) {
      ESP_ERROR_CHECK(gptimer_start(gptimer));
      gptimer_running = true;
    }
    return;
  }

  // Prevent double initialization: if a timer handle already exists,
  // assume it's been created and skip creating a new one.
  if (gptimer != NULL) {
    ESP_LOGW("timer_initialize",
             "gptimer already exists, skipping new creation");
    return;
  }

  // Create a new timer
  ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &gptimer));
  gptimer_created = true;

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
  gptimer_enabled = true;
  ESP_ERROR_CHECK(gptimer_start(gptimer));
  gptimer_running = true;
}
