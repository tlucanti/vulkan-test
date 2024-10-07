
CC = c++
GLSLC = glslc

CFLAGS	= -Wall -Wextra -Werror -std=c++17 -fdiagnostics-color=always
LDFLAGS	= -lglfw -lvulkan

CFLAGS += -g3 -O0
CFLAGS += -D CONFIG_VALIDATION_LAYERS=1

all: extentions triangle

extentions:
	$(CC) $(CFLAGS) extentions.cpp $(LDFLAGS) -o extentions
.PHONY: extentions

triangle:
	$(GLSLC) triangle.vert -o vert.spv
	$(GLSLC) triangle.frag -o frag.spv
	$(CC) $(CFLAGS) triangle.cpp $(LDFLAGS) -o triangle
.PHONY: triangle

