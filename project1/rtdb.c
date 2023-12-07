#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "./include/rtdb.h"

DB* initDataBase(int size_of_data, int amount_of_buffers) {
    DB* db = (DB*) malloc(sizeof(DB));
    db->amount_of_buffers = amount_of_buffers;
    db->size_of_data = size_of_data;
    db->mru = 0;
    db->buffer = (DB_BUFFER*) malloc(sizeof(DB_BUFFER) * amount_of_buffers);

    if (db == NULL || db->buffer == NULL){
        printf("Error: malloc failed in RTDB\n");
        return NULL;
    }

    // Initialize CAB_BUFFER struct
    for(int i = 0; i < amount_of_buffers; i++) {
        db->buffer[i].next = &db->buffer[i+1];
        db->buffer[i].data = (unsigned char*) NULL;
    }

    return db;
}

void* getMostRecentData(DB* dbPtr) {
    if (dbPtr->buffer[dbPtr->mru].data == NULL) {
        fprintf(stderr, "Error [rtdb-getMostRecentData]: no data in database\n");
        return NULL;
    }

    return dbPtr->buffer[dbPtr->mru].data;
}

void setBufferAtIndex(DB* dbPtr, int index, void* data) {
    if (data == (void*)NULL){
        fprintf(stderr, "Error [rtdb-SetBufferAtIndex]: data is NULL\n");
        return;
    }
    
    if (index >= dbPtr->amount_of_buffers) {
        fprintf(stderr, "Error [rtdb-SetBufferAtIndex]: index out of bounds\n");
        return;
    }

    if(dbPtr->buffer[index].data == NULL) {
        dbPtr->buffer[index].data = malloc(dbPtr->size_of_data);
    }

    void* newBufferData = malloc(dbPtr->size_of_data);
    if(newBufferData == NULL) {
        printf("Error allocating memory (setBufferAtIndex)\n");
        return;
    }

    memcpy(newBufferData, data, dbPtr->size_of_data);
    dbPtr->buffer[index].data = newBufferData;

    dbPtr->mru = index;
}