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
	-I ../thirdparty/		\
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
	  shader.spv	\
	  stb_image	\
	  viking_room	\
	  tiny_obj_loader


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

stb_image:
	tar xf					\
		../thirdparty/stb_image.tar	\
		--directory=../thirdparty

tiny_obj_loader:
	tar xf						\
		../thirdparty/tiny_obj_loader.tar	\
		--directory=../thirdparty

viking_room:
	tar xf					\
		../thirdparty/viking_room.tar	\
		--directory=../thirdparty

clean:
	rm -f $(NAME)
