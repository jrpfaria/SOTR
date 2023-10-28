#ifndef __THREADS__
#define __THREADS__

#include "cab.h"

typedef struct thread_arg THREAD_ARG;

struct thread_arg {
    pthread_mutex_t mutex;
    CAB* cab;
    void* content;
};

void* getMessageFromCAB(THREAD_ARG*);
void* ungetMessageFromCAB(THREAD_ARG*);
void* putMessageOnCab(THREAD_ARG*);
void* reserveCab(THREAD_ARG*);

#endif