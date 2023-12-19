#include "rtdb.h"
#include <stdlib.h>
#include <stdio.h>

#define DEBUG 0

RTDB *rtdb_create(void)
{
    RTDB *db = (RTDB *)malloc(sizeof(RTDB));
    db->io = 0x00;

    db->most_recent_temp = 0;
    for (int i = 0; i < 20; i++)
        db->temp[i] = 0;

    // Mutex to protect the database
    k_mutex_init(&db->io_mutex);
    k_mutex_init(&db->t_mutex);

    return db;
}

int rtdb_get_high(RTDB *db)
{
    k_mutex_lock(&db->t_mutex, K_FOREVER);

    int high = db->temp[0];
    for (int i = 0; i < 20; i++)
        if (db->temp[i] > high)
            high = db->temp[i];

    k_mutex_unlock(&db->t_mutex);

    return high;
}

int rtdb_get_low(RTDB *db)
{
    k_mutex_lock(&db->t_mutex, K_FOREVER);

    int low = db->temp[0];
    for (int i = 0; i < 20; i++)
        if (db->temp[i] < low)
            low = db->temp[i];

    k_mutex_unlock(&db->t_mutex);

    return low;
}

void rtdb_insert_temp(RTDB *db, int temp)
{
    k_mutex_lock(&db->t_mutex, K_FOREVER);

    db->temp[db->most_recent_temp] = temp;
    db->most_recent_temp = (db->most_recent_temp + 1) % 20;

    k_mutex_unlock(&db->t_mutex);
}

void rtdb_set_outputs(RTDB *db, unsigned char o)
{
    k_mutex_lock(&db->io_mutex, K_FOREVER);

    db->io = (db->io & 0xF0) | (o & 0x0F);
    if (DEBUG)
        printf("io: %x\n", db->io);

    k_mutex_unlock(&db->io_mutex);
}

void rtdb_set_output_at_index(RTDB *db, int index, unsigned char o)
{
    k_mutex_lock(&db->io_mutex, K_FOREVER);

    if (o)
        db->io |= (1 << index);
    else
        db->io &= ~(1 << index);

    if (DEBUG)
        printf("io: %x\n", db->io);

    k_mutex_unlock(&db->io_mutex);
}

void rtdb_set_inputs(RTDB *db, unsigned char i)
{
    k_mutex_lock(&db->io_mutex, K_FOREVER);

    db->io = (i << 4) | (db->io & 0x0F);

    k_mutex_unlock(&db->io_mutex);
}

unsigned char rtdb_get_outputs(RTDB *db)
{
    k_mutex_lock(&db->io_mutex, K_FOREVER);

    unsigned char o = db->io & 0x0F;

    k_mutex_unlock(&db->io_mutex);

    return o;
}

unsigned char rtdb_get_inputs(RTDB *db)
{
    k_mutex_lock(&db->io_mutex, K_FOREVER);

    unsigned char i = db->io >> 4;

    k_mutex_unlock(&db->io_mutex);

    return i;
}

int rtdb_get_last_temp(RTDB *db)
{
    k_mutex_lock(&db->t_mutex, K_FOREVER);

    int last_temp = db->temp[db->most_recent_temp];

    k_mutex_unlock(&db->t_mutex);

    return last_temp;
}

int *rtdb_get_temps(RTDB *db, int *tmp)
{
    k_mutex_lock(&db->t_mutex, K_FOREVER);

    for (int i = 0; i < 20; i++)
        tmp[i] = db->temp[i];

    k_mutex_unlock(&db->t_mutex);

    return tmp;
}

void rtdb_reset_temp_history(RTDB *db)
{
    k_mutex_lock(&db->t_mutex, K_FOREVER);

    for (int i = 0; i < 20; i++)
        db->temp[i] = 0;
    db->most_recent_temp = 0;

    k_mutex_unlock(&db->t_mutex);
}