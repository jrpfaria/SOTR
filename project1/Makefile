CC=gcc
LDIRS = -L/usr/local/lib 

# Use script to get SDL2 flags (recommended for portability)
SDL2_CONFIG = sdl2-config
CFLAGS = $(shell $(SDL2_CONFIG) --cflags)
LIBS = $(shell $(SDL2_CONFIG) --libs)

# Add other compilation and linking flags if necessary
CFLAGS += -I/usr/local/include
LIBS += -lavformat -lavcodec -lavutil -lavdevice -lswscale

all: webCamCapture imageViewer
.PHONY: all

# Generate application
webCamCapture: webCamCapture.c  
	$(CC) $(CFLAGS) $(LDIRS) $< -o $@ $(LIBS) 
	
imageViewer: imageViewer.c  
	$(CC) $(CFLAGS) $(LDIRS) $< -o $@ $(LIBS) 

.PHONY: clean 

clean:
	rm -f *.o 
	rm webCamCapture
	
