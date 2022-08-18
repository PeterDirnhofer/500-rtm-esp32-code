#include <stdio.h>
#include "esp_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/periph_ctrl.h"
#include "driver/timer.h"

#include "globalVariables.h"
#include "timer.h"

// #define TIMER_DIVIDER         16  //  Hardware timer clock divider
// #define TIMER_SCALE           (TIMER_BASE_CLK / TIMER_DIVIDER)  // convert counter value to seconds
// #define TIMER_INTERVAL0_SEC   (3.4179) // sample test interval for the first timer
// #define TIMER_INTERVAL1_SEC   (5.78)   // sample test interval for the second timer
// #define TEST_WITHOUT_RELOAD   0        // testing will be done without auto reload
// #define TEST_WITH_RELOAD      1        // testing will be done with auto reload

// typedef struct {
//     int type;  // the type of timer's event
//     int timer_group;
//     int timer_idx;
//     uint64_t timer_counter_value;
// } timer_event_t;

void null_task(void *pvParam)
{
    printf("task init\n");
    while(1)
    {
        printf("task \n");

        vTaskSuspend(NULL);
    }

}


// void timer_group0_isr(void *para)
// {
//     timer_spinlock_take(TIMER_GROUP_0);
//     int timer_idx = (int) para;

//     printf("INTERRUPT TIMER\n");
//     fflush(stdout);

//     /* Retrieve the interrupt status and the counter value
//        from the timer that reported the interrupt */
//     uint32_t timer_intr = timer_group_get_intr_status_in_isr(TIMER_GROUP_0);
//     uint64_t timer_counter_value = timer_group_get_counter_value_in_isr(TIMER_GROUP_0,(timer_idx_t) timer_idx);

//     /* Prepare basic event data
//        that will be then sent back to the main program task */
//     timer_event_t evt;
//     evt.timer_group = 0;
//     evt.timer_idx = timer_idx;
//     evt.timer_counter_value = timer_counter_value;

//     /* Clear the interrupt
//        and update the alarm time for the timer with without reload */
//     if (timer_intr & TIMER_INTR_T0) {
//         evt.type = TEST_WITHOUT_RELOAD;
//         timer_group_clr_intr_status_in_isr(TIMER_GROUP_0, TIMER_0);
//         timer_counter_value += (uint64_t) (TIMER_INTERVAL0_SEC * TIMER_SCALE);
//         timer_group_set_alarm_value_in_isr(TIMER_GROUP_0, (timer_idx_t) timer_idx, timer_counter_value);
//     } else if (timer_intr & TIMER_INTR_T1) {
//         evt.type = TEST_WITH_RELOAD;
//         timer_group_clr_intr_status_in_isr(TIMER_GROUP_0, TIMER_1);
//     } else {
//         evt.type = -1; // not supported even type
//     }

//     /* After the alarm has been triggered
//       we need enable it again, so it is triggered the next time */
//     timer_group_enable_alarm_in_isr(TIMER_GROUP_0, (timer_idx_t) timer_idx);

//     /* Now just send the event data back to the main program task */
//     xQueueSendFromISR(timer_queue, &evt, NULL);
//     timer_spinlock_give(TIMER_GROUP_0);
// }

void timer_tg0_isr(void* arg)
{
	static int io_state = 0;

	//Reset irq and set for next time
    TIMERG0.int_clr_timers.t0 = 1;
    TIMERG0.hw_timer[0].config.alarm_en = 1;


    //----- HERE EVERY #uS -----

	//Toggle a pin so we can verify the timer is working using an oscilloscope
	//io_state ^= 1;									//Toggle the pins state
	//gpio_set_direction(GPIO_NUM_5, GPIO_MODE_OUTPUT);
	//gpio_set_level(GPIO_NUM_5, io_state);
    //printf("INT \n");
    vTaskResume(handleControllerLoop);
    //vTaskResume(handleTask);
}

/*
 * Initialize selected timer of the timer group 0
 *
 * timer_idx - the timer number to initialize
 * auto_reload - should the timer auto reload on alarm?
 * timer_interval_sec - the interval of alarm to set
 */
// void example_tg0_timer_init(int timer_idx,
//                                    bool auto_reload, double timer_interval_sec)
// {
//     /* Select and initialize basic parameters of the timer */
//     timer_config_t config = {
//         .alarm_en = TIMER_ALARM_EN,
//         .counter_en = TIMER_PAUSE,
//         .intr_type = TIMER_INTR_LEVEL,
//         .counter_dir = TIMER_COUNT_UP,
//         .auto_reload = TIMER_AUTORELOAD_EN,
//         .divider = TIMER_DIVIDER,
        
//     }; // default clock source is APB
//     timer_init(TIMER_GROUP_0, (timer_idx_t) timer_idx, &config);

//     /* Timer's counter will initially start from value below.
//        Also, if auto_reload is set, this value will be automatically reload on alarm */
//     timer_set_counter_value(TIMER_GROUP_0,(timer_idx_t) timer_idx, 0x00000000ULL);

//     /* Configure the alarm value and the interrupt on alarm. */
//     timer_set_alarm_value(TIMER_GROUP_0, (timer_idx_t) timer_idx, timer_interval_sec * TIMER_SCALE);
//     timer_enable_intr(TIMER_GROUP_0, (timer_idx_t) timer_idx);
//     timer_isr_register(TIMER_GROUP_0, (timer_idx_t) timer_idx, &timer_group0_isr,(void *) timer_idx, ESP_INTR_FLAG_LOWMED, NULL);

//     timer_start(TIMER_GROUP_0, (timer_idx_t) timer_idx);
// }
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
    printf("done init \n");
    timer_start(TIMER_GROUP_0, TIMER_0);
    printf("timer started\n");
}