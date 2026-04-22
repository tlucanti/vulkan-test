#!/bin/bash

include ../.env

CXX	?= clang++
VK_SDK	?= $(PWD)/../vulkan-sdk/
SLANGC  ?= $(VK_SDK)/x86_64/bin/slangc

CXXFLAGS  =				\
	-O0 -g3				\
	-std=c++23			\
	-I $(VK_SDK)/x86_64/include/	\
	-L $(VK_SDK)/lib		\
					\
	-Wall				\
	-Wextra				\
					\
	-D CONFIG_VALIDATION_LAYERS=1	\
	-D CONFIG_VERBOSE=1		\
	-D CONFIG_RESIZABLE=1		\
	-D CONFIG_MAX_FRAMES_IN_FLIGHT=2


NAME	= main.elf

$(NAME): Makefile main.cpp engine.cpp engine.hpp shader.spv
	$(CXX)				\
		$(CXXFLAGS)		\
		main.cpp engine.cpp	\
		-l glfw			\
		-l vulkan		\
		-o main.elf

shader.spv: Makefile shader.slang
	$(SLANGC) \
		shader.slang \
		-profile spirv_1_4 \
		-emit-spirv-directly \
		-fvk-use-entrypoint-name \
		-separate-debug-info \
		\
		-o shader.spv

clean:
	rm -f $(NAME)
