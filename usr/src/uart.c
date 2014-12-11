
#include "uart.h"

#define UART_START  0
#define UART_WAIT   1
#define UART_PRNT   2

#define RST_UART_FSM()  do {s_chUartState = UART_START;} while(0)

extern wl315_data_t tKey1, tKey2, tKey3, tKey4;

void uart_init(void) {
    
    UART1_Init(BAUD_RATE, UART1_WORDLENGTH_8D, UART1_STOPBITS_1, UART1_PARITY_NO, \
               UART1_SYNCMODE_CLOCK_DISABLE, UART1_MODE_TXRX_ENABLE);
    //UART1_ITConfig(UART1_IT_RXNE_OR, ENABLE);
    UART1_Cmd(ENABLE);
}

static bool uart_write_byte(uint8_t chByte) {
    if (UART1_GetFlagStatus(UART1_FLAG_TXE) == SET) {
        UART1_SendData8(chByte);
        return false;
    } else {
        return true;
    }
}

bool uart_print_task(void) {
    
    static uint8_t s_chUartState = UART_START;
    static uint8_t s_chKeyNum;
    wl315_data_t tWl315Tmp;
        
    switch(s_chUartState) {
        case UART_START:
            s_chUartState = UART_WAIT;
            break;
        case UART_WAIT:
            if (!wl315_get_data(&tWl315Tmp)) {
                if (tWl315Tmp.hwPreamble == tKey1.hwPreamble && tWl315Tmp.chData == tKey1.chData) {
                    s_chKeyNum = 0;
                } else if (tWl315Tmp.hwPreamble == tKey2.hwPreamble && tWl315Tmp.chData == tKey2.chData) {
                    s_chKeyNum = 1;
                } else if (tWl315Tmp.hwPreamble == tKey3.hwPreamble && tWl315Tmp.chData == tKey3.chData) {
                    s_chKeyNum = 2;
                } else if (tWl315Tmp.hwPreamble == tKey4.hwPreamble && tWl315Tmp.chData == tKey4.chData) {
                    s_chKeyNum = 3;
                } else {
                    RST_UART_FSM();
                    break;
                }
                s_chUartState = UART_PRNT;
            }
            break;
        case UART_PRNT:
            if (!uart_write_byte('0' + s_chKeyNum)) {
                RST_UART_FSM();
                return false;
            }
    }
    return true;
}