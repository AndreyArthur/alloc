CC=gcc
CFLAGS=-g -Wall -Wextra -Wpedantic

SRC_DIR=src
SRC_SOURCES=$(wildcard $(SRC_DIR)/*.c)

OBJECTS_DIR=objects
SRC_OBJECTS=$(patsubst $(SRC_DIR)/%.c, $(OBJECTS_DIR)/%.o, $(SRC_SOURCES))

TESTS_DIR=tests
TESTS_SOURCES=$(wildcard $(TESTS_DIR)/*.c)

TESTS_OBJECTS=$(patsubst $(TESTS_DIR)/%.c, $(OBJECTS_DIR)/%.o, $(TESTS_SOURCES))

BIN_DIR=bin
TESTS_BINS=$(patsubst $(TESTS_DIR)/%.c, $(BIN_DIR)/%, $(TESTS_SOURCES))

clean:
	rm -rf $(BIN_DIR) $(OBJECTS_DIR)

test: $(TESTS_BINS)

runtest: $(TESTS_BINS)
	./$(BIN_DIR)/heap_test

$(SRC_OBJECTS): $(OBJECTS_DIR) $(SRC_SOURCES)
	$(CC) $(CFLAGS) -c $(SRC_DIR)/heap.c -o $(OBJECTS_DIR)/heap.o

$(TESTS_OBJECTS): $(OBJECTS_DIR) $(TESTS_SOURCES)
	$(CC) $(CFLAGS) -c $(TESTS_DIR)/heap_test.c -o $(OBJECTS_DIR)/heap_test.o

$(TESTS_BINS): $(BIN_DIR) $(SRC_OBJECTS) $(TESTS_OBJECTS)
	$(CC) $(CFLAGS) -o $(BIN_DIR)/heap_test  $(OBJECTS_DIR)/heap_test.o $(SRC_OBJECTS)

$(OBJECTS_DIR):
	mkdir -p $@

$(BIN_DIR):
	mkdir -p $@
