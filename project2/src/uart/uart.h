#include "../rtdb/rtdb.h"

char* uart_interface(RTDB*, char*);
char* generate_payload(unsigned char);
char* uart_apply_checksum(char*, int);
int uart_checkSum(char*);
char uart_lut(char);