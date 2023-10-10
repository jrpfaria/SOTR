/*******************************************************************
 * Paulo Pedreiras
 * pbrp@ua.pt
 * DETI/IT/UA
 * 
 * ffmpeg-based example to capture video from a webcam
 *		Video is converted from raw format to a given format, via sws_scale() 
 * 		Optionally frames can be stored in files (usefull e.g. for debug) 
 * 
 * Dependencies:
 * 		For Ubuntu 22.04 start by installing sdl-devel and ffmpeg
 * 		$sudo apt install libsdl2-dev ffmpeg
 * 		For ffmpeg SW development the following libs should also be installed:
 * 		- libavcodec-dev libavformat-dev libswscale-dev libavdevice-dev
 * 
 *
 * 
 * TODO:
 * 		Basic implementation is working. 
 *      Todo: 		 
 * 		- No robutsness checks
 * 		- No memory leaks tests
 * 		- Checked only with two computers running Ubuntu 20.04 and 22.04 and 
 * 			a logitech USB cam and a toshiva internal webcam
 * 		
 *
 * Information sources
 * 		ffmpeg base documentation (online)
 * 			https://ffmpeg.org/doxygen/trunk/index.html
 *		pgm format info
 * 			https://en.wikipedia.org/wiki/Netpbm_format#PGM_example
 * 
 * The ffmpeg base documentation misses basic concepts
 * 		The following links provided important and helpfull information- leandromoreira
 * 			has a valuable intro to the ffmpeg architecture:  
 * 		- https://github.com/leandromoreira/ffmpeg-libav-tutorial
 * 		- https://github.com/leixiaohua1020/simplest_ffmpeg_device 
 * 		- https://stackoverflow.com/questions/66384258/sws-scale-yuv-to-rgb-conversion
 * 		- http://www.dranger.com/ffmpeg/ffmpeg.html  
 * 
 * Usefull info
 * 		Check available v4l sources
 * 			v4l2-ctl --list-devices 
 * 		Check formats
 * 			v4l2-ctl -d 0 --list-formats-ext ("-d #" - set the video source queried)
 * 		SDL2 compile and link
 * 			sld2-config to get compile and link flags for SDL2 apps
 * 		SDL2 surface management info
 * 			https://benedicthenshaw.com/soft_render_sdl2.html * 			
 * 			https://stackoverflow.com/questions/69259974/pixel-manipulation-with-sdl-surface	
 * 			https://metacpan.org/pod/SDL2::surface
 */


/* Generic includes */
#include <stdio.h>
#include <string.h> // For memcpy
#include <getopt.h> // For getting command args
#include <semaphore.h>	// For semaphores
#include <sys/mman.h>	// For shmem and others
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

/* ffmpeg includes */
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavdevice/avdevice.h>
#include <libavdevice/avdevice.h>
#include <libavutil/imgutils.h>

/* SDL includes */
#include <SDL2/SDL.h>

/* Other includes */
#include "./include/webCamCapture.h"

/* Global variables */
unsigned char appName[]="webCamCapture"; /* Application name*/
uint8_t optDisplayFlag = FALSE;
uint8_t optVideoSource[]=DEFAULTVIDEOSOURCE;
int nimages = 0; /* number of images to store in file */


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
void * shMemPtr = NULL; 		/* Pointer to shared memory region */
sem_t * newDataSemAddr = NULL;	/* Pointer to semaphore */
 
