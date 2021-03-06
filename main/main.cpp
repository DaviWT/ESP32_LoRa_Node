/*******************************************************************************
 * 
 * ttn-esp32 - The Things Network device library for ESP-IDF / SX127x
 * 
 * Copyright (c) 2018 Manuel Bleichenbacher
 * 
 * Licensed under MIT License
 * https://opensource.org/licenses/MIT
 *
 * Sample program showing how to send and receive messages.
 *******************************************************************************
 *******************************************************************************
 * Adapted by: 
 * - Adriano Gamba
 * - Davi Tokikawa
 * - Erika Both
 *******************************************************************************/

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE

#include "ADC.h"
#include "LoRa.h"
#include "OLED.h"
#include "Sleep.h"
#include "driver/gpio.h"
#include "esp_event.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "nvs_flash.h"

#define DEBUG_MODE 0

// Tag to indicate at debug log
static const char *TAG = "MAIN";

static void GPIO_Init()
{
    gpio_set_direction(GPIO_NUM_25, GPIO_MODE_OUTPUT);

    // Initialize the GPIO ISR handler service
    ESP_ERROR_CHECK(gpio_install_isr_service(ESP_INTR_FLAG_IRAM));
}

extern "C" void app_main(void)
{
    // Set default debug output
    esp_log_set_vprintf(vprintf);

#if DEBUG_MODE
    esp_log_level_set("*", ESP_LOG_INFO);
#else
    esp_log_level_set("*", ESP_LOG_NONE);
#endif

    // Inital log to indicate execution start
    ESP_LOGI(TAG, "INITIALIZING SYSTEM MODULES...");

    // Init GPIO
    ESP_LOGI(TAG, "GPIO Init...");
    GPIO_Init();  // @TODO Continuar implementando conforme o desenvolvimento

    // Config ADC module
    ESP_LOGI(TAG, "ADC Init...");
    ADC_ConfigAdc();

    // Init OLED display
    ESP_LOGI(TAG, "Initializing OLED diplay at I2C INTERFACE...");
    OLED_Init(OLED_SDA_PIN, OLED_SCL_PIN, OLED_RST_PIN);

    // Initialize the NVS (non-volatile storage) for saving and restoring the keys
    ESP_LOGI(TAG, "NVS Init...");
    ESP_ERROR_CHECK(nvs_flash_init());

    if(!LoRa_NodeInit())
    {
        ESP_LOGE(TAG, "Couldn't init LoRa module node!");
        esp_restart();
    }

    /********************************************************************************/

    // Check if sleep routine started in previous executions
    Sleep_IsToDeepSleepAfterReset();

    ESP_LOGI(TAG, "SENDING PACKET AND SLEEPING...");
    vTaskDelay(pdMS_TO_TICKS(50));

    // SEND MESSAGE TO TTN
    LoRa_SendMessageToApplication();

    // Set sleep routine
    Sleep_SetFlagToStartSleepRoutine();
}