/*******************************************************************
 * Paulo Pedreiras
 * pbrp@ua.pt
 * DETI/IT/UA
 * 
 * 	Companion app for webCamCapture
 * 		- Allows to show the video from webCamCapture when options to use shared memory are enabled
 * 		- Arrival of a new image is signaled via a semaphore (must also be enabled in webCamCapture))
 * 		- Note that both shmem and semaphore must have the same name in webCamCapture and imageViewer
 *   
 * TODO:
 * 		- Beta version, basic functionality
 * 		- No checks for robustness, memory leaks, ...
 *  	- Simplistic algorithm for demo purposes, only	
 *
 * Information sources, additional info, compiling/install instructions, etc:
 * 		- See webCamCapture
 */


/* Generic includes */
#include <stdio.h>
#include <string.h> 	// For memcpy
#include <getopt.h> 	// For getting command args
#include <semaphore.h>	// For semaphores
#include <sys/mman.h>	// For shmem and others
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

/* SDL includes */
#include <SDL2/SDL.h>

/* Global settings */
#define FALSE   0   /* The usual true and false */
#define TRUE (!0)
#define SUCCESS 0           /* Program terminates normally*/
#define IMGBYTESPERPIXEL 4 	/* Number of bytes per pixel. Allowed formats are RGB888, RGBA8888 and */
                            /* others that use both 4 bytes. Be careful because sometimes it is not obvious */
                            /* e.g. RGB888 in SDL is 32bit word aligned, using 4 bytes */
//#define MANUALCOPY			/* Set to make a byte-per-byte copy, instead of memcpy*/

#define MAX_WIDTH	1024	/* Sets the max allowed image width */
#define MAX_HEIGHT	1024	/* Sets the max allowed image height */

/* Function prototypes */
int imgFindBlueSquare(unsigned char * shMemPtr, uint16_t width, uint16_t height, int16_t *cm_x, int16_t *cm_y);

/* Global variables */
unsigned char appName[]="imageDisplay"; /* Application name*/

/* SDL vars */
SDL_Event event;
SDL_Window * window;
SDL_Texture * screen_texture;
SDL_Renderer * renderer;
int width, height; /* Imge size to display */
unsigned char * pixels; /* Pointer to SDL texture data */

/* Shared memory and sempahore variables */
#define accessPerms (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP) /* Read and write perms for user and group */
#define SHMEMNAMESIZE 100
char shMemName[SHMEMNAMESIZE]; /* name must start with "/" and contain up to 255 chars withou further "/" */
int shMemSize = 0;		/* Size of shmem area, in bytes */
#define SEMNAMESIZE 100
char newDataSemName[SEMNAMESIZE]; /* just chars, please*/

char shMemActiveFlag = 0; /* Flags to signal that shmem/sem were indicated by the user */
char newDataSemActiveFlag = 0;

int fd=0;						/* File descriptor for shared memory */
void * shMemPtr = NULL; 		/* Pointer top shered memory region */
sem_t * newDataSemAddr = NULL;	/* Pointer to semaphore */
 
 
 
