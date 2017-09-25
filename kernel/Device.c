// #include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "Device.h"
#include "ComputerSystem.h"
#include "Processor.h"


int registerStatus=FREE;
int ioLapTime=23;
char * deviceName;
IODATA registerIOData;


int Clock_GetTime();
void Processor_RaiseInterrupt(int);

void Device_Initialize(char * name, int delay) {
	int now=Clock_GetTime();
	deviceName = (char *) malloc((strlen(name)+1)*sizeof(char));
	strcpy(deviceName,name);
	if (delay>0)
		ioLapTime=delay;
	
	ComputerSystem_DebugMessage(4, DEVICE, now);
	ComputerSystem_DebugMessage(54,DEVICE,deviceName,ioLapTime);
}

void Device_UpdateStatus() {
#ifdef DEVICE
	if (registerStatus==BUSY)
		if (registerIOData.IOEndTick <= Clock_GetTime()) {
			// End of I/O
			Device_PrintIOResult();
			registerStatus=FREE;
			Processor_RaiseInterrupt(IOEND_BIT);
		}
#endif
}

void Device_PrintIOResult() {
	int now=Clock_GetTime();
	ComputerSystem_DebugMessage(4, DEVICE, now);
	// Show message ******** Device [deviceName] END Process I/O whit data [info] ********
	ComputerSystem_DebugMessage(53, DEVICE, deviceName, registerIOData.info);
}

void Device_StartIO(int value) {
#ifdef DEVICE
	int now=Clock_GetTime();
	ComputerSystem_DebugMessage(Processor_PSW_BitState(EXECUTION_MODE_BIT)?5:4,DEVICE,now);
  	// Show message [Device] starting Input/Output operation. Output value [value]
  	ComputerSystem_DebugMessage(50, DEVICE, deviceName, value);

	registerStatus=BUSY;
	registerIOData.info=value;
	registerIOData.IOEndTick= now + ioLapTime;
#endif
}

int Device_GetStatus() {
	return registerStatus;
}
