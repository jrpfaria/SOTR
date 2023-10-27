#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include "cab.h"
#include "imageViewer.c"

// Threads
pthread_t detect_object_thread;
pthread_t blue_square_thread;
pthread_t edge_detection_thread;
pthread_t grey_scale_thread;
// pthread_t print_threads_thread;

sem_t sem;

CAB* cab;

int near_object = 0;
int edges = 0;

void* detect_object(void*);
void* blue_square(void*);
void* edge_detection(void*);
void* process_grey_scale(void*);
void* print_threads(void*);

int main(void) {

    sem_init(&sem, 0, 1);

    int size = width * height * sizeof(u_int32_t);

    cab = open_cab(size, 5);

    uint32_t* img = malloc(size);

    pthread_create(&detect_object_thread, NULL, detect_object, NULL);
    pthread_create(&blue_square_thread, NULL, blue_square, NULL);
    pthread_create(&edge_detection_thread, NULL, edge_detection, NULL);
    pthread_create(&grey_scale_thread, NULL, process_grey_scale, NULL);
    // pthread_create(&print_threads_thread, NULL, print_threads, NULL);

    pthread_join(detect_object_thread, NULL);
    pthread_join(blue_square_thread, NULL);
    pthread_join(edge_detection_thread, NULL);
    pthread_join(grey_scale_thread, NULL);
    // pthread_join(print_threads_thread, NULL);

    sem_destroy(&sem);

    // release the cab
    delete_cab(cab);

    return 0;
}

void* image_inCab (void* arg){

    while(1){
        sem_wait(&sem);
        CAB_BUFFER* buffer = reserve(cab);

        if (buffer != NULL){
            put_mes(cab, buffer);
        }

        unget(cab, buffer);
        sem_post(&sem);
    }
    return (void *) 1;
}

void* detect_object(void* arg) {

    while (1) {

        sem_wait(&sem);
        // vai buscar o buffer
        CAB_BUFFER* buffer = reserve(cab);
        
        if (buffer != NULL) {
           
            get_mes(cab);
            // image processing function
            // near_object = process_object_fucntion(img)
        }

        unget(cab, buffer);
        sem_post(&sem);
    }
    return (void *) 1;
}

void* blue_square(void* arg){

    while(1){
        sem_wait(&sem);

        CAB_BUFFER* buffer = reserve(cab);

        if (buffer != NULL){
            get_mes(cab);
            //count_object = process_object_fucntion(img)
        }

        unget(cab, buffer);
        sem_post(&sem);

    }

    return (void *) 1;
}

void* edge_detection(void* arg) {

    while (1) {
        sem_wait(&sem);

        // vai buscar o buffer
        CAB_BUFFER* buffer = reserve(cab);
        
        
        if (buffer != NULL) {
           
            put_mes(cab, buffer);
            // image processing function
            ////edges = fucntion(img)
        }
        unget(cab, buffer);
        sem_post(&sem);
    }
    return (void *) 1;
}

void* process_grey_scale(void* arg) {

    while (1) {
        sem_wait(&sem);

        // vai buscar o buffer
        CAB_BUFFER* buffer = reserve(cab);
        
        
        if (buffer != NULL) {
           
            put_mes(cab, buffer);
            // image processing function
            //imgMakeGrayScale(shMemPtr, width, height,)
        }
        unget(cab, buffer);
        sem_post(&sem);
    }
    return (void *) 1;
}

// void print_all_threads(void*){

//     sem_wait(&sem);

//     while(1){
//         printf("Near objects: %d\n", near_object);
//         printf("Object count: %d\n", object_count);
//         printf("Edges: %d\n", edges);
//     }

//     sem_post(&sem);
// }