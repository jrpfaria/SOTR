#include <stdio.h>
#include <string.h> 	// For memcpy
#include <stdint.h>
#include <stdlib.h>

int imgFindBlueSquare(unsigned char * shMemPtr, uint16_t width, uint16_t height, int16_t *cm_x, int16_t *cm_y);
int imgEdgeDetection(unsigned char *, uint16_t, uint16_t, int16_t *, int16_t *);
int imgDetectObstacles(unsigned char *, uint16_t, uint16_t, int16_t *, int16_t *);