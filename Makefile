CC = gcc
CFLAGS = -Iinclude -Ilib -Wextra -Wall

SRC = $(shell find src lib -name "*.c")
OBJ = $(patsubst src/%.c,obj/%.o,$(SRC))

OUT = bin/nsh

all: $(OUT)

$(OUT): $(OBJ)
	mkdir -p bin
	$(CC) $(CFLAGS) -o $(OUT) $(OBJ) -lcurl

obj/%.o: src/%.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@ 

debug:
	mkdir -p bin
	$(CC) -Iinclude -g -fsanitize=address -o bin/nsh_debug $(SRC)
	
clean:
	rm -rf obj bin

re: clean all

run:
	./bin/nsh