/*
 * main() function
*/
int main(int argc, char* argv[])
{

	/* ffmpeg variables */
	AVFormatContext	*pFormatCtx;/* Pointer to AV container structure */
	AVInputFormat   *ifmt;		/* Pointer to input formt (video4linux, in the case)*/
	AVCodecContext	*pCodecCtx;	/* Pointer to codec structure */	
	AVCodec			*pCodec;	/* Pointer to codec data */
	AVCodecParameters *pCodecParameters; /* Pointer to codec parameters */
	AVPacket *pPacket; 			/* Pointer to packet */
	AVFrame *pFrame;			/* Pointer to frame */		

	/* Generic variables */
	int	i, 		
		err, /* Generic return code variable */ 		
		videoindex; /* Index of video source that will be used */
	
	/* Process input args */	
	/* Remember that ":" after an option name means that the option takes an argument) */
	while ((i = getopt(argc, argv, ":hdv:f:m:s:")) != -1) {
  		switch (i) {
			case 'h':
				printf("Help for %s\n", argv[0]);
				printf("\t %s [-h] [-d] [-v PATH_TO_VIDEO_SOURCE] [-f N_OF_IMAGES] [-m /sm_name] [-s sem_name]\n", argv[0]);
				printf("\t \t [-h]: print this help\n");
				printf("\t \t [-d]: activate local video display (off by default)\n");
				printf("\t \t [-v PATH_TO_VIDEO_SOURCE]: set path to video source device (%s by default)\n",optVideoSource);
				printf("\t\t     Use e.g. v4l2-ctl --list-devices  to locate video sources\n");
				printf("\t \t [-f N_OF_IMAGES]: number of images to save to file (integer, 1...%d, default 0-not save)\n", MAXIMAGES);
				printf("\t \t [-m]: shared memory name, in the form /string with no spaces. Should start with \"/\" and have no further \"/\"\n");
				printf("\t \t [-s]: semaphore name, in the form string with no spaces. \n");				
				return SUCCESS;
				break;
			case 'd':
				printf("Local display active\n");
				optDisplayFlag = TRUE; // Activate local display 
				break;
			case 'v':
				printf("Setting video source to: %s\n", optarg);
				strcpy(optVideoSource,optarg);
				break;
			case 'f':				
				if(atoi(optarg) >= 1 && atoi(optarg) <= MAXIMAGES) {
					nimages=atoi(optarg);
					printf("Save to file option active, saving %d images\n", nimages);
				} else {
					printf("Invalid number of images (must be between 1 and %d)\n\r", MAXIMAGES);
					return ERRINVNUMIMAGES;
				}
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
				
			case '?':
      			printf("Unknown option: %c\n", optopt);
      			break;
    		case ':':
      			printf("Missing arg for option %c\n", optopt);
      		break;
     	}
  	}	
  	
  	
	/* Initialize libavdevice and register all the input and output devices.*/
	avdevice_register_all();
	
	/* Allocate memory to the component "AVFormatContext" that will hold information about the format (container).*/
	pFormatCtx = avformat_alloc_context();
	
	/* Set the input format (video4linux, in the case) */	
	ifmt=av_find_input_format("video4linux");
	
	/* Open an input stream and read the header.*/
	if(avformat_open_input(&pFormatCtx,optVideoSource,ifmt,NULL)!=0){
		printf("Couldn't open input stream.\n");
		return -1;
	}

	/* Read packets of a media file to get stream information. */
	/* pFormatCtx->nb_streams will hold the amount of streams */
	/* and pFormatCtx->streams[i] has data about each i stream. */
	if(avformat_find_stream_info(pFormatCtx,NULL)<0) {
		printf("Couldn't find stream information.\n");
		return -1;
	}
	
	/* Go through the streams to find a Video media type*/ 
	/* Prints some basic info about audio/video types found */
	videoindex=-1;
	for(i=0; i < pFormatCtx->nb_streams; i++) { 
		pCodecParameters = pFormatCtx->streams[i]->codecpar;
		pCodec = avcodec_find_decoder(pCodecParameters->codec_id);
		
		// specific for video and audio
		if (pCodecParameters->codec_type == AVMEDIA_TYPE_VIDEO) {
			printf("Video Codec: resolution %d x %d\n", pCodecParameters->width, pCodecParameters->height);
			videoindex=i; /* Store stream source index */
		} else if (pCodecParameters->codec_type == AVMEDIA_TYPE_AUDIO) {
			printf("Audio Codec: %d channels, sample rate %d\n", pCodecParameters->channels, pCodecParameters->sample_rate);
		}
		// general
		printf("\tCodec %s ID %d bit_rate %ld\n", pCodec->long_name, pCodec->id, pCodecParameters->bit_rate);
	}

	/* If no video source found, terminate */
	if(videoindex==-1) {
		printf("Couldn't find a video stream.\n");
		return -1;
	}
	
	/* Get codec info of found video source */
	pCodecParameters = pFormatCtx->streams[videoindex]->codecpar;
	pCodec = avcodec_find_decoder(pCodecParameters->codec_id);
	if(pCodec==NULL) {
		printf("Codec not found.\n");
		return -1;
	}

	/* Get a pointer to a codec context and load them*/	
	pCodecCtx = avcodec_alloc_context3(pCodec);
	avcodec_parameters_to_context(pCodecCtx, pCodecParameters);	
		
	/* And then open the codec */
	if(avcodec_open2(pCodecCtx, pCodec,NULL)<0)	{
		printf("Could not open codec.\n");
		return -1;
	}
	
	/* At this point we are ready to read the packets from the stream */
	/* and decode them into frames. To this end it is necessary to  */
	/* allocate memory for both packets and frames. */

  	pFrame = av_frame_alloc();
	if (!pFrame) {
    	printf("failed to allocate memory for AVFrame\n\r");
    	return -1;
  	}
  
  	pPacket = av_packet_alloc();
	if (!pPacket) {
    	printf("failed to allocate memory for AVPacket\n\r");
    	return -1;
  	}

	/* If local display active Init SDl window and rendeder to show live image */
	if(optDisplayFlag) {		
		SDL_Init(SDL_INIT_VIDEO);

		window = SDL_CreateWindow(appName,
			SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
			pCodecParameters->width, pCodecParameters->height,
			SDL_WINDOW_RESIZABLE);

		renderer = SDL_CreateRenderer(window,
			-1, SDL_RENDERER_PRESENTVSYNC);

		width = pCodecParameters->width;
		height = pCodecParameters->height;

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
	
	}

	/* If shmem defined, create it */
	if(shMemActiveFlag) {
		shMemSize = pCodecParameters->width * pCodecParameters->height * IMGBYTESPERPIXEL + 2*sizeof(uint16_t);
		printf("shmemsize=%d\n",shMemSize);
		
		fd = shm_open(shMemName,    /* Open file */
			O_RDWR | O_CREAT, /* open for read/write and create if needed */
			accessPerms);     /* set access permissions */		
		if (fd < 0) {
			printf("[shared memory reservation] Can't get file descriptor...\n\r");
			return -1;
		}
		
		/* Set the file size to exactly the desired size */
		ftruncate(fd, shMemSize); /* get the bytes */
		
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
	}
	
	/* if new imaghe sem, create it */
	if(newDataSemActiveFlag) {
		newDataSemAddr = sem_open(newDataSemName, 	/* semaphore name */
				O_CREAT,       					/* create the semaphore */
				accessPerms,   					/* protection perms */
				0);            					/* initial value */
		if (newDataSemAddr == SEM_FAILED) {
			printf("[semaphore creation] Error creating semaphore \n\r");
			return -1;
		}
	}
		
  	/* Fill the Packet with data from the Stream */
	while (av_read_frame(pFormatCtx, pPacket) >= 0) {

		/* Process SDL events. In this case just termination */
		while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) exit(0);
        }

    	/* Check if packet is originated at the desired video stream */
    	if (pPacket->stream_index == videoindex) {
    		printf("AVPacket->pts %" PRId64, pPacket->pts);
      		err = decode_packet(pPacket, pCodecCtx, pFrame);
      		if (err < 0) {
        		printf("decode_packet() error %d\n\r",err);
				break;
			}      	
    	}    	
    	av_packet_unref(pPacket);
  	}

	/* Release resources and terminate */
  	printf("releasing resources and ending \n\r");

	avformat_close_input(&pFormatCtx);
  	av_packet_free(&pPacket);
  	av_frame_free(&pFrame);
	avcodec_free_context(&pCodecCtx);

  	return SUCCESS;
}

