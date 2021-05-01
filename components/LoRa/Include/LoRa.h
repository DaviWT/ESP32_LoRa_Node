#ifndef MAIN_LORA_H_
#define MAIN_LORA_H_

#include "TheThingsNetwork.h"
#include "esp_event.h"
#include "freertos/FreeRTOS.h"
#include "stdint.h"

#define CHANNEL_NUM 1

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

bool LoRa_NodeInit();
bool LoRa_ModemPinoutInit();
void LoRa_SelectChannel(uint8_t channel_number);
void LoRa_ConfigTTNKeys();
void LoRa_ConfigTTNKeys_ABP();
void LoRa_SetMessageRxCallback();
bool LoRa_JoinTTN();

void taskLoRaTX(void *pvParameter);

#endif /* MAIN_LORA_H_ */
