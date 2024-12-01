
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include <stb_image.h>

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdexcept>

#include <array>
#include <chrono>
#include <fstream>
#include <set>
#include <string>
#include <vector>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

#ifndef CONFIG_VALIDATION_LAYERS
# define CONFIG_VALIDATION_LAYERS true
#endif

#define MAX_FRAMES_IN_FLIGHT 2

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

__attribute__((__noreturn__))
static inline void PANIC_VK(const std::string &message)
{
	throw std::runtime_error(message);
}

static inline void CALL_VK(VkResult result, std::string message)
{
	if (result != VK_SUCCESS) {
		message += " (errorcode: " + std::to_string(result) + ")";
		PANIC_VK(message);
	}
}

namespace Color {
	struct ColorImpl {
		const char *color;
	};

	ColorImpl blue = { "" };
	ColorImpl yellow = { "" };
	ColorImpl red = { "" };
	ColorImpl reset = { "" };

	#define istty(x) false
	std::ostream &operator <<(std::ostream &out, const ColorImpl &c)
	{
		if (istty(out)) {
			out << c.color;
		}
		return out;
	}
};

static VkResult CreateDebugUtilsMessengerEXT(
	VkInstance instance,
	const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
	const VkAllocationCallbacks *pAllocator,
	VkDebugUtilsMessengerEXT *pDebugMessenger)
{
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)
		vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

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
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)
		vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");

	if (func != nullptr) {
		func(instance, debugMessenger, pAllocator);
	} else {
		PANIC_VK("vkDestroyDebugUtilsMessengerEXT function is not present");
	}
}

static std::vector<unsigned char> readFile(const std::string &fileName)
{
	std::ifstream file(fileName, std::ios::ate | std::ios::binary);

	if (!file.is_open()) {
		PANIC_VK("failed to open file: " + fileName);
	}

	std::vector<unsigned char> buffer(file.tellg());

	file.seekg(0);
	file.read((char *)buffer.data(), buffer.size());
	return buffer;
}

struct Vertex {
	alignas(16) glm::vec3 pos;
	alignas(16) glm::vec3 color;
	alignas(16) glm::vec2 texCoord;

	Vertex(glm::vec3 pos, glm::vec3 color, glm::vec2 texCoord)
		: pos(pos), color(color), texCoord(texCoord)
	{}

	static VkVertexInputBindingDescription getBindingDescription(void)
	{
		VkVertexInputBindingDescription bindingDescription = {
			.binding = 0,
			.stride = sizeof(Vertex),
			.inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
		};

		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions(void)
	{
		VkVertexInputAttributeDescription posDescription = {
			.location = 0,
			.binding = 0,
			.format = VK_FORMAT_R32G32B32_SFLOAT,
			.offset = offsetof(Vertex, pos),
		};

		VkVertexInputAttributeDescription colorDescription = {
			.location = 1,
			.binding = 0,
			.format = VK_FORMAT_R32G32B32_SFLOAT,
			.offset = offsetof(Vertex, color),
		};

		VkVertexInputAttributeDescription texCoordDescription = {
			.location = 2,
			.binding = 0,
			.format = VK_FORMAT_R32G32_SFLOAT,
			.offset = offsetof(Vertex, texCoord),
		};

		std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions = {
			posDescription,
			colorDescription,
			texCoordDescription,
		};

		return attributeDescriptions;
	}
};

struct UniformBufferObject {
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;
};

const std::vector<Vertex> vertices = {
	Vertex(glm::vec3(-0.5f, -0.5f, 0), glm::vec3(1, 0, 0), glm::vec2(1, 0)),
	Vertex(glm::vec3( 0.5f, -0.5f, 0), glm::vec3(0, 1, 0), glm::vec2(0.5, 0)),
	Vertex(glm::vec3( 0.5f,  0.5f, 0), glm::vec3(1, 1, 1), glm::vec2(0.5, 1)),
	Vertex(glm::vec3(-0.5f,  0.5f, 0), glm::vec3(0, 0, 1), glm::vec2(1, 1)),

	Vertex(glm::vec3(-0.5f, -0.5f, 0), glm::vec3(1, 0, 0), glm::vec2(0, 0)),
	Vertex(glm::vec3( 0.5f, -0.5f, 0), glm::vec3(0, 1, 0), glm::vec2(0.5, 0)),
	Vertex(glm::vec3( 0.5f, -0.5f, -1), glm::vec3(0, 1, 0), glm::vec2(0.5, 1)),
	Vertex(glm::vec3(-0.5f, -0.5f, -1), glm::vec3(1, 0, 0), glm::vec2(0, 1)),

	Vertex(glm::vec3(0.5f, -0.5f, 0), glm::vec3(1, 0, 0), glm::vec2(0, 0)),
	Vertex(glm::vec3(0.5f, 0.5f, 0), glm::vec3(0, 1, 0), glm::vec2(0.5, 0)),
	Vertex(glm::vec3(0.5f, 0.5f, -1), glm::vec3(0, 1, 0), glm::vec2(0.5, 1)),
	Vertex(glm::vec3(0.5f, -0.5f, -1), glm::vec3(1, 0, 0), glm::vec2(0, 1)),

	Vertex(glm::vec3(-0.5f, 0.5f, 0), glm::vec3(1, 0, 0), glm::vec2(0, 0)),
	Vertex(glm::vec3( 0.5f, 0.5f, 0), glm::vec3(0, 1, 0), glm::vec2(0.5, 0)),
	Vertex(glm::vec3( 0.5f, 0.5f, -1), glm::vec3(0, 1, 0), glm::vec2(0.5, 1)),
	Vertex(glm::vec3(-0.5f, 0.5f, -1), glm::vec3(1, 0, 0), glm::vec2(0, 1)),

	Vertex(glm::vec3(-0.5f, -0.5f, 0), glm::vec3(1, 0, 0), glm::vec2(0, 0)),
	Vertex(glm::vec3(-0.5f, 0.5f, 0), glm::vec3(0, 1, 0), glm::vec2(0.5, 0)),
	Vertex(glm::vec3(-0.5f, 0.5f, -1), glm::vec3(0, 1, 0), glm::vec2(0.5, 1)),
	Vertex(glm::vec3(-0.5f, -0.5f, -1), glm::vec3(1, 0, 0), glm::vec2(0, 1)),


	Vertex(glm::vec3(-0.5f, -0.5f, -1), glm::vec3(1, 0, 0), glm::vec2(0, 0.5)),
	Vertex(glm::vec3( 0.0f, -0.5f, -1), glm::vec3(0, 1, 0), glm::vec2(0, 1)),
	Vertex(glm::vec3( 0.0f,  0.5f, -1), glm::vec3(1, 1, 1), glm::vec2(0.5, 1)),
	Vertex(glm::vec3(-0.5f,  0.5f, -1), glm::vec3(0, 0, 1), glm::vec2(0.5, 0.5)),

	Vertex(glm::vec3(0.0f, -0.5f, -1), glm::vec3(1, 0, 0), glm::vec2(0, 1)),
	Vertex(glm::vec3(0.5f, -0.5f, -1), glm::vec3(0, 1, 0), glm::vec2(0, 0.5)),
	Vertex(glm::vec3(0.5f,  0.5f, -1), glm::vec3(1, 1, 1), glm::vec2(0.5, 0.5)),
	Vertex(glm::vec3(0.0f,  0.5f, -1), glm::vec3(0, 0, 1), glm::vec2(0.5, 1)),
};

const std::vector<uint16_t> indices = {
	0, 1, 2, 2, 3, 0,
	6, 5, 4, 4, 7, 6,
	10, 9, 8, 8, 11, 10,
	12, 13, 14, 14, 15, 12,
	16, 17, 18, 18, 19, 16,
	22, 21, 20, 20, 23, 22,
	26, 25, 24, 24, 27, 26,


	// 1, 5, 2, 2, 5, 6,
	// 2, 6, 7, 3, 2, 7,
	// 0, 7, 4, 0, 3, 7,
	// 6, 5, 4, 6, 4, 7,
};

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
	VkSwapchainKHR swapChain;
	std::vector<VkImage> swapChainImages;
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;
	std::vector<VkImageView> swapChainImageViews;
	VkImage textureImage;
	VkDeviceMemory textureImageMemory;
	VkImageView textureImageView;
	VkImage depthImage;
	VkDeviceMemory depthImageMemory;
	VkImageView depthImageView;
	VkSampler textureSampler;
	VkRenderPass renderPass;
	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorPool descriptorPool;
	std::vector<VkDescriptorSet> descriptorSets;
	VkPipelineLayout pipelineLayout;
	VkPipeline graphicsPipeline;
	std::vector<VkFramebuffer> swapChainFramebuffers;
	VkCommandPool commandPool;
	VkBuffer vertexBuffer;
	VkBuffer indexBuffer;
	std::vector<VkBuffer> uniformBuffers;
	VkDeviceMemory vertexBufferMemory;
	VkDeviceMemory indexBufferMemory;
	std::vector<VkDeviceMemory> uniformBuffersMemory;
	std::vector<void *> uniformBuffersMappings;
	std::vector<VkCommandBuffer> commandBuffers;
	std::vector<VkSemaphore> imageAvaliableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;
	bool frameBufferResized;
	uint32_t currentFrame;

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
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

