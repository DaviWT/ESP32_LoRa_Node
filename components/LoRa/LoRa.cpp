#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE

#include "LoRa.h"

#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "stdbool.h"

// Tag to indicate at debug log
static const char *TAG = "LoRa";

// Copy the below hex string from the "Device EUI" field
// on your device's overview page in the TTN console.
const char *devEui = "006507095EF37549";

// Copy the below two lines from bottom of the same page
const char *appEui = "70B3D57ED003C387";
const char *appKey = "C0A9C01E65BB1B3FAB14360971CCBD3C";

static TheThingsNetwork ttn;

const unsigned TX_INTERVAL = 30;
static uint8_t msgData[] = "Hello World!";

static bool LoRa_ModuleSpiBusInit();
static void messageReceived(const uint8_t *message, size_t length, port_t port);

bool LoRa_NodeInit()
{
    // Configure the SX127x pins
    if(!LoRa_ModemPinoutInit())
        return false;

    // Set OTAA TTN access keys (comment after first run)
    LoRa_ConfigTTNKeys();

    // Set callback function at receving LoRa messages
    LoRa_SetMessageRxCallback();

    return true;
}

bool LoRa_ModemPinoutInit()
{
    if(LoRa_ModuleSpiBusInit())
    {
        ESP_LOGE(TAG, "Unable to init and config SPI bus for LoRa module");
        return false;
    }

    ttn.configurePins(TTN_SPI_HOST, TTN_PIN_NSS, TTN_PIN_RXTX, TTN_PIN_RST, TTN_PIN_DIO0, TTN_PIN_DIO1);

    return true;
}

void LoRa_ConfigTTNKeys()
{
    // Can be commented after the first run as the data is saved in NVS
    ttn.provision(devEui, appEui, appKey);
}

void LoRa_SetMessageRxCallback()
{
    ttn.onMessage(messageReceived);
}

bool LoRa_JoinTTN()
{
    ESP_LOGI(TAG, "Joining TTN...");
    if(!ttn.join())
    {
        ESP_LOGE(TAG, "Failed to join!");
        return false;
    }

    ESP_LOGI(TAG, "Joined!");
    return true;
}

static bool LoRa_ModuleSpiBusInit()
{
    spi_bus_config_t spi_bus_config;
    spi_bus_config.miso_io_num = TTN_PIN_SPI_MISO;
    spi_bus_config.mosi_io_num = TTN_PIN_SPI_MOSI;
    spi_bus_config.sclk_io_num = TTN_PIN_SPI_SCLK;
    spi_bus_config.quadwp_io_num = -1;
    spi_bus_config.quadhd_io_num = -1;
    spi_bus_config.max_transfer_sz = 0;

    if(spi_bus_initialize(TTN_SPI_HOST, &spi_bus_config, TTN_SPI_DMA_CHAN) != ESP_OK)
        return false;

    return true;
}

static void messageReceived(const uint8_t *message, size_t length, port_t port)
{
    //TODO: refactor debug log logic to use ESP_LOGI
    ESP_LOGI(TAG, "Message of %d bytes received on port %d:", length, port);
    for(int i = 0; i < length; i++)
        ESP_LOGI(TAG, " %02x", message[i]);
}

void taskLoRaTX(void *pvParameter)
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
        if(!LoRa_JoinTTN())
            return;
    }
}