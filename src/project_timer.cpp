#include <stdio.h>
#include "esp_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
// #include "driver/periph_ctrl.h"
// #include "driver/timer.h"
#include "driver/gptimer.h"

#include "globalVariables.h"
// #include "timer.h"

#include "UsbPcInterface.h"

// https://esp32developer.com/programming-in-c-c/timing/hardware-timers
/*
void null_task(void *pvParam)
{
    printf("task init\n");
    while (1)
    {
        printf("task \n");
        vTaskSuspend(NULL);
    }
}
*/

/**
 * @brief On Trigger start controllerLoop
 * @param arg
 */
void timer_tg0_isr(void *arg)
{

    // timer_group_clr_intr_status_in_isr(TIMER_GROUP_0,TIMER_0);
    // timer_group_enable_alarm_in_isr(TIMER_GROUP_0,TIMER_0);
    vTaskResume(handleControllerLoop);
    // Reset irq and set for next time
    // TIMERG0.int_clr_timers.t0 = 1;
    // TIMERG0.hw_timer[0].config.alarm_en = 1;
}

/// @brief On trigger start adjustLoop
/// @param arg



static bool timer_tg0_isr0(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx){
    vTaskResume(handleControllerLoop);
    return true;
}

static bool timer_tg0_isr1(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx){
    vTaskResume(handleAdjustLoop);
    return true;
}



extern "C" void timer_tg0_initialise(int timer_period_us, uint32_t divider, int mode)
{

    gptimer_handle_t gptimer = NULL;

    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1 * 1000 * 1000, // 1MHz, 1 tick = 1us
    };
    

    ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &gptimer));

    gptimer_alarm_config_t alarm_config = {};
    //alarm_config.alarm_count = 1000000;
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

    gptimer_event_callbacks_t cbs = { };
    if (mode == MODE_MEASURE)
    {
        cbs.on_alarm = timer_tg0_isr0;
    }
    else // mode == MODE_ADJUST_TEST_TIP
    {
        cbs.on_alarm = timer_tg0_isr1;
    }


    ESP_ERROR_CHECK(gptimer_register_event_callbacks(gptimer, &cbs,NULL));
    ESP_ERROR_CHECK(gptimer_enable(gptimer));
    ESP_ERROR_CHECK(gptimer_start(gptimer));

}
