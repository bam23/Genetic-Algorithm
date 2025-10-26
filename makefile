CC = gcc
CFLAGS = -Wall -Wextra -O2 -g
# For debugging, you may temporarily enable:
# CFLAGS += -fsanitize=address -fno-omit-frame-pointer

all: tsp

tsp: graph.o function.o queue.o tester.o
	$(CC) $(CFLAGS) -o tsp graph.o function.o queue.o tester.o

graph.o: graph.c graph.h
	$(CC) $(CFLAGS) -c graph.c

function.o: function.c function.h queue.h graph.h
	$(CC) $(CFLAGS) -c function.c -include queue.h

queue.o: queue.c queue.h function.h
	$(CC) $(CFLAGS) -c queue.c -include queue.h

tester.o: tester.c graph.h function.h
	$(CC) $(CFLAGS) -c tester.c

clean:
	rm -f *.o tsp