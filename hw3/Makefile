CC = gcc
CFLAGS = -Wall -g

all: hw3server hw3client

hw3server: hw3server.c
	$(CC) $(CFLAGS) -o hw3server hw3server.c

hw3client: hw3client.c
	$(CC) $(CFLAGS) -o hw3client hw3client.c

clean:
	rm -f hw3server hw3client *.o