#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "../include/cab.h"

static int k = 0, j = 0;
static const char *testData[] = {"Zero", "Ten", "Twenty", "Thirty", "Forty", "Fifty", "Sixty", "Seventy", "Eighty", "Ninety"};
static CAB_BUFFER* top;

// Dummy task function for testing
void* dummy_task(void* arg) {
    CAB* c = (CAB*)arg;

    fprintf(stderr, "MRB: %p\n", c->mrb);

    if (c->mrb->next == NULL)
        for (int i = 0; i < 5; ++i)
            unget(c, &top[j++]);
    if (j >= 3)
        j = 0;

    CAB_BUFFER* buffer = reserve(c);
    char* new_data = malloc(c->buffer_size);

    char* data = malloc(c->buffer_size);
    
    if (k < 10) {
        strcpy(new_data, testData[k++]);
    } else {
        strcpy(new_data, "Z");
    }

    memcpy(data, new_data, c->buffer_size);

    // Put the result in the CAB
    put_mes(c, buffer, data);
    fprintf(stderr, "Data in MRB: %s\n", (char*)get_mes(c));

    fprintf(stderr, "Data in CAB BUFFERS: ");
    for (int t = 0; t < 6; ++t)
        if (top[t].data != NULL)
            fprintf(stderr, "%s, ", ((char*)top[t].data));
    fprintf(stderr, "\n");

    return NULL;
}

int main() {
    // Create a CAB with a buffer size of 20 chars and 5 tasks
    CAB* myCab = open_cab((sizeof(char) * 20), 5);
    top = &myCab->mrb[0];

    // Create threads and perform tasks
    for (int i = 0; i < 10; ++i) {
        dummy_task((void*)myCab);
    }

    // Clean up
    delete_cab(myCab);

    return 0;
}
