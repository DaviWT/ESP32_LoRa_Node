#ifndef ADCP_H_
#define ADC_H_

#include "driver/gpio.h"
#include "esp_event.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"

void ADC_ConfigAdc();
uint32_t ADC_GetVoltage();

#endif /* ADC_H_ */
