#include <stdio.h>
#include <stdlib.h>

#include "./include/threads.h"
#include "./include/processing.h"

pthread_t threads[50]; /* Thread identifiers */
int thread_id[50]; /* Application-defined thread IDs */

// CAB related threads

void* getMessageFromCAB(THREAD_ARG* arg) {
    pthread_mutex_lock(&arg->mutex);
    arg->content = get_mes((CAB*)arg->source);
    pthread_mutex_unlock(&arg->mutex);

    if (arg->content == NULL){
        fprintf(stderr, "Error: no message\n");
        return NULL;
    }

    return (void*) 1;
}

void* ungetMessageFromCAB(THREAD_ARG* arg) {
    pthread_mutex_lock(&arg->mutex);
    unget(arg->source, (CAB_BUFFER*)arg->content);
    pthread_mutex_unlock(&arg->mutex);

    return (void*) 1;
}

void* putMessageOnCab(THREAD_ARG* arg, CAB_BUFFER* buffer, void* data) {
    pthread_mutex_lock(&arg->mutex);
    put_mes((CAB*)arg->source, buffer, data);
    pthread_mutex_unlock(&arg->mutex);

    return (void*) 1;
}

void* reserveCab(THREAD_ARG* arg) {
    pthread_mutex_lock(&arg->mutex);
    arg->content = (void*) reserve(arg->source);
    pthread_mutex_unlock(&arg->mutex);

    if (arg->content == NULL){
        fprintf(stderr, "Error: reserve failed\n");
        return NULL;
    }

    return (void*) 1;
}

// Common Use Threads

void* initThreadArg(THREAD_ARG* arg, CAB* cab) {
    arg->mutex = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER;
    arg->source = (void*)cab;

    return (void *) 1;
}

void* setThreadAttributes(pthread_attr_t* attr) {
	pthread_attr_init(attr);
	pthread_attr_setdetachstate(attr, PTHREAD_EXPLICIT_SCHED);
	pthread_attr_setschedpolicy(attr, SCHED_RR);

    return (void *) 1;
}

void* setAllThreadSchedParam(pthread_attr_t* attr) {
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

    return (void *) 1;
}
	
// Real Time DB related threads

void* getMessageFromRTDB(THREAD_ARG* arg, DB* db) {
    pthread_mutex_lock(&arg->mutex);
    arg->content = getMostRecentData(db);
    pthread_mutex_unlock(&arg->mutex);

    return (void *) 1;
}

void* setMessageAtRTDB(THREAD_ARG* arg, int index) {
    pthread_mutex_lock(&arg->mutex);
    setBufferAtIndex((DB*)&arg->source, index, &arg->content);
    pthread_mutex_unlock(&arg->mutex);

    return (void *) 1;
}

void* dispatchImageProcessingFunctions(THREAD_ARG* arg, THREAD_ARG* db_arg, pthread_mutex_t proc, long frame_number,  uint16_t width, uint16_t height, int16_t *cm_x, int16_t *cm_y) {
    pthread_mutex_lock(&proc);
    if (frame_number % 2 == 0){
        getMessageFromCAB(arg);
        imgDetectObstacles((unsigned char*)arg->content, width, height, *cm_x, *cm_y);
        printf("frame number: %ld, detecting obstacles\n", frame_number);
        setMessageAtRTDB(&db_arg, 0);
    }
    if (frame_number % 3 == 0){
        getMessageFromCAB((CAB*)arg->source);
        imgEdgeDetection((unsigned char*)arg->content, width, height, *cm_x, *cm_y);
        printf("frame number: %ld, detecting edges\n", frame_number);
        setMessageAtRTDB(&db_arg, 1);
    }
    if (frame_number % 5 == 0){
        getMessageFromCAB((CAB*)arg->source);
        imgFindBlueSquare((unsigned char*)arg->content, width, height, *cm_x, *cm_y);
        printf("frame number: %ld\n, detecting blue squares", frame_number);
        setMessageAtRTDB(&db_arg, 2);
    }
    pthread_mutex_unlock(&proc);

    return (void *) 1;
}