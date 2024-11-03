
CC = clang++
GLSLC = glslc

CFLAGS	= -Wall -Wextra -Werror -std=c++17 -fdiagnostics-color=always
INCLUDE = -I thirdparty
LDFLAGS	= -lglfw -lvulkan

CFLAGS += -g3 -O0
CFLAGS += -D CONFIG_VALIDATION_LAYERS=1

all: stb_image extentions triangle

extentions:
	$(CC) $(CFLAGS) $(INCLUDE) extentions.cpp $(LDFLAGS) -o extentions
.PHONY: extentions

triangle:
	$(GLSLC) shaders/triangle.vert -o vert.spv
	$(GLSLC) shaders/triangle.frag -o frag.spv
	$(CC) $(CFLAGS) triangle.cpp $(LDFLAGS) -o triangle
.PHONY: triangle

stb_image:
	tar xf thirdparty/stb_image.tar --directory=thirdparty
