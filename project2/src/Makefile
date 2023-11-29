# Makefile for C application

# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wall -I./include

# Source files
SRC = main.c uart_interface.c rtdb.c

# Object files
OBJ = $(SRC:.c=.o)

# Executable name
TARGET = test_application

# Default target
all: $(TARGET)

# Linking step
$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

# Compilation step for each source file
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Clean up
clean:
	rm -f $(OBJ) $(TARGET)
