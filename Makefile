CC=gcc

CFLAGS = -I./lib/glad/include -I./lib/glfw/include
LFLAGS = lib/glad/src/glad.o lib/glfw/src/libglfw3.a
LFLAGS += -framework OpenGL -framework IOKit -framework CoreVideo -framework Cocoa

.PHONY: all

all: test

libs:
	cd lib/glad && $(CC) -Iinclude -c src/glad.c -o src/glad.o
	cd lib/glfw && cmake . && make

test: OpenGL-Tutorial.c
	$(CC) -o $@ $^ $(LFLAGS) $(CFLAGS)
