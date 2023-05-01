#include "gpio.h"
#include "sensor_adc.h"

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
#define NUMPINS_PORTA 6
#define NUMPINS_PORTB 8
#define NUMPINS_PORTE 6
#define TOTALPINS 20

int portpinMap[20] = {
2,
3,
4,
5,
6,
7,
0,
1,
2,
3,
4,
5,
6,
7,
0,
1,
2,
3,
4,
5
}; 
int portMap[20] = {
1,
1,
1,
1,
1,
1,
2,
2,
2,
2,
2,
2,
2,
2,
3,
3,
3,
3,
3,
3
};
char* portNameMap[] = {
	"PA2;",
	"PA3;",
	"PA4;",
	"PA5;",
	"PA6;",
	"PA7;",
	"PB0;",
	"PB1;",
	"PB2;",
	"PB3;",
	"PB4;",
	"PB5;",
	"PB6;",
	"PB7;",
	"PE0;",
	"PE1;",
	"PE2;",
	"PE3;",
	"PE4;",
	"PE5;",
};

// 0			PA2
// 1			PA3
// 2			PA4
// 3 			PA5
// 4			PA6
// 5			PA7
// 6			PB0
// 7 			PB1
// 8			PB2
// 9			PB3
// 10			PB4
// 11			PB5
// 12			PB6
// 13			PB7
// 14			PE0 (DO NOT USE)
// 15			PE1
// 16			PE2
// 17			PE3
// 18 		PE4
// 19			PE5
int pins[TOTALPINS];

// D / A		I / O(high) O(low) 	diabled
// 1   2    1   	2				4				0
//
// 11 DI
// 12 DO
// 21 AI (not used)
// 22 AO
// EX: pins[16] == 12 means PE2 digital 
void GPIO_PinMapInit(void){
	for(int pin = 0; pin < TOTALPINS; pin++){
		if(pin == PE0){
			pins[pin] = 21;
		} else {
			pins[pin] = 0;
		}
	}
}

int GPIO_getOutputPins(int* src){
	int j = 0;
	for(int i = 0; i < TOTALPINS; i++){
		if((pins[i] % 2)==1){
			src[j] = i;
			j++;
		}
	}
	return j;
}

void GPIO_setPin(int pin, direction_t dir, signal_t type){
	int code = 0;
	int pinmap_idx = 0;
	
	if(type == DIGITAL){
		code += 10;
	} else {
		code += 20;
	}
	
	if(dir == INPUT){
		code += 1;
	} else {
		code += 2;
	}
	
	pins[pin] = code;
	
	int portpin = portpinMap[pin];
	int GPIO_PIN = 0x00000001;
	GPIO_PIN = GPIO_PIN << portpin;
	
	int port_num = portMap[pin];
	int GPIO_BASE = -1;
	if(port_num == 1){
		GPIO_BASE = GPIO_PORTA_BASE;
	} else if(port_num == 2){
		GPIO_BASE = GPIO_PORTB_BASE;
	} else if(port_num == 3){
		GPIO_BASE = GPIO_PORTE_BASE;
	}
	
	if(code == 11){
		//DIGITAL OUTPUT
		GPIOPinTypeGPIOOutput(GPIO_BASE, GPIO_PIN);
		GPIOPinWrite(GPIO_BASE, GPIO_PIN, GPIO_PIN);
	} else if(code == 12){
		//DIGITAL INPUT
		GPIOPinTypeGPIOInput(GPIO_BASE, GPIO_PIN);
	}
}

int GPIO_readPin(int pin){
	int portpin = portpinMap[pin];
	int GPIO_PIN = 0x00000001;
	GPIO_PIN = GPIO_PIN << portpin;
	
	int port_num = portMap[pin];
	int GPIO_BASE = -1;
	if(port_num == 1){
		GPIO_BASE = GPIO_PORTA_BASE;
	} else if(port_num == 2){
		GPIO_BASE = GPIO_PORTB_BASE;
	} else if(port_num == 3){
		GPIO_BASE = GPIO_PORTE_BASE;
	}
	
	int ret = 0;
	if(pin == PE0){
		ret = sensor_get(1);
	} else {
		ret = GPIOPinRead(GPIO_BASE,GPIO_PIN);
	}

	return ret;
}

