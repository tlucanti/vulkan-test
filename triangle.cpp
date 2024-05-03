
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <cstring>

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

#if CONFIG_VALIDATION_LAYERS
const char *validationLayers[] = {
	"VK_LAYER_KHRONOS_validation",
};
#else
const char *validationLayers[] = {};
#endif

class HelloTriangleApplication {
public:
	void run()
	{
		initGLFW();
		initVulkan();
		mainLoop();
		cleanup();
	}

private:
	GLFWwindow *window;
	VkInstance instance;

	void initGLFW(void)
	{
		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		this->window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "vk-test", nullptr, nullptr);
	}

	void initVulkan(void)
	{
		if (CONFIG_VALIDATION_LAYERS && not checkValidationLayerSupport()) {
			throw std::runtime_error("validation layers requested, but not available!");
		}

		createInstance();
		printExtentions();
	}

	void createInstance(void)
	{
		uint32_t glfwExtentionCount;
		const char **glfwExtentions;

		VkApplicationInfo appInfo = {
			.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
			.pNext = nullptr,
			.pApplicationName = "triangle test",
			.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
			.pEngineName = "no engine",
			.engineVersion = VK_MAKE_VERSION(1, 0, 0),
			.apiVersion = VK_API_VERSION_1_0
		};

		glfwExtentions = glfwGetRequiredInstanceExtensions(&glfwExtentionCount);

		VkInstanceCreateInfo createInfo = {
			.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.pApplicationInfo = &appInfo,
			.enabledLayerCount = ARRAY_SIZE(validationLayers),
			.ppEnabledLayerNames = validationLayers,
			.enabledExtensionCount = glfwExtentionCount,
			.ppEnabledExtensionNames = glfwExtentions,
		};

		if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
			throw std::runtime_error("failed to create instance!");
		}
	}

	void printExtentions(void)
	{
		uint32_t extensionCount;

		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
		auto extensions = new VkExtensionProperties[extensionCount];
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions);

		std::cout << "avaliable extensions:\n";
		for (uint32_t i = 0; i < extensionCount; i++) {
			std::cout << extensions[i].extensionName << '\n';
		}

		delete[] extensions;
	}

	bool checkValidationLayerSupport(void)
	{
		uint32_t layerCount;

		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
		auto layers = new VkLayerProperties[layerCount];
		vkEnumerateInstanceLayerProperties(&layerCount, layers);

		for (uint32_t layerName = 0; layerName < ARRAY_SIZE(validationLayers); layerName++) {
			bool found = false;

			for (uint32_t layerProperties = 0; layerProperties < layerCount; layerProperties++) {
				if (std::strcmp(validationLayers[layerName], layers[layerProperties].layerName) == 0) {
					found = true;
					break;
				}
			}

			if (not found) {
				return false;
			}
		}

		return true;
	}

	void mainLoop(void)
	{
		while (!glfwWindowShouldClose(this->window)) {
			glfwPollEvents();
		}

	}

	void cleanup(void)
	{
		glfwDestroyWindow(this->window);
		vkDestroyInstance(instance, nullptr);
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

