CC=gcc
CFLAGS=-std=gnu11 -Wall -Wextra -Werror -pedantic
LIBS=-lpthread
OBJECTS=serve.o
MAIN=serve

all: $(OBJECTS)
	$(CC) $(CFLAGS) $(LIBS) $(OBJECTS) -o $(MAIN)

%.o : %.c
	$(CC) $(CFLAGS) $(LIBS) -c $<

clean:
	rm *.o $(MAIN)
