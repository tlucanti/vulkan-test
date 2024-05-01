
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

class HelloTriangleApplication {
public:
	void run()
	{
		initVulkan();
		mainLoop();
		cleanup();
	}

private:
	GLFWwindow *window;

	void initVulkan()
	{
		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		this->window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "vk-test", nullptr, nullptr);

	}

	void mainLoop()
	{
		while (!glfwWindowShouldClose(this->window)) {
			glfwPollEvents();
		}

	}

	void cleanup()
	{
		glfwDestroyWindow(this->window);
		glfwTerminate();

	}
};

int main()
{
	HelloTriangleApplication app;

	try {
		app.run();
	} catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

