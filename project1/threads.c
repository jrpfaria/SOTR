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
    if (buffer != NULL) {
        pthread_mutex_lock(&arg->mutex);
        put_mes(arg->source, buffer, data);
        pthread_mutex_unlock(&arg->mutex);
    } else {
        fprintf(stderr, "Error: Buffer is NULL in putMessageOnCab\n");
    }
    // pthread_mutex_lock(&arg->mutex);
    // put_mes((CAB*)arg->source, buffer, data);
    // pthread_mutex_unlock(&arg->mutex);

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

void* initThreadArg(THREAD_ARG* arg, void* cab) {
    arg->mutex = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER;
    arg->source = cab;
    arg->content = NULL;

    return (void *) 1;
}

void* setThreadParam(pthread_attr_t* attr) {
	for (int i = 0; i < 3; i++){
        pthread_attr_init(&attr[i]);
    	pthread_attr_setdetachstate(&attr[i], PTHREAD_EXPLICIT_SCHED);
	    pthread_attr_setschedpolicy(&attr[i], SCHED_RR);
    }
    
    return (void *) 1;
}

void* setAllThreadSchedParam(pthread_attr_t* attr, pthread_mutex_t* mutexes) {    
    // ImgDetectObstacle
    struct sched_param paramObstacleDetection;
	pthread_attr_getschedparam(&attr[0], &paramObstacleDetection);
	paramObstacleDetection.sched_priority = 50;
	pthread_attr_setschedparam(&attr[0], &paramObstacleDetection);

    pthread_mutex_init(&mutexes[0], NULL);

    // ImgEdgeDetection
	struct sched_param paramEdgeDetection;
	pthread_attr_getschedparam(&attr[1], &paramEdgeDetection);
	paramEdgeDetection.sched_priority = 45;
	pthread_attr_setschedparam(&attr[1], &paramEdgeDetection);

    pthread_mutex_init(&mutexes[1], NULL);

    // ImgFindBlueSquare
	struct sched_param paramBlueSquareDetection;
	pthread_attr_getschedparam(&attr[2], &paramBlueSquareDetection);
	paramBlueSquareDetection.sched_priority = 40;
	pthread_attr_setschedparam(&attr[2], &paramBlueSquareDetection);

    pthread_mutex_init(&mutexes[2], NULL);

    return (void *) 1;
}
	
// Real Time DB related threads

void* getMessageFromRTDB(THREAD_ARG* arg) {
    pthread_mutex_lock(&arg->mutex);
    arg->content = getMostRecentData((DB*)arg->source);    
    pthread_mutex_unlock(&arg->mutex);

    return arg;
}

void* putMessageInRTDB(THREAD_ARG* arg, int index) {
	pthread_mutex_lock(&arg->mutex);
    setBufferAtIndex((DB*) arg->source, index, arg->content);
    pthread_mutex_unlock(&arg->mutex);
    return (void *) 1;
}

void* dispatchImageProcessingFunctions(THREAD_ARG* arg, THREAD_ARG* db_arg, pthread_attr_t* attr, pthread_mutex_t* mutexes, long frame_number, THREAD_INPUTS* inputs) {
    if (frame_number % 2 == 0) {
        getMessageFromCAB(arg);
        inputs->source = arg->content;
        inputs->mutex = &mutexes[0];
        pthread_create(&threads[0], &attr[0], (void *)imgDetectObstaclesWrapper, (void *)inputs);
        pthread_join(threads[0], NULL);
        db_arg->content = (unsigned char*)arg->content;
        printf("frame number: %ld, detecting obstacles\n", frame_number);
        // Save results to rtdb
        putMessageInRTDB(db_arg, 0);
    }
    if (frame_number % 3 == 0) {
        getMessageFromCAB(arg);
        inputs->source = arg->content;
        inputs->mutex = &mutexes[1];
        pthread_create(&threads[1], &attr[1], (void *)imgEdgeDetectionWrapper, (void *)inputs);
        pthread_join(threads[1], NULL);
        db_arg->content = (unsigned char*)arg->content;
        printf("frame number: %ld, detecting edges\n", frame_number);
        // Save results to rtdb
        putMessageInRTDB(db_arg, 1);
    }
    if (frame_number % 5 == 0) {
        getMessageFromCAB(arg);
        inputs->source = arg->content;
        inputs->mutex = &mutexes[2];
        pthread_create(&threads[2], &attr[2], (void *)imgFindBlueSquareWrapper, (void *)inputs);
        pthread_join(threads[2], NULL);
        db_arg->content = (unsigned char*)arg->content;
        printf("frame number: %ld, detecting blue squares\n", frame_number);
        // Save results to rtdb
        putMessageInRTDB(db_arg, 2);
    }

    return (void *) 1;
}

void* setThreadInputs(THREAD_INPUTS* args, int height, int width, uint16_t* cm_x, uint16_t* cm_y) {
    args->height = height;
    args->width = width;
    args->cm_x = cm_x;
    args->cm_y = cm_y;

    return (void *) 1;
}