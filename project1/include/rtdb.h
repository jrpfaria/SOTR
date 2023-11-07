#ifndef __RTDB__
#define __RTDB__

typedef struct db DB;
typedef struct db_buffer DB_BUFFER;

struct db{
    int size_of_data;       // size of data in bytes
    int amount_of_buffers;  // amount of buffers
    int mru;                // most recently updated buffer
    DB_BUFFER *buffer;      // pointer to buffer
};

struct db_buffer{
    DB_BUFFER *next;        // pointer to next buffer
    unsigned char* data;             // pointer to data
};

DB* initDataBase(int, int);
void* getMostRecentData(DB*);
void setBufferAtIndex(DB*, int, void*);

#endif