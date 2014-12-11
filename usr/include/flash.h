#ifndef __FLASH_H__
#define __FLASH_H__

#include "stm8s.h"
#include "wl315.h"

#define FLASH_DATA_START_PHSICAL_ADDRESS  0x4000

extern uint16_t g_hwDeviceAdd;

void flash_init(void);
void flash_read_addr(uint16_t *Preamble);
void flash_write_addr(uint16_t Preamble);

void flash_write_key(uint8_t chKeyNum, wl315_data_t *ptM315Structure);
void flash_read_key(uint8_t chKeyNum, wl315_data_t *ptM315Structure);

#endif