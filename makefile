CC ?= cc
CFLAGS ?= -O2 -g
WARNINGS = -std=c11 -Wall -Wextra -Wpedantic -Wconversion -Wshadow \
	-Wstrict-prototypes -Werror
LDLIBS = -lm

PROGRAM = tsp
TEST_PROGRAM = tsp-tests
COMMON_OBJECTS = graph.o function.o queue.o

.PHONY: all run test sanitize clean

all: $(PROGRAM)

$(PROGRAM): $(COMMON_OBJECTS) main.o
	$(CC) $(CFLAGS) $(WARNINGS) -o $@ $^ $(LDLIBS)

$(TEST_PROGRAM): $(COMMON_OBJECTS) tests.o
	$(CC) $(CFLAGS) $(WARNINGS) -o $@ $^ $(LDLIBS)

graph.o: graph.c graph.h
	$(CC) $(CFLAGS) $(WARNINGS) -c graph.c

function.o: function.c function.h graph.h queue.h
	$(CC) $(CFLAGS) $(WARNINGS) -c function.c

queue.o: queue.c queue.h graph.h
	$(CC) $(CFLAGS) $(WARNINGS) -c queue.c

main.o: main.c function.h graph.h queue.h
	$(CC) $(CFLAGS) $(WARNINGS) -c main.c

tests.o: tests.c function.h graph.h queue.h
	$(CC) $(CFLAGS) $(WARNINGS) -c tests.c

run: $(PROGRAM)
	./$(PROGRAM)

test: $(TEST_PROGRAM)
	./$(TEST_PROGRAM)

sanitize:
	$(CC) $(WARNINGS) -O1 -g -fsanitize=address,undefined \
		-fno-omit-frame-pointer graph.c function.c queue.c tests.c \
		-o $(TEST_PROGRAM)-sanitize $(LDLIBS)
	./$(TEST_PROGRAM)-sanitize

clean:
	rm -f *.o $(PROGRAM) $(TEST_PROGRAM) $(TEST_PROGRAM)-sanitize
	rm -rf $(TEST_PROGRAM)-sanitize.dSYM
