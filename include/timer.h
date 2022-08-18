#ifndef TIMER
#define TIMER

void timer_tg0_initialise (int timer_period_us);

//void timer_tg0_isr(void* arg);
void null_task(void *pvParam);

//void timer_group0_isr(void *para);

//void example_tg0_timer_init(int timer_idx, bool auto_reload, double timer_interval_sec);

#endif