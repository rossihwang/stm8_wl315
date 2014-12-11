
#include "wl315.h"
#include "flash.h"

// Constant
#define MAX_CODE_CNT   18

#define MIN_SYN_CNT    110
#define MAX_SYN_CNT    170

// Define
#define WL315_READ()    ((GPIOC->IDR & 0x08) ? true : false)   // PC3
#define LED(_STA_)          GPIO_Write##_STA_##(GPIOD, GPIO_PIN_4)     // LED(High) : LED on , LED(Low) : LED off 
#define LED_REV()       do{GPIOD->ODR ^= 0x10;}while(0)        // PD4
#define KEY_READ()      ((GPIOC->IDR & 0x10) ? true : false)   // PC4

////////// wl315_check_code FSM //////////
#define CODE_START    0
#define CODE_RST      1
#define CODE_HI_LVL   2
#define CODE_LO_LVL   3

#define RST_CODE_FSM()   do{s_chCodeState = CODE_START;} while(0)
/////////////////////////////////////////

////////// wl315_check_syn FSM //////////
#define SYN_START   0
#define SYN_HEAD    1
#define SYN_WAIT    2

#define RST_SYN_FSM()   do{s_chSynState = SYN_START;}while(0)
//////////////////////////////////////////

////////// Learn Key FSM //////////
#define LEARN_START      0
#define LEARN_MODE       1
#define LEARN_WAIT      2

#define RST_LEARN_FSM()   do{s_chLearnState = LEARN_START;} while(0)
/////////////////////////////////////////

////////// LED Blink FSM //////////
#define BLINK_START  0
#define BLINK_REV    1
#define BLINK_S_DLY  2
#define BLINK_L_DLY  3

#define RST_BLINK_FSM()   do{s_chBlinkState = BLINK_START;} while(0)
//////////////////////////////////

////////// LED Delay FSM //////////
#define DLY_START  0
#define DLY_CNT    1

#define RST_DLY_FSM()   do{s_chDelayState = DLY_START;} while(0)
//////////////////////////////

// Variable
uint16_t g_hwWlPreamble;
uint8_t g_chWlData;
uint8_t g_chWlStatus;

// Private Function
static bool wl315_indicator(uint8_t chKeyNum);
static bool wl315_keys_update(uint8_t chKeyNum, wl315_data_t *ptKeyPreSet);

static void tim2_config(void) {
    
    TIM2_TimeBaseInit(TIM2_PRESCALER_16, (83 - 1)); // about 12000hz
    TIM2_PrescalerConfig(TIM2_PRESCALER_16, TIM2_PSCRELOADMODE_IMMEDIATE);
    TIM2_ARRPreloadConfig(ENABLE);
    TIM2_ITConfig(TIM2_IT_UPDATE, ENABLE);
}

void wl315_init(void) {
  
    tim2_config();
    
    GPIO_Init(GPIOC, GPIO_PIN_3, GPIO_MODE_IN_PU_NO_IT); //315m Data Read
    
    GPIO_Init(GPIOC, GPIO_PIN_4, GPIO_MODE_IN_PU_NO_IT);     // User Button
    GPIO_Init(GPIOD, GPIO_PIN_4, GPIO_MODE_OUT_PP_LOW_FAST); // Indicator
}

rt_e wl315_check_code(bool *bBitStatus) {
    
    static uint8_t s_chCodeState = CODE_START;
    static uint8_t s_chHiLvlCnt, s_chLoLvlCnt;
    
    if(bBitStatus != NULL) {
        switch(s_chCodeState) {
            case CODE_START:
                s_chHiLvlCnt = 0;
                s_chLoLvlCnt = 0;
                s_chCodeState = CODE_HI_LVL;
                break;
            case CODE_HI_LVL:
                if(WL315_READ() == true) {
                    s_chHiLvlCnt++;   //累加高电平次数
                } else {
                    if(s_chHiLvlCnt > MAX_CODE_CNT) { //高电平时间过长   
                        RST_CODE_FSM();
                        return RT_ERR;
                    } else {
                        s_chCodeState = CODE_LO_LVL;
                    }
                }
                break;
            case CODE_LO_LVL:
                if(WL315_READ() == true) {
                    if(s_chLoLvlCnt > MAX_CODE_CNT) {  //低电平时间过长  
                        RST_CODE_FSM();
                        return RT_ERR;
                    } else {
                        if(s_chHiLvlCnt > s_chLoLvlCnt) { // 判断数据类型
                            *bBitStatus = false;
                        } else {
                            *bBitStatus = true;
                        }
                        RST_CODE_FSM();
                        return RT_OK;
                    }              
                } else {
                    s_chLoLvlCnt++;     //累加低电平次数
                }
                break;     
        }
    } else {
        return RT_ERR;
    }
    
    return RT_ON_GOING;
}

