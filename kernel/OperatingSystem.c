#include "OperatingSystem.h"
#include "OperatingSystemBase.h"
#include "MMU.h"
#include "Processor.h"
#include "Buses.h"
#include "Heap.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <time.h>
#include "Device.h"

// Functions prototypes
void OperatingSystem_CreateDaemons();
void OperatingSystem_PCBInitialization(int, int, int, int, int);
void OperatingSystem_MoveToTheREADYState(int);
void OperatingSystem_Dispatch(int);
void OperatingSystem_RestoreContext(int);
void OperatingSystem_SaveContext(int);
void OperatingSystem_TerminateProcess();
int OperatingSystem_LongTermScheduler();
void OperatingSystem_PreemptRunningProcess();
int OperatingSystem_CreateProcess(USER_PROGRAMS_DATA);
int OperatingSystem_ObtainMainMemory(int, int);
int OperatingSystem_ShortTermScheduler();
int OperatingSystem_ExtractFromReadyToRun();
void OperatingSystem_HandleException();
void OperatingSystem_HandleSystemCall();
void OperatingSystem_PrintReadyToRunQueue();
void OperatingSystem_HandleClockInterrupt();
void OperatingSystem_MoveToTheBLOCKEDState();
void OperatingSystem_CheckPriority();
void OperatingSystem_PrintStatus();
int Processor_GetExceptionType();
int OperatingSystem_InitializePartitionTable();
int OperatingSystem_getBiggestPart();
void OperatingSystem_ReleaseMainMemory();
void OperatingSystem_AsignaMemoria(int,int);
int OperatingSystem_ThereArePrograms();
void OperatingSystem_HandleIOEndInterrupt();
void OperatingSystem_IOScheduler();
void OperatingSystem_DeviceControlerStartIOOperation();
int Device_GetStatus();
void Device_StartIO(int);
int QueueFIFO_add(int,int[],int *,int);
int QueueFIFO_poll(int[], int*);
void Device_Initialize(char *, int);
int OperatingSystem_DeviceControlerEndIOOperation();




//Estados de los procesos
char * statesNames [5]={"NEW","READY","EXECUTING","BLOCKED","EXIT"}; 

// The process table
PCB processTable[PROCESSTABLEMAXSIZE];

// Address base for OS code
int OS_address_base = PROCESSTABLEMAXSIZE * MAINMEMORYSECTIONSIZE;
// Identifier of the current executing process
int executingProcessID=NOPROCESS;

// Identifier of the System Idle Process
int sipID;

// Array that contains the identifiers of the READY processes
int readyToRunQueue[NUMBEROFQUEUES][PROCESSTABLEMAXSIZE];
int numberOfReadyToRunProcesses[NUMBEROFQUEUES];

// Variable containing the number of not terminated user processes
int numberOfNotTerminatedUserProcesses=0;

//Variable que controla si hay que crear un proceso de usuario o de sistema
int isDemon=0;
int numberOfClockInterrupts=0;

// Heap with blocked processes sort by when to wakeup
int sleepingProcessesQueue[PROCESSTABLEMAXSIZE];
int numberOfSleepingProcesses=0;

//Numero de particiones
int numberOfPartitions=0;
//Numero de particiones libres
int freePart;

//Cola FIFO
int IOWaitingProcessesQueue[PROCESSTABLEMAXSIZE];
int numberOfIOWaitingProcesses=0; 


