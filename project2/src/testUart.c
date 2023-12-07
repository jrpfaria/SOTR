#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    char cmd[9] = {'!', '0', '0', '1', '1', '1', '9', '4', '#'};
    int temp[5] = {0, -2, -26, 10, 54};

    int size = 1;
    char* payload = malloc(sizeof(char) * 3 * size);
    int divisor = 1;

    for (int i = 0; i < size; i++){
        for (int j = 0; j < 2; j++){
            if (j == 0) payload[3*i] = temp[i] < 0 ? '-' : '+';
            else
                payload[3*i + 1] = abs(temp[i]) / 10 + '0',
                payload[3*i + 2] = abs(temp[i]) % 10 + '0';
        }
    }

    printf("payload: %s\n", payload);
    
    payload = malloc(sizeof(char) * 3 * size);
    size = 5;

    for (int i = 0; i < size; i++){
        for (int j = 0; j < 2; j++){
            if (j == 0) payload[3*i] = temp[i] < 0 ? '-' : '+';
            else
                payload[3*i + 1] = abs(temp[i]) / 10 + '0',
                payload[3*i + 2] = abs(temp[i]) % 10 + '0';
        }
    }

    printf("payload: %s\n", payload);

    return 0;
}