/* 
* Function to decode a packet, generate images and display/store them 
*/
static int decode_packet(AVPacket *pPacket, AVCodecContext *pCodecContext, AVFrame *pFrame)
{
	AVFrame * pConvFrame;

	char frame_filename[1024];	
	int err;
	unsigned char * imgptr; 

	/* Supply raw packet data as input to a decoder */
	err = avcodec_send_packet(pCodecContext, pPacket);
	if (err < 0) {
		printf("Error while sending a packet to the decoder: %s\n\r", av_err2str(err));
		return err;
	}

	while (err >= 0) {
		
		err = avcodec_receive_frame(pCodecContext, pFrame);
		if (err == AVERROR(EAGAIN) || err == AVERROR_EOF) { /* No more data */
			break;
		} else {
			if (err < 0) {
				printf("Error while receiving a frame from the decoder: %s\n\r", av_err2str(err));
				return err;
			}
		}

		/* At this point a frame was fully received */
		if (err >= 0) {
			
			
			/* Print frame info */
			printf( "Frame %d (type=%c, size=%d bytes, format=%d) pts %ld key_frame %d [DTS %d]\n\r",
				pCodecContext->frame_number,
				av_get_picture_type_char(pFrame->pict_type),
				pFrame->pkt_size,
				pFrame->format,
				pFrame->pts,
				pFrame->key_frame,
				pFrame->coded_picture_number
			);

			if(optDisplayFlag) {
				/* Display live image */
				/* Convert a frame to a given output format */
				/* Formats are defined at pixfmt.h, see */
				/* https://ffmpeg.org/doxygen/trunk/pixfmt_8h_source.html */
				/* Note that avFrameConvvertPixelFormat() allocates an AVFrame */
				/* 	for the converted format that must be freed after use */
				/* Be careful with packet formats. There are many with confusing designations ... */
				/*    E.g. RGB24 in packed format: R,G and B components with 8 bits each, pixel by pixel, in a single array*/
				/*      In memory: R0 G0 B0 R1 G1 B1 R2 G2 B3 (where i is the pixel)*/
				/*    There are planar formats in which show up all Rs, then all Gs then all Bs ... */
				/*    and in many cases there are word-alignment issues ...*/
				pConvFrame = avFrameConvertPixelFormat(pFrame, AV_PIX_FMT_0RGB32); 
			
				imgptr = pConvFrame->data[0];

	#ifdef MANUALCOPY
				/* Copy the image byte-by-byte. Usefuk just for debugging and */
				/* checking file formats. Incompatible RGB(A) orders in ffmpeg format and SDL can 
				/*   be sorted out modifying the pointers access order / offsets*/
				for (int y = 0; y < height; ++y) {
					for (int x = 0; x < width; x++) {	
						pixels[4*(x + y * width)] = *(imgptr++);
						pixels[4*(x + y * width)+1] = *(imgptr++);
						pixels[4*(x + y * width)+2] = *(imgptr++);
						pixels[4*(x + y * width)+3] = *(imgptr++);					
					}
				}
	#else
				/* Do a more efficient memmcpy instead of byte-by-byte copy*/
				memcpy(pixels,imgptr,width*height*IMGBYTESPERPIXEL);
	#endif

				SDL_RenderClear(renderer);
				SDL_UpdateTexture(screen_texture, NULL, pixels, width * IMGBYTESPERPIXEL );
				SDL_RenderCopy(renderer, screen_texture, NULL, NULL);
				SDL_RenderPresent(renderer);

				/* Release AVFrame */
				av_frame_free(&pConvFrame);	
			}
			
			/* Copy image to shmem, if suitable */
			if(shMemActiveFlag) {				
				/* Get a pointer to the image frame */
				pConvFrame = avFrameConvertPixelFormat(pFrame, AV_PIX_FMT_0RGB32); 			
				imgptr = pConvFrame->data[0];			
				/* Copy data to shmem.  */					
				memcpy(shMemPtr,pConvFrame->data[0],width*height*IMGBYTESPERPIXEL);	
				/* Release AVFrame */
				av_frame_free(&pConvFrame);	
				
				printf("shmem updated\n");
			}
			
			/* Post new image semaphore, if active */			
			if(newDataSemActiveFlag) {
				if(sem_post(newDataSemAddr)){
					printf("[semaphore post] Error on sem_post \n\r");
					return -1;
				}
				printf("sem post writen\n");
			}
			
      		/* Save to pgm grayscale file first n images */
			if (nimages-- > 0) {
				
				/* Generate filename */
				snprintf(frame_filename, sizeof(frame_filename), "%s-%d.pgm", "frame", pCodecContext->frame_number);
				pConvFrame = avFrameConvertPixelFormat(pFrame, AV_PIX_FMT_YUV420P);
				
				/* Save a grayscale frame into a .pgm file, input frame format is YUV420P */
				save_YUV420P_to_gray_frame(pConvFrame->data[0], pConvFrame->linesize[0], pConvFrame->width, pConvFrame->height, frame_filename);
			
				/* Release AVFrame */
				av_frame_free(&pConvFrame);	
			}
		
			

		}
	}
	return 0;
}

