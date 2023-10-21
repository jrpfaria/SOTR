#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "cab.h"

CAB *open_cab(int size, int num_of_tasks)
{
    // Allocate memory for CAB struct
    // num(pixels) * (3(rgb) + 1(align)) * num_of_tasks + 1)
    CAB* c = malloc(size * sizeof(CAB) * (num_of_tasks + 1));
    
    if (c == NULL){
        printf("Error: malloc failed\n");
        return NULL;
    }

    // Initialize CAB struct
    c->free = NULL;
    c->mrb = NULL;
    c->max_buffer = num_of_tasks + 1;
    c->buffer_size = size;

    return c;
}

CAB_BUFFER *reserve(CAB* c)
{
    if (c->mrb == NULL){
        printf("Error: no message\n");
        return NULL;
    }

    CAB_BUFFER *p = c->mrb;
    c->free = p->next;
    return p;
}

void put_mes(CAB *c, CAB_BUFFER *buffer)
{
    if (c->mrb == NULL){
        printf("Error: no message\n");
        return;
    }

    if(c->mrb->use == 0) {
        c->mrb->next = c->free;
        c->free = c->mrb;
    }
    
    c->mrb = buffer;
}

void *get_mes(CAB *c)
{
    if (c->mrb == NULL){
        printf("Error: no message\n");
        return NULL;
    }
   
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