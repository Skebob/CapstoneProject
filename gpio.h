#ifndef __GPIO_H_
#define __GPIO_H_

#define PA2 0
#define PA3 1
#define PA4 2
#define PA5 3
#define PA6 4
#define PA7 5
#define PB0 6
#define PB1 7
#define PB2 8
#define PB3 9
#define PB4 10
#define PB5 11
#define PB6 12
#define PB7 13
#define PE0 14
#define PE1 15
#define PE2 16
#define PE3 17
#define PE4 18
#define PE5 19

typedef enum{
PORTA, PORTB, PORTE, PORTD
}
port_t;

typedef enum{
INPUT, OUTPUT
}
direction_t;

typedef enum{
DIGITAL, ANALOG
}
signal_t;

void GPIO_Module_Init(void);

int GPIO_getOutputPins(int* src);

void GPIO_setPin(int pin, direction_t dir, signal_t type);

int GPIO_readPin(int pin);

#endif