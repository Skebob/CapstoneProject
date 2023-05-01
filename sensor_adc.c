#include "sensor_adc.h"
uint32_t buffer[4];
uint32_t dig_val;

void sensor_init(void){	
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
	SysCtlPeripheralReset(SYSCTL_PERIPH_GPIOE);
	GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_0);
	
	SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);
	while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOE)){};
	while(!SysCtlPeripheralReady(SYSCTL_PERIPH_ADC0)){};

	ADCSequenceConfigure(ADC0_BASE, 2, ADC_TRIGGER_PROCESSOR, 0);
	ADCSequenceStepConfigure(ADC0_BASE, 2, 0, ADC_CTL_END | ADC_CTL_IE | ADC_CTL_CH3);
	ADCSequenceEnable(ADC0_BASE, 2);
	
	ADCIntClear(ADC0_BASE, 2);
}

void sensor_read(void){
	// Trigger the sample sequence.
	//
	ADCProcessorTrigger(ADC0_BASE, 2);
	//
	// Wait until the sample sequence has completed.
	//
	while(!ADCIntStatus(ADC0_BASE, 2, false)){};
	//
	// Read the value from the ADC.
	//
	ADCSequenceDataGet(ADC0_BASE, 2, buffer);
}

// sensor get only return PE0 result
int sensor_get(int sensorID){
	int ret = 0;
	
	sensor_read();
	
	if(sensorID == 0){
		ret = dig_val;
	}
	else if(sensorID == 1){
		ret = buffer[0];
	}
	else if(sensorID == 2){
		ret = buffer[0];
	}
	else{
		ret = buffer[0];
	}
	
	return ret;
}