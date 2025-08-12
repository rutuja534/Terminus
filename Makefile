# Compiler
CC=gcc

# Compiler flags: -g for debugging, -Wall to show all warnings
CFLAGS=-g -Wall

# The target executable
TARGET=myshell

all: $(TARGET)

$(TARGET): main.c
	 $(CC) $(CFLAGS) -o $(TARGET) main.c

clean:
	 rm -f $(TARGET)
