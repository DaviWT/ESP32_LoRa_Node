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

#define DEBUG_MODE 0

#define NUM_OF_PACKETS_DISCHARGE_TEST 1000

// Tag to indicate at debug log
static const char *TAG = "MAIN";

static void GPIO_Init()
{
    gpio_set_direction(GPIO_NUM_25, GPIO_MODE_OUTPUT);

    // Initialize the GPIO ISR handler service
    ESP_ERROR_CHECK(gpio_install_isr_service(ESP_INTR_FLAG_IRAM));
}

static void blinkLed_TEST()
{
    gpio_set_level(GPIO_NUM_25, 0);
    int i;
    for(i = 0; i < 10; i++)
    {
        gpio_set_level(GPIO_NUM_25, i % 2);
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    gpio_set_level(GPIO_NUM_25, 0);
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
    GPIO_Init();  // @TODO Continuar implementando conforme o desenvolvimento

    // Config ADC module
    ADC_ConfigAdc();

    // Initialize the NVS (non-volatile storage) for saving and restoring the keys
    ESP_ERROR_CHECK(nvs_flash_init());

    if(!LoRa_NodeInit())
    {
        ESP_LOGE(TAG, "Couldn't init LoRa module node!");
        esp_restart();
    }

    /********************************************************************************/

    ESP_LOGI(TAG, "SENDING PACKET AND SLEEPING...");
    vTaskDelay(pdMS_TO_TICKS(50));

    // SEND MESSAGE TO TTN
    gpio_set_level(GPIO_NUM_25, 1);
    // int i;
    // for(i = 0; i < NUM_OF_PACKETS_DISCHARGE_TEST; i++)
    //     LoRa_SendMessageToApplication();

    // blinkLed_TEST();

    // Sleep_EnterSleepMode(KEEP_ALIVE_TIMEOUT_uS);

    // NOT SUPPOSED TO REACH HERE
    // ESP_LOGE(TAG, "INSOMNIA! SHOULD'VE SLEPT!");
    // vTaskDelay(pdMS_TO_TICKS(500));

    while(1)
        vTaskDelay(pdMS_TO_TICKS(500));
}