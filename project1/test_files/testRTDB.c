#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/rtdb.h"

int main() {
    // Test parameters
    int size_of_data = sizeof(char) * 20;  // Assuming a fixed-size string of length 20
    int amount_of_buffers = 5;

    // Initialize the database
    DB *myDB = initDataBase(size_of_data, amount_of_buffers);

    // Check if initialization was successful
    if (myDB == NULL) {
        fprintf(stderr, "Error initializing the database\n");
        return EXIT_FAILURE;
    }

    const char *testData[] = {"Zero", "Ten", "Twenty", "Thirty", "Forty"};
    
    // Set data in buffers
    for (int i = 0; i < amount_of_buffers; ++i) {
        if (getMostRecentData(myDB) != NULL)
            printf("Most Recent Data: %s\n", (char*)getMostRecentData(myDB));
        setBufferAtIndex(myDB, i, (void*)testData[i]);
    }

    for (int i = 3; i >= 0; --i) {
        setBufferAtIndex(myDB, i, (void*)testData[i]);
        printf("Most Recent Data: %s\n", (char*)getMostRecentData(myDB));
    }

    setBufferAtIndex(myDB, 1, (void*)testData[1]);
    printf("Most Recent Data: %s\n", (char*)getMostRecentData(myDB));

    for (int i = 0; i < amount_of_buffers; ++i) {
        setBufferAtIndex(myDB, i, (void*)testData[i]);
        printf("Most Recent Data: %s\n", (char*)getMostRecentData(myDB));
    }

    // Clean up
    for (int i = 0; i < amount_of_buffers; ++i) {
        free(myDB->buffer[i].data);
    }
    free(myDB->buffer);
    free(myDB);

    return EXIT_SUCCESS;
}
