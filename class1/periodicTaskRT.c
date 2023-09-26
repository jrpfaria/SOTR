/******************************************************************
 * Paulo Pedreiras, Sept/2023
 * DETI/UA/IT - Real-Time Operating Systems course
 *
 * Base code for periodic thread execution using clock_nanosleep
 * Assumes that periods and execution times are below 1 second
 * 
 * Applies SCHED_FIFO, locks memory and locks task to CPU0
 * 
 * IMPORTANT: note that access to SCHED_FIFO and SCHED_RR 
 * 		requires root access 
 *    
 *
 *****************************************************************/
#define _GNU_SOURCE             /* Must precede #include <sched.h> for sched_setaffinity */ 

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


/* ***********************************************
* App specific defines
* ***********************************************/
#define NS_IN_SEC 1000000000L

#define PERIOD_NS (100*1000*1000) 	// Period (ns component)
#define PERIOD_S (0)				// Period (seconds component)

#define BOOT_ITER 10				// Number of activations for warm-up
									// There is an initial transient in which first activations
									// often have an irregular behaviour (cache issues, ..)
#define DEFAULT_PRIO 50				// Default (fixed) thread priority  
#define THREAD_INIT_OFFSET 1000000	// Initial offset (i.e. delay) of rt thread


/* ***********************************************
* Prototypes
* ***********************************************/
void Heavy_Work(void);
struct  timespec TsAdd(struct  timespec  ts1, struct  timespec  ts2);
struct  timespec TsSub(struct  timespec  ts1, struct  timespec  ts2);


/* ***********************************************
* Global variables
* ***********************************************/
uint64_t min_iat, max_iat; // Hold the minimum/maximum observed time between successive executions
int periodicity = 0;

/* *************************
* Thread_1 code 
* **************************/

void * Thread_1_code(void *arg)
{
	/* Timespec variables to manage time */
	struct timespec ts, // thread next activation time (absolute)
			ta, 		// activation time of current thread activation (absolute)
			tit, 		// thread time from last execution,
			ta_ant, 	// activation time of last instance (absolute),
			tp; 		// Thread period
		
	/* Other variables */
	int niter = 0; 	// Activation counter
	int update; 	// Flag to signal that min/max should be updated
	int policy;    // To obtain scheduling policy
	struct sched_param parm; // To get therad priority
	int i; // general counter 
	cpu_set_t cpuset_test; // To check thread affinity
	
	/* Delays theread execution start to prevent output of main() and thread to get garbled */
	usleep(THREAD_INIT_OFFSET);
	
	/* Set absolute activation time of first instance */
	if (periodicity == 0){
		tp.tv_nsec = PERIOD_NS;
		tp.tv_sec = PERIOD_S;	
	}
	else{
		tp.tv_nsec = periodicity;
		tp.tv_sec = PERIOD_S;
	}
	clock_gettime(CLOCK_MONOTONIC, &ts);
	ts = TsAdd(ts,tp);	
	
	/* Print thread RT info */
	printf("\n\r RT Thread ID: %d\n\r", gettid());
	policy=sched_getscheduler(0);
	switch(policy){
		case SCHED_FIFO:
			sched_getparam(0,&parm);
			printf("\n\r RT thread, SCHED_FIFO, priority %d\n\r", parm.sched_priority);
			break;
		case SCHED_RR:
			sched_getparam(0,&parm);
			printf("\n\r RT thread, SCHED_RR, priority %d\n\r", parm.sched_priority);
			break;
		default:
			printf("\n\r ERROR, NOT SCHED_RR nor SCHED_FIFO\n\r");
	}

	/* Check thread affinity */
	printf("\n\r Thread affinity:");
	sched_getaffinity(0, sizeof(cpuset_test),&cpuset_test);
	for(i=0;i<CPU_COUNT(&cpuset_test);i++){
		if(CPU_ISSET(i,&cpuset_test))
			printf("CPU %d - Yes, ",i);
		else
			printf("CPU %d - No, ",i);		
	}
	printf("\n\r");
		
	/* Periodic jobs ...*/ 
	while(1) {

		/* Wait until next cycle */
		clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME,&ts,NULL);
		clock_gettime(CLOCK_MONOTONIC, &ta);		
		ts = TsAdd(ts,tp);		
		
		niter++; // Coount number of activations
		
		/* Compute latency and jitter */
		if( niter == 1) 
			ta_ant = ta; // Init ta_ant at first activation
			
			tit=TsSub(ta,ta_ant);  // Compute time since last activation
		
		if( niter == BOOT_ITER) {	// Boot time finsihed. Init max/min variables	    
			  min_iat = tit.tv_nsec;
			  max_iat = tit.tv_nsec;
			  update = 1;
		}else
		if( niter > BOOT_ITER){ 	// Update max/min, if boot time elapsed 	    
			if(tit.tv_nsec < min_iat) {
			  min_iat = tit.tv_nsec;
			  update = 1;
			}
			if(tit.tv_nsec > max_iat) {
			  max_iat = tit.tv_nsec;
			  update = 1;
			}
		}
		ta_ant = ta; // Update ta_ant
	
		  
  		/* Print maximum/minimum mount of time between successive executions */
		if(update) {		  
		  printf("Task %s: time between successive executions (approximation, us): min: %10.3f / max: %10.3f \n\r",(char *) arg, (float)min_iat/1000, (float)max_iat/1000);
		  update = 0;
		}
		
		/* Do the actual processing */
		Heavy_Work();		
	}  
  
	return NULL;
}

