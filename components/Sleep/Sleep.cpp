#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE

#include "Sleep.h"

#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "stdbool.h"

static void SetPinsToHold();
static void SetWakeUpConfig(uint64_t microsecondsToWakeUp);

/**
 * @brief Enter in deep sleep state
 * 
 * @param microsecondsToWakeUp microsseconds to wake up. If it's NO_TIMER_WAKEUP, it won't wake up by timer
 */
void Sleep_EnterSleepMode(uint64_t microsecondsToWakeUp)
{
    // @TODO Desligar modulo LORA
    // LORA_Shutdown();

    // @TODO Desligar OLED (Maybe)
    // OLED_Shutdown();

    // Set pins to hold during the deep sleep
    SetPinsToHold();

    // Set wake up sources
    SetWakeUpConfig(microsecondsToWakeUp);

    // Start deep sleep
    esp_deep_sleep_start();
}

/**
 * @brief Set pins to hold their state during the deep sleep 
 * 
 */
static void SetPinsToHold()
{
    gpio_set_level(GPIO_NUM_25, 0);
    gpio_hold_en(GPIO_NUM_25);
    gpio_deep_sleep_hold_en();
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
