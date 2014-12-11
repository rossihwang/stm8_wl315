#ifndef __M315_H__
#define __M315_H__

#include "stm8s.h"
// #include "flash.h"
#include <stdbool.h>
#include <stdio.h>

//typedef
typedef enum rt_e_ {
    RT_ON_GOING = 0,
    RT_OK  = 1,
    RT_ERR  = 2,
}rt_e;

typedef struct wl315_data_t_{
    uint16_t hwPreamble;
    uint8_t chData;
} wl315_data_t;


//定时中断状态机状态
#define RECV_START      0
#define RECV_RESET      1
#define RECV_CHECK_SYN  2
#define RECV_CHECK_CODE   3
#define RECV_PRCSS      4

#define RST_RECV_FSM()   do{s_chRecvState = RECV_START;}while(0)

#define WL315_START()   do{TIM2_Cmd(ENABLE);}while(0)
#define WL315_STOP()    do{TIM2_Cmd(DISABLE);}while(0)

//M315状态常量
#define WL_RNE    0x01
//#define M315_VALID  0x02

//study mode constants
#define LEARN_KEY1  1
#define LEARN_KEY2  2
#define LEARN_KEY3  3
#define LEARN_KEY4  4

#define WLSTA_CHECK_RNE()  (g_chWlStatus & WL_RNE)
#define WLSTA_SET_RNE()   do {g_chWlStatus |= WL_RNE;} while(0)
#define WLSTA_CLEAR_RNE()  do {g_chWlStatus &= ~WL_RNE;} while(0)

extern uint16_t g_hwWlPreamble;
extern uint8_t g_chWlData;
extern uint8_t g_chWlStatus;


void wl315_init(void);
rt_e wl315_check_code(bool *bBitStatus);
rt_e wl315_check_syn(void);
bool wl315_get_data(wl315_data_t *ptM315Structure);
bool wl315_learn_task(void);

#endif