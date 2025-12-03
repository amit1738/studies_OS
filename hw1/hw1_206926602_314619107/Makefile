# Compiler and Flags
CC = gcc
CFLAGS = -g -Wall

# Target Executable Name (as per instructions)
TARGET = hw1shell

# Object Files (The compiled versions of your .c files)
OBJS = shell.o internal.o process_control.o error_handling.o

# Default Target (Type 'make' to build this)
all: $(TARGET)

# Link all object files to create the final executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# Compilation Rules for each .c file
shell.o: shell.c internal.h process_control.h error_handling.h
	$(CC) $(CFLAGS) -c shell.c

internal.o: internal.c internal.h process_control.h error_handling.h
	$(CC) $(CFLAGS) -c internal.c

process_control.o: process_control.c process_control.h error_handling.h
	$(CC) $(CFLAGS) -c process_control.c

error_handling.o: error_handling.c error_handling.h
	$(CC) $(CFLAGS) -c error_handling.c

# Clean Target (Type 'make clean' to remove built files)
clean:
	rm -f $(OBJS) $(TARGET)