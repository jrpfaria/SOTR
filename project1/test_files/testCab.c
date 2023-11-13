#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include "./include/cab.h"

static int k = 0, j = 0;
static char *names[10] = {"A", "B", "C", "D", "E", "F", "G", "H", "I", "J"}; 
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static CAB_BUFFER* top;

// Dummy task function for testing
void* dummy_task(void* arg) {
    pthread_mutex_lock(&mutex);
    CAB* c = (CAB*)arg;

    fprintf(stderr, "Free: %p\n", c->free);
    fprintf(stderr, "MRB: %p\n", c->mrb);

    if (c->free->next == NULL)
        unget(c, &top[j++]);
    if (j >= 3)
        j = 0;

    CAB_BUFFER* buffer = reserve(c);
    char* new_data = malloc(sizeof(char));

    // Perform some task
    char* data = malloc(sizeof(char));
    
    if (k<10){
        *new_data = names[k++][0];
    } else {
        *new_data = 'Z';
    }

    memcpy(data, new_data, sizeof(char));

    // Put the result in the CAB
    put_mes(c, buffer, data);
    fprintf(stderr, "Data in MRB: %c\n", *((char*)get_mes(c)));

    free(data);
    pthread_mutex_unlock(&mutex);

    return NULL;
}

int main() {
    // Create a CAB with a buffer size of 128 and 5 tasks
    CAB* myCab = open_cab(sizeof(char), 5);
    top = &myCab->mrb[0];

    // Create an array of threads to simulate tasks
    pthread_t threads[10];

    // Create threads and perform tasks
    for (int i = 0; i < 10; ++i) {
        pthread_create(&threads[i], NULL, dummy_task, (void*)myCab);
    }

    // Wait for threads to finish
    for (int i = 0; i < 10; ++i) {
        pthread_join(threads[i], NULL);
    }

    // Clean up
    delete_cab(myCab);

    return 0;
}
