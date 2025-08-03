
CC = clang++
GLSLC = glslc

CFLAGS	= -Wall -Wextra -Werror -std=c++17 -fdiagnostics-color=always
INCLUDE = -I thirdparty
LDFLAGS	= -lglfw -lvulkan

CFLAGS += -g3 -O0
CFLAGS += -D CONFIG_VALIDATION_LAYERS=1

all: stb_image tiny_obj_loader model main

main:
	$(GLSLC) test-triangle/shaders/triangle.vert -o vert.spv
	$(GLSLC) test-triangle/shaders/triangle.frag -o frag.spv
	$(CC) $(CFLAGS) $(INCLUDE) main.cpp $(LDFLAGS) -o main
.PHONY: main

stb_image:
	tar xf thirdparty/stb_image.tar --directory=thirdparty

tiny_obj_loader:
	tar xf thirdparty/tiny_obj_loader.tar --directory=thirdparty

model:
	tar xf test-triangle/models/viking_room.tar --directory=test-triangle/models
