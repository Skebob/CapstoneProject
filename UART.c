#include "UART.H"

// !!!!!!!!!!DO NOT USE UART0!!!!!!!!!!!

uint16_t rdy_ok[9] = {'A','T',0x0D,0x0D,0x0A,'O','K',0x0D,0x0A};

#define READ_BUFF_SIZE 100

#define LINES 2
#define TOPIC_MAX 20
#define MSG_MAX 20

// reads to here should occur after AT+SMPUB to ensure most recent data is posted
char BUFFER_TOPIC[TOPIC_MAX];
int topic_len;
char BUFFER_MSG[MSG_MAX];
int msg_len;

// if topic == true then put c into topic buffer at i
// else put c into msg buffer at i
void bufferPut(char c, int i, bool topic){ 
	if(i >= TOPIC_MAX){
		i = TOPIC_MAX - 1;
	}
	if(topic){
		BUFFER_TOPIC[i] = c;
	} else {
		BUFFER_MSG[i] = c;
	}
}

char bufferGet(int i){
	char c = BUFFER_MSG[i];
	return c;
}

bool buffer_is_topic_webcmd(void){
	bool ret = ((BUFFER_TOPIC[8] == 'c') && (BUFFER_TOPIC[9] == 'm') && (BUFFER_TOPIC[10] == 'd'));
	return ret;
}

bool msg_is_this(const char* src, int len){
	bool ret = false;
	if(len != msg_len){
		return ret;
	}
	
	return str_cmp(src, BUFFER_MSG, len);
}

bool modem_sync = false;

bool mcu_waiting_ok = false;

bool modem_mqtt_connected = false;

// [x]ms delay
void delayMS(int x){
	while(x > 0){
		SysCtlDelay(SysCtlClockGet() / (1000 * 3)); // 1 ms delay
		x--; // delay for x ms 
	}
}

// [x]s delay
void delaySec(int x){
	while(x > 0){
		delayMS(1000);
		x--;
	}
}

bool isModemSync(void){
	return modem_sync;
}

bool uint16_str_cmp(const uint16_t* s1, const uint16_t* s2, int len){
	bool result = true;
	
	for(int i = 0; i < len; i++){
		if(s1[i] != s2[i]){
			result = false;
		}
	}
	
	return result;
}

bool str_cmp(const char* s1, const char* s2, int len){
	bool result = true;
	
	for(int i = 0; i < len; i++){
		if(s1[i] != s2[i]){
			result = false;
		}
	}
	
	return result;
}

// sends a command on UART1
bool UART1_Send_Receive(const char* send){
	bool success = false;
	
	int writes = 0;
	// send command characters over UART1 one by one
	while(send[writes] != ';'){
		success = UART1_write(send[writes]);
		if(success){
		// successful write of char, move onto next one
			writes++;
		}
	}
	while(ROM_UARTBusy(UART1_BASE)){}; // wait untill cmd fully sends before moving on
	
	UART1_write(CR_ascii); // send CR to designate end of command: 0x0D [CR]
	
	while(ROM_UARTBusy(UART1_BASE)){}; // wait untill CR fully sends before moving on
	
	return success;
}

bool UART1_write(uint8_t data){
	bool result = false;
	
	result = UARTCharPutNonBlocking(UART1_BASE, data);
	
	return result;
	
}

enum {
STATE_FOUND_NONE, 
STATE_FOUND_O, 
STATE_FOUND_PLUS, 
STATE_FOUND_PLUS_S,
STATE_FOUND_PLUS_SM,
STATE_FOUND_PLUS_SMS,
STATE_FOUND_PLUS_SMSU,
STATE_FOUND_PLUS_SMSUB,
STATE_FOUND_SMSUB,   
STATE_FOUND_TOPIC, 
STATE_FOUND_TOPIC_END, 
STATE_FOUND_MSG
};

