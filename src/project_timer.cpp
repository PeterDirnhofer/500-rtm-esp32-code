#include <stdio.h>
#include "esp_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gptimer.h"
#include "globalVariables.h"
// #include "timer.h"

#include "UsbPcInterface.h"
// https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/gptimer.html



static bool m_tick_measure(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx)
{
    vTaskResume(handleControllerLoop);
    return true;
}

static bool m_tick_adjust(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx)
{
    vTaskResume(handleAdjustLoop);
    return true;
}
/// @brief Start cyclic timer to trigger MEASURE or ADJUST loop
/// @param mode MODE_MEASURE or MODE_ADJUST_TEST_TIP
extern "C" void timer_initialize(int mode)
{
    gptimer_handle_t gptimer = NULL;
    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1 * 1000 * 1000, // 1MHz, 1 tick = 1us
    };

    ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &gptimer));

    gptimer_alarm_config_t alarm_config = {};
    alarm_config.reload_count = 0;
    alarm_config.flags.auto_reload_on_alarm = true;

    if (mode == MODE_MEASURE)
    {
        alarm_config.alarm_count = 1260; // 1260 us
    }
    else // mode == MODE_ADJUST_TEST_TIP
    {
        alarm_config.alarm_count = 1000 * 1000; // 1000 * 1000 us = 1 Sekunde
    }

    ESP_ERROR_CHECK(gptimer_set_alarm_action(gptimer, &alarm_config));

    gptimer_event_callbacks_t cbs = {};
    if (mode == MODE_MEASURE)
    {
        cbs.on_alarm = m_tick_measure;
    }
    else // mode == MODE_ADJUST_TEST_TIP
    {
        cbs.on_alarm = m_tick_adjust;
    }

    ESP_ERROR_CHECK(gptimer_register_event_callbacks(gptimer, &cbs, NULL));
    ESP_ERROR_CHECK(gptimer_enable(gptimer));
    ESP_ERROR_CHECK(gptimer_start(gptimer));
}
