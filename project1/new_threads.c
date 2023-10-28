#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "./include/threads.h"

void* getMessageFromCAB(THREAD_ARG* arg) {
    THREAD_ARG* parg = (THREAD_ARG*) arg;

    pthread_mutex_lock(&parg->mutex);
    parg->content = get_mes(parg->cab);
    pthread_mutex_unlock(&parg->mutex);

    if (parg->content == NULL){
        fprintf(stderr, "Error: no message\n");
        return NULL;
    }

    return (void*) 1;
}

void* ungetMessageFromCAB(THREAD_ARG* arg) {
    THREAD_ARG* parg = (THREAD_ARG*) arg;

    pthread_mutex_lock(&parg->mutex);
    put_mes(parg->cab, (CAB_BUFFER*) parg->content);
    pthread_mutex_unlock(&parg->mutex);

    return (void*) 1;
}

void* putMessageOnCab(THREAD_ARG* arg) {
    THREAD_ARG* parg = (THREAD_ARG*) arg;

    pthread_mutex_lock(&parg->mutex);
    put_mes(parg->cab, (CAB_BUFFER*) parg->content);
    pthread_mutex_unlock(&parg->mutex);

    return (void*) 1;
}

void* reserveCab(THREAD_ARG* arg) {
    THREAD_ARG* parg = (THREAD_ARG*) arg;

    pthread_mutex_lock(&parg->mutex);
    parg->content = (void*) reserve(parg->cab);
    pthread_mutex_unlock(&parg->mutex);

    if (parg->content == NULL){
        fprintf(stderr, "Error: reserve failed\n");
        return NULL;
    }

    return (void*) 1;
}