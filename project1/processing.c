#include "./include/processing.h"

#define MAX_WIDTH	1280	/* Sets the max allowed image width */
#define MAX_HEIGHT	1024	/* Sets the max allowed image height */
#define IMGBYTESPERPIXEL 4  /* Number of bytes per pixel in the image */


void dispatchImageProcessingFunctions(long frame_number){
    if (frame_number % 2 == 0){
        printf("frame number: %ld, detecting obstacle\n", frame_number);
    }
    if (frame_number % 3 == 0){
        printf("frame number: %ld, detecting edges\n", frame_number);
    }
    if (frame_number % 5 == 0){
        printf("frame number: %ld\n, detecting blue squares", frame_number);
    }
}

/* Detects the center of mass of a blue square (or something similar) */
int imgFindBlueSquare(unsigned char * shMemPtr, uint16_t width, uint16_t height, int16_t *cm_x, int16_t *cm_y){
	#define FINDBLUE_DBG 	0	// Flag to activate output of image processing debug info 
	
	/* Note: the following settings are strongly dependent on illumination intensity and color, ...*/
	/* 		There are much more robust approaches! */
	#define MAGNITUDE 		1.5 		// minimum ratio between Blue and other colors to be considered blue
	#define PIX_THRESHOLD 	30 	// Minimum number of pixels to be considered an object of interest 
	#define LPF_SAMPLES		4 	// Simple average for filtering - number of samples to average 
	
	/* Variables */
	unsigned char *imgPtr;		/* Pointer to image */
	int i,x,y;					/* Indexes */
	int pixCountX[MAX_WIDTH], pixCountY[MAX_HEIGHT];  	/* Count of pixels by row and column */
	int in_edge, out_edge;			/* Coordinates of obgect edges */ 
	
	/* Check image size */
	if(width > MAX_WIDTH || height > MAX_HEIGHT) {
		printf("[imgFindBlueSquare]ERROR: image size exceeds the limits allowed\n\r");
		return -1;
	}
	
	/* Reset counters */
	for(x=0; x<MAX_WIDTH; x++)
		pixCountX[x]=0;
	
	for(y=0; y<MAX_HEIGHT; y++)
		pixCountY[y]=0;
		
	/* Process image - find count of blue pixels in each row */	
	/* Note that in memory images show as follows (in pixels, each pixel BGRF): */
	/* Pixel in a row show up in contiguous memory addresses */
	/*	Start from top-left corner */
	/* (0,0) (1,0) (2,0) (3,0) ... (width-1,0)*/
	/* (0,1) (1,1) (2,1) (3,1) ... */
	/* (0,2) (1,2) (2,2) (3,2) ... */
	/*  ...   ...   ...            */
	/* (0,height-1) (1,height-1) (2, height-1) ... (height-1, width-1)*/
	
	imgPtr = shMemPtr;
	for (y = 0; y < height; ++y) {
		if(FINDBLUE_DBG) printf("\n");
		for (x = 0; x < width; x++) {	
			if(FINDBLUE_DBG) {
				if(x < 20) {
					printf("%x:%x:%x - ",*imgPtr, *(imgPtr+1), *(imgPtr+2));
				}			}
			/* Remember that for each pix the access is B+G+R+filler */
			/* Simple approach: intensity of one componet much greater than the other two */
			/* There are much robust approaches ... */
			if(*imgPtr > (MAGNITUDE * (*(imgPtr+1))) && *imgPtr > (MAGNITUDE * (*(imgPtr+2))))
				pixCountY[y]+=1;
			
			/* Step to next pixel */
			imgPtr+=4;
		}		
	}
		
	/* Process image - find count of blue pixels in each column */	
	for (x = 0; x < width; x++) {	
		imgPtr = shMemPtr + x * 4; // Offset to the xth column in the firts row
		for (y = 0; y < height; y++) {		
			if(*imgPtr > (MAGNITUDE * (*(imgPtr+1))) && *imgPtr > (MAGNITUDE * (*(imgPtr+2))))
				pixCountX[x]+=1;
			
			/* Step to teh same pixel i the next row */
			imgPtr+=4*width;
		}		
	}
	
	if(FINDBLUE_DBG) {
		printf("\n Image processing results - raw - by row :");
		for(x=0; x<MAX_WIDTH; x++)
			printf("%5d ",pixCountX[x]);
		printf("\n---------------------------\n");
		
		printf("\n Image processing results - raw - by column :");
		for(y=0; y<MAX_HEIGHT; y++)
			printf("%5d ",pixCountY[y]);
		printf("\n---------------------------\n");
	}
	
	/* Apply a simple averaging filter to pixe count */
	for(x=0; x < (MAX_WIDTH - LPF_SAMPLES); x++) {
		for(i = 1; i < LPF_SAMPLES; i++) {
			pixCountX[x] += pixCountX[x+i];
		}
		pixCountX[x] = pixCountX[x] / LPF_SAMPLES; 
	}
	
	for(y=0; y < (MAX_HEIGHT - LPF_SAMPLES); y++) {
		for(i = 1; i < LPF_SAMPLES; i++) {
			pixCountY[y] += pixCountY[y+i];
		}
		pixCountY[y] = pixCountY[y] / LPF_SAMPLES; 
	}
	
	if(FINDBLUE_DBG) {
		printf("\n Image processing results - after filtering - by row:");
		for(x=0; x < (MAX_WIDTH - LPF_SAMPLES); x++)
			printf("%5d ",pixCountX[x]);
		printf("\n---------------------------\n");
		
		printf("\n Image processing results - after filtering - by columnn:");
		for(y=0; y < (MAX_HEIGHT - LPF_SAMPLES); y++)
			printf("%5d ",pixCountY[y]);
		printf("\n---------------------------\n");
	}
		
		
	/* Detect Center of Mass (just one object) */
	*cm_x = -1; 	// By default not found
	*cm_y = -1;		
		
	/* Detect YY CoM */
	in_edge = -1;	// By default not found
	out_edge= -1;
	
	/* Detect rising edge - beginning */
	for(y=0; y < (MAX_HEIGHT - LPF_SAMPLES -1); y++) {
		if((pixCountY[y] < PIX_THRESHOLD) && ((pixCountY[y+1] >= PIX_THRESHOLD))) {
			in_edge = y;
			break;
		}
	}
	/* Detect falling edge - ending */
	for(y=0; y < (MAX_HEIGHT - LPF_SAMPLES -1); y++) {
		if((pixCountY[y] >= PIX_THRESHOLD) && ((pixCountY[y+1] < PIX_THRESHOLD))) {
			out_edge = y;
			break;
		}
	}	
			
	/* Process edges to determine center of mass existence and position */ 		
	/* If object in the left image edge */
	if(out_edge > 0 && in_edge == -1)
		in_edge = 0;
	
	if((in_edge >= 0) && (out_edge >= 0))
		*cm_y = (out_edge-in_edge)/2+in_edge;
		
	if(FINDBLUE_DBG) {
		if(cm_y >= 0) {
			printf("Blue square center of mass y = %d\n", *cm_y);
		} else {
			printf("Blue square center of mass y = NF\n");
		}
	}				
	
		
	/* Detect XX CoM */
	in_edge = -1;	// By default not found
	out_edge= -1;
	
	/* Detect rising edge - beginning */
	for(x=0; x < (MAX_WIDTH - LPF_SAMPLES -1); x++) {
		if((pixCountX[x] < PIX_THRESHOLD) && ((pixCountX[x+1] >= PIX_THRESHOLD))) {
			in_edge = x;
			break;
		}
	}
	/* Detect falling edge - ending */
	for(x=0; x < (MAX_WIDTH - LPF_SAMPLES -1); x++) {
		if((pixCountX[x] >= PIX_THRESHOLD) && ((pixCountX[x+1] < PIX_THRESHOLD))) {
			out_edge = x;
			break;
		}
	}	
			
	/* Process edges to determine center of mass existence and position */ 		
	/* If object in the left image edge */
	if(out_edge > 0 && in_edge == -1)
		in_edge = 0;
	
	if((in_edge >= 0) && (out_edge >= 0))
		*cm_x = (out_edge-in_edge)/2+in_edge;
		
	if(FINDBLUE_DBG) {
		if(cm_x >= 0) {
			printf("Blue square center of mass x = %d\n", *cm_x);
		} else {
			printf("Blue square center of mass x = NF\n");
		}
	}					
		
	/* Return with suitable error code */
	if(*cm_x >= 0 && *cm_y >= 0)
		return 0;	// Success
	else
		return -1; // No objecty
}

