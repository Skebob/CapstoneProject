//#include "./inc/tm4c123gh6pm.h"
#include "inc/simcom_sim.h"
#include "inc/sensor_adc.h"
#include "inc/UART.h"
#include "inc/gpio.h"
#include <stdbool.h>
#include <stdint.h>

void init_periph(void){
	// sys init (system clock)
	SysCtlClockSet(SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL | SYSCTL_OSC_INT | SYSCTL_XTAL_16MHZ);
	volatile int x = 33;
	
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
	// Enable the GPIO pins for the LED (PF2) (PF1).
  GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_2);
	GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1);
	
	// ensures SIM7000A has powered on an UART init (init 2 wire UART on UART1)
	SIM7000_Init();
	
	// ADC init (init private sensor variables, enable PE0,1 as analog IN)
	sensor_init();
	
	// GPIO init (init internal pin map, enable sys periphs for port A,B,E,D)
	GPIO_Module_Init();
	
	delaySec(10);
}

//--------------------------------------------------------------------------------------
int output_pins[20];
extern int portPinMap[];
extern int portMap[];
extern char* portNameMap[];
char web_cmd_code[3];

int strtoint(char* str){
	int d2 = str[0] - '0';
	int d1 = str[1] - '0';
	int d0 = str[2] - '0';
	
	int result = (d2*100) + (d1 * 10) + d0;
}

int main(void){
	volatile int x = 1;
	
	// -------------- START CODE HERE -------------
	init_periph();
	int light;
	
	SIM7000_SyncBaud();
	
	SIM7000_MQTT_CONNECT();
	
	GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, 0);
	int humid = 0;
	
	int mcu_curr_state = 0; // 1 busy; 0 idle
	int mcu_next_state = 0;
	enum {LED_RED, LED_BLUE, LED_OFF};
	
	int desired_led = LED_OFF; // 0 off; 1 red; 2 blue
	
	while(1){
		if(SIM7000_MQTT_CHECK_CONNECT()){
//-------------------------------------------------------------		
//----------------------------IDLE-----------------------------		
			while(mcu_curr_state == 0){
				// check "Inputs"
				if(MQTT_CHECK_TOPIC_WEBSTATE("REQUESTING", 10)){
					// move onto busy state
					mcu_next_state = 1;
					// write MCU BUSY
					SIM7000_MQTT_PUBLISH_STATE("BUSY;");
					// do pins need to be read?
					if(MQTT_CHECK_TOPIC_WEBCMD("RED", 3)){
						desired_led = LED_RED;
					}
					if(MQTT_CHECK_TOPIC_WEBCMD("BLUE", 4)){
						desired_led = LED_BLUE;
					}
					if(MQTT_CHECK_TOPIC_WEBCMD("OFF", 3)){
						desired_led = LED_OFF;
					}
					mcu_next_state = 1;
				} else {
					SIM7000_MQTT_PUBLISH_STATE("IDLE;");
					int pins_to_read = GPIO_getOutputPins(output_pins);
					if(pins_to_read > 0){
						// read and publish pins
						for(int i = 0; i < pins_to_read; i++){
							int curr_pin = output_pins[i];
							int val = GPIO_readPin(curr_pin);
							SIM7000_MQTT_PUBLISH_4INT(val, 5, portNameMap[curr_pin]);
						}
					}
				}
				mcu_curr_state = mcu_next_state;
			}
//-------------------------------------------------------------		
//----------------------------BUSY-----------------------------		
			while(mcu_curr_state == 1){
				// get code
				bool response = false;
				delaySec(1);
				
				if(desired_led == LED_OFF){
					GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, 0); // off
					GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, 0); // on
				}
				if(desired_led == LED_RED){
					GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, GPIO_PIN_1); // red
				}
				if(desired_led == LED_BLUE){
					GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, GPIO_PIN_2); // blue
				}
				
				SIM7000_MQTT_PUBLISH_STATE("IDLE;");
				
				mcu_next_state = 0;
				
				mcu_curr_state = mcu_next_state;
			}
//-------------------------------------------------------------		
		}
		// No longer connected if here
	}
}