/* *************************
* main()
* **************************/

int main(int argc, char *argv[])
{
	// General vars
	int err, i;
	unsigned char procname[] = "RTThread";
		
	// For thread with RT attributes
	pthread_t threadid;
	struct sched_param parm;
	pthread_attr_t attr;
	cpu_set_t cpuset_test; // To check process affinity

	// For RT scheduler
	int policy, prio=DEFAULT_PRIO;

	if(argc <= 3 && argc > 1){
		int arg0 = atoi(argv[1]);
		int arg1 = atoi(argv[2]);
		
		// printf("\n\r -- %d %d --", arg0, arg1);
		if(arg0 < 1 || arg0 > 99)
			printf("\n\r -Invalid priority value, must be in the [1,99] interval\n\n");
		else prio = arg0;
		
		if(arg1 < 50 || arg1 > 500)
			printf("\n\r -Invalid periodicity level, must be in the [50, 500] interval\n\n");
		else periodicity = arg1 * 1000000;
	}
					 
	/* Create periodic thread/task with RT scheduling attributes*/
	pthread_attr_init(&attr);
	pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
	pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
	parm.sched_priority = prio;
	pthread_attr_setschedparam(&attr, &parm);
	
	/* Lock memory */
	mlockall(MCL_CURRENT | MCL_FUTURE);
	
	err=pthread_create(&threadid, &attr, Thread_1_code, &procname);
	if(err != 0)
		printf("\n\r Error creating Thread [%s]", strerror(err));
	else {
		printf("\n\r Process ID: %d", getpid());
		policy=sched_getscheduler(0);
		switch(policy){
			case SCHED_FIFO:
				sched_getparam(0,&parm);
				printf("\n\r Default thread, SCHED_FIFO, priority %d\n\r", parm.sched_priority);
				break;
			case SCHED_RR:
				sched_getparam(0,&parm);
				printf("\n\r Default thread, SCHED_RR, priority %d\n\r", parm.sched_priority);
				break;
			default:
				printf("\n\r Default thread, NOT SCHED_RR nor SCHED_FIFO\n\r");
		}
			
		/* Check process affinity */
		printf("\n\r Process affinity:");
		sched_getaffinity(0, sizeof(cpuset_test),&cpuset_test);
		for(i=0;i<CPU_COUNT(&cpuset_test);i++){
			if(CPU_ISSET(i,&cpuset_test))
				printf("CPU %d - Yes, ",i);
			else
				printf("CPU %d - No, ",i);		
		}
		printf("\n\r");
		

		while(1); // Ok. Thread shall run
	}
		
	return -1;
}




/* ***********************************************
* Auxiliary functions 
* ************************************************/

// Task load. In the case integrates numerically a function
#define f(x) 1/(1+pow(x,2)) /* Define function to integrate*/
void Heavy_Work(void)
{
	float lower, upper, integration=0.0, stepSize, k;
	int i, subInterval;
	
	struct timespec ts, // Function start time
			tf; // Function finish time
			
	static int first = 0; // Flag to signal first execution		
	
	/* Get start time */
	clock_gettime(CLOCK_MONOTONIC, &ts);
	
	/* Integration parameters */
	/*These values can be tunned to cause a desired load*/
	lower=0;
	upper=400;
	subInterval=500000;

	 /* Calculation */
	 /* Finding step size */
	 stepSize = (upper - lower)/subInterval;

	 /* Finding Integration Value */
	 integration = f(lower) + f(upper);
	 for(i=1; i<= subInterval-1; i++) {
		k = lower + i*stepSize;
		integration = integration + 2 * f(k);
 	}
	integration = integration * stepSize/2;
 	
 	/* Get finish time and show results */
 	if (!first) {
		clock_gettime(CLOCK_MONOTONIC, &tf);
		tf=TsSub(tf,ts);  // Compute time difference form start to finish
 	
		printf("Integration value is: %.3f. It took %9lu ns to compute.\n", integration, tf.tv_nsec);
		
		first = 1;
	}

}


// Adds two timespect variables
struct  timespec  TsAdd(struct  timespec  ts1, struct  timespec  ts2){
	
	struct  timespec  tr;
	
	// Add the two timespec variables
		tr.tv_sec = ts1.tv_sec + ts2.tv_sec ;
		tr.tv_nsec = ts1.tv_nsec + ts2.tv_nsec ;
	// Check for nsec overflow	
	if (tr.tv_nsec >= NS_IN_SEC) {
			tr.tv_sec++ ;
		tr.tv_nsec = tr.tv_nsec - NS_IN_SEC ;
		}

	return (tr) ;
}

// Subtracts two timespect variables
struct  timespec  TsSub (struct  timespec  ts1, struct  timespec  ts2) {
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
		} else {
			tr.tv_nsec = ts1.tv_nsec - ts2.tv_nsec ;
		}
	}

	return (tr) ;

}