/* 
* Save gray picture (one byte per pixel - 0..255 intensity) to a pgm format file  
*/
static void save_YUV420P_to_gray_frame(unsigned char *buf, int wrap, int xsize, int ysize, char *filename)
{
    FILE *f;
    int i;
    f = fopen(filename,"w");
    /* Output frame to Portable Graymap Format (pgm) */
    /* See pgm format info at https://en.wikipedia.org/wiki/Netpbm_format#PGM_example */
	/* "P5" is the magic number (to identify file type), followd by img with and weight (in pixels) */
	/* and finaly the maximum value that a pixel can take (the white color value, 0 is black)*/
    fprintf(f, "P5\n%d %d\n%d\n", xsize, ysize, 255);

    /* Write pixels to file, line by line */
    for (i = 0; i < ysize; i++)
        fwrite(buf + i * wrap, 1, xsize, f);
    fclose(f);
}


/*
 * Allocate a frame buffer, giving as input the image format and size
 */
static AVFrame* allocPicture(enum AVPixelFormat pix_fmt, int width, int height)
{
  
	AVFrame* frame; /* Pointer to AVFrame */
	
	/* Allocate AVFrame structure */
	frame = av_frame_alloc();
	if (frame == NULL) {
		fprintf(stderr, "av_frame_alloc failed\n\r");
	}

	/* Allocate an image with size w and h and pixel format pix_fmt, and fill pointers and linesizes accordingly*/
	if (av_image_alloc(frame->data, frame->linesize, width, height, pix_fmt, 1) < 0)
	{
		fprintf(stderr, "av_image_alloc failed");
	}

	/* Fill in the AVFrame structure fields */
	frame->width = width;
	frame->height = height;
	frame->format = pix_fmt;

	return frame;
}

