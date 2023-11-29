#include <stdio.h>
#include "include/uart_interface.h"

int main(void)
{
    RTDB* db = rtdb_create();
    char cmd[9] = {'!', '0', '1', 'F', '1', '1', '9', '4', '#'};

    for (int i = 0; i < 9; i++) {
        printf("%c", cmd[i]);
    }
    printf("\n");

    uart_interface(db, cmd);

    return 0;
}