#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "./include/threads.h"

void* getMessageFromCAB(THREAD_ARG* arg) {
    pthread_mutex_lock(&arg->mutex);
    arg->content = get_mes(arg->cab);
    pthread_mutex_unlock(&arg->mutex);

    if (arg->content == NULL){
        fprintf(stderr, "Error: no message\n");
        return NULL;
    }

    return (void*) 1;
}

void* ungetMessageFromCAB(THREAD_ARG* arg) {
    pthread_mutex_lock(&arg->mutex);
    put_mes(arg->cab, (CAB_BUFFER*) arg->content);
    pthread_mutex_unlock(&arg->mutex);

    return (void*) 1;
}

void* putMessageOnCab(THREAD_ARG* arg) {
    pthread_mutex_lock(&arg->mutex);
    put_mes(arg->cab, (CAB_BUFFER*) arg->content);
    pthread_mutex_unlock(&arg->mutex);

    return (void*) 1;
}

void* reserveCab(THREAD_ARG* arg) {
    pthread_mutex_lock(&arg->mutex);
    arg->content = (void*) reserve(arg->cab);
    pthread_mutex_unlock(&arg->mutex);

    if (arg->content == NULL){
        fprintf(stderr, "Error: reserve failed\n");
        return NULL;
    }

    return (void*) 1;
}