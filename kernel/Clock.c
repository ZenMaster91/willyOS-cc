#include "Clock.h"
#include "Processor.h"
#define INTERVALBETWEENINTERRUPS 5 

int tics=0;

void Processor_RaiseInterrupt();
void Device_UpdateStatus();

void Clock_Update() {
	tics++;
	if(Clock_GetTime()%INTERVALBETWEENINTERRUPS==0){
		Processor_RaiseInterrupt(CLOCKINT_BIT);
	}
	//Comprobamos si ya ha acabado la operación de entrada/salida
	Device_UpdateStatus();
}


int Clock_GetTime() {

	return tics;
}
