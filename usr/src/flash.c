#include "flash.h"
#include "stdbool.h"
#include "wl315.h"

/*
---------------------------
0x00004000      device address high byte
0x00004001      device address low byte

0x00004002      high byte of the preamble of the key1
0x00004003      low byte of the preamble of the key1
0x00004004      data of the key1

0x00004005      high byte of the preamble of the key2
0x00004006      low byte of the preamble of the key2
0x00004007      data of the key2

0x00004008      high byte of the preamble of the key3
0x00004009      low byte of the preamble of the key3
0x0000400A      data of the key3

0x0000400B      high byte of the preamble of the key4
0x0000400C      low byte of the preamble of the key4
0x0000400D      data of the key4
*/

uint16_t g_hwDeviceAdd;

void flash_init(void) {
    FLASH_SetProgrammingTime(FLASH_PROGRAMTIME_STANDARD);
}

void flash_read_addr(uint16_t *Preamble) {

    FLASH_Unlock(FLASH_MEMTYPE_DATA); 
    *Preamble = FLASH_ReadByte(0x00004000);
    *Preamble <<= 8;
    *Preamble |= FLASH_ReadByte(0x00004001);
    FLASH_Unlock(FLASH_MEMTYPE_DATA);
}

void flash_write_addr(uint16_t Preamble) {

    FLASH_Unlock(FLASH_MEMTYPE_DATA); 
    FLASH_ProgramByte(0x00004000, (uint8_t)(Preamble >> 8));//ÏÈ´æ¸ß°ËÎ»
    FLASH_ProgramByte(0x00004001, (uint8_t)Preamble);
    FLASH_Unlock(FLASH_MEMTYPE_DATA);
}

void flash_write_key(uint8_t chKeyNum, wl315_data_t *ptM315Structure) {
    FLASH_Unlock(FLASH_MEMTYPE_DATA);
    FLASH_ProgramByte(0x00004002 + (chKeyNum * 3), (uint8_t)(ptM315Structure->hwPreamble >> 8));
    FLASH_ProgramByte(0x00004002 + (chKeyNum * 3) + 1, (uint8_t)(ptM315Structure->hwPreamble));
    FLASH_ProgramByte(0x00004002 + (chKeyNum * 3) + 2, ptM315Structure->chData);
    FLASH_Unlock(FLASH_MEMTYPE_DATA);
}

/***********************************************************************************************************
* Func   Name: void Flash_ReadKey(uint8_t chKeyNum, m315_t *ptM315Structure)
* Input  Para: uint8_t chKeyNum, m315_t *ptM315Structure
* Return Para: void  
* Description: read preamble and value of a key, chKeyNum from 0 to 3
* Writed By   : rossih
* Date       : 2014/6/4
************************************************************************************************************/
void flash_read_key(uint8_t chKeyNum, wl315_data_t *ptM315Structure) {
    
    uint16_t hwTmp;
    FLASH_Unlock(FLASH_MEMTYPE_DATA);
    hwTmp = FLASH_ReadByte(0x00004002 + (chKeyNum * 3));
    ptM315Structure->hwPreamble = hwTmp << 8 | FLASH_ReadByte(0x00004002 + (chKeyNum * 3) + 1);
    ptM315Structure->chData = FLASH_ReadByte(0x00004002 + (chKeyNum * 3) + 2);
    FLASH_Unlock(FLASH_MEMTYPE_DATA);
}
