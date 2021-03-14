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
#include "Sleep.h"
#include "TheThingsNetwork.h"
#include "driver/gpio.h"
#include "esp_event.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "nvs_flash.h"

// Tag to indicate at debug log
static const char *TAG = "MAIN";

// NOTE:
// The LoRaWAN frequency and the radio chip must be configured by running 'make menuconfig'.
// Go to Components / The Things Network, select the appropriate values and save.

// Copy the below hex string from the "Device EUI" field
// on your device's overview page in the TTN console.
const char *devEui = "006507095EF37549";

// Copy the below two lines from bottom of the same page
const char *appEui = "70B3D57ED003C387";
const char *appKey = "C0A9C01E65BB1B3FAB14360971CCBD3C";

// Pins and other resources
#define TTN_SPI_HOST     HSPI_HOST
#define TTN_SPI_DMA_CHAN 1
#define TTN_PIN_SPI_SCLK 5
#define TTN_PIN_SPI_MOSI 27
#define TTN_PIN_SPI_MISO 19
#define TTN_PIN_NSS      18
#define TTN_PIN_RXTX     TTN_NOT_CONNECTED
#define TTN_PIN_RST      14
#define TTN_PIN_DIO0     26
#define TTN_PIN_DIO1     35

static TheThingsNetwork ttn;

const unsigned TX_INTERVAL = 30;
static uint8_t msgData[] = "Hello World!";

bool join()
{
    ESP_LOGI(TAG, "Joining...");
    if(ttn.join())
    {
        ESP_LOGI(TAG, "Joined.");
        return true;
    }
    else
    {
        ESP_LOGI(TAG, "Join failed. Goodbye");
        return false;
    }
}

void sendMessages(void *pvParameter)
{
    while(1)
    {
        // Send 2 messages
        for(int i = 0; i < 2; i++)
        {
            ESP_LOGI(TAG, "Sending message...");
            TTNResponseCode res = ttn.transmitMessage(msgData, sizeof(msgData) - 1);
            if(res == kTTNSuccessfulTransmission)
                ESP_LOGI(TAG, "Message sent.");
            else
                ESP_LOGI(TAG, "Transmission failed.");

            vTaskDelay(TX_INTERVAL * pdMS_TO_TICKS(1000));
        }

        // shutdown
        ttn.shutdown();

        // go to sleep
        ESP_LOGI(TAG, "Sleeping for 30s...");
        vTaskDelay(pdMS_TO_TICKS(30000));

        // startup
        ttn.startup();

        // join again
        if(!join())
            return;
    }
}

void messageReceived(const uint8_t *message, size_t length, port_t port)
{
    //TODO: refactor debug log logic to use ESP_LOGI
    ESP_LOGI(TAG, "Message of %d bytes received on port %d:", length, port);
    for(int i = 0; i < length; i++)
        ESP_LOGI(TAG, " %02x", message[i]);
}

static void GPIO_Init()
{
    gpio_set_direction(GPIO_NUM_25, GPIO_MODE_OUTPUT);

    // Initialize the GPIO ISR handler service
    ESP_ERROR_CHECK(gpio_install_isr_service(ESP_INTR_FLAG_IRAM));
}

static void SPI_init()
{
    spi_bus_config_t spi_bus_config;
    spi_bus_config.miso_io_num = TTN_PIN_SPI_MISO;
    spi_bus_config.mosi_io_num = TTN_PIN_SPI_MOSI;
    spi_bus_config.sclk_io_num = TTN_PIN_SPI_SCLK;
    spi_bus_config.quadwp_io_num = -1;
    spi_bus_config.quadhd_io_num = -1;
    spi_bus_config.max_transfer_sz = 0;

    ESP_ERROR_CHECK(spi_bus_initialize(TTN_SPI_HOST, &spi_bus_config, TTN_SPI_DMA_CHAN));
}

extern "C" void app_main(void)
{
    // Set default debug output
    esp_log_set_vprintf(vprintf);

    // Set debug log level
    esp_log_level_set("*", ESP_LOG_INFO);

    // Inital log to indicate execution start
    ESP_LOGI(TAG, "FW START!");
    vTaskDelay(pdMS_TO_TICKS(500));

    // Set debug log level
    esp_log_level_set("*", ESP_LOG_INFO);

    // Init GPIO
    GPIO_Init();  // @TODO Continuar implementando conforme o desenvolvimento

    // Initialize the NVS (non-volatile storage) for saving and restoring the keys
    ESP_ERROR_CHECK(nvs_flash_init());

    // Initialize SPI bus
    SPI_init();

    // Configure the SX127x pins
    ttn.configurePins(TTN_SPI_HOST, TTN_PIN_NSS, TTN_PIN_RXTX, TTN_PIN_RST, TTN_PIN_DIO0, TTN_PIN_DIO1);

    // The below line can be commented after the first run as the data is saved in NVS
    ttn.provision(devEui, appEui, appKey);

    ttn.onMessage(messageReceived);

    if(join())
    {
        xTaskCreate(sendMessages, "send_messages", 1024 * 4, (void *)0, 3, nullptr);
    }

    // Creat ADC task
    xTaskCreate(&taskAdc, "ADC", 1024 * 2, (void *)0, 5, nullptr);
}