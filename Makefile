#!/bin/bash

CXX	= clang++
VK_SDK	= $(PWD)/../vulkan-sdk/

CXXFLAGS  = \
	-O0 -g3 \
	-std=c++23 \
	-I $(VK_SDK)/x86_64/include/ \
	\
	-Wall \
	-Wextra \
	-Werror \
	-Wno-eager-load-cxx-named-modules \
	\
	-D CONFIG_VALIDATION_LAYERS=1 \
	-D CONFIG_VERBOSE=1 \
	-D CONFIG_RESIZABLE=1

NAME	= main.elf

$(NAME): Makefile vulkan.pcm main.cpp engine.cpp engine.hpp shader.spv
	$(CXX) \
		$(CXXFLAGS) \
		-fmodule-file=vulkan.pcm \
		main.cpp engine.cpp \
		-l glfw \
		-o main.elf

vulkan.pcm: Makefile
	$(CXX) \
		$(CXXFLAGS) \
		--precompile \
		-o vulkan.pcm \
		$(VK_SDK)/x86_64/include/vulkan/vulkan.cppm

shader.spv: Makefile shader.slang
	slangc \
		shader.slang \
		-profile spirv_1_4 \
		-emit-spirv-directly \
		-fvk-use-entrypoint-name \
		-separate-debug-info \
		\
		-o shader.spv

clean:
	rm -f $(NAME) vulkan.pcm
