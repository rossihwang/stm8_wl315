
#include "stm8s.h"
#include "wl315.h"

#define BAUD_RATE  9600

void uart_init(void);
static bool uart_write_byte(uint8_t chByte);
bool uart_print_task(void);