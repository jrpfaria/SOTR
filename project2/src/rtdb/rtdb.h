#ifndef __RTDB__
#define __RTDB__
#include <zephyr/kernel.h>

typedef struct rtdb RTDB;

struct rtdb{
    // Since we are using a bitfield, and there are 4 LEDs and 4 Buttons 
    // we can use a single char to store the state of all of them
    unsigned char io;
    // Array to store the past 20 temperature values
    int temp[20];
    int max;
    int min;
    struct k_mutex mutex;
};

RTDB* rtdb_create(void);

// Temperature functions
int rtdb_get_high(RTDB*);
int rtdb_get_low(RTDB*);
void rtdb_insert_temp(RTDB*, int);
void rtdb_reset_temp_history(RTDB*);
int* rtdb_get_temps(RTDB*, int*);
int rtdb_get_last_temp(RTDB*);

// IO functions
void rtdb_set_outputs(RTDB*, unsigned char);
void rtdb_set_output_at_index(RTDB*, int, unsigned char);
void rtdb_set_inputs(RTDB*, unsigned char);
unsigned char rtdb_get_inputs(RTDB*);
unsigned char rtdb_get_outputs(RTDB*);

#endif