// private to this file 
// 	code:	000 not configured				(0)
//				100 configured -> DIG O		(4)
//				101 configured -> DIG I		(5)
//				111 configured -> ANA I		(7)
// code for each pin to have quick way of knowing pins status

static uint8_t PortAMapCode[NUMPINS_PORTA]; 
static uint8_t PortBMapCode[NUMPINS_PORTB];
static uint8_t PortEMapCode[NUMPINS_PORTE];
void GPIO_PinMapInit1(void){
	for(int i = 0; i < NUMPINS_PORTA; i++){
		PortAMapCode[i] = 0;
	}
	for(int i = 0; i < NUMPINS_PORTB; i++){
		PortBMapCode[i] = 0;
	}
	for(int i = 0; i < NUMPINS_PORTE; i++){
		PortEMapCode[i] = 0;
	}
}

// ASSUMES CALLER WILL PROVIDE CORRECT PIN, DRECTION, SIG, AND ENABLE SETTINGS 
// (EX, call to port:B pin:1 sig:analog dir:output can be called but will hard fault)!!!!!!!!!!!!!
void GPIO_PinMapConfigure(port_t port, int pin, direction_t dir, signal_t sig, bool enable){
	int code = 0; // code: zzzzz000 <- LSB; code is 8 bit int but we only need to look at 3 (LSB -> b0 b1 b2)
	
	if(enable){
		code += 4; // code: 1xx | 0xx // modify bit 3
	}

	if(sig == ANALOG){
		code += 2; // code: x1x | x0x // modify bit 2
	}
	
	if(dir == INPUT){
		code += 1; // code: xx1 | xx0 // modify bit 1
	}

	if(port == PORTA){
		// port A 2-7 
		PortAMapCode[pin] = code;
	}
	else if(port == PORTB){
		// port B 0-7 (PB3 analog for sensors)
		PortBMapCode[pin] = code;
	}
	else if(port == PORTE){
		// port E 0-5 (PE0,1,2 analog for sensors)
		PortEMapCode[pin] = code;
	}
	else{
		// not valid port
		code = 9;
	}
}

uint32_t PinTypeToPinMask(int pin){
	uint32_t pin_ofst = 0x00000001;
	
	pin_ofst = pin_ofst << pin;
	
	return pin_ofst;
}

uint32_t PortTypeToPortBase(port_t port){
	uint32_t portbase = NULL;
	
	if(port == PORTA){
		portbase = GPIO_PORTA_BASE;
	}
	else if(port == PORTB){
		portbase = GPIO_PORTB_BASE;
	}
	else if(port == PORTE){
		portbase = GPIO_PORTE_BASE;
	}
	else if(port == PORTD){
		portbase = GPIO_PORTD_BASE;
	} else {
		portbase = GPIO_PORTA_BASE;
	}
	
	return portbase;
}

uint8_t* PortTypeToPortPtr(port_t port){
	uint8_t* portptr = NULL;
	
	if(port == PORTA){
		portptr = PortAMapCode;
	}
	else if(port == PORTB){
		portptr = PortBMapCode;
	}
	else if(port == PORTE){
		portptr = PortEMapCode;
	} else {
		portptr = PortAMapCode;
	}
	
	return portptr;
}

void GPIO_HardwareConfigure(port_t port, int pin){
	
	uint8_t* port_map = PortTypeToPortPtr(port);
	uint8_t code = port_map[pin];
	
	int port_base = PortTypeToPortBase(port);
	int pin_mask = PinTypeToPinMask(pin);
	
	if(code == 0){
		// disable
	}
	else if(code == 4){
		// digital output
		GPIOPinTypeGPIOOutput(port_base, pin_mask);
	}
	else if(code == 5){
		// digtal input
		GPIOPinTypeGPIOInput(port_base, pin_mask);
	}
	else if(code == 6){
		// analog input
		// TODO: variable ADC configure
	} else {
		// wtf is code?
	}
}

void GPIO_Module_Init(void){
	GPIO_PinMapInit();
	//PA2-7 
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
	while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOA)){};
	//PB0-7 (PB3 analog)
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
	while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOB)){};
	//PD1-3 (DTR, PWR_EN, GNS_EN)
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
	while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOD)){};
	
	GPIO_PinMapInit();
}