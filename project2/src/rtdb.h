#ifndef __RTDB__
#define __RTDB__

typedef struct rtdb RTDB;

struct rtdb{
    // Since we are using a bitfield, and there are 4 LEDs and 4 Buttons 
    // we can use a single char to store the state of all of them
    char io;
    // Float array to store the past 20 temperature values
    float temp[20];
};

RTDB* rtdb_create(void);

// Temperature functions
float rtdb_get_high(RTDB*);
float rtdb_get_low(RTDB*);
void rtdb_insert_temp(RTDB*, float);

// IO functions
void set_outputs(RTDB*, char);
void set_output_at_index(RTDB*, int, char);
void set_inputs(RTDB*, char);

#endif