// Initial set of tasks of the OS
void OperatingSystem_Initialize() {
	
	int i, selectedProcess;
	int numberOfSuccessfullyCreatedProcesses=0;
	FILE *programFile; // For load Operating System Code


	// Obtain the memory requirements of the program
	int processSize=OperatingSystem_ObtainProgramSize(&programFile, "OperatingSystemCode");

	// Load Operating System Code
	OperatingSystem_LoadProgram(programFile, OS_address_base, processSize);
	
	// Initialize random feed
	srand(time(NULL));

	// Process table initialization (all entries are free)
	for (i=0; i<PROCESSTABLEMAXSIZE;i++)
		processTable[i].busy=0;
	
	// Initialization of the interrupt vector table of the processor
	Processor_InitializeInterruptVectorTable(OS_address_base);
	
	//Ejercicio 5 Inicializa la tabla de particiones y devuelve el número de particiones
	numberOfPartitions=OperatingSystem_InitializePartitionTable();
	freePart=numberOfPartitions;
	
	
	// Create all system daemon processes
	OperatingSystem_CreateDaemons();
	
	//Llamada al dispositivo de entrada/salida
	Device_Initialize ("OutputDevice-2017", 7); 
	
	if (sipID<0) {
		// Show message "ERROR: Missing SIP program!\n"
		ComputerSystem_DebugMessage(21,SHUTDOWN);
		exit(1);
	}
	
	OperatingSystem_PrintStatus();
	
	// Create all user processes from the information given in the command line
	numberOfSuccessfullyCreatedProcesses=OperatingSystem_LongTermScheduler();
	if(numberOfSuccessfullyCreatedProcesses==0 && OperatingSystem_IsThereANewProgram()==-1){
		OperatingSystem_ReadyToShutdown();
	}
	
	// At least, one user process has been created
	// Select the first process that is going to use the processor
	selectedProcess=OperatingSystem_ShortTermScheduler();
	
	// Assign the processor to the selected process
	OperatingSystem_Dispatch(selectedProcess);

	// Initial operation for Operating System
	Processor_SetPC(OS_address_base);
}


// Daemon processes are system processes, that is, they work together with the OS.
// The System Idle Process uses the CPU whenever a user process is able to use it
void OperatingSystem_CreateDaemons() {
  
	USER_PROGRAMS_DATA systemIdleProcess;
	
	systemIdleProcess.executableName="SystemIdleProcess";
	isDemon=1;
	sipID=OperatingSystem_CreateProcess(systemIdleProcess); 
	processTable[sipID].copyOfPCRegister=processTable[sipID].initialPhysicalAddress;
	processTable[sipID].copyOfPSWRegister=Processor_GetPSW();
}


// The LTS is responsible of the admission of new processes in the system.
// Initially, it creates a process from each program specified in the command line
int OperatingSystem_LongTermScheduler() {
  
	int PID,
	numberOfSuccessfullyCreatedProcesses=0;
	int process=0;	

	while(OperatingSystem_IsThereANewProgram()==1){
			process=Heap_poll(arrivalTimeQueue,QUEUE_ARRIVAL,&numberOfProgramsInArrivalTimeQueue);
			PID=OperatingSystem_CreateProcess(*userProgramsList[process]);
			numberOfSuccessfullyCreatedProcesses++;
		
		//Si el PID es igual a NOFREENTRY decrementamos el numberOfSuccessfullyCreatedProcesses porque 
		//el proceso no se ha creado correctamente
		if(PID==NOFREEENTRY){
			OperatingSystem_ShowTime(ERROR);
			ComputerSystem_DebugMessage(103,ERROR,userProgramsList[process]->executableName);
			numberOfSuccessfullyCreatedProcesses--;
		}
		else if(PID==PROGRAMDOESNOTEXIST){
			OperatingSystem_ShowTime(ERROR);
			ComputerSystem_DebugMessage(104,ERROR,userProgramsList[process]->executableName," it does not exist");
			numberOfSuccessfullyCreatedProcesses--;
		}
		else if(PID==PROGRAMNOTVALID){
			OperatingSystem_ShowTime(ERROR);
			ComputerSystem_DebugMessage(104,ERROR,userProgramsList[process]->executableName,"invalid priority or size");
			numberOfSuccessfullyCreatedProcesses--;
		}
		else if(PID==TOOBIGPROCESS){
			OperatingSystem_ShowTime(ERROR);
			ComputerSystem_DebugMessage(105,ERROR,userProgramsList[process]->executableName);
			numberOfSuccessfullyCreatedProcesses--;
		}
		else if(PID==MEMORYFULL){
			OperatingSystem_ShowTime(ERROR);
			ComputerSystem_DebugMessage(144,ERROR,userProgramsList[process]->executableName);
			numberOfSuccessfullyCreatedProcesses--;
		}
		else if(PID!=0){
		// Show message "Process [PID] created from program [executableName]\n"
		OperatingSystem_ShowTime(INIT);
		ComputerSystem_DebugMessage(22,INIT,PID,userProgramsList[process]->executableName);
		}
		
	}
	if(numberOfSuccessfullyCreatedProcesses>0){
		OperatingSystem_PrintStatus();
	}
	numberOfNotTerminatedUserProcesses+=numberOfSuccessfullyCreatedProcesses;
	// Return the number of succesfully created processes
	
	int shoot = OperatingSystem_ThereArePrograms();
	
	if(OperatingSystem_IsThereANewProgram()==-1 && executingProcessID == sipID && shoot==0){
		OperatingSystem_ReadyToShutdown();
	}
	return numberOfSuccessfullyCreatedProcesses;
}


