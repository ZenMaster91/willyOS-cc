#include <stdlib.h>
#include "Heap.h"
#include "OperatingSystem.h"

// Internal Functions prototypes
void Heap_swap_Up(int, int[], int);
void Heap_swap_Down(int, int[], int, int);


// Insertion of a PID into a binary heap
// info: PID to insert
// heap: Binary heap to insert: user o daemon ready queue, sleeping queue, ...
// numElem: number of elements actually into the queue, if successfull is increased by one
// limit: max size of the queue
// return 0/-1  ok/fail
int QueueFIFO_add(int info, int fifo[], int *numElem, int limit) {
	if (*numElem >= limit || info<0)
		return -1;
	fifo[*numElem]=info;
	(*numElem)++;
	return 0;
}

// Extract the more priority item
// heap: Binary heap to extract: user o daemon ready queue, sleeping queue, ...
// queueType: QUEUE_PRIORITY, QUEUE_WAKEUP, QUEUE_ARRIVAL, ...
// numElem: number of elements actually into the queue, if successfull is decremented by one
// return more priority item into the queue
int QueueFIFO_poll(int fifo[], int *numElem) {
	int info = fifo[0];
	int i;
	if (*numElem==0)
		return -1; // empty queue
	else {
		// memcpy((void *) fifo, (void *) &fifo[1], (*numElem-1)*sizeof(int));
		for (i=0; i< (*numElem)-1; i++)
			fifo[i]=fifo[i+1];
		(*numElem)--;
	}
	return info;		
}

