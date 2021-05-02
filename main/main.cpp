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

// Task handles
TaskHandle_t xtaskLoRaTX = NULL;

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