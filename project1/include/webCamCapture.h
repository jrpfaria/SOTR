#ifndef WEBCAMCAPTURE_H_
#define WEBCAMCAPTURE_H_
#define FALSE   0   /* The usual true and false */
#define TRUE (!0)

#define SUCCESS 0           /* Program terminates normally*/
#define ERRINVNUMIMAGES -1  /* Program aborts with error - invalid number of files to store */

//#define MANUALCOPY  /* If defined, it is made a byte-by-byte copy btween ffmpeg and SDl instead of a memcpy*/

#define DEFAULTVIDEOSOURCE "/dev/video2"
#define MAXIMAGES 99 /* Maximum number of files/images that are saved */
#define IMGBYTESPERPIXEL 4 /* Number of bytes per pixel. Allowed formats are RGB888, RGBA8888 and */
                            /* others that use both 4 bytes. Be careful because sometimes it is not obvious */
                            /* e.g. RGB888 in SDL is 32bit word aligned, using 4 bytes */
/* Function prototypes */

/* Decode packets into frames */
static int decode_packet(AVPacket *pPacket, AVCodecContext *pCodecContext, AVFrame *pFrame);

/* Save a frame/image into a greyscale .pgm file */
static void save_YUV420P_to_gray_frame(unsigned char *buf, int wrap, int xsize, int ysize, char *filename);

/* Allocate a AVFrame of a given format and size */
static AVFrame* allocPicture(enum AVPixelFormat pix_fmt, int width, int height);

/* Converts an AVFrame to a given format */
static AVFrame* avFrameConvertPixelFormat(const AVFrame* src, enum AVPixelFormat dstFormat);

#endif