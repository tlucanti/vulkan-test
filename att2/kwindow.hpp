
#ifndef KWINDOW_HPP
#define KWINDOW_HPP

#include "vkcommon.hpp"

#include <utility>

struct kwindow_info {
	unsigned width    = 800;
	unsigned height   = 600;
	const char *title = "";
	bool resizable    = false;
};

struct kwindow {
	void create(const kwindow_info &create_info)
	{
		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, create_info.resizable);

		this->window = glfwCreateWindow(create_info.width,
						create_info.height,
						create_info.title,
						nullptr, nullptr);

		// glfwSetWindowUserPointer(window, this);
		// glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
	}

	void destroy(void)
	{
		glfwDestroyWindow(this->window);
		glfwTerminate();
	}

	bool should_close(void)
	{
		return glfwWindowShouldClose(this->window);
	}

	std::pair<unsigned, unsigned> get_framebuffer_size(void) const
	{
		int w, h;

		glfwGetFramebufferSize(this->window, &w, &h);
		return {w, h};
	}

	void wait_events(void) const
	{
		glfwWaitEvents();
	}

	void poll_events(void) const
	{
		glfwPollEvents();
	}

	GLFWwindow *__get_window(void) const
	{
		return this->window;
	}

private:
	GLFWwindow *window = nullptr;
};

#endif /* KWINDOW_HPP */