/* Process image to turn image into gray scale*/
int imgEdgeDetection(unsigned char * shMemPtr, uint16_t width, uint16_t height, int16_t *cm_x, int16_t *cm_y){
	/* Variables */
	unsigned char *imgPtr;		/* Pointer to image */
	int i,x,y;					/* Indexes */

	/* Check image size */
	if(width > MAX_WIDTH || height > MAX_HEIGHT) {
		printf("[imgEdgeDetection]ERROR: image size exceeds the limits allowed\n\r");
		return -1;
	}

	unsigned char* auxImage = malloc(width*height*IMGBYTESPERPIXEL);
	unsigned char* auxImg = auxImage;
	memcpy(auxImage, shMemPtr, width*height*IMGBYTESPERPIXEL);
	
	unsigned char gray, reference;
	imgPtr = shMemPtr;
	for (y = 0; y < height; ++y) {
		for (x = 0; x < width; x+=2) {		
			reference = 0;
			for(i = 0; i < 2; i++) {
				gray = (*(auxImage) + *(auxImage+1) + *(auxImage+2))/3;
				
				if (reference == 0)
					reference = gray;
				else if (abs(gray - reference) > 10)
					*imgPtr = *(imgPtr+1) = *(imgPtr+2) = 255;
				
				/* Step to next pixel */
				imgPtr+=4;
				auxImage+=4;	
			}
		}		
	}	

	free(auxImg);

	if(*cm_x >= 0 && *cm_y >= 0)
		return 0;	// Success
	else
		return -1; // No objecty
}

