CC =  gcc # Path to compiler
L_FLAGS = -lrt -lpthread -lm
#C_FLAGS = -g

all: ptrt
.PHONY: all

# Project compilation
ptrt: periodicTaskRT.c
	$(CC) $< -o $@ $(C_FLAGS) $(L_FLAGS)
	
.PHONY: clean 

clean:
	rm -f *.c~ 
	rm -f ptrt
	
# Some notes
# $@ represents the left side of the ":"
# $^ represents the right side of the ":"
# $< represents the first item in the dependency list   

