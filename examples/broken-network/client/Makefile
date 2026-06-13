CC = gcc
CFLAGS = -Wall -pedantic -std=c11


all: client

client: client.c
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f ./client
