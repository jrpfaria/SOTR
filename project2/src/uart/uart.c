#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include "uart.h"

#define DEBUG 0

char uart_lut(char c)
{
    if (isdigit(c))
    {
        return c - '0';
    }
    else if (isxdigit(c))
    {
        return toupper(c) - 'A' + 10;
    }
    else
    {
        // Handle invalid characters if needed
        return 0; // Default value for invalid characters
    }
}

int uart_checkSum(char *cmd)
{
    int i = -1, sum = 0, checksum = 0;
    i = strlen(cmd) - 2;

    int aux_iterator = i - 3;
    int aux_multiplier = 1;

    for (; i > aux_iterator; i--, aux_multiplier *= 10)
        sum += (cmd[i] - '0') * aux_multiplier;

    for (; i > 0; i--)
        checksum += cmd[i];

    return sum == checksum;
}

char *uart_apply_checksum(char *cmd, int size)
{
    int sum = 0;

    for (int i = 1; i < size - 3; i++)
        sum += cmd[i];

    char checksumDigits[5];

    for (int i = 2; i >= 0; i--)
    {
        checksumDigits[i] = (sum % 10) + '0';
        sum /= 10;
    }
    checksumDigits[3] = '#';
    checksumDigits[4] = '\0';

    return strcat(cmd, checksumDigits);
}

char *uart_generate_io_payload(unsigned char c)
{
    char *payload = (char *)malloc(sizeof(char) * 5);
    if (payload == NULL)
        printk("failed\n");

    for (int i = 0; i < 4; i++)
    {
        // payload[3-i] = (c & (1 << i)) ? '1' : '0'; // db(C) -> 1100
        payload[i] = (c & (1 << i)) ? '1' : '0'; // db(C) -> 0011
    }
    payload[4] = '\0';

    return payload;
}

char *uart_generate_temp_payload(int *temp, int size)
{
    char *payload = malloc(3 * sizeof(char) * size + sizeof(char));
    if (payload == NULL)
        printk("failed\n");

    for (int i = 0; i < size; i++)
    {
        payload[3 * i] = temp[i] < 0 ? '-' : '+';
        payload[3 * i + 1] = abs(temp[i]) / 10 + '0',
                        payload[3 * i + 2] = abs(temp[i]) % 10 + '0';
    }

    payload[3 * size] = '\0';

    return payload;
}

char *create_response(char *cmd, char *payload)
{
    char *response = malloc(strlen(cmd) + strlen(payload) + sizeof(char) * 6); // cmd + payload + checksum + #\n\n

    response[0] = '\0';
    strcat(response, cmd);
    strcat(response, payload);
    uart_apply_checksum(response, strlen(response) + 4);
    strcat(response, "\r\n");
    return response;
}

char *uart_interface(RTDB *db, char *cmd)
{
    if (DEBUG)
        printk("cmd: %s\n", cmd);

    int index;
    unsigned char bin;
    char *payload;
    int highlow[2], temps[20];

    if (cmd[1] == '0')
    {
        switch (cmd[2])
        {
        case '0':
            if (DEBUG)
                printk("Case 0; Setting Output at Index\n");
            index = (int)uart_lut(cmd[3]);
            bin = uart_lut(cmd[4]);
            rtdb_set_output_at_index(db, --index, bin);
            if (DEBUG)
                printk("outputs: %x\n", rtdb_get_outputs(db));
            return "\r\n";

        case '1':
            if (DEBUG)
                printk("Case 1; Setting All Outputs\n");
            bin = uart_lut(cmd[3]);
            rtdb_set_outputs(db, bin);
            if (DEBUG)
                printk("outputs: %x\n", rtdb_get_outputs(db));
            return "\r\n";

        case '2':
            if (DEBUG)
                printk("Case 2; Getting All Inputs\n");
            unsigned char i = rtdb_get_inputs(db);
            if (DEBUG)
                printk("i: %x\n", i);
            payload = uart_generate_io_payload(i);
            return create_response("!1A", payload);

        case '3':
            if (DEBUG)
                printk("Case 3; Getting All Outputs\n");
            unsigned char o = rtdb_get_outputs(db);
            if (DEBUG)
                printk("o: %x\n", o);
            payload = uart_generate_io_payload(o);
            return create_response("!1B", payload);
            ;

        case '4':
            if (DEBUG)
                printk("Case 4; Getting Last Temp\n");
            int last_temp = rtdb_get_last_temp(db);
            if (DEBUG)
                printk("last_temp: %d\n", last_temp);
            payload = uart_generate_temp_payload(&last_temp, 1);
            return create_response("!1C", payload);

        case '5':
            if (DEBUG)
                printk("Case 5; Getting Temps\n");
            rtdb_get_temps(db, temps);
            if (DEBUG)
            {
                printk("temps: ");
                for (int i = 0; i < 20; i++)
                    printk("%d, ", temps[i]);
                printk("\n");
            }
            payload = uart_generate_temp_payload(temps, 20);
            return create_response("!1D", payload);

        case '6':
            if (DEBUG)
                printk("Case 6; Getting High and Low\n");
            highlow[0] = rtdb_get_high(db);
            highlow[1] = rtdb_get_low(db);
            if (DEBUG)
                printk("high: %d   | low: %d \n", highlow[0], highlow[1]);
            payload = uart_generate_temp_payload(highlow, 2);
            return create_response("!1E", payload);

        case '7':
            if (DEBUG)
                printk("Case 7; Resetting history\n");
            rtdb_reset_temp_history(db);
            rtdb_get_temps(db, temps);
            if (DEBUG)
            {
                printk("temps: ");
                for (int i = 0; i < 20; i++)
                    printk("%d, ", temps[i]);
                printk("\n");
            }
            highlow[0] = rtdb_get_high(db);
            highlow[1] = rtdb_get_low(db);
            if (DEBUG)
                printk("high: %d   | low: %d \n", highlow[0], highlow[1]);
            return "\r\n";

        default:
            break;
        }
    }
    return "";
}