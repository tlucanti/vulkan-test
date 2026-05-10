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
	-Wextra


NAME	= main.elf
DEP	= Makefile	\
			\
	  main.cpp	\
	  engine.cpp	\
	  engine.hpp	\
	  vertex.hpp	\
	  util.cpp	\
	  config.h      \
			\
	  shader.spv


$(NAME): $(DEP)
	$(CXX)				\
		$(CXXFLAGS)		\
					\
		main.cpp		\
		engine.cpp		\
		util.cpp		\
					\
		-l glfw			\
		-l vulkan		\
					\
		-o main.elf

shader.spv: Makefile shader.slang
	$(SLANGC)			\
		shader.slang		\
		-profile spirv_1_4	\
		-emit-spirv-directly	\
		-fvk-use-entrypoint-name\
		-separate-debug-info	\
					\
		-o shader.spv

clean:
	rm -f $(NAME)