int handler_state;
int topic_i = 0;
int msg_i = 0;
// need to find OK or +SMPUB: 
void UART1_Handler(void){
	uint32_t ui32Status;
	
	ui32Status = ROM_UARTIntStatus(UART1_BASE, true);
	
	ROM_UARTIntClear(UART1_BASE, ui32Status);
	
	// 0 - has not found 'O' 	has not found 'K' 
	// 1 - has found 'O'			has not found 'K'
	
	//GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_2, GPIO_HIGH_LEVEL);
	//GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_3, GPIO_HIGH_LEVEL);
		
	char data = 'Z';
	int reads = 0;
	
	bool handler_found_target = false;
	
	int handler_next_state = 0;
	
	// for STATE_FOUND_PLUS_S to parse for SMSUB: without adding explicit states
	char SMSUB_str[6] = {'S','M','S','U','B',':'};
	
	int smsub_str_matches = 0;
	//GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, GPIO_PIN_2);
	
	while(ROM_UARTCharsAvail(UART1_BASE)){
		data = (char)ROM_UARTCharGetNonBlocking(UART1_BASE);
		
		if(handler_state == STATE_FOUND_NONE){
			// look for first char O or +
			if(data == 'O'){
				// O
				handler_next_state = STATE_FOUND_O;
			} else if(data == '+'){
				// +
				handler_next_state = STATE_FOUND_PLUS;
				smsub_str_matches = 0;
			} else {
				// {X}
				handler_next_state = STATE_FOUND_NONE;
			}
		} else if(handler_state == STATE_FOUND_O){
			// found O last
			if(data == 'K'){
				// O, K
				handler_next_state = STATE_FOUND_NONE; // we found OK but we dont need a state for 'Found K', just reset the parser
				handler_found_target = true;
			} else {
				handler_next_state = STATE_FOUND_NONE;
			}
		} else if(handler_state == STATE_FOUND_PLUS){
			// found + last
			if(data == 'S'){
				handler_next_state = STATE_FOUND_PLUS_S;
			} else {
				// +, {X}
				handler_next_state = STATE_FOUND_NONE;
			}
		} else if(handler_state == STATE_FOUND_PLUS_S){
			// +, S
			if(data == 'M'){
				handler_next_state = STATE_FOUND_PLUS_SM;
			} else {
				handler_next_state = STATE_FOUND_NONE;
			}
		} else if(handler_state == STATE_FOUND_PLUS_SM){
			// + S M
			if(data == 'S'){
				handler_next_state = STATE_FOUND_PLUS_SMS;
			} else {
				handler_next_state = STATE_FOUND_NONE;
			}
		} else if(handler_state == STATE_FOUND_PLUS_SMS){
			// + S M S
 			if(data == 'U'){
				handler_next_state = STATE_FOUND_PLUS_SMSU;
			} else {
				handler_next_state = STATE_FOUND_NONE;
			}
		} else if(handler_state == STATE_FOUND_PLUS_SMSU){
			// + S M S U
			if(data == 'B'){
				handler_next_state = STATE_FOUND_PLUS_SMSUB;
			} else {
				handler_next_state = STATE_FOUND_NONE;
			}
		}	else if(handler_state == STATE_FOUND_PLUS_SMSUB){
			// + S M S U B
			if(data == ':'){
				GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, GPIO_PIN_2);
				handler_next_state = STATE_FOUND_SMSUB;
			} else {
				handler_next_state = STATE_FOUND_NONE;
			}
		} else if(handler_state == STATE_FOUND_SMSUB){
			// found +SMSUB: last, need to parse untill we see opening " for topic
			if(data == '\"'){
				// found opening quote of TOPIC
				//GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, GPIO_PIN_1);
				handler_next_state = STATE_FOUND_TOPIC;
				topic_i = 0;
				msg_i = 0;
			} else {
				// not found opening quote, keep looking
				handler_next_state = STATE_FOUND_SMSUB;
			}
		} else if(handler_state == STATE_FOUND_TOPIC){
			// parse topic chars until we see closing "
			if(data == '\"'){
				// found closing "
				handler_next_state = STATE_FOUND_TOPIC_END;
			} else {
				// not found closing quote, keep reading
				bufferPut(data, topic_i, true);
				topic_i++;
				handler_next_state = STATE_FOUND_TOPIC;
			}
		} else if(handler_state == STATE_FOUND_TOPIC_END){
			if(data == '\"'){
				// found opening quote of MSG
				handler_next_state = STATE_FOUND_MSG;
			} else {
				// not found opening quote, keep looking
				handler_next_state = STATE_FOUND_TOPIC_END;
			}
		} else if(handler_state == STATE_FOUND_MSG){
			if(data == '\"'){
				// found closing quote of MSG
				handler_next_state = STATE_FOUND_NONE;
				topic_len = topic_i;
				msg_len = msg_i;
				//GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, 0);
				//GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, 0);
			} else {
				// not closing opening quote, keep reading
				bufferPut(data, msg_i, false);
				msg_i++;
				handler_next_state = STATE_FOUND_MSG;
			}
		} else {
			handler_next_state = STATE_FOUND_NONE;
		}
		
		handler_state = handler_next_state;
		reads++;
	}
	handler_state = handler_next_state;
	
	if(handler_found_target == true){
		if(modem_sync == false){
			modem_sync = true;
		}
		if(mcu_waiting_ok == true){
			mcu_waiting_ok = false;
		}
	}
	
	//GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, 0);
}

