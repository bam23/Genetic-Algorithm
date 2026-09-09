CMAKE ?= cmake
CTEST ?= ctest

BUILD_DIR = build
PROGRAM = tsp
TEST_PROGRAM = tsp-tests
SANITIZE_PROGRAM = tsp-tests-sanitize

.PHONY: all run test sanitize fno-common cpp-check clean

all:
	$(CMAKE) --preset default
	$(CMAKE) --build --preset default --target tsp_cli
	$(CMAKE) -E copy_if_different $(BUILD_DIR)/$(PROGRAM) ./$(PROGRAM)

run: all
	./$(PROGRAM)

test:
	$(CMAKE) --preset default
	$(CMAKE) --build --preset default --target tsp_tests
	$(CMAKE) -E copy_if_different $(BUILD_DIR)/$(TEST_PROGRAM) ./$(TEST_PROGRAM)
	$(CTEST) --preset default

sanitize:
	$(CMAKE) --preset sanitize
	$(CMAKE) --build --preset sanitize --target tsp_tests
	$(CMAKE) -E copy_if_different \
		$(BUILD_DIR)/sanitize/$(TEST_PROGRAM) ./$(SANITIZE_PROGRAM)
	$(CTEST) --preset sanitize

fno-common:
	$(CMAKE) --preset fno-common
	$(CMAKE) --build --preset fno-common --target tsp_tests
	$(CTEST) --preset fno-common

cpp-check:
	$(CMAKE) --preset cpp-check
	$(CMAKE) --build --preset cpp-check --target tsp_cpp_header_check

clean:
	$(CMAKE) -E rm -rf $(BUILD_DIR)
	$(CMAKE) -E rm -f *.o $(PROGRAM) $(TEST_PROGRAM) $(SANITIZE_PROGRAM)
	$(CMAKE) -E rm -rf $(SANITIZE_PROGRAM).dSYM
