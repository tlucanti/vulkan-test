
CFLAGS	= -Wall -Wextra -Werror -g3 -O0 -std=c++17 -fdiagnostics-color=always
LDFLAGS	= -lglfw -lvulkan

all: extentions triangle

extentions:
	g++	$(CFLAGS) extentions.cpp $(LDFLAGS) -o extentions
.PHONY: extentions

triangle:
	g++	$(CFLAGS) triangle.cpp $(LDFLAGS) -o triangle
.PHONY: triangle

