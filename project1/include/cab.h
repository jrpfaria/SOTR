#ifndef __CAB__
#define __CAB__

typedef struct cab CAB;
typedef struct cab_buffer CAB_BUFFER;

struct cab {
    CAB_BUFFER* free;
    CAB_BUFFER* mrb;
    int max_buffer;
    int buffer_size;
};

struct cab_buffer {
    CAB_BUFFER* next;
    int use;
    void* data;
};

CAB *open_cab(int, int);

CAB_BUFFER *reserve(CAB*);

void put_mes(CAB*, CAB_BUFFER*, void*);

void *get_mes(CAB*);

void unget(CAB*, CAB_BUFFER*);

void delete_cab(CAB*);

#endif