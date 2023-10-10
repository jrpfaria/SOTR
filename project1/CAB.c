#include <stdio.h>
#include <stdlib.h>

#define BUFFER_SIZE 10

typedef struct {
    char* buffer;
    int size;
    int front;
    int rear;
} CyclicBuffer;

CyclicBuffer* createBuffer(int size) {
    CyclicBuffer* newBuffer = (CyclicBuffer*)malloc(sizeof(CyclicBuffer));
    newBuffer->buffer = (char*)malloc(size * sizeof(char));
    newBuffer->size = size;
    newBuffer->front = -1;
    newBuffer->rear = -1;
    return newBuffer;
}

void printBufferInfo(CyclicBuffer* buffer);

int isFull(CyclicBuffer* buffer) {
    return (buffer->front == 0 && buffer->rear == buffer->size - 1) ||
           (buffer->rear == (buffer->front - 1) % (buffer->size - 1));
}

int isEmpty(CyclicBuffer* buffer) {
    return buffer->front == -1;
}

void enqueue(CyclicBuffer* buffer, char data) {
    if (isFull(buffer)) {
        printf("Buffer is full. Cannot enqueue.\n");
        return;
    }

    if (isEmpty(buffer))
        buffer->front = buffer->rear = 0;
    else
        buffer->rear = (buffer->rear + 1) % buffer->size;

    buffer->buffer[buffer->rear] = data;
    printf("Enqueued: %c\n", data);
    printBufferInfo(buffer);
}

char dequeue(CyclicBuffer* buffer) {
    char data;

    if (isEmpty(buffer)) {
        printf("Buffer is empty. Cannot dequeue.\n");
        return '\0';
    }

    data = buffer->buffer[buffer->front];

    if (buffer->front == buffer->rear)
        buffer->front = buffer->rear = -1;
    else
        buffer->front = (buffer->front + 1) % buffer->size;

    printf("Dequeued: %c\n", data);
    printBufferInfo(buffer);

    return data;
}

void printBufferInfo(CyclicBuffer* buffer) {
    printf("Buffer Info: Front=%d, Rear=%d, Full=%s, Empty=%s\n",
           buffer->front, buffer->rear,
           isFull(buffer) ? "Yes" : "No",
           isEmpty(buffer) ? "Yes" : "No");

    printf("Elements: ");
    int i;
    for (i = 0; i < buffer->size; ++i) {
        int index = (buffer->front + i) % buffer->size;
        if (index <= buffer->rear && buffer->buffer[index] != '\0') {
            printf("%c ", buffer->buffer[index]);
        } else {
            printf("_ ");
        }
    }
    printf("\n");
}


void destroyBuffer(CyclicBuffer* buffer) {
    free(buffer->buffer);
    free(buffer);
}

int main() {
    CyclicBuffer* buffer = createBuffer(BUFFER_SIZE);

    enqueue(buffer, 'A');
    enqueue(buffer, 'B');
    enqueue(buffer, 'C');
    enqueue(buffer, 'D');
    enqueue(buffer, 'E');
    enqueue(buffer, 'F');
    enqueue(buffer, 'G');
    enqueue(buffer, 'H');
    enqueue(buffer, 'I');
    enqueue(buffer, 'J');
    enqueue(buffer, 'K'); // This will exceed the buffer size and trigger a "Buffer is full" message

    dequeue(buffer);

    enqueue(buffer, 'L');

    dequeue(buffer);
    dequeue(buffer);
    dequeue(buffer);

    destroyBuffer(buffer);

    return 0;
}