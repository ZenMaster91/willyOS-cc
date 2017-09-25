#ifndef DISPOSITIVO_H
#define DISPOSITIVO_H

// #define IOLAPTIME 7

enum DeviceStatus {FREE, BUSY};

typedef struct {
	int info;
	int IOEndTick;
} IODATA;


// Prototipos de las funciones
void Device_UpdateStatus();
void Device_PrintIOResult();
void Device_StartIO(int);
int Device_GetStatus();
void Device_Initialize(char *, int);

#endif
