#!/bin/bash

CXX	= clang++
VK_SDK	= $(HOME)/git/vulkan-sdk/

NAME	= main.elf

$(NAME): vulkan.pcm main.cpp
	$(CXX) \
		-std=c++23 \
		-fmodule-file=vulkan.pcm \
		-Wno-eager-load-cxx-named-modules \
		main.cpp \
		-l glfw \
		-o main.elf

vulkan.pcm:
	$(CXX) \
		-D VULKAN_HPP_NO_STRUCT_CONSTRUCTORS \
		-I $(VK_SDK)/x86_64/include/ \
		--precompile \
		-o vulkan.pcm \
		-std=c++23 \
		$(VK_SDK)/x86_64/include/vulkan/vulkan.cppm

clean:
	rm -f $(NAME) vulkan.pcm
