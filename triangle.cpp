
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdexcept>

#include <set>
#include <string>
#include <vector>
#include <fstream>

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
		throw std::runtime_error("vkDestroyDebugUtilsMessengerEXT function is not present");
	}
}

static std::vector<unsigned char> readFile(const std::string &fileName)
{
	std::ifstream file(fileName, std::ios::ate | std::ios::binary);

	if (!file.is_open()) {
		throw std::runtime_error("failed to open file: " + fileName);
	}

	std::vector<unsigned char> buffer(file.tellg());

	file.seekg(0);
	file.read((char *)buffer.data(), buffer.size());
	return buffer;
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
	VkSwapchainKHR swapChain;
	std::vector<VkImage> swapChainImages;
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;
	std::vector<VkImageView> swapChainImageViews;
	VkRenderPass renderPass;
	VkPipelineLayout pipelineLayout;
	VkPipeline graphicsPipeline;
	std::vector<VkFramebuffer> swapChainFramebuffers;
	VkCommandPool commandPool;
	std::vector<VkCommandBuffer> commandBuffers;
	std::vector<VkSemaphore> imageAvaliableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;
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
		createSwapChain();
		createImageViews();
		createRenderPass();
		createGraphicsPipeline();
		createFramebuffers();
		createCommandPool();
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
				return presentMode;
			}
		}

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
		height = std::min(height, capabilities.maxImageExtent.width);

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

	void createImageViews(void)
	{
		swapChainImageViews.resize(swapChainImages.size());

		for (int i = 0; i < (int)swapChainImages.size(); i++) {
			VkImageViewCreateInfo createInfo = {
				.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.image = swapChainImages.at(i),
				.viewType = VK_IMAGE_VIEW_TYPE_2D,
				.format = swapChainImageFormat,
				.components = {
					.r = VK_COMPONENT_SWIZZLE_IDENTITY,
					.g = VK_COMPONENT_SWIZZLE_IDENTITY,
					.b = VK_COMPONENT_SWIZZLE_IDENTITY,
					.a = VK_COMPONENT_SWIZZLE_IDENTITY,
				},
				.subresourceRange = {
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.baseMipLevel = 0,
					.levelCount = 1,
					.baseArrayLayer = 0,
					.layerCount = 1,
				},
			};

			CALL_VK(vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews.at(i)),
				"failed to create image view");
		}
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

		VkAttachmentReference colorAttachmentRef = {
			.attachment = 0,
			.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		};

		VkSubpassDescription subpass = {
			.flags = 0,
			.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
			.inputAttachmentCount = 0,
			.pInputAttachments = nullptr,
			.colorAttachmentCount = 1,
			.pColorAttachments = &colorAttachmentRef,
			.pResolveAttachments = nullptr,
			.pDepthStencilAttachment = nullptr,
			.preserveAttachmentCount = 0,
			.pPreserveAttachments = nullptr,
		};

		VkSubpassDependency dependency = {
			.srcSubpass = VK_SUBPASS_EXTERNAL,
			.dstSubpass = 0,
			.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.srcAccessMask = 0,
			.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			.dependencyFlags = 0,
		};

		VkRenderPassCreateInfo renderPassCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.attachmentCount = 1,
			.pAttachments = &colorAttachment,
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

	void createGraphicsPipeline(void)
	{
		auto vertShaderCode = readFile("vert.spv");
		auto fragShaderCode = readFile("frag.spv");

		std::cout << "loaded vertex shader code of size " << vertShaderCode.size() << "bytes\n";
		std::cout << "loaded fragment shader code of size " << fragShaderCode.size() << "bytes\n";

		VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
		VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

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

		// std::vector<VkDynamicState> dynamicStates = {
		// 	VK_DYNAMIC_STATE_VIEWPORT,
		// 	VK_DYNAMIC_STATE_SCISSOR,
		// };
		//
		// VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {
		// 	.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		// 	.pNext = nullptr,
		// 	.flags = 0,
		// 	.dynamicStateCount = (uint32_t)dynamicStates.size(),
		// 	.pDynamicStates = dynamicStates.data(),
		// };

		VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.vertexBindingDescriptionCount = 0,
			.pVertexBindingDescriptions = nullptr,
			.vertexAttributeDescriptionCount = 0,
			.pVertexAttributeDescriptions = nullptr,
		};

		VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
			.primitiveRestartEnable = VK_FALSE,
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

		VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.viewportCount = 1,
			.pViewports = &viewport,
			.scissorCount = 1,
			.pScissors = &scissor,
		};

		VkPipelineRasterizationStateCreateInfo rasterizer = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.depthClampEnable = VK_FALSE,
			.rasterizerDiscardEnable = VK_FALSE,
			.polygonMode = VK_POLYGON_MODE_FILL,
			.cullMode = VK_CULL_MODE_BACK_BIT,
			.frontFace = VK_FRONT_FACE_CLOCKWISE,
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
			.setLayoutCount = 0,
			.pSetLayouts = nullptr,
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
			.pDepthStencilState = nullptr,
			.pColorBlendState = &colorBlending,
			.pDynamicState = nullptr,
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
			VkImageView attachments[] = {
				swapChainImageViews.at(i),
			};

			VkFramebufferCreateInfo framebufferCreateInfo = {
				.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.renderPass = renderPass,
				.attachmentCount = 1,
				.pAttachments = attachments,
				.width = swapChainExtent.width,
				.height = swapChainExtent.height,
				.layers = 1,
			};

			CALL_VK(vkCreateFramebuffer(device, &framebufferCreateInfo, nullptr, &swapChainFramebuffers.at(i)),
				"failed to create framebuffer");
		}
		std::cout << "created " << swapChainFramebuffers.size() << " frame buffers\n";
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

		VkClearValue clearColor = {{{ 0, 0, 0, 1 }}};

		VkRenderPassBeginInfo renderPassInfo = {
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			.pNext = nullptr,
			.renderPass = renderPass,
			.framebuffer = swapChainFramebuffers[imageIndex],
			.renderArea = {
				.offset = { 0, 0 },
				.extent = swapChainExtent,
			},
			.clearValueCount = 1,
			.pClearValues = &clearColor,
		};

		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

		vkCmdDraw(commandBuffer, 3, 1, 0, 0);

		vkCmdEndRenderPass(commandBuffer);

		CALL_VK(vkEndCommandBuffer(commandBuffer),
			"failed to record command buffer");

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

	void drawFrame(void)
	{
		uint32_t imageIndex;

		CALL_VK(vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX),
			"failed to wait in render fence");
		CALL_VK(vkResetFences(device, 1, &inFlightFences[currentFrame]),
			"failed to reset reset fence");

		CALL_VK(vkAcquireNextImageKHR(device, swapChain, UINT64_MAX,
					      imageAvaliableSemaphores[currentFrame],
					      VK_NULL_HANDLE, &imageIndex),
			"failed to acquire swap chain image");

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

		CALL_VK(vkQueuePresentKHR(presentQueue, &presentInfo),
			"failed to present image");

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

		for (auto &framebuffer : swapChainFramebuffers) {
			vkDestroyFramebuffer(device, framebuffer, nullptr);
		}

		vkDestroyPipeline(device, graphicsPipeline, nullptr);
		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
		vkDestroyRenderPass(device, renderPass, nullptr);

		for (auto &imageView : swapChainImageViews) {
			vkDestroyImageView(device, imageView, nullptr);
		}

		vkDestroySwapchainKHR(device, swapChain, nullptr);
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


