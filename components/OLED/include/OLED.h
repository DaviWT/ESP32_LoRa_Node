#ifndef OLED_H_
#define OLED_H_

#include "driver/gpio.h"

#define OLED_SDA_PIN GPIO_NUM_4
#define OLED_SCL_PIN GPIO_NUM_15
#define OLED_RST_PIN GPIO_NUM_16

extern "C" void OLED_Init(gpio_num_t sdaPin, gpio_num_t sclPin, gpio_num_t rstPin);
extern "C" bool OLED_EnterSleep();

#endif /* OLED_H_ */