/* Process image to detect obstacles */
int imgDetectObstacles(unsigned char * shMemPtr, uint16_t width, uint16_t height, int16_t *cm_x, int16_t *cm_y, float *closeness){
	#define FINDBLUE_DBG 	0	// Flag to activate output of image processing debug info 
	
	/* Note: the following settings are strongly dependent on illumination intensity and color, ...*/
	/* 		There are much more robust approaches! */
	#define MAGNITUDE 		1.5		// minimum ratio between Blue and other colors to be considered blue
	#define PIX_THRESHOLD 	30 	// Minimum number of pixels to be considered an object of interest 
	#define LPF_SAMPLES		4 	// Simple average for filtering - number of samples to average 
	
	/* Variables */
	unsigned char *imgPtr;		/* Pointer to image */
	int i,x,y;					/* Indexes */
	int pixCountX[MAX_WIDTH], pixCountY[MAX_HEIGHT];  	/* Count of pixels by row and column */
	int in_edge, out_edge;			/* Coordinates of obgect edges */ 

	// Just to facilitate iterating through the image's desired area, configurable through hardcoded constants
	uint16_t left_limit = width / 4;
	uint16_t right_limit = 3 * width / 4;

	uint16_t top_limit = 0;
	uint16_t bottom_limit = height;

	/* Check image size */
	if(width > MAX_WIDTH || height > MAX_HEIGHT) {
		printf("[imgFindBlueSquare]ERROR: image size exceeds the limits allowed\n\r");
		return -1;
	}
	
	/* Reset counters */
	for(x=0; x<MAX_WIDTH; x++)
		pixCountX[x]=0;
	
	for(y=0; y<MAX_HEIGHT; y++)
		pixCountY[y]=0;
		
	/* Process image - find count of blue pixels in each row */	
	/* Note that in memory images show as follows (in pixels, each pixel BGRF): */
	/* Pixel in a row show up in contiguous memory addresses */
	/*	Start from top-left corner */
	/* (0,0) (1,0) (2,0) (3,0) ... (width-1,0)*/
	/* (0,1) (1,1) (2,1) (3,1) ... */
	/* (0,2) (1,2) (2,2) (3,2) ... */
	/*  ...   ...   ...            */
	/* (0,height-1) (1,height-1) (2, height-1) ... (height-1, width-1)*/
	
	imgPtr = shMemPtr;

	imgPtr += 4 * top_limit * 4 * left_limit;
	for (y = top_limit ; y < bottom_limit; ++y){
		for (x = left_limit; x < right_limit; x++){
			/* Remember that for each pix the access is B+G+R+filler */
			/* Simple approach: intensity of one componet much greater than the other two */
			/* There are much robust approaches ... */
			if(0xFF > (MAGNITUDE*2 * (*(imgPtr+1))) && 0xFF > (MAGNITUDE*2 * (*(imgPtr+1))) && 0xFF > (MAGNITUDE*2 * (*(imgPtr+2))))
				pixCountY[y]+=1;
			
			/* Step to next pixel */
			imgPtr+=4;
		}
	}
		
	/* Process image - find count of blue pixels in each column */	
	for (x = left_limit; x < right_limit; x++) {	
		imgPtr = shMemPtr + x * 4; // Offset to the xth column in the firts row
		for (y = top_limit; y < bottom_limit; y++) {		
			if(0xFF > (MAGNITUDE*2 * (*(imgPtr+1))) && 0xFF > (MAGNITUDE*2 * (*(imgPtr+1))) && 0xFF > (MAGNITUDE*2 * (*(imgPtr+2))))
				pixCountX[x]+=1;
			
			/* Step to teh same pixel i the next row */
			imgPtr+=4*width;
		}		
	}
	
	/* Apply a simple averaging filter to pixe count */
	for(x=0; x < (MAX_WIDTH - LPF_SAMPLES); x++) {
		for(i = 1; i < LPF_SAMPLES; i++) {
			pixCountX[x] += pixCountX[x+i];
		}
		pixCountX[x] = pixCountX[x] / LPF_SAMPLES; 
	}
	
	for(y=0; y < (MAX_HEIGHT - LPF_SAMPLES); y++) {
		for(i = 1; i < LPF_SAMPLES; i++) {
			pixCountY[y] += pixCountY[y+i];
		}
		pixCountY[y] = pixCountY[y] / LPF_SAMPLES; 
	}

	/* Detect Center of Mass (just one object) */
	*cm_x = -1; 	// By default not found
	*cm_y = -1;		
		
	/* Detect YY CoM */
	in_edge = -1;	// By default not found
	out_edge= -1;
	
	/* Detect rising edge - beginning */
	for(y=0; y < (MAX_HEIGHT - LPF_SAMPLES -1); y++) {
		if((pixCountY[y] < PIX_THRESHOLD) && ((pixCountY[y+1] >= PIX_THRESHOLD))) {
			in_edge = y;
			break;
		}
	}
	/* Detect falling edge - ending */
	for(y=0; y < (MAX_HEIGHT - LPF_SAMPLES -1); y++) {
		if((pixCountY[y] >= PIX_THRESHOLD) && ((pixCountY[y+1] < PIX_THRESHOLD))) {
			out_edge = y;
			break;
		}
	}	
		
	/* Process edges to determine center of mass existence and position */ 		
	/* If object in the left image edge */
	if(out_edge > 0 && in_edge == -1)
		in_edge = 0;
	
	if((in_edge >= 0) && (out_edge >= 0))
		*cm_y = (out_edge-in_edge)/2+in_edge;
		
	/* Detect XX CoM */
	in_edge = -1;	// By default not found
	out_edge= -1;
	
	/* Detect rising edge - beginning */
	for(x=0; x < (MAX_WIDTH - LPF_SAMPLES -1); x++) {
		if((pixCountX[x] < PIX_THRESHOLD) && ((pixCountX[x+1] >= PIX_THRESHOLD))) {
			in_edge = x;
			break;
		}
	}
	/* Detect falling edge - ending */
	for(x=0; x < (MAX_WIDTH - LPF_SAMPLES -1); x++) {
		if((pixCountX[x] >= PIX_THRESHOLD) && ((pixCountX[x+1] < PIX_THRESHOLD))) {
			out_edge = x;
			break;
		}
	}	
			
	/* Process edges to determine center of mass existence and position */ 		
	/* If object in the left image edge */
	if(out_edge > 0 && in_edge == -1)
		in_edge = 0;
	
	if((in_edge >= 0) && (out_edge >= 0))
		*cm_x = (out_edge-in_edge)/2+in_edge;		
		
	/* Return with suitable error code */
	if(*cm_x >= 0 && *cm_y >= 0)
		return 0;	// Success
	else
		return -1; // No objecty
}