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
#include "Sleep.h"
#include "driver/gpio.h"
#include "esp_event.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "nvs_flash.h"

// Tag to indicate at debug log
static const char *TAG = "MAIN";

#define KEEP_ALIVE_TIMEOUT  6 * 3600 * 1000  // 6 hours
#define MSG_TYPE_KEEP_ALIVE 0
#define MSG_TYPE_INFORM_PIN 1

// Task handles
TaskHandle_t xtaskLoRaTX = NULL;

static void MakePayloadMsg(char *strPayload);

static void GPIO_Init()
{
    gpio_set_direction(GPIO_NUM_25, GPIO_MODE_OUTPUT);

    // Initialize the GPIO ISR handler service
    ESP_ERROR_CHECK(gpio_install_isr_service(ESP_INTR_FLAG_IRAM));
}

static void MakePayloadMsg(char *strPayload)
{
    if(strPayload == NULL)
    {
        ESP_LOGE(TAG, "Failed to set payload message");
        return;
    }

    // Get current battery voltage
    uint32_t vBat = ADC_GetVoltage();

    // Check last reset reason
    esp_reset_reason_t resetReason = esp_reset_reason();
    if(resetReason != ESP_RST_DEEPSLEEP)
    {
        sprintf(strPayload, "%d|%u|", MSG_TYPE_KEEP_ALIVE, vBat);
        return;
    }

    // Check last wake-up reason
    esp_sleep_wakeup_cause_t wakeUpReason = esp_sleep_get_wakeup_cause();
    switch(wakeUpReason)
    {
        case ESP_SLEEP_WAKEUP_TIMER:
            sprintf(strPayload, "%d|%u|", MSG_TYPE_KEEP_ALIVE, vBat);
            break;
        case ESP_SLEEP_WAKEUP_EXT0:
            sprintf(strPayload, "%d|%u|", MSG_TYPE_INFORM_PIN, vBat);
            break;
        default:
            ESP_LOGE(TAG, "Failed to set payload message because of unexpected wake-up reason");
            break;
    }
}

extern "C" void app_main(void)
{
    // Set default debug output
    esp_log_set_vprintf(vprintf);

    // Set debug log level
    esp_log_level_set("*", ESP_LOG_INFO);

    // Inital log to indicate execution start
    ESP_LOGW(TAG, "BEGINNING OF APP_MAIN()!");
    vTaskDelay(pdMS_TO_TICKS(500));

    // Init GPIO
    GPIO_Init();  // @TODO Continuar implementando conforme o desenvolvimento

    // Config ADC module
    ADC_ConfigAdc();

    // Get battery voltage
    uint32_t vBat = ADC_GetVoltage();

    ESP_LOGI(TAG, "vBat = %u", vBat);

    // Initialize the NVS (non-volatile storage) for saving and restoring the keys
    ESP_ERROR_CHECK(nvs_flash_init());

    if(!LoRa_NodeInit())
    {
        ESP_LOGE(TAG, "Couldn't init LoRa module node!");
        esp_restart();
    }

    vTaskDelay(pdMS_TO_TICKS(1000));

    if(xTaskCreate(&taskLoRaTX, "LoRa_TX", 4096, NULL, 5, &xtaskLoRaTX) != pdTRUE)
    {
        ESP_LOGE(TAG, "Failed to init LoRa_TX task!");
    }

    ESP_LOGW(TAG, "END OF APP_MAIN()!");
}