#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "cab.h"

CAB *open_cab(int size, int num_of_tasks)
{
    // Alloc memory for CAB struct
    // num(pixels) * (3(rgb) + 1(align)) * num_of_tasks + 1)
    CAB *cab = malloc(size * sizeof(uint32_t) * (num_of_tasks + 1));

    if (cab == NULL)
    {
        printf("Error: malloc failed\n");
        return NULL;
    }

    // Set CAB values
    cab->most_recent = NULL;
    cab->size_of_data = size;
    cab->num_of_tasks = num_of_tasks;

    return cab;
}

void *reserve(CAB *cabId)
{
    void *p;
    
    for (int i = 0; i < cabId->num_of_tasks; i++)
    {
        if (cabId->buffers[i][1] == 0)
        {
            cabId->buffers[i][1] = 1;
            p = cabId->buffers[i][0];
            return p;
        }
    }

    printf("Error: no available buffers\n");
    return NULL;
}

void put_mes(CAB *cabId, void *buffer)
{
    // if c.mrb.use == 0
    //    c.mrb.next = c.free (if not accessed deallocate the mrb)
    //    c.mrb.free = c.mrb
    if (cabId->buffers[cabId->most_recent][1] == 0)
    {

    }
    // c.mrb = p (update mrb)
}

void *get_mes(CAB *cabId)
{
    void *p;
    // p = c.mrb (get pointer to most recent buffer)
    // p.use = p.use + 1 (increment use)
    return p;
}

void unget(CAB *cabId, void *pointer)
{
    // p.use = p.use - 1 (decrement use)
    // if p.use == 0 && p != c.mrb
    //    p.next = c.free
    //    c.free = p
}

void delete_cab(CAB *cabId)
{
    free(cabId);
}