#include "rtdb.h"

RTDB* rtdb_create(void) {
    RTDB* db = (RTDB*)malloc(sizeof(RTDB));
    db->io = 0x00;
    
    for (int i = 0; i < 20; i++) 
        db->temp[i] = 0.0;

    return db;
}

float rtdb_get_high(RTDB* db) {
    float high = db->temp[0];
    for (int i = 1; i < 20; i++)
        if (db->temp[i] > high)
            high = db->temp[i];
    return high;
}

float rtdb_get_low(RTDB* db) {
    float low = db->temp[0];
    for (int i = 1; i < 20; i++)
        if (db->temp[i] < low)
            low = db->temp[i];
    return low;
}

void rtdb_insert_temp(RTDB* db, float temp) {
    for (int i = 1; i < 20; i++)
        db->temp[i] = db->temp[i-1];
    db->temp[0] = temp;
}

void set_outputs(RTDB* db, char o) {
    db->io = db->io & 0xF0 | o & 0x0F;
}

void set_output_at_index(RTDB* db, int index, char o) {
    if (o) 
        db->io |= (1 << index);
    else 
        db->io &= ~(1 << index);
}

void set_inputs(RTDB* db, char i) {
    db->io = (i << 4) | db->io & 0x0F;
}