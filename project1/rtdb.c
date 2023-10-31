#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "./include/rtdb.h"

DB* initDataBase(int size_of_data, int amount_of_buffers) {
    DB* db = (DB*) malloc(sizeof(DB));
    db->amount_of_buffers = amount_of_buffers;
    db->size_of_data = size_of_data;
    db->mru = -1;
    db->buffer = (DB_BUFFER*) malloc(sizeof(DB_BUFFER) * amount_of_buffers);

    DB_BUFFER* buffer = db->buffer;

    // Initialize CAB_BUFFER struct
    for(int i = 0; i < amount_of_buffers; i++) {
        buffer->next = (i != amount_of_buffers - 1) ? buffer + sizeof(DB_BUFFER) : NULL;
        buffer->data = NULL;
        buffer = buffer->next;        
    }

    return db;
}

void* getMostRecentData(DB* dbPtr) {
    return dbPtr + (dbPtr->mru * sizeof(DB_BUFFER));
}

void setBufferAtIndex(DB* dbPtr, int index, void* data) {
    if (index >= dbPtr->amount_of_buffers) {
        fprintf(stderr, "Error [rtdb-SetBufferAtIndex]: index out of bounds\n");
        return;
    }
    DB_BUFFER* buffer = dbPtr + (index * sizeof(DB_BUFFER));
    memcpy(buffer->data, data, dbPtr->size_of_data);
    dbPtr->mru = index;
}