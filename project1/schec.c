#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <sched.h> //sched_setscheduler
#include <pthread.h>
#include <errno.h>
#include <signal.h> // Timers
#include <stdint.h>
#include <unistd.h>
#include <sys/mman.h>
#include <math.h>

#include "./include/threads.h"
#include "./include/webCamCapture.h"

#define NS_IN_SEC 1000000000L

// Adds two timespect variables
struct timespec TsAdd(struct timespec ts1, struct timespec ts2) {
	struct  timespec  tr;

	// Add the two timespec variables
	tr.tv_sec = ts1.tv_sec + ts2.tv_sec ;
	tr.tv_nsec = ts1.tv_nsec + ts2.tv_nsec ;

	// Check for nsec overflow	
	if (tr.tv_nsec >= NS_IN_SEC) {
		tr.tv_sec++ ;
		tr.tv_nsec = tr.tv_nsec - NS_IN_SEC ;
	}
	
	return (tr);
}

// Subtracts two timespect variables
struct  timespec  TsSub (struct timespec ts1, struct timespec ts2) {
	struct  timespec  tr;

	// Subtract second arg from first one 
	if ((ts1.tv_sec < ts2.tv_sec) || ((ts1.tv_sec == ts2.tv_sec) && (ts1.tv_nsec <= ts2.tv_nsec))) {
		// Result would be negative. Return 0
		tr.tv_sec = tr.tv_nsec = 0 ;  
	} else {						
		// If T1 > T2, proceed 
		tr.tv_sec = ts1.tv_sec - ts2.tv_sec ;
		if (ts1.tv_nsec < ts2.tv_nsec) {
			tr.tv_nsec = ts1.tv_nsec + NS_IN_SEC - ts2.tv_nsec ;
			tr.tv_sec-- ;				
		}
		else 
			tr.tv_nsec = ts1.tv_nsec - ts2.tv_nsec ;
	}

    return (tr) ;

}