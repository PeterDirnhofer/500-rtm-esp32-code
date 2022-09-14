#include <stdio.h>
#include "esp_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/periph_ctrl.h"
#include "driver/timer.h"

#include "globalVariables.h"
#include "timer.h"

// https://esp32developer.com/programming-in-c-c/timing/hardware-timers

void null_task(void *pvParam)
{
    printf("task init\n");
    while(1)
    {
        printf("task \n");

        vTaskSuspend(NULL);
    }

}


/**
 * @brief Timerinterrupt retriggert zyklisch die controllerLoop
 * 
 * @param arg 
 */
void timer_tg0_isr(void* arg)
{
	static int io_state = 0;

	//Reset irq and set for next time
    TIMERG0.int_clr_timers.t0 = 1;
    TIMERG0.hw_timer[0].config.alarm_en = 1;


    vTaskResume(handleControllerLoop);
}
   



void timer_tg0_initialise (int timer_period_us)
{
    timer_config_t config = {
            .alarm_en = TIMER_ALARM_EN,				//Alarm Enable
            .counter_en = TIMER_PAUSE,			//If the counter is enabled it will start incrementing / decrementing immediately after calling timer_init()
            .intr_type = TIMER_INTR_LEVEL,	//Is interrupt is triggered on timer’s alarm (timer_intr_mode_t)
            .counter_dir = TIMER_COUNT_UP,	//Does counter increment or decrement (timer_count_dir_t)
            .auto_reload = TIMER_AUTORELOAD_EN,			//If counter should auto_reload a specific initial value on the timer’s alarm, or continue incrementing or decrementing.
            .divider = 80   				//Divisor of the incoming 80 MHz (12.5nS) APB_CLK clock. E.g. 80 = 1uS per timer tick
    };

    timer_init(TIMER_GROUP_0, TIMER_0, &config);
    timer_set_counter_value(TIMER_GROUP_0, TIMER_0, 0);
    timer_set_alarm_value(TIMER_GROUP_0, TIMER_0, timer_period_us);
    timer_enable_intr(TIMER_GROUP_0, TIMER_0);
    timer_isr_register(TIMER_GROUP_0, TIMER_0, &timer_tg0_isr, NULL, 0, &s_timer_handle);
    //printf("done init \n");
    timer_start(TIMER_GROUP_0, TIMER_0);
    printf("*** Sekundentimer Interrupt für resume controllerLoop gestartet\n");
}