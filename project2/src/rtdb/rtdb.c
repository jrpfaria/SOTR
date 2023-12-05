#include "rtdb.h"
#include <stdlib.h>
#include <stdio.h>

RTDB* rtdb_create(void) {
    RTDB* db = (RTDB*)malloc(sizeof(RTDB));
    db->io = 0x00;
    
    for (int i = 0; i < 20; i++) 
        db->temp[i] = 0;

    return db;
}

int rtdb_get_high(RTDB* db) {
    int high = db->temp[0];
    for (int i = 1; i < 20; i++)
        if (db->temp[i] > high)
            high = db->temp[i];
    return high;
}

int rtdb_get_low(RTDB* db) {
    int low = db->temp[0];
    for (int i = 1; i < 20; i++)
        if (db->temp[i] < low)
            low = db->temp[i];
    return low;
}

void rtdb_insert_temp(RTDB* db, int temp) {
    for (int i = 1; i < 20; i++)
        db->temp[i] = db->temp[i-1];
    db->temp[0] = temp;
}

void set_outputs(RTDB* db, unsigned char o) {
    db->io = (db->io & 0xF0) | (o & 0x0F);
    printf("io: %x\n", db->io);
}

void set_output_at_index(RTDB* db, int index, unsigned char o) {
    if (o) 
        db->io |= (1 << index);
    else 
        db->io &= ~(1 << index);

    printf("io: %x\n", db->io);
}

void set_inputs(RTDB* db, unsigned char i) {
    db->io = (i << 4) | (db->io & 0x0F);
}