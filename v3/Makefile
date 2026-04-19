
CC = clang++
GLSLC = glslc

THIRDPARTY = ../thirdparty/

CFLAGS	= -Wall -Wextra -Werror -std=c++17 -fdiagnostics-color=always
INCLUDE = -I $(THIRDPARTY)
LDFLAGS	= -lglfw -lvulkan

CFLAGS += -g3 -O0
CFLAGS += -D CONFIG_VALIDATION_LAYERS=1

all: stb_image tiny_obj_loader main

main:
	$(CC) $(CFLAGS) $(INCLUDE) main.cpp $(LDFLAGS) -o main
.PHONY: main

stb_image:
	tar xf $(THIRDPARTY)/stb_image.tar --directory=$(THIRDPARTY)

tiny_obj_loader:
	tar xf $(THIRDPARTY)/tiny_obj_loader.tar --directory=$(THIRDPARTY)

