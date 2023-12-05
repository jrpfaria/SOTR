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
    struct k_mutex mutex;
};

RTDB* rtdb_create(void);

// Temperature functions
int rtdb_get_high(RTDB*);
int rtdb_get_low(RTDB*);
void rtdb_insert_temp(RTDB*, int);

// IO functions
void set_outputs(RTDB*, unsigned char);
void set_output_at_index(RTDB*, int, unsigned char);
void set_inputs(RTDB*, unsigned char);

#endif
