CC ?= cc
CXX ?= c++
AR ?= ar
CFLAGS ?= -O2 -g
CXXFLAGS ?= -O2 -g
ARFLAGS = rcs

WARNINGS = -std=c11 -Wall -Wextra -Wpedantic -Wconversion -Wshadow \
	-Wstrict-prototypes -Werror
CXX_WARNINGS = -std=c++17 -Wall -Wextra -Wpedantic -Werror
LDLIBS = -lm

BUILD_DIR = build
LIBRARY = $(BUILD_DIR)/libtsp.a
PROGRAM = tsp
TEST_PROGRAM = tsp-tests
SANITIZE_PROGRAM = tsp-tests-sanitize

CORE_SOURCES = src/graph.c src/heap.c src/solver.c src/status.c
CORE_OBJECTS = $(CORE_SOURCES:src/%.c=$(BUILD_DIR)/src/%.o)
CLI_OBJECT = $(BUILD_DIR)/cli/main.o
TEST_OBJECT = $(BUILD_DIR)/tests/tests.o

.PHONY: all run test sanitize fno-common cpp-check clean

all: $(PROGRAM)

$(PROGRAM): $(CLI_OBJECT) $(LIBRARY)
	$(CC) $(CFLAGS) $(WARNINGS) -o $@ $(CLI_OBJECT) $(LIBRARY) $(LDLIBS)

$(LIBRARY): $(CORE_OBJECTS)
	$(AR) $(ARFLAGS) $@ $^

$(BUILD_DIR)/src/%.o: src/%.c include/tsp/tsp.h
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(WARNINGS) -Iinclude -Isrc -c $< -o $@

$(BUILD_DIR)/src/heap.o: src/heap.h
$(BUILD_DIR)/src/solver.o: src/heap.h src/solver_internal.h

$(CLI_OBJECT): cli/main.c include/tsp/tsp.h
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(WARNINGS) -Iinclude -c $< -o $@

$(TEST_OBJECT): tests/tests.c include/tsp/tsp.h src/heap.h \
	src/solver_internal.h
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(WARNINGS) -Iinclude -Isrc -c $< -o $@

$(TEST_PROGRAM): $(TEST_OBJECT) $(LIBRARY)
	$(CC) $(CFLAGS) $(WARNINGS) -o $@ $(TEST_OBJECT) $(LIBRARY) $(LDLIBS)

run: $(PROGRAM)
	./$(PROGRAM)

test: $(TEST_PROGRAM)
	./$(TEST_PROGRAM)

sanitize:
	$(CC) $(WARNINGS) -O1 -g -fsanitize=address,undefined \
		-fno-omit-frame-pointer -Iinclude -Isrc \
		$(CORE_SOURCES) tests/tests.c -o $(SANITIZE_PROGRAM) $(LDLIBS)
	./$(SANITIZE_PROGRAM)

fno-common:
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(WARNINGS) -fno-common -Iinclude -Isrc \
		$(CORE_SOURCES) tests/tests.c \
		-o $(BUILD_DIR)/tsp-tests-fno-common $(LDLIBS)
	./$(BUILD_DIR)/tsp-tests-fno-common

cpp-check:
	printf '%s\n' '#include <tsp/tsp.h>' 'int main() { return 0; }' | \
		$(CXX) $(CXXFLAGS) $(CXX_WARNINGS) -Iinclude -x c++ -fsyntax-only -

clean:
	rm -rf $(BUILD_DIR)
	rm -f *.o
	rm -f $(PROGRAM) $(TEST_PROGRAM) $(SANITIZE_PROGRAM)
	rm -rf $(SANITIZE_PROGRAM).dSYM
