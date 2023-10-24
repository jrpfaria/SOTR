#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include "cab.h"

#include "imageViewer.c"


// treads init
pthread_t obj_detection;
pthread_t obj_position;
pthread_t landmarks;
//pthread_t distance;

sem_t sem;

CAB* cab;


// define threads priority
#define OBJ_DETECTION_PRIORITY 1 // run at all frames
#define OBJ_POSITION_PRIORITY 2 
#define LANDMARKS_PRIORITY 1
//#define DISTANCE_PRIORITY 4

void* detect_object(void*);
void* calculate_object_position(void*);
void* process_landmarks(void*);
//void* calculate_distance(void*);

int main(void) {

    // init sem
    sem_init(&sem, 0, 1);

    int size = width * height * sizeof(u_int32_t);
  
    cab = open_cab(size, 1);
   
    pthread_create(&obj_detection, NULL, detect_object, NULL);
    pthread_create(&obj_position, NULL, calculate_object_position, NULL);
    pthread_create(&landmarks, NULL, process_landmarks, NULL);
    
    // dar release do cab
    delete_cab(cab); 
    return 0;
}

void* detect_object(void* arg) {

    while (1) {
        sem_wait(&sem);

        // vai buscar o buffer
        CAB_BUFFER* buffer = reserve(cab);
        
        
        if (buffer != NULL) {
           
            put_mes(cab, buffer);

            // image processing function
            //detect_object_function();
        }

        unget(cab, buffer);
        sem_post(&sem);
    }
    return (void *) 1;
}

void* object_position(void* arg){

    while(1){
        sem_wait(&sem);

        CAB_BUFFER* buffer = reserve(cab);

        if (buffer != NULL){
            get_mes(cab);
        }

        unget(cab, buffer);
        sem_post(&sem);

    }

    return (void *) 1;
}


