#ifndef __THREADS__
#define __THREADS__

#include <pthread.h>
#include "rtdb.h"
#include "cab.h"
#include "processing.h"

typedef struct thread_arg THREAD_ARG;

struct thread_arg {
    pthread_mutex_t mutex;
    void* source;
    void* content;
};

void* getMessageFromCAB(THREAD_ARG*);
void* ungetMessageFromCAB(THREAD_ARG*);
void* putMessageOnCab(THREAD_ARG*, CAB_BUFFER*, void*);
void* reserveCab(THREAD_ARG*);

void* initThreadArg(THREAD_ARG*, void*);
void* setThreadParam(pthread_attr_t*);
void* setAllThreadSchedParam(pthread_attr_t*);

void* getMessageFromRTDB(THREAD_ARG*);
void* setMessageAtRTDB(THREAD_ARG*, int);

void* dispatchImageProcessingFunctions(THREAD_ARG*, THREAD_ARG*, pthread_mutex_t, long, int, int, uint16_t*, uint16_t*);

#endif