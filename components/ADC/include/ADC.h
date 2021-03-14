#ifndef ADCP_H_
#define ADC_H_

#include "driver/gpio.h"
#include "esp_event.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"

void taskAdc(void *pvParameter);

#endif /* ADC_H_ */
