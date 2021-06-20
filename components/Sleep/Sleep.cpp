#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE

#include "Sleep.h"

#include "LoRa.h"
#include "OLED.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "stdbool.h"

RTC_NOINIT_ATTR uint8_t u8SwRstFlagIsToDeepSleepNow;

// Tag to indicate at debug log
static const char *TAG = "SLEEP";

static void SetWakeUpConfig(uint64_t microsecondsToWakeUp);

/**
 * @brief Check if it's to sleep after a software reset was done
 * 
 * @note This is being done because of radio lora isn't sleep 100% on the previous execution
 * 
 */
void Sleep_IsToDeepSleepAfterReset()
{
    if(u8SwRstFlagIsToDeepSleepNow == 1)
    {
        u8SwRstFlagIsToDeepSleepNow = 0;
        ESP_LOGI(TAG, "Sleep flag is set. Entering sleep mode");
        Sleep_EnterSleepMode(KEEP_ALIVE_TIMEOUT_uS);

        // NOT SUPPOSED TO REACH HERE
        ESP_LOGE(TAG, "INSOMNIA! SHOULD'VE SLEPT!");
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    else
    {
        u8SwRstFlagIsToDeepSleepNow = 0;
    }
}

/**
 * @brief Set flag to start sleep routine
 * 
 */
void Sleep_SetFlagToStartSleepRoutine()
{
    ESP_LOGI(TAG, "Setting sleep flag to start sleep routine");
    u8SwRstFlagIsToDeepSleepNow = 1;
    vTaskDelay(pdMS_TO_TICKS(50));
    esp_restart();
}

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
    ESP_LOGW(TAG, "ENTERING DEEP SLEEP...");
    vTaskDelay(pdMS_TO_TICKS(100));
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
