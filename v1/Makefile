
CC = clang++
GLSLC = glslc

CFLAGS	= -Wall -Wextra -Werror -std=c++17 -fdiagnostics-color=always
INCLUDE = -I ../thirdparty
LDFLAGS	= -lglfw -lvulkan

CFLAGS += -g3 -O0
CFLAGS += -D CONFIG_VALIDATION_LAYERS=1

all: extentions

extentions:
	$(CC) $(CFLAGS) $(INCLUDE) extentions.cpp $(LDFLAGS) -o extentions
.PHONY: extentions