rt_e wl315_check_syn(void) {
    
    static uint8_t s_chSynState = SYN_START;
    static uint8_t s_chSynCnt;
    
    switch(s_chSynState) {
        case SYN_START:
            s_chSynCnt = 0;
            s_chSynState = SYN_HEAD;
            break;
        case SYN_HEAD:
            if(WL315_READ() == true) {
                s_chSynState = SYN_WAIT;
            }
            break;
        case SYN_WAIT:
            if(WL315_READ() == false) {
                s_chSynCnt++;
                if(s_chSynCnt > MAX_SYN_CNT) {
                    RST_SYN_FSM();
                }
            } else {
                RST_SYN_FSM();
                if((s_chSynCnt >= MIN_SYN_CNT) && (s_chSynCnt <= MAX_SYN_CNT)) {
                    return RT_OK;
                }
            }
            break;
    }
    return RT_ON_GOING;
}

bool wl315_get_data(wl315_data_t *ptWl315Structure) {
    
    static uint16_t s_hwPrePreamble;
    static uint8_t s_chPreData;
    
    if(WLSTA_CHECK_RNE()) {
        WLSTA_CLEAR_RNE();
        if((s_hwPrePreamble == g_hwWlPreamble) && (s_chPreData == g_chWlData)) { // the result is the same as the last time
            ptWl315Structure->hwPreamble = s_hwPrePreamble;
            ptWl315Structure->chData = s_chPreData;
            s_hwPrePreamble = 0;   //clear history
            s_chPreData = 0;
            return false;
        } else {
            s_hwPrePreamble = g_hwWlPreamble;  // save the result
            s_chPreData = g_chWlData;
        }    
    }
    return true;
}

bool wl315_learn_task(void) {
    
    static uint8_t s_chLearnState;
    static uint32_t s_wLearnCnt;
    static uint8_t s_chLearnKey;
    
    wl315_data_t tWlData;
    
    switch(s_chLearnState) {
        case LEARN_START:
            s_wLearnCnt = 0;
            s_chLearnKey = 0;
            s_chLearnState = LEARN_MODE;
            break;
        case LEARN_MODE:
            if(!KEY_READ()) {
                s_wLearnCnt++;
                if((s_wLearnCnt % 50000 == 0) && (s_wLearnCnt >= 100000)) {  // Study Key to control the LED
                    LED_REV();
                }
            } else if (s_wLearnCnt > 100000){
                if(s_wLearnCnt < 200000) { // Choose modes 
                    s_chLearnKey = LEARN_KEY1;
                } else if(s_wLearnCnt < 300000) {
                    s_chLearnKey = LEARN_KEY2;
                } else if(s_wLearnCnt < 400000) {
                    s_chLearnKey = LEARN_KEY3;
                } else if(s_wLearnCnt < 500000) {
                    s_chLearnKey = LEARN_KEY4;
                } else {
                    RST_LEARN_FSM();
                }
                
                if(s_chLearnKey != 0) {
                    WL315_START(); //start 315m module
                    LED(Low);
                    s_chLearnState = LEARN_WAIT;
                }
            }  
            break;
        case LEARN_WAIT:
            // maybe need a timeout in this state
            wl315_indicator(s_chLearnKey);  // LED blink task 
            
            // wait for the 315m message
            if(!wl315_get_data(&tWlData)) {
                flash_write_key(s_chLearnKey, &tWlData); //write the key to flash
                // maybe need a check here
                wl315_keys_update(s_chLearnKey, &tWlData);
                RST_LEARN_FSM();
                return false;
            }     
    }    
    return true;
}

bool wl315_delay (uint32_t wWait) {
    static uint32_t s_wDelayCnt, s_wDelayDst;
    static uint8_t s_chDelayState;
    
    switch(s_chDelayState) {
        case DLY_START:
            s_wDelayCnt = 0;
            s_wDelayDst = wWait;
            s_chDelayState = DLY_CNT;
        case DLY_CNT:
            s_wDelayCnt++;
            if(s_wDelayCnt > s_wDelayDst) {
                RST_DLY_FSM();
                return false;
            }
    }
    return true;
}

bool wl315_indicator (uint8_t chKeyNum) {

    static uint8_t s_chBlinkCnt;
    static uint8_t s_chBlinkState;
    
    switch(s_chBlinkState) {
        case BLINK_START:
            s_chBlinkCnt = chKeyNum << 1;  // Parameter to control the LED flash times
            s_chBlinkState = BLINK_REV;
            break;
        case BLINK_REV:
            LED_REV();
            s_chBlinkCnt--;
            if(0 == s_chBlinkCnt) {
                s_chBlinkState = BLINK_L_DLY;
            } else {
                s_chBlinkState = BLINK_S_DLY;
            }
            break;
        case BLINK_S_DLY: //short delay
            if(!wl315_delay(9000)) {
                s_chBlinkState = BLINK_REV;
            }
            break;
        case BLINK_L_DLY: //long delay
            if(!wl315_delay(90000)) {
                RST_BLINK_FSM();
                return false;
            }
    }
    return true;
}

bool wl315_keys_update(uint8_t chKeyNum, wl315_data_t *ptKeyPreSet) {
    
    if (ptKeyPreSet == NULL) {
        flash_read_key(chKeyNum, ptKeyPreSet);  
        return true;
    }
    return false;
}

