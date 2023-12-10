#include "../rtdb/rtdb.h"

char* uart_interface(RTDB*, char*);
char* uart_generate_io_payload(unsigned char);
char* uart_generate_temp_payload(int*, int);
char* uart_apply_checksum(char*, int);
int uart_checkSum(char*);
char uart_lut(char);
char* create_response(char*, char*);