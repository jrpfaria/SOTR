#include <stdio.h>
#include <stdlib.h>

#include "./include/threads.h"
#include "./include/processing.h"

pthread_t threads[50]; /* Thread identifiers */
int thread_id[50]; /* Application-defined thread IDs */

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
    unget(arg->cab, (CAB_BUFFER*)arg->content);
    pthread_mutex_unlock(&arg->mutex);

    return (void*) 1;
}

void* putMessageOnCab(THREAD_ARG* arg, CAB_BUFFER* buffer, void* data) {
    pthread_mutex_lock(&arg->mutex);
    put_mes(arg->cab, buffer, data);
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

void initThreadArg(THREAD_ARG* arg, CAB* cab) {
    arg->mutex = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER;
    arg->cab = cab;
}

void setThreadAttributes(pthread_attr_t* attr) {
	pthread_attr_init(attr);
	pthread_attr_setdetachstate(attr, PTHREAD_EXPLICIT_SCHED);
	pthread_attr_setschedpolicy(attr, SCHED_RR);
}

void setAllThreadSchedParam(pthread_attr_t* attr) {
    // ImgDetectObstacle
    struct sched_param paramObstacleDetection;
	pthread_attr_getschedparam(&attr, &paramObstacleDetection);
	paramObstacleDetection.sched_priority = 50;
	pthread_attr_setschedparam(&attr, &paramObstacleDetection);

    pthread_create(&threads[0], &attr, (void *)imgDetectObstacles, &thread_id[0]);
    pthread_mutex_t imgDetectObstacles_mutex;
    pthread_mutex_init(&imgDetectObstacles_mutex, NULL);

    // ImgEdgeDetection
	struct sched_param paramEdgeDetection;
	pthread_attr_getschedparam(&attr, &paramEdgeDetection);
	paramEdgeDetection.sched_priority = 45;
	pthread_attr_setschedparam(&attr, &paramEdgeDetection);

    pthread_create(&threads[1], &attr, (void *)imgEdgeDetection, &thread_id[1]);
    pthread_mutex_t imgEdgeDetection_mutex;
    pthread_mutex_init(&imgEdgeDetection_mutex, NULL);

    // ImgFindBlueSquare
	struct sched_param paramBlueSquareDetection;
	pthread_attr_getschedparam(&attr, &paramBlueSquareDetection);
	paramBlueSquareDetection.sched_priority = 40;
	pthread_attr_setschedparam(&attr, &paramBlueSquareDetection);

    pthread_create(&threads[2], &attr, (void *)imgFindBlueSquare, &thread_id[3]);
    pthread_mutex_t imgFindBlueSquare_mutex;
    pthread_mutex_init(&imgFindBlueSquare_mutex, NULL);
}
	

	