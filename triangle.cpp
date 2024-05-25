
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdexcept>

#include <set>
#include <string>
#include <vector>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

#if CONFIG_VALIDATION_LAYERS
const std::vector<const char *> validationLayers = {
	"VK_LAYER_KHRONOS_validation",
};
#else
const std::vector<const char *> validationLayers;
#endif

const std::vector<const char *> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};

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
	VkDevice device;
	VkQueue graphicsQueue;
	VkQueue presentQueue;
	VkSurfaceKHR surface;

	struct QueueFamilyIndices {
		uint32_t graphicsFamily;
		uint32_t presentFamily;
	};

	struct SwapChainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

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
		createSurface();
		printPhysicalDevices();
		pickPhysicalDevice();
		createLogicalDevice();
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
			"failed to create instance");
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

	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device)
	{
		uint32_t queueFamilyCount;
		std::vector<VkQueueFamilyProperties> queueFamilies;

		uint32_t i = 0;
		QueueFamilyIndices indices = {
			.graphicsFamily = (uint32_t)-1,
			.presentFamily = (uint32_t)-1,
		};

		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
		queueFamilies.resize(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		for (const auto &queueFamily : queueFamilies) {
			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				indices.graphicsFamily = i;
			}

			VkBool32 surfaceSupport = false;
			CALL_VK(vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &surfaceSupport),
				"failed to check device surface support");
			if (surfaceSupport) {
				indices.presentFamily = i;
			}

			i++;
		}

		return indices;
	}

	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device)
	{
		SwapChainSupportDetails details;
		uint32_t formatCount;
		uint32_t presentModeCount;

		CALL_VK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities),
			"failed to get surface capabilities");

		CALL_VK(vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr),
			"failed to get surface formats");
		details.formats.resize(formatCount);
		CALL_VK(vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data()),
			"failed to get surface formats");

		CALL_VK(vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr),
			"failed to get surface present modes");
		details.presentModes.resize(presentModeCount);
		CALL_VK(vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data()),
			"failed to get surface present modes");

		return details;
	}

	bool checkDeviceExtensionSupport(VkPhysicalDevice device)
	{
		uint32_t extensionCount;
		std::vector<VkExtensionProperties> avaliableExtensions;

		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
		avaliableExtensions.resize(extensionCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, avaliableExtensions.data());

		std::set<std::string> requieredExtensions(deviceExtensions.begin(), deviceExtensions.end());
		for (const auto &extension : avaliableExtensions) {
			requieredExtensions.erase(extension.extensionName);
		}

		return requieredExtensions.empty();
	}

	bool isDeviceSutable(const VkPhysicalDevice &device)
	{
		VkPhysicalDeviceProperties deviceProperties;
		VkPhysicalDeviceFeatures deviceFeatures;
		QueueFamilyIndices indices;
		SwapChainSupportDetails swapChainSupport;
		bool deviceOk = true;

		vkGetPhysicalDeviceProperties(device, &deviceProperties);
		vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
		deviceOk &=
			(deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ||
			 deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU ||
			 deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU) &&
			 deviceFeatures.geometryShader == VK_TRUE;

		indices = findQueueFamilies(device);
		deviceOk &= (indices.graphicsFamily != (uint32_t)-1);

		deviceOk &= checkDeviceExtensionSupport(device);

		swapChainSupport = querySwapChainSupport(device);
		deviceOk &= (not swapChainSupport.formats.empty() &&
			     not swapChainSupport.presentModes.empty());

		if (deviceOk) {
			std::cout << "selecting physical device " << deviceProperties.deviceName << '\n';
		}
		return deviceOk;
	}

	void pickPhysicalDevice(void)
	{
		physicalDevice = VK_NULL_HANDLE;
		uint32_t deviceCount;
		std::vector<VkPhysicalDevice> devices;

		CALL_VK(vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr),
			"failed to get physical devices count");

		if (deviceCount == 0) {
			throw std::runtime_error("no GPU devices with vulkan support");
		}

		devices.resize(deviceCount);
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

	void createLogicalDevice(void)
	{
		QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

		std::set<uint32_t> uniqueQueueFamilies = {
			indices.graphicsFamily,
			indices.presentFamily
		};

		for (uint32_t queueFamily : uniqueQueueFamilies) {
			float queuePrioriy = 1.0f;
			VkDeviceQueueCreateInfo queueCreateInfo = {
				.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.queueFamilyIndex = queueFamily,
				.queueCount = 1,
				.pQueuePriorities = &queuePrioriy,
			};

			queueCreateInfos.push_back(queueCreateInfo);
		}

		VkPhysicalDeviceFeatures deviceFeatures = {};

		VkDeviceCreateInfo createInfo = {
			.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.queueCreateInfoCount = (uint32_t)queueCreateInfos.size(),
			.pQueueCreateInfos = queueCreateInfos.data(),
			.enabledLayerCount = 0,
			.ppEnabledLayerNames = nullptr,
			.enabledExtensionCount = (uint32_t)deviceExtensions.size(),
			.ppEnabledExtensionNames = deviceExtensions.data(),
			.pEnabledFeatures = &deviceFeatures,
		};

		if (CONFIG_VALIDATION_LAYERS) {
			createInfo.enabledLayerCount = validationLayers.size();
			createInfo.ppEnabledLayerNames = validationLayers.data();
		}

		CALL_VK(vkCreateDevice(physicalDevice, &createInfo, nullptr, &device),
			"failed to create logical device");

		vkGetDeviceQueue(device, indices.graphicsFamily, 0, &graphicsQueue);
		vkGetDeviceQueue(device, indices.presentFamily, 0, &presentQueue);
	}

	void createSurface(void)
	{
		CALL_VK(glfwCreateWindowSurface(instance, window, nullptr, &surface),
			"failed to create window surface");
	}

	void printExtentions(void)
	{
		uint32_t extensionCount;
		std::vector<VkExtensionProperties> extensions;

		CALL_VK(vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr),
			"failed to get vulkan extentions count");
		extensions.resize(extensionCount);
		CALL_VK(vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data()),
			"faild to get vulkan extentions list");

		std::cout << "avaliable extensions:\n";
		for (const auto &extensionProperties : extensions) {
			std::cout << '\t' << extensionProperties.extensionName << '\n';
		}
		std::cout << '\n';
	}

	void printDeviceExtensinos(VkPhysicalDevice device)
	{
		uint32_t extensionCount;
		std::vector<VkExtensionProperties> avaliableExtensions;

		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
		avaliableExtensions.resize(extensionCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, avaliableExtensions.data());

		for (const auto &extension : avaliableExtensions) {
			std::cout << "\t\t" << extension.extensionName << '\n';
		}
	}

	void printPhysicalDevices(void)
	{
		uint32_t deviceCount;
		std::vector<VkPhysicalDevice> devices;

		CALL_VK(vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr),
			"failed to get physical device count");

		std::cout << "avaliable physical devices:\n";
		if (deviceCount == 0) {
			std::cout << "\tNO DEVICES AVALIABLE\n";
			return;
		}

		devices.resize(deviceCount);
		CALL_VK(vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data()),
			"failed to get physical devices list");

		for (const auto &device : devices) {
			VkPhysicalDeviceProperties deviceProperties;

			vkGetPhysicalDeviceProperties(device, &deviceProperties);
			std::cout << '\t' << deviceProperties.deviceName << '\n';
			std::cout << "\tdevice extensions:\n";

			printDeviceExtensinos(device);
			std::cout << '\n';
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
		std::vector<VkLayerProperties> layers;

		CALL_VK(vkEnumerateInstanceLayerProperties(&layerCount, nullptr),
			"failed to get validation layer count");
		layers.resize(layerCount);
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
		vkDestroyDevice(device, nullptr);
		vkDestroySurfaceKHR(instance, surface, nullptr);
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

