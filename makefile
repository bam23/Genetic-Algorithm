CC ?= cc
CFLAGS ?= -O2 -g
WARNINGS = -std=c11 -Wall -Wextra -Wpedantic -Wconversion -Wshadow \
	-Wstrict-prototypes -Werror
LDLIBS = -lm

PROGRAM = tsp
COMMON_OBJECTS = graph.o function.o queue.o

.PHONY: all run clean

all: $(PROGRAM)

$(PROGRAM): $(COMMON_OBJECTS) main.o
	$(CC) $(CFLAGS) $(WARNINGS) -o $@ $^ $(LDLIBS)

graph.o: graph.c graph.h
	$(CC) $(CFLAGS) $(WARNINGS) -c graph.c

function.o: function.c function.h graph.h queue.h
	$(CC) $(CFLAGS) $(WARNINGS) -c function.c

queue.o: queue.c queue.h graph.h
	$(CC) $(CFLAGS) $(WARNINGS) -c queue.c

main.o: main.c function.h graph.h queue.h
	$(CC) $(CFLAGS) $(WARNINGS) -c main.c

run: $(PROGRAM)
	./$(PROGRAM)

clean:
	rm -f *.o $(PROGRAM)
