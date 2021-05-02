#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE

#include "LoRa.h"

#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "lmic.h"
#include "stdbool.h"

// Tag to indicate at debug log
static const char *TAG = "LoRa";

/* =============== OTAA =============== */
const char *devEui = "006507095EF37549";
const char *appEui = "70B3D57ED003C387";
const char *appKey = "C0A9C01E65BB1B3FAB14360971CCBD3C";
/* ==================================== */

/* =============== ABP =============== */
// These should be in big-endian (aka msb).
static u1_t NWKSKEY[16] = {0x8E, 0xE3, 0x20, 0xB5, 0xA8, 0xE1, 0xC4, 0xEF, 0x5A, 0xF1, 0x59, 0xB5, 0x3F, 0xD0, 0xA3, 0x67};

static u1_t APPSKEY[16] = {0x43, 0xFB, 0x7F, 0xFF, 0x20, 0xE9, 0x09, 0x81, 0x77, 0xAC, 0x28, 0x88, 0x16, 0xE1, 0x0E, 0x79};
// The library converts the address to network byte order as needed, so this should be in big-endian (aka msb) too.
static const u4_t DEVADDR = 0x2603168E;
/* =================================== */

static TheThingsNetwork ttn;

const unsigned TX_INTERVAL = 5;
static uint8_t msgData[] = "Davi, Hello World!";

static bool LoRa_ModuleSpiBusInit();
static void LoRa_LmicReset();
static void messageReceived(const uint8_t *message, size_t length, port_t port);

bool LoRa_NodeInit()
{
    // Configure the SX127x pins
    if(!LoRa_ModemPinoutInit())
        return false;

    LoRa_LmicReset();

    return true;
}

bool LoRa_ModemPinoutInit()
{
    if(!LoRa_ModuleSpiBusInit())
    {
        ESP_LOGE(TAG, "Unable to init and config SPI bus for LoRa module");
        return false;
    }

    // Configures PINS and init LMIC OS
    ttn.configurePins(TTN_SPI_HOST, TTN_PIN_NSS, TTN_PIN_RXTX, TTN_PIN_RST, TTN_PIN_DIO0, TTN_PIN_DIO1);

    return true;
}

void LoRa_SelectChannel(uint8_t channel_number)
{
    for(int b = 0; b < 8; ++b)
    {
        LMIC_disableSubBand(b);
    }
    LMIC_enableChannel(8 + channel_number);
}

void LoRa_ConfigTTNKeys()
{
    // Can be commented after the first run as the data is saved in NVS
    ttn.provision(devEui, appEui, appKey);
}

void LoRa_ConfigTTNKeys_ABP()
{
    LMIC_setSession(0x13, DEVADDR, NWKSKEY, APPSKEY);
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

bool LoRa_SendPacket(uint8_t *txData, size_t dataSize)
{
    for(int i = 0; i < MAX_TX_ATTEMPTS; i++)
    {
        ESP_LOGI(TAG, "Sending packet... (attempt %d)", i + 1);
        if(ttn.transmitMessage(txData, dataSize) == kTTNSuccessfulTransmission)
        {
            ESP_LOGI(TAG, "Packet sent!");
            return true;
        }
        ESP_LOGE(TAG, "Transmission attempt %d failed!", i + 1);
    }

    return false;
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
    spi_bus_config.flags = 0;
    spi_bus_config.intr_flags = 0;

    esp_err_t err = spi_bus_initialize(TTN_SPI_HOST, &spi_bus_config, TTN_SPI_DMA_CHAN);
    if(err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to init SPI bus = %s", esp_err_to_name(err));
        return false;
    }

    return true;
}

static void LoRa_LmicReset()
{
    vTaskDelay(pdMS_TO_TICKS(100));

    // Set ABP TTN access keys
    LoRa_ConfigTTNKeys_ABP();

    //Selects single channel from certain band
    LoRa_SelectChannel(CHANNEL_NUM);

    // Disable link check validation
    LMIC_setLinkCheckMode(0);

    // TTN uses SF9 for its RX2 window.
    LMIC.dn2Dr = DR_SF9;

    // Set data rate and transmit power for uplink
    LMIC_setDrTxpow(DR_SF9, 14);

    vTaskDelay(pdMS_TO_TICKS(100));
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
            if(LoRa_SendPacket(msgData, strlen((char *)msgData)))
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
        LoRa_LmicReset();
    }
}