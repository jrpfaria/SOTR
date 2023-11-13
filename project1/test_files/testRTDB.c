#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "./include/rtdb.h"

int main() {
    // Test parameters
    int size_of_data = sizeof(int);
    int amount_of_buffers = 5;

    // Initialize the database
    DB* myDB = initDataBase(size_of_data, amount_of_buffers);

    // Check if initialization was successful
    if (myDB == NULL) {
        fprintf(stderr, "Error initializing the database\n");
        return EXIT_FAILURE;
    }

    // Populate data for testing
    int testData[amount_of_buffers];
    for (int i = 0; i < amount_of_buffers; ++i) {
        testData[i] = i * 10;
    }

    // Set data in buffers
    for (int i = 0; i < amount_of_buffers; ++i) {
        if (getMostRecentData(myDB) != NULL)
            printf("Most Recent Data: %d\n", *((int*)getMostRecentData(myDB)));
        setBufferAtIndex(myDB, i, &testData[i]);
    }

    for (int i = 3; i >= 0; --i) {
        setBufferAtIndex(myDB, i, &testData[i]);
        printf("Most Recent Data: %d\n", *((int*)getMostRecentData(myDB)));
    }

    setBufferAtIndex(myDB, 1, &testData[1]);
    printf("Most Recent Data: %d\n", *((int*)getMostRecentData(myDB)));

    for (int i = 0; i < amount_of_buffers; ++i) {
        setBufferAtIndex(myDB, i, &testData[i]);
        printf("Most Recent Data: %d\n", *((int*)getMostRecentData(myDB)));
    }

    // Clean up
    free(myDB->buffer);
    free(myDB);

    return EXIT_SUCCESS;
}
