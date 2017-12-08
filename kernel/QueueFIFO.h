#ifndef QUEUEFIFO_H
#define QUEUEFIFO_H

#define QUEUE_WAKEUP 0
#define QUEUE_PRIORITY 1
#define QUEUE_ARRIVAL 2
#define QUEUE_IO 3

// Implements the insertion operation in FIFO queue. 
// Parameters are:
//    info: PID to be inserted 
//    queue: the corresponding queue
//    numElem: number of current elements inside the queue, if successfull is increased by one
//    limit: maximum capacity of the queue
// return 0/-1  ok/fail
int QueueFIFO_add(int, int[], int*, int);

// Implements the extraction operation (the first element).
// Parameters are:
//    queue: the corresponding queue
//    numElem: number of current elements inside the queue, if successfull is decremented by one
// Returns: the first item in the queue or -1 if empty queue
int QueueFIFO_poll(int[], int*);


#endif