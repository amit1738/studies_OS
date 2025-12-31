CC = gcc
CFLAGS = -Wall -g -pthread
TARGET = hw2
OBJS = dispatcher.o worker.o queue.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

dispatcher.o: dispatcher.c worker.h queue.h
	$(CC) $(CFLAGS) -c dispatcher.c

worker.o: worker.c worker.h
	$(CC) $(CFLAGS) -c worker.c

queue.o: queue.c queue.h worker.h
	$(CC) $(CFLAGS) -c queue.c

clean:
	rm -f $(TARGET) $(OBJS) *.txt

.PHONY: all clean
