#ifndef UART_H__
#define UART_H__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "driverlib/uart.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/rom.h"
#include "driverlib/pin_map.h"

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_ints.h"

#define CR_ascii 0x0D
#define NL_ascii 0x0A

bool msg_is_this(const char* src, int len);

bool buffer_is_topic_webcmd(void);

char bufferGet(int i);

bool UART1_Send_Receive(const char* send);

bool isModemSync(void);

bool UART1_CMD(const char* cmd, uint16_t* response);

bool UART1_CMD_POLL(const char* cmd);

// WRITES TO UART HAPPEN IN BATCHES OF 8 bits (1 byte)
bool UART1_write(uint8_t data); // single write

bool UART1_write_BLK(const uint8_t* src, int len); // {len} number of writes // FUNCTION CAN SPIN HERE

void UART1_init(int baudrate);

bool str_cmp(const char* s1, const char* s2, int len);

typedef enum {
	resp_OK,
	resp_ERROR,
	resp_RDY,
	resp_SMSUB,
	resp_CMD_ECHO,
} sim_resp_t;

void UART1_WaitForResp(sim_resp_t waitingfor);

bool UART1_AT_CMD(const char* cmd, const char* parse_target, int target_len, int waittime);

void delayMS(int x);

void delaySec(int x);

#endif
