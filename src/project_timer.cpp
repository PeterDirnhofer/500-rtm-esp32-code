#include <stdio.h>
#include "esp_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gptimer.h"
#include "globalVariables.h"
#include "UsbPcInterface.h"

// Timer handle
static gptimer_handle_t gptimer = NULL;

// Callback function for MEASURE timer tick
static bool tickMeasure(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx)
{
    vTaskResume(handleControllerLoop);
    return true;
}

// Callback function for TUNNEL_FIND timer tick
static bool tickTunnelFind(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx)
{
    vTaskResume(handleTunnelLoop);
    return true;
}

// Callback function for ADJUST_TEST_TIP timer tick
static bool tickAdjust(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx)
{
    vTaskResume(handleAdjustLoop);
    return true;
}

// Start the timer
extern "C" void timer_start()
{
    ESP_ERROR_CHECK(gptimer_start(gptimer));
}

// Stop the timer
extern "C" void timer_stop()
{
    ESP_ERROR_CHECK(gptimer_stop(gptimer));
}

/**
 * @brief Start a cyclic timer to trigger MEASURE or TUNNEL_FIND loop.
 *
 * This function initializes a general-purpose timer with specific
 * configurations based on the provided mode. The timer will
 * trigger an alarm at specified intervals depending on the mode
 * set (MODE_MEASURE, MODE_TUNNEL_FIND, or MODE_ADJUST_TEST_TIP).
 *
 * @param mode The mode to set for the timer. This can be one of the following:
 *              - MODE_MEASURE: Triggers every 1260 microseconds.
 *              - MODE_TUNNEL_FIND: Triggers every 1 second (1000 ms).
 *              - MODE_ADJUST_TEST_TIP: Triggers every 1 second (1000 ms).
 */
extern "C" void timer_initialize(int mode)
{
    // Timer configuration
    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1 * 1000 * 1000, // 1 MHz, 1 tick = 1 us
        .intr_priority = 0,
        .flags = {
            .intr_shared = true,
            .backup_before_sleep = false},
    };

    // Create a new timer
    ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &gptimer));

    // Alarm configuration
    gptimer_alarm_config_t alarm_config = {};
    alarm_config.reload_count = 0;
    alarm_config.flags.auto_reload_on_alarm = true;

    // Set the alarm count based on the mode
    if (mode == MODE_MEASURE)
    {
        alarm_config.alarm_count = 1260; // 1260 us
    }
    else if (mode == MODE_TUNNEL_FIND)
    {
        alarm_config.alarm_count = 1000 * TUNNEL_TIMER_MS; // 1 second
    }
    else // mode == MODE_ADJUST_TEST_TIP
    {
        alarm_config.alarm_count = 1000 * 1000; // 1 second
    }

    // Set the alarm action
    ESP_ERROR_CHECK(gptimer_set_alarm_action(gptimer, &alarm_config));

    // Event callbacks
    gptimer_event_callbacks_t cbs = {};

    // Set the callback function based on the mode
    if (mode == MODE_MEASURE)
    {
        cbs.on_alarm = tickMeasure;
    }
    else if (mode == MODE_TUNNEL_FIND)
    {
        cbs.on_alarm = tickTunnelFind;
    }
    else // mode == MODE_ADJUST_TEST_TIP
    {
        cbs.on_alarm = tickAdjust;
    }

    // Register the event callbacks and enable the timer
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(gptimer, &cbs, NULL));
    ESP_ERROR_CHECK(gptimer_enable(gptimer));
    ESP_ERROR_CHECK(gptimer_start(gptimer));
}