// This function creates a process from an executable program
int OperatingSystem_CreateProcess(USER_PROGRAMS_DATA executableProgram) {
  
	int PID;
	int processSize;
	int loadingPhysicalAddress;
	int priority;
	int queueID=isDemon; //Cola a la que pertenece el proceso
	FILE *programFile;

	// Obtain a process ID
	PID=OperatingSystem_ObtainAnEntryInTheProcessTable();
	//Si OperatingSystem_ObtainAnEntryInTheProcessTable() fracasa devolvemos NOFREENTRY
	if(PID==NOFREEENTRY){
		return PID;
	}

	// Obtain the memory requirements of the program
	processSize=OperatingSystem_ObtainProgramSize(&programFile, executableProgram.executableName);
	//Si el tamaño del programa no es válido entra en la condición
	if(processSize==PROGRAMNOTVALID){
		return processSize;
	}
	//Si el nombre del programa no es válido entra en la condición
	if(processSize==PROGRAMDOESNOTEXIST){
		return processSize;
	}
	
	// Obtain the priority for the process
	priority=OperatingSystem_ObtainPriority(programFile);
	if(priority==PROGRAMNOTVALID){
		return priority;
	}
	
	// Obtain enough memory space
 	loadingPhysicalAddress=OperatingSystem_ObtainMainMemory(processSize, PID);
	if(loadingPhysicalAddress==TOOBIGPROCESS){
		return loadingPhysicalAddress;
	}
	if(loadingPhysicalAddress==MEMORYFULL){
		return loadingPhysicalAddress;
	}
	
	// Load program in the allocated memory
	int big = OperatingSystem_LoadProgram(programFile,partitionsTable[loadingPhysicalAddress].initAddress, processSize);
	if(big==TOOBIGPROCESS){
		return big;
	}
	if(loadingPhysicalAddress!=MEMORYFULL){
	OperatingSystem_AsignaMemoria(loadingPhysicalAddress,PID);
	}
	
	//Controlamos la cola a la que quiere acceder
	if(isDemon==1){
		queueID=1;
		isDemon=0;
	}
	// PCB initialization
	OperatingSystem_PCBInitialization(PID,partitionsTable[loadingPhysicalAddress].initAddress, processSize, priority, queueID);
	
	// Move process to the ready state
	OperatingSystem_MoveToTheREADYState(PID);
	
	return PID;
}