void UART1_wait_for_modemresponse(void){
	while(mcu_waiting_ok == true){};
}

bool UART1_AT_CMD(const char* cmd, const char* parse_target, int target_len, int waittime){
	bool waitOK = false;
	
	if(target_len > 0){		
		mcu_waiting_ok = true;
	}
	
	UART1_Send_Receive(cmd);
	
	if(waitOK){
		UART1_wait_for_modemresponse();
	}
	
	delayMS(waittime);
}

// sends a command on UART1, does not expect a response
bool UART1_CMD_POLL(const char* cmd){
	bool sucess = false;
	
	int writes = 0;
	// send command characters over UART1 one by one
	while(cmd[writes] != ';'){
		sucess = UART1_write(cmd[writes]);
		if(sucess){
		// successful write of char, move onto next one
			writes++;
		}
	}
	UART1_write(CR_ascii); // send CR to designate end of command: 0x0D [CR]
	
	while(ROM_UARTBusy(UART1_BASE)){}; // wait untill cmd fully sends before moving on
	
	return sucess;
	
}

void UART1_init(int baudrate){
	//Enable UART1 and port C 
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);
	// WAIT for port C
	while(!ROM_SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOC)){}; // LOOP
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART1);
	// wait for UART0 module
	while(!ROM_SysCtlPeripheralReady(SYSCTL_PERIPH_UART1)){}; // LOOP
	
	// set PC4 as (Rx) and PC5 as (Tx)
	ROM_GPIOPinTypeUART(GPIO_PORTC_BASE, GPIO_PIN_5 | GPIO_PIN_4);
	
	ROM_GPIOPinConfigure(GPIO_PC4_U1RX);
  ROM_GPIOPinConfigure(GPIO_PC5_U1TX);
	GPIOPinTypeUART(GPIO_PORTC_BASE, GPIO_PIN_5 | GPIO_PIN_4);

	//enable processor/uart interrupts
	ROM_IntMasterEnable();
	ROM_IntEnable(INT_UART1);
  ROM_UARTIntEnable(UART1_BASE, UART_INT_RX | UART_INT_RT);

	// {baudrate} 8-N-1
	ROM_UARTConfigSetExpClk(
		UART1_BASE, 
		SysCtlClockGet(), 
		baudrate, // baud rate from parameter
		(UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE) // 8 data bits, one stop bit, no parity bits
	); 

	handler_state = STATE_FOUND_NONE;
	topic_len = 0;
	msg_len = 0;
}
