#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include "uart.h"

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

int uart_checkSum(char* cmd){
    int i = -1, sum = 0, checksum = 0;
    i = strlen(cmd)-2;
    
    int aux_iterator = i - 3;
    int aux_multiplier = 1;
    
    for (; i > aux_iterator; i--, aux_multiplier *= 10)
        sum += (cmd[i] - '0') * aux_multiplier;

    for (; i > 0; i--) checksum += cmd[i];

    return sum == checksum;
}

char* uart_apply_checksum(char* cmd, int size){
    int sum = 0;

    for(int i = 1; i < size - 3; i++)
        sum += cmd[i];
    
    char checksumDigits[4];

    for(int i = 2; i >= 0; i--){
        checksumDigits[i] = (sum % 10) + '0';
        sum /= 10;
    }
    checksumDigits[3] = '#';

    return strcat(cmd, checksumDigits);
}

char* generate_io_payload(unsigned char c){
    char* payload = malloc(4 * sizeof(char));
    
    for (int i = 0; i < 4; i++){
        // não sei como queremos o payload, se é 1100 ou 0011 tendo em conta como estão as coisas na db
        // payload[3-i] = (c & (1 << i)) ? '1' : '0'; db(C) -> 1100
        payload[i] = (c & (1 << i)) ? '1' : '0'; // db(C) -> 0011
    }

    printk("payload: %s\n", payload);

    return payload;
}

char* generate_temp_payload(int* temp, int size){
    char* payload = malloc(sizeof(char) * 3 * size);

    for (int i = 0; i < size; i++){
        for (int j = 0; j < 2; j++){
            if (j == 0) payload[3*i] = temp[i] < 0 ? '-' : '+';
            else
                payload[3*i + 1] = abs(temp[i]) / 10 + '0',
                payload[3*i + 2] = abs(temp[i]) % 10 + '0';
        }
    }

    printf("payload: %s\n", payload);

    return payload;
}

char* uart_interface(RTDB* db, char* cmd){
    printk("cmd: %s\n", cmd);

    int index;
    unsigned char bin;
    unsigned char* payload;
    unsigned char* response;
    int highlow[2], temps[20];

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
                payload = generate_io_payload(i);
                response = strcat("!1A", payload);
                // apply checksum and send cmd
                break;

            case '3':
                printk("Case 3; Getting All Outputs\n");
                unsigned char o = rtdb_get_outputs(db);
                printf("o: %x\n", o);
                payload = generate_io_payload(o);
                response = strcat("!1B", payload);
                // apply checksum and send cmd
                break;

            case '4':
                printk("Case 4; Getting Last Temp\n");
                int last_temp = rtdb_get_last_temp(db);
                printf("last_temp: %d\n", last_temp);
                payload = generate_temp_payload(&last_temp, 1);
                response = strcat("!1C", payload);
                // apply checksum and send cmd
                break;

            case '5':
                printk("Case 5; Getting Temps\n");
                rtdb_get_temps(db, temps);
                printk("temps: ");
                for (int i = 0; i < 20; i++)
                    printk("%d, ", temps[i]);
                printk("\n");
                payload = generate_temp_payload(temps, 20);
                response = strcat("!1D", payload);
                // apply checksum and send cmd
                break;

            case '6':
                printk("Case 6; Getting High and Low\n");
                highlow[0] = rtdb_get_high(db);
                highlow[1] = rtdb_get_low(db);
                printk("high: %d   | low: %d \n", highlow[0], highlow[1]);
                payload = generate_temp_payload(highlow, 2);
                response = strcat("!1E", payload);
                // apply checksum and send cmd
                break;

            case '7':
                printk("Case 7; Resetting history\n");
                rtdb_reset_temp_history(db);
                rtdb_get_temps(db, temps);
                printk("temps: ");
                for (int i = 0; i < 20; i++)
                    printk("%d, ", temps[i]);
                printk("\n");
                highlow[0] = rtdb_get_high(db);
                highlow[1] = rtdb_get_low(db);
                printk("high: %d   | low: %d \n", highlow[0], highlow[1]);
                break;
                
            default: break;
        }
    }
}