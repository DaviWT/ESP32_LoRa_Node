#ifndef MAIN_SLEEP_H_
#define MAIN_SLEEP_H_

#include "esp_event.h"
#include "freertos/FreeRTOS.h"
#include "stdint.h"

#define NO_TIMER_WAKEUP   0
#define WAKE_ON_LOW_LEVEL 0

void Sleep_EnterSleepMode(uint64_t microsecondsToWakeUp);

#endif /* MAIN_SLEEP_H_ */
