SRC_DIR = src
INC_DIR = inc
OBJ_DIR = obj
BIN_DIR = bin
TEST_DIR = test

TARGET = $(BIN_DIR)/project_lucario

CC = gcc
CFLAGS = -Wall -Wextra -g -I$(INC_DIR)

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

MAIN_OBJ = $(OBJ_DIR)/main.o
TEST_OBJS = $(filter-out $(MAIN_OBJ), $(OBJS))

all: $(TARGET)
	@echo -e "\e[1;32m[SUCCESS]\e[0m Compiled in normal mode"

debug: CFLAGS += -DDEBUG
debug: $(TARGET)
	@echo -e "\e[1;32m[SUCCESS]\e[0m Compiled in debug mode"

$(TARGET): $(OBJS)
	@mkdir -p $(BIN_DIR)
	@echo -e "\e[1;33m[INFO]\e[0m Linking executable: $@"
	$(CC) $(CFLAGS) $^ -o $@
	@echo -e "\e[1;32m[SUCCESS]\e[0m Build successful!"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	@echo -e "\e[1;33m[INFO]\e[0m Compiling: $<"
	$(CC) $(CFLAGS) -c $< -o $@

test: $(OBJS)
ifndef mod
	@echo -e "\e[1;31m[ERROR]\e[0m You must specify a module. Usage: make test mod=name"
else
	@mkdir -p $(BIN_DIR)
	@echo -e "\e[1;33m[INFO]\e[0m Compiling tests for: $(mod)"
	$(CC) $(CFLAGS) $(TEST_DIR)/test_$(mod).c $(TEST_OBJS) -o $(BIN_DIR)/test_$(mod)
	@echo -e "\e[1;34m[TEST]\e[0m Running tests..."
	./$(BIN_DIR)/test_$(mod)
endif

clean:
	@echo -e "\e[1;33m[INFO]\e[0m Cleaning up..."
	rm -rf $(OBJ_DIR) $(BIN_DIR)

run:
	@if [ ! -x "$(TARGET)" ]; then \
		echo -e "\e[1;31m[ERROR]\e[0m Executable '$(TARGET)' not found. Please run 'make' first."; \
		exit 1; \
	fi
	@echo -e "\e[1;33m[INFO]\e[0m Running..."
	./$(TARGET)

docs:
	@if ! command -v doxygen &> /dev/null; then \
		echo -e "\e[1;31m[ERROR]\e[0m doxygen not found. Please install it first."; \
		exit 1; \
	fi
	@echo -e "\e[1;33m[INFO]\e[0m Running..."
	doxygen Doxyfile

.PHONY: all debug test clean run docs
