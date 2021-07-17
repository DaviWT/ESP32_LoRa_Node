#define LOG_LOCAL_LEVEL ESP_LOG_INFO

#include "LoRa.h"

#include "ADC.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_sleep.h"
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

static bool LoRa_ModuleSpiBusInit();
static void LoRa_LmicReset();
static void messageReceived(const uint8_t *message, size_t length, port_t port);
static void LoRa_MakePayloadMsg(char *strPayload);
static void LoRa_MakeTestPacket(char *strPayload);
static void LoRa_SetTxConfig(tx_config_e mode);

static u1_t initialOpMode = 0xFF;

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

void LoRa_ConfigTTNKeys_ABP()
{
    LMIC_setSession(0x13, DEVADDR, NWKSKEY, APPSKEY);
}

void LoRa_SetMessageRxCallback()
{
    ttn.onMessage(messageReceived);
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

bool LoRa_SendMessageToApplication()
{
    char packetStr[100] = "";
    // LoRa_MakePayloadMsg(packetStr);
    LoRa_MakeTestPacket(packetStr);

    if(!LoRa_SendPacket((uint8_t *)packetStr, strlen(packetStr)))
    {
        ESP_LOGE(TAG, "Failed to send message to application!");
        return false;
    }

    ESP_LOGI(TAG, "SENT: %s", packetStr);
    return true;
}

void LORA_Shutdown()
{
    ESP_LOGD(TAG, "OPMODE 1 = 0x%.2X", readOpMode());

    // shutdown
    ttn.shutdown();

    hal_pin_rst(2);  // configure RST pin floating!
    vTaskDelay(pdMS_TO_TICKS(1));
    hal_pin_rst(0);  // drive RST pin low
    vTaskDelay(pdMS_TO_TICKS(1));
    hal_pin_rst(2);  // configure RST pin floating!
    vTaskDelay(pdMS_TO_TICKS(10));

    ESP_LOGD(TAG, "OPMODE 2 = 0x%.2X", readOpMode());

    if(radio_reinit() == 0)
    {
        ESP_LOGE(TAG, "ERROR SETTING RADIO TO STANDARD OPERATION!");
    }

    ESP_LOGD(TAG, "OPMODE 3 = 0x%.2X", readOpMode());

    vTaskDelay(pdMS_TO_TICKS(100));
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

    // Configures LoRa TX Mode
    LoRa_SetTxConfig(TX_MODE_2);

    vTaskDelay(pdMS_TO_TICKS(100));
}

static void messageReceived(const uint8_t *message, size_t length, port_t port)
{
    //TODO: refactor debug log logic to use ESP_LOGI
    ESP_LOGI(TAG, "Message of %d bytes received on port %d:", length, port);
    for(int i = 0; i < length; i++)
        ESP_LOGI(TAG, " %02x", message[i]);
}

static void LoRa_MakePayloadMsg(char *strPayload)
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
        ESP_LOGI(TAG, "Normal boot. Keep-alive message.");
        sprintf(strPayload, "%d|%u|", MSG_TYPE_KEEP_ALIVE, vBat);
        return;
    }

    // Check last wake-up reason
    esp_sleep_wakeup_cause_t wakeUpReason = esp_sleep_get_wakeup_cause();
    switch(wakeUpReason)
    {
        case ESP_SLEEP_WAKEUP_TIMER:
            ESP_LOGI(TAG, "Timer wakeup. Keep-alive message.");
            sprintf(strPayload, "%d|%u|", MSG_TYPE_KEEP_ALIVE, vBat);
            break;
        case ESP_SLEEP_WAKEUP_EXT0:
            ESP_LOGI(TAG, "Ext GPIO wakeup. Alarm message.");
            sprintf(strPayload, "%d|%u|", MSG_TYPE_INFORM_PIN, vBat);
            break;
        default:
            ESP_LOGE(TAG, "Failed to set payload message because of unexpected wake-up reason");
            break;
    }
}

static void LoRa_MakeTestPacket(char *strPayload)
{
    strcpy(strPayload, "CONSELHODOSMAGOS");
}

void LoRa_SetInitialOpModeVariable(u1_t initOp)
{
    initialOpMode = initOp;
}

static void LoRa_SetTxConfig(tx_config_e mode)
{
    switch(mode)
    {
        //NOTE: All bandwidths are set to 125kHz since only channel 9 is activated
        case TX_MODE_1:
            // TTN uses SF9 for its RX2 window.
            LMIC.dn2Dr = DR_SF10;
            // Set data rate and transmit power for uplink
            LMIC_setDrTxpow(DR_SF10, 14);
            break;
        case TX_MODE_2:
            // TTN uses SF9 for its RX2 window.
            LMIC.dn2Dr = DR_SF7;
            // Set data rate and transmit power for uplink
            LMIC_setDrTxpow(DR_SF7, 14);
            break;
        case TX_MODE_3:
            // TTN uses SF9 for its RX2 window.
            LMIC.dn2Dr = DR_SF10;
            // Set data rate and transmit power for uplink
            LMIC_setDrTxpow(DR_SF10, 7);
            break;
        case TX_MODE_4:
            // TTN uses SF9 for its RX2 window.
            LMIC.dn2Dr = DR_SF7;
            // Set data rate and transmit power for uplink
            LMIC_setDrTxpow(DR_SF7, 7);
            break;
        default:
            ESP_LOGE(TAG, "Unknown LoRa TX mode!");
            break;
    }
}