// Main memory is assigned in chunks. All chunks are the same size. A process
// always obtains the chunk whose position in memory is equal to the processor identifier
int OperatingSystem_ObtainMainMemory(int processSize, int PID) {

	int i;
	int mejorAjuste;
	int indice=MEMORYFULL;
	int resta;
	int first = 0;
	
	OperatingSystem_ShowTime(SYSMEM);
	ComputerSystem_DebugMessage(142,SYSMEM,PID,processSize);	
	
	if(processSize>OperatingSystem_getBiggestPart()){
		return TOOBIGPROCESS;
	}
	
	if (freePart==0){
		return MEMORYFULL;
	}
		
	
		for(i=0;i<numberOfPartitions;i++){		
			resta =  partitionsTable[i].size - processSize;
			//Comprobamos que el tamaño del proceso no sea mayor que la particion
			if(processSize <= partitionsTable[i].size){
				if(partitionsTable[i].occupied==0){
			    //Si es menor nos quedamos con ella
					if(resta < mejorAjuste || first==0){
						first=1;
						mejorAjuste = resta;
						indice = i;
					}
				}
			
			    //Si es igual comprobamos las direcciones físicas
			    //Si la direccion física es menor nos quedamos con ella
			    if(resta == mejorAjuste ){
				   if(partitionsTable[i].initAddress < partitionsTable[indice].initAddress){
				        indice = i;
				   }
			    }
			}
		}		
 	return indice;
}


// Assign initial values to all fields inside the PCB
void OperatingSystem_PCBInitialization(int PID, int initialPhysicalAddress, int processSize, int priority,int queue) {

	processTable[PID].busy=1;
	processTable[PID].initialPhysicalAddress=initialPhysicalAddress;
	processTable[PID].processSize=processSize;
	processTable[PID].state=NEW;
	OperatingSystem_ShowTime(SYSPROC);
	ComputerSystem_DebugMessage(111,SYSPROC,PID,statesNames[0]);
	processTable[PID].priority=priority;
	processTable[PID].copyOfPCRegister=0;
	processTable[PID].copyOfPSWRegister=0;
	processTable[PID].queueID=queue;
	processTable[PID].copyOfAcumulator=0;
}


// Move a process to the READY state: it will be inserted, depending on its priority, in
// a queue of identifiers of READY processes
void OperatingSystem_MoveToTheREADYState(int PID) {
	int cola;
	if(processTable[PID].queueID==0){
		cola=0;
	}else if(processTable[PID].queueID==1){
		cola=1;
	}
	
	if (Heap_add(PID, readyToRunQueue[cola],QUEUE_PRIORITY ,&numberOfReadyToRunProcesses[cola] ,PROCESSTABLEMAXSIZE)>=0) {
		OperatingSystem_ShowTime(SYSPROC);
		ComputerSystem_DebugMessage(110,SYSPROC,PID,statesNames[processTable[PID].state],statesNames[1]);
		processTable[PID].state=READY;
	} 
}


// The STS is responsible of deciding which process to execute when specific events occur.
// It uses processes priorities to make the decission. Given that the READY queue is ordered
// depending on processes priority, the STS just selects the process in front of the READY queue
int OperatingSystem_ShortTermScheduler() {
	
	int selectedProcess;

	selectedProcess=OperatingSystem_ExtractFromReadyToRun();
	
	return selectedProcess;
}


// Return PID of more priority process in the READY queue
int OperatingSystem_ExtractFromReadyToRun() {
	int selectedProcess=NOPROCESS;
	int cola=1;
	int exit=0;
	int i;
	for(i=0;i <numberOfReadyToRunProcesses[0];i++){
		if(readyToRunQueue[0][i] >= 0 && exit==0){
			cola=0;
			exit=1;
			}
		}
	
	selectedProcess=Heap_poll(readyToRunQueue[cola],QUEUE_PRIORITY ,&numberOfReadyToRunProcesses[cola]);
	
	// Return most priority process or NOPROCESS if empty queue
	return selectedProcess; 
}


// Function that assigns the processor to a process
void OperatingSystem_Dispatch(int PID) {

	// The process identified by PID becomes the current executing process
	executingProcessID=PID;
	// Change the process' state
	OperatingSystem_ShowTime(SYSPROC);
	ComputerSystem_DebugMessage(110,SYSPROC,executingProcessID,statesNames[processTable[executingProcessID].state],statesNames[2]);
	processTable[PID].state=EXECUTING;
	// Modify hardware registers with appropriate values for the process identified by PID
	OperatingSystem_RestoreContext(PID);
	
	
}


