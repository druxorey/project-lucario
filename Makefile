SRC_DIR = src
INC_DIR = inc
OBJ_DIR = obj
BIN_DIR = bin

TARGET = $(BIN_DIR)/project_lucario

CC = gcc
CFLAGS = -Wall -Wextra -g -I$(INC_DIR)
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

all: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p $(BIN_DIR)
	@echo -e "\e[1;34m[INFO]\e[0m Linking executable: $@"
	$(CC) $(CFLAGS) $^ -o $@
	@echo -e "\e[1;32m[SUCCESS]\e[0m Build successful!"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	@echo -e "\e[1;34m[INFO]\e[0m Compiling: $<"
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	@echo -e "\e[1;34m[INFO]\e[0m Cleaning up..."
	rm -rf $(OBJ_DIR) $(BIN_DIR)

run: all
	@echo -e "\e[1;34m[INFO]\e[0m Running..."
	./$(TARGET)

run_debug: CFLAGS += -DDEBUG
run_debug: all
	@echo -e "\e[1;34m[INFO]\e[0m Running in debug mode..."
	./$(TARGET)

.PHONY: all clean run run_debug
