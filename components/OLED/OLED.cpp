#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE

#include "OLED.h"

#include "esp_log.h"
#include "ssd1306.h"

static const char *TAG = "OLED";

static SSD1306_t dev;
bool displayInitialized = false;

extern "C" void OLED_Init(gpio_num_t sdaPin, gpio_num_t sclPin, gpio_num_t rstPin)
{
    ESP_LOGI(TAG, "Initializing OLED diplay at I2C INTERFACE");
    i2c_master_init(&dev, sdaPin, sclPin, rstPin);

    displayInitialized = true;
}

extern "C" bool OLED_EnterSleep()
{
    if(!displayInitialized)
    {
        ESP_LOGE(TAG, "OLED display not initialized yet!");
        return false;
    }

    if(!i2c_display_off(&dev))
    {
        ESP_LOGE(TAG, "Unable to set OLED display to sleep mode!");
        return false;
    }

    return true;
}