// Modify hardware registers with appropriate values for the process identified by PID
void OperatingSystem_RestoreContext(int PID) {
  
	// New values for the CPU registers are obtained from the PCB
	Processor_CopyInSystemStack(MAINMEMORYSIZE-1,processTable[PID].copyOfPCRegister);
	Processor_CopyInSystemStack(MAINMEMORYSIZE-2,processTable[PID].copyOfPSWRegister);
	Processor_SetAccumulator(processTable[PID].copyOfAcumulator);
	
	// Same thing for the MMU registers
	MMU_SetBase(processTable[PID].initialPhysicalAddress);
	MMU_SetLimit(processTable[PID].processSize);
}


// Function invoked when the executing process leaves the CPU 
void OperatingSystem_PreemptRunningProcess() {

	// Save in the process' PCB essential values stored in hardware registers and the system stack
	OperatingSystem_SaveContext(executingProcessID);
	// Change the process' state
	OperatingSystem_MoveToTheREADYState(executingProcessID);
	// The processor is not assigned until the OS selects another process
	executingProcessID=NOPROCESS;
}


// Save in the process' PCB essential values stored in hardware registers and the system stack
void OperatingSystem_SaveContext(int PID) {
	
	// Load PC saved for interrupt manager
	processTable[PID].copyOfPCRegister=Processor_CopyFromSystemStack(MAINMEMORYSIZE-1);
	
	// Load PSW saved for interrupt manager
	processTable[PID].copyOfPSWRegister=Processor_CopyFromSystemStack(MAINMEMORYSIZE-2);
	
	//Load acumulator saved for interrupt manager
	processTable[PID].copyOfAcumulator=Processor_GetAccumulator();
}


// Exception management routine
void OperatingSystem_HandleException() {
  
	// Show message "Process [executingProcessID] has generated an exception and is terminating\n"
//	OperatingSystem_ShowTime(SYSPROC);
//	ComputerSystem_DebugMessage(23,SYSPROC,executingProcessID);
	if(Processor_GetExceptionType()==DIVISIONBYZERO){
		OperatingSystem_ShowTime(INTERRUPT);
		ComputerSystem_DebugMessage(140,INTERRUPT,executingProcessID,"division by zero");
	}
	if(Processor_GetExceptionType()==INVALIDPROCESSORMODE){
		OperatingSystem_ShowTime(INTERRUPT);
		ComputerSystem_DebugMessage(140,INTERRUPT,executingProcessID,"invalid processor mode");
	}
	if(Processor_GetExceptionType()==INVALIDADDRESS){
		OperatingSystem_ShowTime(INTERRUPT);
		ComputerSystem_DebugMessage(140,INTERRUPT,executingProcessID,"invalid address");
	}
	if(Processor_GetExceptionType()==INVALIDINSTRUCTION){
		OperatingSystem_ShowTime(INTERRUPT);
		ComputerSystem_DebugMessage(140,INTERRUPT,executingProcessID,"invalid instruction");
	}
	
	OperatingSystem_TerminateProcess();
	OperatingSystem_PrintStatus();
}


// All tasks regarding the removal of the process
void OperatingSystem_TerminateProcess() {
  
	int selectedProcess;
	OperatingSystem_ShowTime(SYSPROC);
	ComputerSystem_DebugMessage(110,SYSPROC,executingProcessID,statesNames[processTable[executingProcessID].state],statesNames[4]);
	processTable[executingProcessID].state=EXIT;
	processTable[executingProcessID].busy=0;
	
	OperatingSystem_ShowPartitionTable("before releasing memory");
	OperatingSystem_ReleaseMainMemory();
	OperatingSystem_ShowPartitionTable("after releasing memory");
	
	// One more process that has terminated
	numberOfNotTerminatedUserProcesses--;
	
	if (numberOfNotTerminatedUserProcesses<=0 && OperatingSystem_IsThereANewProgram()==-1) {
		// Simulation must finish 
		OperatingSystem_ReadyToShutdown();
	}
	// Select the next process to execute (sipID if no more user processes)
	selectedProcess=OperatingSystem_ShortTermScheduler();
	// Assign the processor to that process
	OperatingSystem_Dispatch(selectedProcess);
}


