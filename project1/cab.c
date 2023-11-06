#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>

#include "./include/cab.h"

CAB *open_cab(int size, int num_of_tasks)
{
    // Allocate memory for CAB and CAB_BUFFER structs
    CAB* c = malloc(sizeof(CAB));
    CAB_BUFFER* buffer = malloc(sizeof(CAB_BUFFER) * (++num_of_tasks));

    if (c == NULL || buffer == NULL){
        printf("Error: malloc failed in CAB\n");
        return NULL;
    }

    // Initialize CAB struct
    c->free = buffer;
    c->mrb = buffer;
    c->max_buffer = num_of_tasks;
    c->buffer_size = size;

    // Initialize CAB_BUFFER struct
    for(int i = 0; i < num_of_tasks; i++) {
        c->mrb[i].next = &c->mrb[i+1];
        c->mrb[i].use = 0;
        c->mrb[i].data = NULL;    
    }

    c->mrb[num_of_tasks - 1].next = NULL;

    return c;
}

CAB_BUFFER *reserve(CAB* c)
{
    CAB_BUFFER* p = NULL;
    for (int i = 0; i < c->max_buffer; i++)
        if (c->mrb[i].use == 0) 
            p = &c->mrb[i];
    return p;
}

void put_mes(CAB *c, CAB_BUFFER* buffer, void* data)
{
    if(buffer->data == NULL) {
        buffer->data = malloc(c->buffer_size);
    }
    void* newBufferData = malloc(c->buffer_size);
    if(newBufferData == NULL) {
        printf("Error allocating memory (cab-put_mes)");
    }
    memcpy(newBufferData, data, c->buffer_size);
    void* aux = buffer->data;
    buffer->data = newBufferData;
    free(aux);
    
    printf("put_mes\n");
    c->mrb = buffer;
}

void *get_mes(CAB *c)
{
    c->mrb->use++;
    return c->mrb->data;
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