#ifndef __THREADS__
#define __THREADS__

#include <pthread.h>
#include "cab.h"

typedef struct thread_arg THREAD_ARG;

struct thread_arg {
    pthread_mutex_t mutex;
    CAB* cab;
    void* content;
};

void* getMessageFromCAB(THREAD_ARG*);
void* ungetMessageFromCAB(THREAD_ARG*);
void* putMessageOnCab(THREAD_ARG*, CAB_BUFFER*, void*);
void* reserveCab(THREAD_ARG*);
void initThreadArg(THREAD_ARG*, CAB*);
void setThreadParam(pthread_attr_t*);

#endif