// System call management routine
void OperatingSystem_HandleSystemCall() {
  
	int systemCallID;
	int queue=processTable[executingProcessID].queueID;
	int oldProcess=executingProcessID;
	int processToRun=readyToRunQueue[queue][0];

	// Register A contains the identifier of the issued system call
	systemCallID=Processor_GetRegisterA();
	
	switch (systemCallID) {
		case SYSCALL_PRINTEXECPID:
			// Show message: "Process [executingProcessID] has the processor assigned\n"
			OperatingSystem_ShowTime(SYSPROC);
			ComputerSystem_DebugMessage(24,SYSPROC,executingProcessID);
			break;

		case SYSCALL_END:
			// Show message: "Process [executingProcessID] has requested to terminate\n"
			OperatingSystem_ShowTime(SYSPROC);
			ComputerSystem_DebugMessage(25,SYSPROC,executingProcessID);
			OperatingSystem_TerminateProcess();
			OperatingSystem_PrintStatus();
			break;
		//Cede el procesador si hay uno mas prioritario	
		case SYSCALL_YIELD:			
				if(executingProcessID !=processToRun && processTable[processToRun].priority==processTable[executingProcessID].priority){					
					OperatingSystem_PreemptRunningProcess();	
					OperatingSystem_Dispatch(OperatingSystem_ShortTermScheduler());	
					OperatingSystem_ShowTime(SHORTTERMSCHEDULE);					
					ComputerSystem_DebugMessage(115,SHORTTERMSCHEDULE,oldProcess,processToRun);
					OperatingSystem_PrintStatus();
				}
			break;
		
		case SYSCALL_SLEEP:	
			OperatingSystem_SaveContext(executingProcessID);
			processTable[executingProcessID].whenToWakeUp = abs(Processor_GetAccumulator()) + numberOfClockInterrupts +1;
			OperatingSystem_MoveToTheBLOCKEDState(executingProcessID);
			executingProcessID = NOPROCESS;
			int selectedProcess = OperatingSystem_ShortTermScheduler();
			OperatingSystem_Dispatch(selectedProcess);		
			OperatingSystem_PrintStatus();
			break;
			
			
		case SYSCALL_IO:
			//Bloqueamos el proceso y se pasa ejecutar el siguiente
			OperatingSystem_SaveContext(executingProcessID);
			//OperatingSystem_MoveToTheBLOCKEDState(executingProcessID);
			processTable[executingProcessID].state=BLOCKED;
			//Invocamos al manejador independiente
			OperatingSystem_IOScheduler();
			executingProcessID = NOPROCESS;
			//int selectedProcess = OperatingSystem_ShortTermScheduler();
			OperatingSystem_Dispatch(OperatingSystem_ShortTermScheduler());		
			break;
			
		default:
			OperatingSystem_ShowTime(INTERRUPT);					
			ComputerSystem_DebugMessage(141,INTERRUPT,executingProcessID,systemCallID);
			OperatingSystem_TerminateProcess();
			OperatingSystem_PrintStatus();
			break;
	}
}
	
