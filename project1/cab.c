#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>

#include "./include/cab.h"
#include "./include/threads.h"

CAB *open_cab(int size, int num_of_tasks)
{
    // Allocate memory for CAB and CAB_BUFFER structs
    CAB* c = malloc(sizeof(CAB));
    CAB_BUFFER* buffer = malloc(sizeof(CAB_BUFFER) * (++num_of_tasks));

    if (c == NULL){
        printf("Error: malloc failed\n");
        return NULL;
    }

    // Initialize CAB struct
    c->free = buffer;
    c->mrb = buffer;
    c->max_buffer = num_of_tasks;
    c->buffer_size = size;

    // Initialize CAB_BUFFER struct
    for(int i = 0; i < num_of_tasks; i++) {
        buffer->next = (i != num_of_tasks - 1) ? buffer + sizeof(CAB_BUFFER) : NULL;
        buffer->use = 0;
        buffer->data = NULL;
        buffer = buffer->next;        
    }

    return c;
}

CAB_BUFFER *reserve(CAB* c)
{
    CAB_BUFFER *p = c->mrb;
    c->free = p->next;
    
    return p;
}

void put_mes(CAB *c, CAB_BUFFER *buffer)
{
    if(c->mrb->use == 0) {
        c->mrb->next = c->free;
        c->free = c->mrb;
    }
    
    c->mrb = buffer;
}

void *get_mes(CAB *c)
{
    CAB_BUFFER *p = c->mrb;
    p = c->mrb;
    p->use = p->use + 1;

    return p;
}

void unget(CAB *c, CAB_BUFFER *p)
{
    p->use = p->use - 1;
    if((p->use == 0) && (p != c->mrb)) {
        p->next = c->free;
        c->free = p;
    }
}

void delete_cab(CAB *c)
{
    free(c);
}