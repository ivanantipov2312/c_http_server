CC=gcc

SRC_DIR=src
OBJ_DIR=obj

CFLAGS=-pthread -I$(SRC_DIR)

BIN=http_server
SRCS=$(wildcard $(SRC_DIR)/*.c)
OBJS=$(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

$(BIN): $(OBJS) $(OBJ_DIR)
	$(CC) $(OBJS) -o $@

$(OBJ_DIR):
	mkdir -p $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@