//	Implement interrupt logic calling appropriate interrupt handle
void OperatingSystem_InterruptLogic(int entryPoint){
	switch (entryPoint){
		case SYSCALL_BIT: // SYSCALL_BIT=2
			OperatingSystem_HandleSystemCall();
			break;
		case EXCEPTION_BIT: // EXCEPTION_BIT=6
			OperatingSystem_HandleException();
			break;		
		case CLOCKINT_BIT:  //INTERRUPCIÓN DE RELOJ
			OperatingSystem_HandleClockInterrupt();
			break;
		case IOEND_BIT:		//INTERRUPCIÓN DE ENTRADA/SALIDA
			OperatingSystem_HandleIOEndInterrupt();
			break;
			
	}

}
 //Muestra por pantalla el contenido de la tabla de listos y la prioridad de cada proceso
 void OperatingSystem_PrintReadyToRunQueue(){
	 int i;
	 int exit=1; 
	 OperatingSystem_ShowTime(SYSPROC);
	 ComputerSystem_DebugMessage(106,SHORTTERMSCHEDULE);
	 //Recorremos la cola de usuarios
	 ComputerSystem_DebugMessage(200,SHORTTERMSCHEDULE);
	 for(i=0;i <numberOfReadyToRunProcesses[0];i++){   
		 int pid = readyToRunQueue[0][i];
		 if(exit==1){
			ComputerSystem_DebugMessage(107,SHORTTERMSCHEDULE,pid,processTable[pid].priority);
			exit=0;
		 }else{
			 ComputerSystem_DebugMessage(108,SHORTTERMSCHEDULE,pid,processTable[pid].priority);
		 }	 
	 }
	 ComputerSystem_DebugMessage(109,SHORTTERMSCHEDULE);
	 //Recorremos la cola del sistema
	 ComputerSystem_DebugMessage(201,SHORTTERMSCHEDULE);
	 exit=1;
	 for(i=0;i <numberOfReadyToRunProcesses[1];i++){   
		 int pid = readyToRunQueue[1][i];
		 if(exit==1){
			ComputerSystem_DebugMessage(112,SHORTTERMSCHEDULE,pid,processTable[pid].priority);
			exit=0;
		 }else{
			 ComputerSystem_DebugMessage(113,SHORTTERMSCHEDULE,pid,processTable[pid].priority);
		 }	 
	 }
	 ComputerSystem_DebugMessage(109,SHORTTERMSCHEDULE);
 }
 
 void OperatingSystem_HandleClockInterrupt(){
	 int cabecera = sleepingProcessesQueue[0];
	 int processToExecute;
	 int exit=0;
	 int creados=0;
	 numberOfClockInterrupts++;
	 OperatingSystem_ShowTime(INTERRUPT);
	 ComputerSystem_DebugMessage(120,INTERRUPT,numberOfClockInterrupts);
	 while(exit==0 && numberOfClockInterrupts == processTable[cabecera].whenToWakeUp){
		 processToExecute=Heap_poll(sleepingProcessesQueue,QUEUE_WAKEUP,&numberOfSleepingProcesses);
		 if(processToExecute>=0){		
			OperatingSystem_MoveToTheREADYState(processToExecute);
			cabecera = sleepingProcessesQueue[0];		
		 }else{
			exit=1;
			}
	 }
	 creados = OperatingSystem_LongTermScheduler();
	 if(exit==1){
		OperatingSystem_PrintStatus();
		OperatingSystem_CheckPriority();
		exit=0;
	 }else if(creados > 0){
		OperatingSystem_CheckPriority();
	 }
 } 
 
 
//Este método mueve el proceso que se está ejecutando a la cola de bloqueados
 void OperatingSystem_MoveToTheBLOCKEDState(int PID) {
	if (Heap_add(PID,sleepingProcessesQueue,QUEUE_WAKEUP,&numberOfSleepingProcesses,PROCESSTABLEMAXSIZE)>=0) {
		OperatingSystem_ShowTime(SYSPROC);
		ComputerSystem_DebugMessage(110,SYSPROC,executingProcessID,statesNames[processTable[executingProcessID].state],statesNames[3]);
		processTable[PID].state=BLOCKED;
	} 
 }
	

