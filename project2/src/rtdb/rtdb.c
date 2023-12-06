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

    int high = db->max;
    
    k_mutex_unlock(&db->mutex);

    return high;
}

int rtdb_get_low(RTDB* db) {
    k_mutex_lock(&db->mutex, K_FOREVER);

    int low = db->min;
    
    k_mutex_unlock(&db->mutex);

    return low;
}

void rtdb_insert_temp(RTDB* db, int temp) {
    k_mutex_lock(&db->mutex, K_FOREVER);
    
    int new_max = 0, new_min = 0;
    int lost_temp = db->temp[19];
    
    if (lost_temp == db->max) 
        new_max = 1;
    else if(lost_temp == db->min)
        new_min = 1;

    for (int i = 1; i < 20; i++)
        db->temp[i] = db->temp[i-1];
    db->temp[0] = temp;

    if (new_max){
        db->max = temp;
        for (int i = 0; i < 20; i++)
            if (db->temp[i] > db->max)
                db->max = db->temp[i];
    }

    if (new_min){
        db->min = temp;
        for (int i = 0; i < 20; i++)
            if (db->temp[i] < db->min)
                db->min = db->temp[i];
    }

    k_mutex_unlock(&db->mutex);
}

void rtdb_set_outputs(RTDB* db, unsigned char o) {
    k_mutex_lock(&db->mutex, K_FOREVER);
    
    db->io = (db->io & 0xF0) | (o & 0x0F);
    printf("io: %x\n", db->io);
    
    k_mutex_unlock(&db->mutex);
}

void rtdb_set_output_at_index(RTDB* db, int index, unsigned char o) {
    k_mutex_lock(&db->mutex, K_FOREVER);

    if (o) 
        db->io |= (1 << index);
    else 
        db->io &= ~(1 << index);

    printf("io: %x\n", db->io);

    k_mutex_unlock(&db->mutex);
}

void rtdb_set_inputs(RTDB* db, unsigned char i) {
    k_mutex_lock(&db->mutex, K_FOREVER);

    db->io = (i << 4) | (db->io & 0x0F);

    k_mutex_unlock(&db->mutex);
}

unsigned char rtdb_get_outputs(RTDB* db) {
    k_mutex_lock(&db->mutex, K_FOREVER);

    unsigned char o = db->io & 0x0F;

    k_mutex_unlock(&db->mutex);

    return o;
}

unsigned char rtdb_get_inputs(RTDB* db) {
    k_mutex_lock(&db->mutex, K_FOREVER);

    unsigned char i = db->io >> 4;

    k_mutex_unlock(&db->mutex);

    return i;
}

int rtdb_get_last_temp(RTDB* db) {
    k_mutex_lock(&db->mutex, K_FOREVER);

    int last_temp = db->temp[0];

    k_mutex_unlock(&db->mutex);

    return last_temp;
}

int* rtdb_get_temps(RTDB* db, int* tmp) {
    k_mutex_lock(&db->mutex, K_FOREVER);

    for (int i = 0; i < 20; i++)
        tmp[i] = db->temp[i];

    k_mutex_unlock(&db->mutex);

    return tmp;
}

void rtdb_reset_temp_history(RTDB* db) {
    k_mutex_lock(&db->mutex, K_FOREVER);

    for (int i = 0; i < 20; i++)
        db->temp[i] = 0;
    
    db->max = 0;
    
    db->min = 0;

    k_mutex_unlock(&db->mutex);
}