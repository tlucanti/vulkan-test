
CFLAGS	= -Wall -Wextra -std=c++17 -fdiagnostics-color=always
LDFLAGS	= -lglfw -lvulkan

CFLAGS += -g3 -O0
CFLAGS += -D CONFIG_VALIDATION_LAYERS=1

all: extentions triangle

extentions:
	g++	$(CFLAGS) extentions.cpp $(LDFLAGS) -o extentions
.PHONY: extentions

triangle:
	g++	$(CFLAGS) triangle.cpp $(LDFLAGS) -o triangle
.PHONY: triangle

