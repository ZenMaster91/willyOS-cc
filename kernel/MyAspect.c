#include "Clock.h"
#include "Device.h"

before(): execution(void Processor_FetchInstruction()) {
	Clock_Update();
}

before(): execution(void OperatingSystem_InterruptLogic(int)){
	Clock_Update();
}

after(): execution(void Processor_DecodeAndExecuteInstruction()) {
	Device_UpdateStatus();
}
