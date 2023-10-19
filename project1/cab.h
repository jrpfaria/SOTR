#ifndef __CAB__
#define __CAB__

typedef struct{
    int most_recent; // the most recent buffer
    int size_of_data; // size of the data in the buffer
    int num_of_tasks; // number of tasks
    void *buffers[num_of_tasks][2]; // array of buffers (2D array to know if they're being used or not)
}CAB;

CAB *open_cab(int size, int num_of_tasks);

void *reserve(CAB *cabId);

void put_mes(CAB *cabId, void *buffer);

void *get_mes(CAB *cabId);

void unget(void *img_pointer, CAB *cabId);

void delete_cab(CAB *cabId);

#endif