#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE

#include "Sleep.h"

#include "LoRa.h"
#include "OLED.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "stdbool.h"

static void SetWakeUpConfig(uint64_t microsecondsToWakeUp);

/**
 * @brief Enter in deep sleep state
 * 
 * @param microsecondsToWakeUp microsseconds to wake up. If it's NO_TIMER_WAKEUP, it won't wake up by timer
 */
void Sleep_EnterSleepMode(uint64_t microsecondsToWakeUp)
{
    // Desliga modulo LORA
    LORA_Shutdown();

    // Desliga OLED
    OLED_EnterSleep();

    // Set wake up sources
    SetWakeUpConfig(microsecondsToWakeUp);

    // Start deep sleep
    esp_deep_sleep_start();
}

/**
 * @brief Config wake up sources
 * 
 * @param microsecondsToWakeUp 
 */
static void SetWakeUpConfig(uint64_t microsecondsToWakeUp)
{
    // Set wake up pin and enable sleep GPIO wakeup (ext0)
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, WAKE_ON_LOW_LEVEL);  // @TODO Definir pino definitivo para wake up

    // Set wake up by timer
    if(microsecondsToWakeUp != NO_TIMER_WAKEUP)
        esp_sleep_enable_timer_wakeup(microsecondsToWakeUp);
}
