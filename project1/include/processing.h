#include <stdio.h>
#include <string.h> 	// For memcpy
#include <stdint.h>
#include <stdlib.h>

#include "threads.h"

#define MAX_WIDTH	1280	/* Sets the max allowed image width */
#define MAX_HEIGHT	1024	/* Sets the max allowed image height */
#define IMGBYTESPERPIXEL 4  /* Number of bytes per pixel in the image */

int imgFindBlueSquare(unsigned char * shMemPtr, int width, int height, int16_t *cm_x, int16_t *cm_y);
int imgEdgeDetection(unsigned char *, int, int, int16_t *, int16_t *);
int imgDetectObstacles(unsigned char *, int, int, int16_t *, int16_t *);

void* imgFindBlueSquareWrapper(void*);
void* imgEdgeDetectionWrapper(void*);
void* imgDetectObstaclesWrapper(void*);