/* **************************************************
 * main() function
*****************************************************/
int main(int argc, char* argv[])
{

	/* Generic variables */
	int	i, 		
		err; /* Generic return code variable */ 		
	unsigned char * imgPtr;
	int16_t cm_x, cm_y;
		

	/* Process input args */	
	/* Remember that ":" after an option name means that the option takes an argument) */		
	while ((i = getopt(argc, argv, ":hm:s:x:y: ")) != -1) {
  		switch (i) {
			case 'h':
				printf("Help for %s\n", argv[0]);
				printf("\t %s [h] -m /shmeme_name -s sem_name -x img_width -y img_height\n", argv[0]);
				printf("\t \t [-h]: print this help\n");
				printf("\t \t -m: shared memory name, in the form /string with no spaces. Should start with \"/\" and have no further \"/\"\n");
				printf("\t \t -s: semaphore name, in the form string with no spaces. \n");				
				printf("\t \t -x: image width, in pixels \n");				
				printf("\t \t -y: image height, in pixels \n");				
				return SUCCESS;
				break;			
			case 'm':
				printf("Setting the sharem memory area name to: %s\n", optarg);
				strcpy(shMemName,optarg);
				shMemActiveFlag = 1;
				break;
			case 's':
				printf("Setting the semaphore area name to: %s\n", optarg);
				strcpy(newDataSemName,optarg);
				newDataSemActiveFlag = 1;
				break;
			case 'x':
				printf("Setting the image width to: %s\n", optarg);
				width=atoi(optarg);				
				break;
			case 'y':
				printf("Setting the image height to: %s\n", optarg);
				height=atoi(optarg);				
				break;
			case '?':
      			printf("Unknown option: %c\n", optopt);
      			break;
    		case ':':
      			printf("Missing arg for option %c\n", optopt);
      		break;
     	}
  	}	
  	
  	printf("\n Starting %s with parameters: \n\t shmem name: %s \n\t semaphore name: \n\t %s image width:%d \n\t image height: %d \n", argv[0],shMemName,newDataSemName,width, height);
  	  	
	SDL_Init(SDL_INIT_VIDEO);

	window = SDL_CreateWindow(appName,
	SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
	width, height,SDL_WINDOW_RESIZABLE);

	renderer = SDL_CreateRenderer(window,
			-1, SDL_RENDERER_PRESENTVSYNC);

	/* Limit the window size so that it cannot */
	/* be smaller than teh webcam image size */
	SDL_SetWindowMinimumSize(window, width, height);

	SDL_RenderSetLogicalSize(renderer, width, height);
	SDL_RenderSetIntegerScale(renderer, 1);

	screen_texture = SDL_CreateTexture(renderer,
		SDL_PIXELFORMAT_RGB888, SDL_TEXTUREACCESS_STREAMING,
		width, height);

	/* Be careful with formats. Note e.g. that RGB888 uses 4 bytes as it is 32 bit word aligned */
	pixels = malloc(width * height * IMGBYTESPERPIXEL);		
	

	/* Get access to shmem */	
	shMemSize = width * height * IMGBYTESPERPIXEL;
	printf("shmemsize=%d\n",shMemSize);
	
	fd = shm_open(shMemName,    /* Open file */
			O_RDWR , 			/* Open for read/write */
			accessPerms);     /* set access permissions */		
	if (fd < 0) {
		printf("[shared memory reservation] Can't get file descriptor...\n\r");
		return -1;
	}
			
	/* Get the pointer */
	shMemPtr = mmap(NULL,       /* no hints on address */
		shMemSize,   			/* shmem size, in bytes */
		PROT_READ | PROT_WRITE, /* allow read and write */
		MAP_SHARED, /* modifications visible to other processes */
		fd,         /* file descriptor */
		0);         /* no offset*/
	if( shMemPtr == MAP_FAILED){
		printf("[shared memory reservation] mmap failed... \n\r");
		return -1;
	}
	printf("Shared mem address: %p with size %d\n", shMemPtr, shMemSize);
	printf("Filesystem entry:       /dev/shm%s\n", shMemName );

	
	/* Create semaphore */
	newDataSemAddr = sem_open(newDataSemName, 	/* semaphore name */
			O_CREAT,       					/* create the semaphore */
			accessPerms,   					/* protection perms */
			0);            					/* initial value */
	if (newDataSemAddr == SEM_FAILED) {
		printf("[semaphore creation] Error creating semaphore \n\r");
		return -1;
	}
	
		
  	/* Get data from shmem and show it */
  	i=0;
	while (1) {

		/* Process SDL events. In this case just termination */
		while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) exit(0);
        }    	
        
        /* Check if new image arrived ...*/
        if (!sem_wait(newDataSemAddr)) { /* sem_wait returns 0 on success */
			printf("[imageViewer] New image in shmem signaled [%d]\n\r", i++);			
			
			/* Here you can call image processing functions. E.g. */
			if(!imgFindBlueSquare(shMemPtr, width, height, &cm_x, &cm_y)) {
				printf("BlueSquare found at (%3d,%3d)\n", cm_x, cm_y);
			} else {
				printf("BlueSquare not found\n");
			}			
			
			/* Then display the message via SDL */
#ifdef MANUALCOPY
			/* Copy the image byte-by-byte. Useful for debugging and  processing */
			/* Format assumed in this case is RGB32 that shows up in memroy - ascending order- as B+G+R+filler */ 			
			/* It is a packed format, i.e., in memory, ascending order, R+G+B+Filler of pix 1, then R+G+B+Filler of pix 2, ... */
			imgPtr = shMemPtr;
			for (int y = 0; y < height; ++y) {
				for (int x = 0; x < width; x++) {	
					pixels[4*(x + y * width)] = *(imgPtr++); 	// Blue 
					pixels[4*(x + y * width)+1] = *(imgPtr++);	// Green
					pixels[4*(x + y * width)+2] = *(imgPtr++);	// Red 
					pixels[4*(x + y * width)+3] = *(imgPtr++);	//Alpha/Filler									
				}
			}			
#else
			/* Do a more efficient memmcpy instead of byte-by-byte copy*/
			memcpy(pixels,shMemPtr,width*height*IMGBYTESPERPIXEL);
#endif
			SDL_RenderClear(renderer);
			SDL_UpdateTexture(screen_texture, NULL, pixels, width * IMGBYTESPERPIXEL );
			SDL_RenderCopy(renderer, screen_texture, NULL, NULL);
			SDL_RenderPresent(renderer);			
			
		} else {
			printf("[imageViewer] Error on sem_wait\n\r");
		}
		
  	}

	/* Release resources and terminate */
  	printf("releasing resources and ending \n\r");
	

  	return SUCCESS;
}

/* ***************************************************************
 * 
 * Function implementation 
 * 
 **************************************************************** */

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
