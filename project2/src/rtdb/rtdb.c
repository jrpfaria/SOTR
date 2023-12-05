#include "rtdb.h"
#include <stdlib.h>
#include <stdio.h>

RTDB* rtdb_create(void) {
    RTDB* db = (RTDB*)malloc(sizeof(RTDB));
    db->io = 0x00;
    
    for (int i = 0; i < 20; i++) 
        db->temp[i] = 0;

    // Mutex to protect the database
    k_mutex_init(&db->mutex);

    return db;
}

int rtdb_get_high(RTDB* db) {
    k_mutex_lock(&db->mutex, K_FOREVER);

    int high = db->temp[0];
    for (int i = 1; i < 20; i++)
        if (db->temp[i] > high)
            high = db->temp[i];
    
    k_mutex_unlock(&db->mutex);

    return high;
}

int rtdb_get_low(RTDB* db) {
    k_mutex_lock(&db->mutex, K_FOREVER);

    int low = db->temp[0];
    for (int i = 1; i < 20; i++)
        if (db->temp[i] < low)
            low = db->temp[i];
    
    k_mutex_unlock(&db->mutex);

    return low;
}

void rtdb_insert_temp(RTDB* db, int temp) {
    k_mutex_lock(&db->mutex, K_FOREVER);

    for (int i = 1; i < 20; i++)
        db->temp[i] = db->temp[i-1];
    db->temp[0] = temp;

    k_mutex_unlock(&db->mutex);
}

void set_outputs(RTDB* db, unsigned char o) {
    k_mutex_lock(&db->mutex, K_FOREVER);
    
    db->io = (db->io & 0xF0) | (o & 0x0F);
    printf("io: %x\n", db->io);
    
    k_mutex_unlock(&db->mutex);
}

void set_output_at_index(RTDB* db, int index, unsigned char o) {
    k_mutex_lock(&db->mutex, K_FOREVER);

    if (o) 
        db->io |= (1 << index);
    else 
        db->io &= ~(1 << index);

    printf("io: %x\n", db->io);

    k_mutex_unlock(&db->mutex);
}

void set_inputs(RTDB* db, unsigned char i) {
    k_mutex_lock(&db->mutex, K_FOREVER);

    db->io = (i << 4) | (db->io & 0x0F);

    k_mutex_unlock(&db->mutex);
}