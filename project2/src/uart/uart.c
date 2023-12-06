#include "../rtdb/rtdb.h"
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

int checkSum(char* cmd){
    int i = -1, sum = 0, checksum = 0;

    while(cmd[i] != '#') i++;    
    
    int aux_iterator = i - 3;
    int aux_multiplier = 1;
    
    for (; i > aux_iterator; i--, aux_multiplier *= 10)
        sum += (cmd[i] - '0') * aux_multiplier;

    for (; i > 0; i--) checksum += cmd[i];

    return sum == checksum;
}

void uart_interface(RTDB* db, char* cmd){
    printf("cmd: %s\n", cmd);

    int index;
    char bin;
    int high, low;
    int temps[20];

    if (cmd[1] == '0'){
        switch (cmd[2])
        {
            case '0':
                printk("Case 0; Setting Output at Index\n");
                index = (int) uart_lut(cmd[3]);
                bin = uart_lut(cmd[4]);
                rtdb_set_output_at_index(db, --index, bin);
                printf("outputs: %x\n", rtdb_get_outputs(db));
                break;
            
            case '1':
                printk("Case 1; Setting All Outputs\n");
                bin = uart_lut(cmd[3]);
                rtdb_set_outputs(db, bin);
                printf("outputs: %x\n", rtdb_get_outputs(db));
                break;

            case '2':
                printk("Case 2; Getting All Inputs\n");
                unsigned char i = rtdb_get_inputs(db);
                printf("i: %x\n", i);
                break;

            case '3':
                printk("Case 3; Getting All Outputs\n");
                unsigned char o = rtdb_get_outputs(db);
                printf("o: %x\n", o);
                break;

            case '4':
                printk("Case 4; Getting Last Temp\n");
                int last_temp = rtdb_get_last_temp(db);
                printf("last_temp: %d\n", last_temp);
                break;

            case '5':
                printk("Case 5; Getting Temps\n");
                rtdb_get_temps(db, temps);
                printk("temps: ");
                for (int i = 0; i < 20; i++)
                    printk("%d, ", temps[i]);
                printk("\n");
                break;

            case '6':
                printk("Case 6; Getting High and Low\n");
                high = rtdb_get_high(db);
                low = rtdb_get_low(db);
                printk("high: %d   | low: %d \n", high, low);
                break;

            case '7':
                printk("Case 7; Resetting history\n");
                rtdb_reset_temp_history(db);
                rtdb_get_temps(db, temps);
                printk("temps: ");
                for (int i = 0; i < 20; i++)
                    printk("%d, ", temps[i]);
                printk("\n");
                high = rtdb_get_high(db);
                low = rtdb_get_low(db);
                printk("high: %d   | low: %d \n", high, low);
                break;
                
            default: break;
        }
    }
}