//Este método comprueba si hay algún proceso que tiene más prioridad que el que se está ejecutando	
 void OperatingSystem_CheckPriority(){
	 int processToRun=readyToRunQueue[0][0];
	 
	 if(processTable[processToRun].priority < processTable[executingProcessID].priority || executingProcessID==0){	
		OperatingSystem_ShowTime(SHORTTERMSCHEDULE);
		ComputerSystem_DebugMessage(121,SHORTTERMSCHEDULE,executingProcessID,processToRun);
		OperatingSystem_PreemptRunningProcess();	
		OperatingSystem_Dispatch(OperatingSystem_ShortTermScheduler());	
		OperatingSystem_PrintStatus();
	 }
 }
 
 int OperatingSystem_GetExecutingProcessID() {
	return executingProcessID;
}	


void OperatingSystem_ReleaseMainMemory(){
	int i;
	for(i=0;i<numberOfPartitions;i++){
		if(partitionsTable[i].PID==executingProcessID && partitionsTable[i].occupied !=0){
			partitionsTable[i].occupied=0;
			freePart++;
			OperatingSystem_ShowTime(SYSMEM);
			ComputerSystem_DebugMessage(145,SYSMEM,i,partitionsTable[i].initAddress,partitionsTable[i].size,executingProcessID);	
		}
	}
}

void OperatingSystem_AsignaMemoria(int indice,int PID){
	
	OperatingSystem_ShowPartitionTable("before allocating memory");
	
	OperatingSystem_ShowTime(SYSMEM);		
	ComputerSystem_DebugMessage(143,SYSMEM,indice,partitionsTable[indice].initAddress,partitionsTable[indice].size,PID);
	freePart--;		
	partitionsTable[indice].PID=PID;
	partitionsTable[indice].occupied=1;
	
	OperatingSystem_ShowPartitionTable("after allocating memory");	
}

int OperatingSystem_ThereArePrograms(){
	int i;
	int progs=0;
	for(i=0;i<numberOfReadyToRunProcesses[0];i++){
		if(readyToRunQueue[0][i] > 0){
			progs++;
		}
	}
	return progs;
}


//Manejador interrupciones entrada/salida
void OperatingSystem_HandleIOEndInterrupt(){
	int processToExecute;
	//Despertamos al proceso que termina la operación de entrada/salida
	 processToExecute=QueueFIFO_poll(IOWaitingProcessesQueue,&numberOfIOWaitingProcesses);
	 numberOfIOWaitingProcesses--;
	 OperatingSystem_MoveToTheREADYState(processToExecute);
	 
	 //Pedir al manejador dependiente del dispositivo que se disponga a gestionar la siguiente petición pendiente
	 OperatingSystem_DeviceControlerEndIOOperation(processToExecute);
	 
	 //Requisar el procesador al proceso en ejecución si fuese necesario
	 OperatingSystem_CheckPriority();
}


//Manejador Independiente
void OperatingSystem_IOScheduler(){
	//Añadimos la petición a la cola de peticiones
	//Hay que avisar al manegador dependiente si la operación se realiza con exito  ;  //PID,COLA,NUMERO_ELEMENTS,LIMIT
	if(QueueFIFO_add(executingProcessID,IOWaitingProcessesQueue,&numberOfIOWaitingProcesses,PROCESSTABLEMAXSIZE) >=0){
		numberOfIOWaitingProcesses++;
		OperatingSystem_DeviceControlerStartIOOperation();
	}	
}



//Funcion 1 Manejador dependiente
void OperatingSystem_DeviceControlerStartIOOperation(){
	
	if(Device_GetStatus()==FREE){  //Si el dispositivo está libre le pasamos el pid
		Device_StartIO(IOWaitingProcessesQueue[0]);
		
	}
}

//Funcion 2 Manejador Dependiente
int OperatingSystem_DeviceControlerEndIOOperation(int PID){
	if(numberOfIOWaitingProcesses>0){
		OperatingSystem_DeviceControlerStartIOOperation();
	}
	return PID;
}