/* 
*  Takes as input a AVFrame in raw format and convert it to a desired format 
*  Formats are defined at pixfmt.h, see: 
*  https://ffmpeg.org/doxygen/trunk/pixfmt_8h_source.html 
*/
static AVFrame* avFrameConvertPixelFormat( const AVFrame* src, enum AVPixelFormat dstFormat)
{
	AVFrame* dst; /* Pointer to destination AVFrame*/
	struct SwsContext* conversion; /* Context variable for conversion */

	/* Allocate and fill in dst AVFrame according to image size and format */		
	int width = src->width;
	int height = src->height;
	dst = allocPicture(dstFormat, width, height);

	/* Init context convesrion structure */
	conversion = sws_getContext(width,
								height,
								(enum AVPixelFormat)src->format,
								width,
								height,
								dstFormat,
								SWS_FAST_BILINEAR | SWS_FULL_CHR_H_INT | SWS_ACCURATE_RND,
								NULL,
								NULL,
								NULL);

	/* Do the actual conversion and fill in fields of output context variable */
	sws_scale(conversion,  (const uint8_t * const*)src->data, src->linesize, 0, height, dst->data, dst->linesize);
	
	dst->format = dstFormat;
	dst->width = src->width;
	dst->height = src->height;
	
	/* Release resources and return*/
	sws_freeContext(conversion);

	return dst;
}