		window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "vulkan", nullptr, nullptr);
		glfwSetWindowUserPointer(window, this);
		glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
	}

	static void framebufferResizeCallback(GLFWwindow *window, int width, int height)
	{
		(void)width;
		(void)height;
		auto app = reinterpret_cast<HelloTriangleApplication *>(glfwGetWindowUserPointer(window));
		app->frameBufferResized = true;
	}

	void initVulkan(void)
	{
		if (CONFIG_VALIDATION_LAYERS && not checkValidationLayerSupport()) {
			PANIC_VK("validation layers requested, but not available!");
		}
		frameBufferResized = false;

		printExtentions();
		createInstance();
		setupDebugMessenger();
		createSurface();
		printPhysicalDevices();
		pickPhysicalDevice();
		createLogicalDevice();
		createSwapChain();
		createImageViews();
		createRenderPass();
		createDescriptorSetLayout();
		createGraphicsPipeline();
		createCommandPool();
		createDepthResources();
		createFramebuffers();
		createTextureImage();
		createTextureImageView();
		createTextureSampler();
		printMemoryTypes();
		createVertexBuffer();
		createIndexBuffer();
		createUniformBuffers();
		createDescriptorPool();
		createDescriptorSets();
		createCommandBuffers();
		createSyncObjects();
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

		CALL_VK(vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr),
			"failed to get device extensions");
		avaliableExtensions.resize(extensionCount);
		CALL_VK(vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, avaliableExtensions.data()),
			"failed to get device extensions");

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

		deviceOk &= deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ||
			    deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU ||
			    deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU;

		deviceOk &= deviceFeatures.geometryShader == VK_TRUE;
		deviceOk &= deviceFeatures.samplerAnisotropy == VK_TRUE;

		indices = findQueueFamilies(device);
		deviceOk &= (indices.graphicsFamily != (uint32_t)-1);

		deviceOk &= checkDeviceExtensionSupport(device);

		swapChainSupport = querySwapChainSupport(device);
		deviceOk &= (not swapChainSupport.formats.empty() &&
			     not swapChainSupport.presentModes.empty());

		if (deviceOk) {
			std::cout << "selecting physical device: " << deviceProperties.deviceName << '\n';
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
			PANIC_VK("no GPU devices with vulkan support");
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
			PANIC_VK("failed to find sutable GPU");
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
		deviceFeatures.samplerAnisotropy = VK_TRUE;

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

	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &avaliableFormats)
	{
		for (const auto &format : avaliableFormats) {
			if (format.format == VK_FORMAT_B8G8R8_SRGB &&
			    format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			{
				return format;
			}
		}

		return avaliableFormats.at(0);
	}

	VkPresentModeKHR choosePresentMode(const std::vector<VkPresentModeKHR> &avaliablePresentModes)
	{
		for (const auto &presentMode : avaliablePresentModes) {
			if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
				std::cout << "using present mode: " << "VK_PRESENT_MODE_MAILBOX_KHR\n";
				return presentMode;
			}
		}

		std::cout << "using present mode: " << "VK_PRESENT_MODE_FIFO_KHR\n";
		return VK_PRESENT_MODE_FIFO_KHR;
	}

	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities)
	{
		uint32_t width, height;
		VkExtent2D extent;

		glfwGetFramebufferSize(window, (int *)&width, (int *)&height);

		width = std::max(width, capabilities.minImageExtent.width);
		width = std::min(width, capabilities.maxImageExtent.width);
		height = std::max(height, capabilities.minImageExtent.height);
		height = std::min(height, capabilities.maxImageExtent.height);

		extent.width = width;
		extent.height = height;

		return extent;
	}

	void createSwapChain()
	{
		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

		VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
		VkPresentModeKHR presentMode = choosePresentMode(swapChainSupport.presentModes);
		VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

		uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
		if (swapChainSupport.capabilities.maxImageCount != 0 &&
		    imageCount > swapChainSupport.capabilities.maxImageCount) {
			imageCount = swapChainSupport.capabilities.minImageCount;
		}
		std::cout << "using " << imageCount << " images in swap chain\n";

		VkSwapchainCreateInfoKHR createInfo = {
			.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
			.pNext = nullptr,
			.flags = 0,
			.surface = surface,
			.minImageCount = imageCount,
			.imageFormat = surfaceFormat.format,
			.imageColorSpace = surfaceFormat.colorSpace,
			.imageExtent = extent,
			.imageArrayLayers = 1,
			.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
			.queueFamilyIndexCount = 0,
			.pQueueFamilyIndices = nullptr,
			.preTransform = swapChainSupport.capabilities.currentTransform,
			.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
			.presentMode = presentMode,
			.clipped = VK_TRUE,
			.oldSwapchain = VK_NULL_HANDLE,
		};

		QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
		uint32_t queueFamilyIndices[] = { indices.graphicsFamily, indices.presentFamily };
		if (indices.graphicsFamily != indices.presentFamily) {
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		}

		CALL_VK(vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain),
			"failed to create swap chain");

		CALL_VK(vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr),
			"failed to get image handles");
		swapChainImages.resize(imageCount);
		CALL_VK(vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data()),
			"failed to get image handles");

		swapChainImageFormat = surfaceFormat.format;
		swapChainExtent = extent;
	}

	void cleanupSwapChain(void)
	{
		vkDestroyImageView(device, depthImageView, nullptr);
		vkDestroyImage(device, depthImage, nullptr);
		vkFreeMemory(device, depthImageMemory, nullptr);

		for (auto &framebuffer : swapChainFramebuffers) {
			vkDestroyFramebuffer(device, framebuffer, nullptr);
		}

		for (auto &imageView : swapChainImageViews) {
			vkDestroyImageView(device, imageView, nullptr);
		}

		vkDestroySwapchainKHR(device, swapChain, nullptr);
	}

	void recreateSwapChain(void)
	{
		int width, height;

		glfwGetFramebufferSize(window, &width, &height);
		while (width == 0 || height == 0) {
			std::cout << "sleeping\n";
			glfwWaitEvents();
			glfwGetFramebufferSize(window, &width, &height);
		}

		vkDeviceWaitIdle(device);

		cleanupSwapChain();

		createSwapChain();
		createImageViews();
		createDepthResources();
		createFramebuffers();
		// TODO: check compatibility with render pass and recreate it if needed
	}

	VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
	{
		VkImageViewCreateInfo imageViewCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.image = image,
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = format,
			.components = {
				.r = VK_COMPONENT_SWIZZLE_IDENTITY,
				.g = VK_COMPONENT_SWIZZLE_IDENTITY,
				.b = VK_COMPONENT_SWIZZLE_IDENTITY,
				.a = VK_COMPONENT_SWIZZLE_IDENTITY,
			},
			.subresourceRange = {
				.aspectMask = aspectFlags,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1,
			},
		};

		VkImageView imageView;
		CALL_VK(vkCreateImageView(device, &imageViewCreateInfo, nullptr, &imageView),
			"failed to create image view");

		return imageView;
	}

	void createImageViews(void)
	{
		swapChainImageViews.resize(swapChainImages.size());

		for (int i = 0; i < (int)swapChainImages.size(); i++) {
			swapChainImageViews[i] =
				createImageView(swapChainImages[i], swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
		}
	}

	void createTextureImageView(void)
	{
		textureImageView = createImageView(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
	}

	void createTextureSampler(void)
	{
		VkPhysicalDeviceProperties properties;

		vkGetPhysicalDeviceProperties(physicalDevice, &properties);

		VkSamplerCreateInfo samplerCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.magFilter = VK_FILTER_LINEAR,
			.minFilter = VK_FILTER_LINEAR,
			.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
			.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
			.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
			.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
			.mipLodBias = 0,
			.anisotropyEnable = VK_TRUE,
			.maxAnisotropy = properties.limits.maxSamplerAnisotropy,
			.compareEnable = VK_FALSE,
			.compareOp = VK_COMPARE_OP_ALWAYS,
			.minLod = 0,
			.maxLod = 0,
			.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
			.unnormalizedCoordinates = VK_FALSE,
		};

		CALL_VK(vkCreateSampler(device, &samplerCreateInfo, nullptr, &textureSampler),
			"failed to create texture smapler");
	}

	void createRenderPass(void)
	{
		VkAttachmentDescription colorAttachment = {
			.flags = 0,
			.format = swapChainImageFormat,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		};

		VkAttachmentDescription depthAttachment = {
			.flags = 0,
			.format = findDepthFormat(),
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		};

		std::array<VkAttachmentDescription, 2> attachments = {
			colorAttachment,
			depthAttachment,
		};

		VkAttachmentReference colorAttachmentRef = {
			.attachment = 0,
			.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		};

		VkAttachmentReference depthAttachmentRef = {
			.attachment = 1,
			.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		};

		VkSubpassDescription subpass = {
			.flags = 0,
			.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
			.inputAttachmentCount = 0,
			.pInputAttachments = nullptr,
			.colorAttachmentCount = 1,
			.pColorAttachments = &colorAttachmentRef,
			.pResolveAttachments = nullptr,
			.pDepthStencilAttachment = &depthAttachmentRef,
			.preserveAttachmentCount = 0,
			.pPreserveAttachments = nullptr,
		};

		VkSubpassDependency dependency = {
			.srcSubpass = VK_SUBPASS_EXTERNAL,
			.dstSubpass = 0,
			.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
					VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
					VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
			.srcAccessMask = 0,
			.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
					 VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			.dependencyFlags = 0,
		};

		VkRenderPassCreateInfo renderPassCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.attachmentCount = attachments.size(),
			.pAttachments = attachments.data(),
			.subpassCount = 1,
			.pSubpasses = &subpass,
			.dependencyCount = 1,
			.pDependencies = &dependency,
		};

		CALL_VK(vkCreateRenderPass(device, &renderPassCreateInfo, nullptr, &renderPass),
			"failed to create render pass");
	}

	VkShaderModule createShaderModule(const std::vector<unsigned char> &code)
	{
		VkShaderModule shaderModule;

		VkShaderModuleCreateInfo createInfo = {
			.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.codeSize = code.size(),
			.pCode = (uint32_t *)code.data(),
		};

		CALL_VK(vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule),
			"failed to create shader module");

		return shaderModule;
	}

	void createDescriptorSetLayout(void)
	{
		VkDescriptorSetLayoutBinding uboLayoutBinding = {
			.binding = 0,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
			.pImmutableSamplers = nullptr,
		};

		VkDescriptorSetLayoutBinding samplerLayoutBinding = {
			.binding = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
			.pImmutableSamplers = nullptr,
		};

		std::array<VkDescriptorSetLayoutBinding, 2> bindings = {
			uboLayoutBinding,
			samplerLayoutBinding,
		};

		VkDescriptorSetLayoutCreateInfo layoutCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.bindingCount = bindings.size(),
			.pBindings = bindings.data(),
		};

		CALL_VK(vkCreateDescriptorSetLayout(device, &layoutCreateInfo, nullptr, &descriptorSetLayout),
			"failed to create descriptor set layout");

	}

	void createGraphicsPipeline(void)
	{
		auto vertShaderCode = readFile("vert.spv");
		auto fragShaderCode = readFile("frag.spv");

		std::cout << "loaded vertex shader code of size " << vertShaderCode.size() << "bytes\n";
		std::cout << "loaded fragment shader code of size " << fragShaderCode.size() << "bytes\n";

		VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
		VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

		VkVertexInputBindingDescription bindingDescription = Vertex::getBindingDescription();
		auto attributeDescriptions = Vertex::getAttributeDescriptions();

		VkPipelineShaderStageCreateInfo vertShaderStageCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.stage = VK_SHADER_STAGE_VERTEX_BIT,
			.module = vertShaderModule,
			.pName = "main",
			.pSpecializationInfo = nullptr,
		};

		VkPipelineShaderStageCreateInfo fragShaderStageCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
			.module = fragShaderModule,
			.pName = "main",
			.pSpecializationInfo = nullptr,
		};

		VkPipelineShaderStageCreateInfo shaderStages[] = {
			vertShaderStageCreateInfo, fragShaderStageCreateInfo
		};

		std::vector<VkDynamicState> dynamicStates = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR,
		};

		VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.dynamicStateCount = (uint32_t)dynamicStates.size(),
			.pDynamicStates = dynamicStates.data(),
		};

		VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.vertexBindingDescriptionCount = 1,
			.pVertexBindingDescriptions = &bindingDescription,
			.vertexAttributeDescriptionCount = attributeDescriptions.size(),
			.pVertexAttributeDescriptions = attributeDescriptions.data(),
		};

		VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
			.primitiveRestartEnable = VK_FALSE,
		};

		VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.viewportCount = 1,
			.pViewports = nullptr,
			.scissorCount = 1,
			.pScissors = nullptr,
		};

		VkPipelineRasterizationStateCreateInfo rasterizer = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.depthClampEnable = VK_FALSE,
			.rasterizerDiscardEnable = VK_FALSE,
			.polygonMode = VK_POLYGON_MODE_FILL,
			.cullMode = VK_CULL_MODE_BACK_BIT,
			.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
			.depthBiasEnable = VK_FALSE,
			.depthBiasConstantFactor = 0,
			.depthBiasClamp = 0,
			.depthBiasSlopeFactor = 0,
			.lineWidth = 1,
		};

		VkPipelineMultisampleStateCreateInfo multisampling = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
			.sampleShadingEnable = VK_FALSE,
			.minSampleShading = 1,
			.pSampleMask = nullptr,
			.alphaToCoverageEnable = VK_FALSE,
			.alphaToOneEnable = VK_FALSE,
		};

		VkPipelineDepthStencilStateCreateInfo depthStencil = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.depthTestEnable = VK_TRUE,
			.depthWriteEnable = VK_TRUE,
			.depthCompareOp = VK_COMPARE_OP_LESS,
			.depthBoundsTestEnable = VK_FALSE,
			.stencilTestEnable = VK_FALSE,
			.front = {},
			.back = {},
			.minDepthBounds = 0.0f,
			.maxDepthBounds = 1.0f,
		};

		VkPipelineColorBlendAttachmentState colorBlendAttachment = {
			.blendEnable = VK_FALSE,
			.srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
			.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
			.colorBlendOp = VK_BLEND_OP_ADD,
			.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
			.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
			.alphaBlendOp = VK_BLEND_OP_ADD,
			.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
					  VK_COLOR_COMPONENT_G_BIT |
					  VK_COLOR_COMPONENT_B_BIT |
					  VK_COLOR_COMPONENT_A_BIT,
		};

		VkPipelineColorBlendStateCreateInfo colorBlending = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.logicOpEnable = VK_FALSE,
			.logicOp = VK_LOGIC_OP_COPY,
			.attachmentCount = 1,
			.pAttachments = &colorBlendAttachment,
			.blendConstants = { 0, 0, 0, 0 },
		};

		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.setLayoutCount = 1,
			.pSetLayouts = &descriptorSetLayout,
			.pushConstantRangeCount = 0,
			.pPushConstantRanges = nullptr,
		};

		CALL_VK(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout),
			"failed to create pipeline layout");

		VkGraphicsPipelineCreateInfo pipelineCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.stageCount = 2,
			.pStages = shaderStages,
			.pVertexInputState = &vertexInputStateCreateInfo,
			.pInputAssemblyState = &inputAssemblyCreateInfo,
			.pTessellationState = nullptr,
			.pViewportState = &viewportStateCreateInfo,
			.pRasterizationState = &rasterizer,
			.pMultisampleState = &multisampling,
			.pDepthStencilState = &depthStencil,
			.pColorBlendState = &colorBlending,
			.pDynamicState = &dynamicStateCreateInfo,
			.layout = pipelineLayout,
			.renderPass = renderPass,
			.subpass = 0,
			.basePipelineHandle = VK_NULL_HANDLE,
			.basePipelineIndex = -1,
		};

		CALL_VK(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &graphicsPipeline),
			"failed to create graphics pipeline");

		vkDestroyShaderModule(device, vertShaderModule, nullptr);
		vkDestroyShaderModule(device, fragShaderModule, nullptr);

		std::cout << "created pipeline with " << pipelineCreateInfo.stageCount << " stages\n";
	}

	void createFramebuffers(void)
	{
		swapChainFramebuffers.resize(swapChainImageViews.size());

		for (int i = 0; i < (int)swapChainImageViews.size(); i++) {
			std::array<VkImageView, 2> attachments = {
				swapChainImageViews.at(i),
				depthImageView,
			};

			VkFramebufferCreateInfo framebufferCreateInfo = {
				.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.renderPass = renderPass,
				.attachmentCount = attachments.size(),
				.pAttachments = attachments.data(),
				.width = swapChainExtent.width,
				.height = swapChainExtent.height,
				.layers = 1,
			};

			CALL_VK(vkCreateFramebuffer(device, &framebufferCreateInfo, nullptr, &swapChainFramebuffers.at(i)),
				"failed to create framebuffer");
		}
		std::cout << "created " << swapChainFramebuffers.size() << " frame buffers\n";
	}

	void createTextureImage(void)
	{
		int texWidth, texHeight, texChannels;
		stbi_uc *pixels;
		VkDeviceSize imageSize;

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		void *gpuMemory;

		pixels = stbi_load("textures/texture.png",&texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
		imageSize = texWidth * texHeight * 4;

		if (pixels == nullptr) {
			PANIC_VK("failed to load texture image");
		}

		createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			     &stagingBuffer, &stagingBufferMemory, "staging texture buffer");

		vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &gpuMemory);
		memcpy(gpuMemory, pixels, imageSize);
		vkUnmapMemory(device, stagingBufferMemory);

		stbi_image_free(pixels);

		createImage(texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
			    VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &textureImage, &textureImageMemory,
			    "texture image");
		transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB,
				      VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		copyBufferToImage(stagingBuffer, textureImage, texWidth, texHeight);
		transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB,
				      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		vkDestroyBuffer(device, stagingBuffer, nullptr);
		vkFreeMemory(device, stagingBufferMemory, nullptr);
	}

	void createDepthResources(void)
	{
		VkFormat depthFormat = findDepthFormat();

		createImage(swapChainExtent.width, swapChainExtent.height, depthFormat,
			    VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
			    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &depthImage, &depthImageMemory,
			    "depth buffer");

		depthImageView = createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);

		transitionImageLayout(depthImage, depthFormat,
				      VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
	}

	VkFormat findSupportedFormat(const std::vector<VkFormat> &candidates,
				     VkImageTiling tiling, VkFormatFeatureFlags features)
	{
		for (VkFormat format : candidates) {
			VkFormatProperties props;

			vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

			if (tiling == VK_IMAGE_TILING_LINEAR) {
				if ((props.linearTilingFeatures & features) == features) {
					return format;
				}
			} else if (tiling == VK_IMAGE_TILING_OPTIMAL){
				if ((props.optimalTilingFeatures & features) == features) {
					return format;
				}
			} else {
				PANIC_VK("invalid tiling");
			}
		}

		PANIC_VK("failed to find supported format");
	}

	VkFormat findDepthFormat(void)
	{
		return findSupportedFormat(
			{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
			VK_IMAGE_TILING_OPTIMAL,
			VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
		);
	}

	bool hasStencilComponent(VkFormat format)
	{
		return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
	}

	void createCommandPool(void)
	{
		QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

		VkCommandPoolCreateInfo poolCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.pNext = nullptr,
			.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
			.queueFamilyIndex = queueFamilyIndices.graphicsFamily,
		};

		CALL_VK(vkCreateCommandPool(device, &poolCreateInfo, nullptr, &commandPool),
			"failed to create command pool");
	}

	void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
	{
		VkCommandBufferBeginInfo beginInfo = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.pNext = nullptr,
			.flags = 0,
			.pInheritanceInfo = nullptr,
		};

		CALL_VK(vkBeginCommandBuffer(commandBuffer, &beginInfo),
			"failed to begin recording command buffer");

		std::array<VkClearValue, 2> clearValues;
		clearValues.at(0).color = {{ 0.1f, 0.1f, 0.1f, 1 }};
		clearValues.at(1).depthStencil = { 1, 0 };

		VkRenderPassBeginInfo renderPassInfo = {
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			.pNext = nullptr,
			.renderPass = renderPass,
			.framebuffer = swapChainFramebuffers[imageIndex],
			.renderArea = {
				.offset = { 0, 0 },
				.extent = swapChainExtent,
			},
			.clearValueCount = clearValues.size(),
			.pClearValues = clearValues.data(),
		};

		VkViewport viewport = {
			.x = 0,
			.y = 0,
			.width = (float)swapChainExtent.width,
			.height = (float)swapChainExtent.height,
			.minDepth = 0,
			.maxDepth = 1,
		};

		VkRect2D scissor = {
			.offset = { 0, 0 },
			.extent = swapChainExtent,
		};

		VkBuffer vertexBuffers[] = { vertexBuffer };
		VkDeviceSize offsets[] = { 0 };

		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT16);

		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
					0, 1, &descriptorSets[currentFrame], 0, nullptr);

		vkCmdDrawIndexed(commandBuffer, indices.size(), 1, 0, 0, 0);

		vkCmdEndRenderPass(commandBuffer);

		CALL_VK(vkEndCommandBuffer(commandBuffer),
			"failed to record command buffer");

	}

	void printMemoryTypes(void)
	{
		VkPhysicalDeviceMemoryProperties memProperties;

		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

		std::cout << "avaliable memory types\n";
		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
			std::cout << "\tflags: " << memProperties.memoryTypes[i].propertyFlags << '\n';
		}
		std::cout << '\n';
	}

	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
	{
		VkPhysicalDeviceMemoryProperties memProperties;

		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
			if ((typeFilter & (1 << i)) &&
			    (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
				return i;
			}
		}

		PANIC_VK("failed to find suitable memory type");
	}

	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
			  VkMemoryPropertyFlags properties, VkBuffer *pBuffer,
			  VkDeviceMemory *pBufferMemory, const char *name)
	{
		VkMemoryRequirements memRequirements;
		uint32_t typeIndex;

		VkBufferCreateInfo bufferCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.size = size,
			.usage = usage,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
			.queueFamilyIndexCount = 0,
			.pQueueFamilyIndices = nullptr,
		};

		CALL_VK(vkCreateBuffer(device, &bufferCreateInfo, nullptr, pBuffer),
			std::string("failed to create ") + name);

		vkGetBufferMemoryRequirements(device, *pBuffer, &memRequirements);
		typeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

		VkMemoryAllocateInfo allocInfo = {
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			.pNext = nullptr,
			.allocationSize = memRequirements.size,
			.memoryTypeIndex = typeIndex,
		};

		CALL_VK(vkAllocateMemory(device, &allocInfo, nullptr, pBufferMemory),
			std::string("failed to allocate ") + name + " memory");

		CALL_VK(vkBindBufferMemory(device, *pBuffer, *pBufferMemory, 0),
			std::string("failed to bind ") + name + " memory");
	}

	void createImage(uint32_t width, uint32_t height, VkFormat format,
			 VkImageTiling tiling, VkImageUsageFlags usage,
			 VkMemoryPropertyFlags properties, VkImage *pImage,
			 VkDeviceMemory *pImageMemory, const char *name)
	{
		VkMemoryRequirements memRequirements;

		VkImageCreateInfo imageCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.imageType = VK_IMAGE_TYPE_2D,
			.format = format,
			.extent = {
				.width = width,
				.height = height,
				.depth = 1,
			},
			.mipLevels = 1,
			.arrayLayers = 1,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.tiling = tiling,
			.usage = usage,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
			.queueFamilyIndexCount = 0,
			.pQueueFamilyIndices = 0,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		};

		CALL_VK(vkCreateImage(device, &imageCreateInfo, nullptr, pImage),
			std::string("failed to create ") + name);

		vkGetImageMemoryRequirements(device, *pImage, &memRequirements);

		VkMemoryAllocateInfo memAllocInfo = {
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			.pNext = nullptr,
			.allocationSize = memRequirements.size,
			.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties),
		};

		CALL_VK(vkAllocateMemory(device, &memAllocInfo, nullptr, pImageMemory),
			std::string("failed to allocate ") + name + " memory");

		CALL_VK(vkBindImageMemory(device, *pImage, *pImageMemory, 0),
			std::string("failed to bind ") + name + " memory");
	}

	void createVertexBuffer(void)
	{
		VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
		void *gpuMemory;

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;

		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			     &stagingBuffer, &stagingBufferMemory, "staging vertex buffer");

		CALL_VK(vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &gpuMemory),
			"failed to map device memory");
		memcpy(gpuMemory, vertices.data(), bufferSize);
		vkUnmapMemory(device, stagingBufferMemory);

		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &vertexBuffer, &vertexBufferMemory,
			     "vertex buffer");

		copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

		vkDestroyBuffer(device, stagingBuffer, nullptr);
		vkFreeMemory(device, stagingBufferMemory, nullptr);
	}

	void createIndexBuffer(void)
	{
		VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();
		void *gpuMemory;

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;

		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			     &stagingBuffer, &stagingBufferMemory, "staging index buffer");

		CALL_VK(vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &gpuMemory),
			"failed to map device memory");
		memcpy(gpuMemory, indices.data(), bufferSize);
		vkUnmapMemory(device, stagingBufferMemory);

		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &indexBuffer, &indexBufferMemory,
			     "index buffer");

		copyBuffer(stagingBuffer, indexBuffer, bufferSize);

		vkDestroyBuffer(device, stagingBuffer, nullptr);
		vkFreeMemory(device, stagingBufferMemory, nullptr);
	}

	void createUniformBuffers(void)
	{
		VkDeviceSize bufferSize = sizeof(UniformBufferObject);

		uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
		uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
		uniformBuffersMappings.resize(MAX_FRAMES_IN_FLIGHT);

		for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				     &uniformBuffers[i], &uniformBuffersMemory[i], "uniform buffer");

			CALL_VK(vkMapMemory(device, uniformBuffersMemory[i], 0, bufferSize, 0, &uniformBuffersMappings[i]),
				"failed to map uniform buffer memory");
		}
	}

	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
	{
		VkCommandBuffer commandBuffer = beginSingleTimeCommands();

		VkBufferCopy copyRegion = {
			.srcOffset = 0,
			.dstOffset = 0,
			.size = size,
		};

		vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

		endSingleTimeCommands(commandBuffer);
	}

	void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
	{
		VkCommandBuffer commandBuffer = beginSingleTimeCommands();

		VkBufferImageCopy region = {
			.bufferOffset = 0,
			.bufferRowLength = 0,
			.bufferImageHeight = 0,
			.imageSubresource = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.mipLevel = 0,
				.baseArrayLayer = 0,
				.layerCount = 1,

			},
			.imageOffset = { 0, 0, 0 },
			.imageExtent = { width, height, 1 },
		};

		vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

		endSingleTimeCommands(commandBuffer);
	}

	void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
	{
		(void)format;
		VkCommandBuffer commandBuffer = beginSingleTimeCommands();

		VkImageMemoryBarrier barrier = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			.pNext = nullptr,
			.srcAccessMask = 0,
			.dstAccessMask = 0,
			.oldLayout = oldLayout,
			.newLayout = newLayout,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.image = image,
			.subresourceRange = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1,
			},
		};

		VkPipelineStageFlags srcStage;
		VkPipelineStageFlags dstStage;

		if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
		    newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		} else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
			   newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		} else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
			   newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			if (hasStencilComponent(format)) {
				barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
			}

			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
						VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

			srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		} else {
			PANIC_VK("unsupported layout transition");
		}

		vkCmdPipelineBarrier(commandBuffer, srcStage, dstStage, 0,
				     0, nullptr,
				     0, nullptr,
				     1, &barrier);

		endSingleTimeCommands(commandBuffer);
	}

	VkCommandBuffer beginSingleTimeCommands(void)
	{
		VkCommandBuffer commandBuffer;

		VkCommandBufferAllocateInfo allocInfo = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.pNext = nullptr,
			.commandPool = commandPool,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = 1,
		};

		VkCommandBufferBeginInfo beginInfo = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.pNext = nullptr,
			.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
			.pInheritanceInfo = nullptr,
		};

		CALL_VK(vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer),
			"failed to allocate single-time buffer command buffer");

		CALL_VK(vkBeginCommandBuffer(commandBuffer, &beginInfo),
			"failed to begin single-time buffer command buffer");

		return commandBuffer;
	}

	void endSingleTimeCommands(VkCommandBuffer commandBuffer)
	{
		VkSubmitInfo submitInfo = {
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.pNext = nullptr,
			.waitSemaphoreCount = 0,
			.pWaitSemaphores = nullptr,
			.pWaitDstStageMask = nullptr,
			.commandBufferCount = 1,
			.pCommandBuffers = &commandBuffer,
			.signalSemaphoreCount = 0,
			.pSignalSemaphores = nullptr,
		};

		CALL_VK(vkEndCommandBuffer(commandBuffer),
			"failed to end single-time commandBuffer");

		CALL_VK(vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE),
			"failed to submit single-time buffer queue");
		CALL_VK(vkQueueWaitIdle(graphicsQueue),
			"failed to flush single-time buffer queue");

		vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
	}

	void createDescriptorPool(void)
	{
		VkDescriptorPoolSize uboPoolSize = {
			.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = MAX_FRAMES_IN_FLIGHT,
		};

		VkDescriptorPoolSize samplerPoolSize = {
			.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = MAX_FRAMES_IN_FLIGHT,
		};

		std::array<VkDescriptorPoolSize, 2> poolSizes = {
			uboPoolSize,
			samplerPoolSize,
		};

		VkDescriptorPoolCreateInfo poolInfo = {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.maxSets = MAX_FRAMES_IN_FLIGHT,
			.poolSizeCount = poolSizes.size(),
			.pPoolSizes = poolSizes.data(),
		};

		CALL_VK(vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool),
			"failed to create descriptor pool");
	}

	void createDescriptorSets(void)
	{
		std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
		VkDescriptorSetAllocateInfo allocInfo = {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.pNext = nullptr,
			.descriptorPool = descriptorPool,
			.descriptorSetCount = MAX_FRAMES_IN_FLIGHT,
			.pSetLayouts = layouts.data(),
		};

		descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
		CALL_VK(vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()),
			"failed to allocate descriptor sets");

		for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			VkDescriptorBufferInfo bufferInfo = {
				.buffer = uniformBuffers[i],
				.offset = 0,
				.range = sizeof(UniformBufferObject),
			};

			VkDescriptorImageInfo imageInfo = {
				.sampler = textureSampler,
				.imageView = textureImageView,
				.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			};

			VkWriteDescriptorSet uboDescriptorWrite = {
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.pNext = nullptr,
				.dstSet = descriptorSets[i],
				.dstBinding = 0,
				.dstArrayElement = 0,
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				.pImageInfo = nullptr,
				.pBufferInfo = &bufferInfo,
				.pTexelBufferView = nullptr,
			};

			VkWriteDescriptorSet samplerDescriptorWrite = {
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.pNext = nullptr,
				.dstSet = descriptorSets[i],
				.dstBinding = 1,
				.dstArrayElement = 0,
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				.pImageInfo = &imageInfo,
				.pBufferInfo = nullptr,
				.pTexelBufferView = nullptr,
			};

			std::array<VkWriteDescriptorSet, 2> descriptorWrites = {
				uboDescriptorWrite,
				samplerDescriptorWrite,
			};

			vkUpdateDescriptorSets(device, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
		}
	}

	void createCommandBuffers(void)
	{
		commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

		VkCommandBufferAllocateInfo allocInfo = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.pNext = nullptr,
			.commandPool = commandPool,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = (uint32_t)commandBuffers.size(),
		};

		CALL_VK(vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()),
			"failed to allocated command buffer");
	}

	void createSyncObjects(void)
	{
		imageAvaliableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

		VkSemaphoreCreateInfo semaphoreInfo = {
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
		};

		VkFenceCreateInfo fenceInfo = {
			.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
			.pNext = nullptr,
			.flags = VK_FENCE_CREATE_SIGNALED_BIT,
		};

		for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			CALL_VK(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvaliableSemaphores.at(i)),
				"failed to create image semaphore");
			CALL_VK(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores.at(i)),
				"failed to create render semaphore");
			CALL_VK(vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences.at(i)),
				"failed to create render fence");
		}
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

		CALL_VK(vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr),
			"failed to get device extensions");
		avaliableExtensions.resize(extensionCount);
		CALL_VK(vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, avaliableExtensions.data()),
			"failed to get device extensions");

		std::cout << "\t\t";
		for (const auto &extension : avaliableExtensions) {
			std::cout << extension.extensionName << ", ";
		}
		std::cout << '\n';
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
				std::cerr << Color::blue << "[VALIDATION INFO]: " << Color::reset;
				break;
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
				std::cerr << Color::yellow << "[VALIDATION WARNING]: " << Color::reset;
				break;
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
				std::cerr << Color::red << "[VALIDATION ERROR]: " << Color::reset;
				break;
			default:
				PANIC_VK("unknown message severity");
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

	void updateUniformBuffer(uint32_t currentFrame)
	{
		static auto startTime = std::chrono::high_resolution_clock::now();
		auto currentTime = std::chrono::high_resolution_clock::now();
		float delta = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

		UniformBufferObject ubo;
		const float fov = glm::radians(45.f);
		const float aspectRatio = (float)swapChainExtent.width / swapChainExtent.height;

		ubo.model = glm::rotate(glm::mat4(1), -delta * glm::radians(90.f), glm::vec3(0, 0, 1));
		ubo.view = glm::lookAt(glm::vec3(3, 3, glm::sin(delta) * 3), glm::vec3(0, 0, 0), glm::vec3(0, 0, 1));
		ubo.proj = glm::perspective(fov, aspectRatio, 0.1f, 10.f);
		ubo.proj[1][1] *= -1;

		memcpy(uniformBuffersMappings[currentFrame], &ubo, sizeof(ubo));
	}

	void drawFrame(void)
	{
		uint32_t imageIndex;
		VkResult result;

		updateUniformBuffer(currentFrame);

		CALL_VK(vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX),
			"failed to wait in render fence");

		result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX,
					      imageAvaliableSemaphores[currentFrame],
					      VK_NULL_HANDLE, &imageIndex);
		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			recreateSwapChain();
			std::cout << "acquire recreate\n";
			return;
		} else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			CALL_VK(result, "failed to acquire swap chain image");
		}

		CALL_VK(vkResetCommandBuffer(commandBuffers[currentFrame], 0),
			"failed to reset command buffer");
		recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

		VkSemaphore waitSemaphores[] = { imageAvaliableSemaphores[currentFrame] };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
		VkSubmitInfo submitInfo = {
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.pNext = nullptr,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = waitSemaphores,
			.pWaitDstStageMask = waitStages,
			.commandBufferCount = 1,
			.pCommandBuffers = &commandBuffers[currentFrame],
			.signalSemaphoreCount = 1,
			.pSignalSemaphores = signalSemaphores,
		};

		CALL_VK(vkResetFences(device, 1, &inFlightFences[currentFrame]),
			"failed to reset reset fence");
		CALL_VK(vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]),
			"failed to submit draw command buffer to queue");

		VkSwapchainKHR swapChains[] = { swapChain };

		VkPresentInfoKHR presentInfo = {
			.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
			.pNext = nullptr,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = signalSemaphores,
			.swapchainCount = 1,
			.pSwapchains = swapChains,
			.pImageIndices = &imageIndex,
			.pResults = nullptr,
		};

		result = vkQueuePresentKHR(presentQueue, &presentInfo);
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || frameBufferResized) {
			recreateSwapChain();
			frameBufferResized = false;
			std::cout << "present recreate\n";
			return;
		} else {
			CALL_VK(result, "failed to present image");
		}

		currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	}

	void mainLoop(void)
	{
		currentFrame = 0;

		while (!glfwWindowShouldClose(this->window)) {
			glfwPollEvents();
			drawFrame();
		}

		vkDeviceWaitIdle(device);
	}

	void cleanup(void)
	{
		for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			vkDestroySemaphore(device, imageAvaliableSemaphores.at(i), nullptr);
			vkDestroySemaphore(device, renderFinishedSemaphores.at(i), nullptr);
			vkDestroyFence(device, inFlightFences.at(i), nullptr);
		}

		vkDestroyCommandPool(device, commandPool, nullptr);
		cleanupSwapChain();

		vkDestroySampler(device, textureSampler, nullptr);
		vkDestroyImageView(device, textureImageView, nullptr);

		vkDestroyImage(device, textureImage, nullptr);
		vkFreeMemory(device, textureImageMemory, nullptr);

		for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			vkDestroyBuffer(device, uniformBuffers[i], nullptr);
			vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
		}
		vkDestroyDescriptorPool(device, descriptorPool, nullptr);
		vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

		vkDestroyBuffer(device, vertexBuffer, nullptr);
		vkDestroyBuffer(device, indexBuffer, nullptr);
		vkFreeMemory(device, vertexBufferMemory, nullptr);
		vkFreeMemory(device, indexBufferMemory, nullptr);

		vkDestroyPipeline(device, graphicsPipeline, nullptr);
		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
		vkDestroyRenderPass(device, renderPass, nullptr);

		vkDestroyDevice(device, nullptr);

		if (CONFIG_VALIDATION_LAYERS) {
			DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
		}

		vkDestroySurfaceKHR(instance, surface, nullptr);
		vkDestroyInstance(instance, nullptr);

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


