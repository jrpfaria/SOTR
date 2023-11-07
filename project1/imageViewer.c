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

/* My includes */
#include "./include/threads.h"

/* Global settings */
#define num_of_tasks 3
#define FALSE   0   /* The usual true and false */
#define TRUE (!0)
#define SUCCESS 0           /* Program terminates normally*/
#define IMGBYTESPERPIXEL 4 	/* Number of bytes per pixel. Allowed formats are RGB888, RGBA8888 and */
                            /* others that use both 4 bytes. Be careful because sometimes it is not obvious */
                            /* e.g. RGB888 in SDL is 32bit word aligned, using 4 bytes */
//#define MANUALCOPY			/* Set to make a byte-per-byte copy, instead of memcpy*/

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

	// Open Cab Structure
	int size_of_data = width*height*IMGBYTESPERPIXEL;
	CAB *cab = open_cab(size_of_data, num_of_tasks);

	// Open RTDB Structure
	DB *db = initDataBase(size_of_data, num_of_tasks);

	THREAD_INPUTS* inputs = malloc(sizeof(THREAD_INPUTS));
	setThreadInputs(inputs, height, width, &cm_x, &cm_y);

	// Set Thread Attributes
	pthread_attr_t attr[3];
	pthread_mutex_t mutexes[3];
	setThreadParam(attr);

	// Set Thread Scheduler Parameters
	setAllThreadSchedParam(attr, mutexes);

	SDL_Init(SDL_INIT_VIDEO);

	// Create SDL window and renderer
	window = SDL_CreateWindow(appName, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height,SDL_WINDOW_RESIZABLE);
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);

	/* Limit the window size so that it cannot */
	/* be smaller than teh webcam image size */
	SDL_SetWindowMinimumSize(window, width, height);
	SDL_RenderSetLogicalSize(renderer, width, height);
	SDL_RenderSetIntegerScale(renderer, 1);

	screen_texture = SDL_CreateTexture(renderer,
		SDL_PIXELFORMAT_RGB888, SDL_TEXTUREACCESS_STREAMING,
		width, height);

	/* Be careful with formats. Note e.g. that RGB888 uses 4 bytes as it is 32 bit word aligned */
	pixels = malloc(size_of_data);		
	/* Get access to shmem */	
	shMemSize = size_of_data;
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

	// Initialize Auxiliar Thread Structures
	THREAD_ARG* cab_arg = malloc(sizeof(THREAD_ARG));
	initThreadArg(cab_arg, (void *) cab);

	THREAD_ARG* db_arg = malloc(sizeof(THREAD_ARG));
	initThreadArg(db_arg, (void *) db);

  	/* Get data from shmem and show it */
  	i=0;
	long frame_counter = 0;
	while (1) {
		/* Process SDL events. In this case just termination */
		while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) exit(0);
        }    	
        
        /* Check if new image arrived ...*/
        if (!sem_wait(newDataSemAddr)) { /* sem_wait returns 0 on success */
			printf("[imageViewer] New image in shmem signaled [%d]\n\r", i++);			
			
			/* Here you can call image processing functions. E.g. */
			printf("Before reserveCab()\n");
			reserveCab(cab_arg);

			printf("Before putMessageOnCab()\n");
			putMessageOnCab(cab_arg, (CAB_BUFFER*) cab_arg->content, (void*) shMemPtr);

			printf("Starting dispatchImageProcessingFunctions with width: %d and height: %d\n\n", width, height);
			dispatchImageProcessingFunctions(cab_arg, db_arg, attr, mutexes, frame_counter++, inputs);		
			
			ungetMessageFromCAB(cab_arg);

			/* Then display the message via SDL */
			// Copy the image from RTDB to the SDL texture
			unsigned char *imageToDisplay = (unsigned char*)getMessageFromRTDB(db_arg);
			memcpy(pixels,imageToDisplay,size_of_data);
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
	delete_cab(cab);

  	return SUCCESS;
}