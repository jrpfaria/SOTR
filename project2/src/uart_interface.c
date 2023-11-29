#include "uart_interface.h"
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

char uart_lut(char c) {
    if (isdigit(c)) {
        return c - '0';
    } else if (isxdigit(c)) {
        return toupper(c) - 'A' + 10;
    } else {
        // Handle invalid characters if needed
        return 0; // Default value for invalid characters
    }
}

void uart_interface(RTDB* db, char* cmd){
    printf("cmd: %s\n", cmd);

    int index;
    char bin;

    if (cmd[1] == '0'){
        switch (cmd[2])
        {
            case '0':
                index = (int) uart_lut(cmd[3]);
                bin = uart_lut(cmd[4]);
                set_inputs(db, 0xFF);
                set_output_at_index(db, --index, bin);
                break;
            
            case '1':
                bin = uart_lut(cmd[3]);
                set_inputs(db, 0xFF);
                set_outputs(db, bin);
                break;

            default: break;
        }
    }
}