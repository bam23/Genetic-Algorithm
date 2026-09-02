CC ?= cc
AR ?= ar
CFLAGS ?= -O2 -g
ARFLAGS = rcs

WARNINGS = -std=c11 -Wall -Wextra -Wpedantic -Wconversion -Wshadow \
	-Wstrict-prototypes -Werror
LDLIBS = -lm

BUILD_DIR = build
LIBRARY = $(BUILD_DIR)/libtsp.a
PROGRAM = tsp
TEST_PROGRAM = tsp-tests
SANITIZE_PROGRAM = tsp-tests-sanitize

CORE_SOURCES = src/graph.c src/solver.c src/heap.c
CORE_OBJECTS = $(CORE_SOURCES:src/%.c=$(BUILD_DIR)/src/%.o)
CLI_OBJECT = $(BUILD_DIR)/cli/main.o
TEST_OBJECT = $(BUILD_DIR)/tests/tests.o

.PHONY: all run test sanitize clean

all: $(PROGRAM)

$(PROGRAM): $(CLI_OBJECT) $(LIBRARY)
	$(CC) $(CFLAGS) $(WARNINGS) -o $@ $(CLI_OBJECT) $(LIBRARY) $(LDLIBS)

$(LIBRARY): $(CORE_OBJECTS)
	$(AR) $(ARFLAGS) $@ $^

$(BUILD_DIR)/src/%.o: src/%.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(WARNINGS) -Isrc -c $< -o $@

$(BUILD_DIR)/src/graph.o: src/graph.h
$(BUILD_DIR)/src/solver.o: src/solver_internal.h src/graph.h src/heap.h
$(BUILD_DIR)/src/heap.o: src/heap.h src/graph.h

$(CLI_OBJECT): cli/main.c src/solver_internal.h src/graph.h src/heap.h
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(WARNINGS) -Isrc -c $< -o $@

$(TEST_OBJECT): tests/tests.c src/solver_internal.h src/graph.h src/heap.h
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(WARNINGS) -Isrc -c $< -o $@

$(TEST_PROGRAM): $(TEST_OBJECT) $(LIBRARY)
	$(CC) $(CFLAGS) $(WARNINGS) -o $@ $(TEST_OBJECT) $(LIBRARY) $(LDLIBS)

run: $(PROGRAM)
	./$(PROGRAM)

test: $(TEST_PROGRAM)
	./$(TEST_PROGRAM)

sanitize:
	$(CC) $(WARNINGS) -O1 -g -fsanitize=address,undefined \
		-fno-omit-frame-pointer -Isrc $(CORE_SOURCES) tests/tests.c \
		-o $(SANITIZE_PROGRAM) $(LDLIBS)
	./$(SANITIZE_PROGRAM)

clean:
	rm -rf $(BUILD_DIR)
	rm -f *.o
	rm -f $(PROGRAM) $(TEST_PROGRAM) $(SANITIZE_PROGRAM)
	rm -rf $(SANITIZE_PROGRAM).dSYM
