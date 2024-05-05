
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <string>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

#if CONFIG_VALIDATION_LAYERS
std::vector<const char *> validationLayers = {
	"VK_LAYER_KHRONOS_validation",
};
#else
std::vector<const char *> validationLayers;
#endif

static inline void CALL_VK(VkResult result, std::string message)
{
	if (result != VK_SUCCESS) {
		message += " (errorcode: " + std::to_string(result) + ")";
		throw std::runtime_error(message);
	}
}

static VkResult CreateDebugUtilsMessengerEXT(
	VkInstance instance,
	const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
	const VkAllocationCallbacks *pAllocator,
	VkDebugUtilsMessengerEXT *pDebugMessenger)
{
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
		instance, "vkCreateDebugUtilsMessengerEXT");

	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	} else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

static void
DestroyDebugUtilsMessengerEXT(VkInstance instance,
			      VkDebugUtilsMessengerEXT debugMessenger,
			      const VkAllocationCallbacks *pAllocator)
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
		instance, "vkDestroyDebugUtilsMessengerEXT");

	if (func != nullptr) {
		func(instance, debugMessenger, pAllocator);
	} else {
		throw std::runtime_error("vkDestroyDebugUtilsMessengerEXT function is not present");
	}
}


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
	VkDebugUtilsMessengerEXT debugMessenger;
	VkPhysicalDevice physicalDevice;

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

		printExtentions();
		createInstance();
		setupDebugMessenger();
		printPhysicalDevices();
		pickPhysicalDevice();
	}

	std::vector<const char *> getRequieredExtensions()
	{
		uint32_t glfwExtensionCount;
		const char **glfwExtentions;

		glfwExtentions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
		std::vector<const char *> extensions(glfwExtentions, glfwExtentions + glfwExtensionCount);

		std::cout << "requiered glfw extensions:\n";
		for (const char *extension : extensions) {
			std::cout << '\t' << extension << '\n';
		}
		std::cout << '\n';

		if (CONFIG_VALIDATION_LAYERS) {
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		return extensions;
	}


	void createInstance(void)
	{
		VkApplicationInfo appInfo = {
			.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
			.pNext = nullptr,
			.pApplicationName = "triangle test",
			.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
			.pEngineName = "no engine",
			.engineVersion = VK_MAKE_VERSION(1, 0, 0),
			.apiVersion = VK_API_VERSION_1_0
		};

		auto requieredExtensions = getRequieredExtensions();
		VkInstanceCreateInfo createInfo = {
			.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.pApplicationInfo = &appInfo,
			.enabledLayerCount = 0,
			.ppEnabledLayerNames = nullptr,
			.enabledExtensionCount = (uint32_t)requieredExtensions.size(),
			.ppEnabledExtensionNames = requieredExtensions.data(),
		};

		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
		if (CONFIG_VALIDATION_LAYERS) {

			populateDebugMessengerCreateInfo(&debugCreateInfo);

			createInfo.enabledLayerCount = (uint32_t)validationLayers.size();
			createInfo.ppEnabledLayerNames = validationLayers.data();
			createInfo.pNext = &debugCreateInfo;
		}


		CALL_VK(vkCreateInstance(&createInfo, nullptr, &instance),
			"filed to create instance");
	}

	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT *createInfo)
	{
		uint32_t severity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
				    VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
				    VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		uint32_t type = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

		createInfo->sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo->pNext = nullptr;
		createInfo->flags = 0;
		createInfo->messageSeverity = severity;
		createInfo->messageType = type;
		createInfo->pfnUserCallback = debugCallback;
		createInfo->pUserData = nullptr;
	}


	void setupDebugMessenger(void)
	{
		VkDebugUtilsMessengerCreateInfoEXT createInfo;

		if (not CONFIG_VALIDATION_LAYERS) {
			return;
		}

		populateDebugMessengerCreateInfo(&createInfo);
		CALL_VK(CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger),
			"failed to set up debug messenger!");
	}

	bool isDeviceSutable(const VkPhysicalDevice &device)
	{
		VkPhysicalDeviceProperties deviceProperties;
		VkPhysicalDeviceFeatures deviceFeatures;

		vkGetPhysicalDeviceProperties(device, &deviceProperties);
		vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

		if ((deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ||
		     deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU ||
		     deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU) &&
		    deviceFeatures.geometryShader == VK_TRUE) {
			std::cout << "selecting physical device " << deviceProperties.deviceName << '\n';
			return true;
		} else {
			return false;
		}
	}

	void pickPhysicalDevice(void)
	{
		physicalDevice = VK_NULL_HANDLE;
		uint32_t deviceCount;

		CALL_VK(vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr),
			"failed to get physical devices count");

		if (deviceCount == 0) {
			throw std::runtime_error("no GPU devices with vulkan support");
		}

		std::vector<VkPhysicalDevice> devices(deviceCount);
		CALL_VK(vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data()),
			"failed to get physical devices list");

		for (const auto &device : devices) {
			if (isDeviceSutable(device)) {
				physicalDevice = device;
				break;
			}
		}

		if (physicalDevice == VK_NULL_HANDLE) {
			throw std::runtime_error("failed to find sutable GPU");
		}
	}

	void printExtentions(void)
	{
		uint32_t extensionCount;

		CALL_VK(vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr),
			"failed to get vulkan extentions count");
		std::vector<VkExtensionProperties> extensions(extensionCount);
		CALL_VK(vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data()),
			"faild to get vulkan extentions list");

		std::cout << "avaliable extensions:\n";
		for (const auto &extensionProperties : extensions) {
			std::cout << '\t' << extensionProperties.extensionName << '\n';
		}
		std::cout << '\n';
	}

	void printPhysicalDevices(void)
	{
		uint32_t deviceCount;
		CALL_VK(vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr),
			"failed to get physical device count");

		std::cout << "avaliable physical devices:\n";
		if (deviceCount == 0) {
			std::cout << "\tNO DEVICES AVALIABLE\n";
			return;
		}

		std::vector<VkPhysicalDevice> devices(deviceCount);
		CALL_VK(vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data()),
			"failed to get physical devices list");

		for (const auto &device : devices) {
			VkPhysicalDeviceProperties deviceProperties;

			vkGetPhysicalDeviceProperties(device, &deviceProperties);
			std::cout << '\t' << deviceProperties.deviceName << '\n';
		}
		std::cout << '\n';
	}

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData)
	{
		(void)messageSeverity;
		(void)messageType;
		(void)pUserData;

		switch (messageSeverity) {
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
				// std::cerr << "validation layer: ";
				// break;
				return VK_FALSE;
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
				std::cerr << "[VALIDATION INFO]: ";
				break;
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
				std::cerr << "[VALIDATION WARNING]: ";
				break;
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
				std::cerr << "[VALIDATION ERROR]: ";
				break;
			default:
				throw std::runtime_error("unknown message severity");
		}
		std::cerr << pCallbackData->pMessage << '\n';

		return VK_FALSE;
	}

	bool checkValidationLayerSupport(void)
	{
		uint32_t layerCount;

		CALL_VK(vkEnumerateInstanceLayerProperties(&layerCount, nullptr),
			"failed to get validation layer count");
		std::vector<VkLayerProperties> layers(layerCount);
		CALL_VK(vkEnumerateInstanceLayerProperties(&layerCount, layers.data()),
			"failed to get validation layer properties list");

		for (const char *layerName : validationLayers) {
			bool found = false;

			for (const auto &layerProperties : layers) {
				if (std::strcmp(layerName, layerProperties.layerName) == 0) {
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
		if (CONFIG_VALIDATION_LAYERS) {
			DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
		}
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

