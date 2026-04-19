
#ifndef VKINSTANCE_HPP
#define VKINSTANCE_HPP

#include "vkcommon.hpp"
#include "kwindow.hpp"

#include <vector>
#include <iostream>
#include <vulkan/vulkan_core.h>

struct vkinstance_info {
	bool debug   = true;
	bool verbose = true;
};

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

struct vkinstance {
	void create(const vkinstance_info &create_info)
	{
		create_instance(create_info);
		create_debug_messanger(create_info);
	}

	void destroy(void)
	{
		DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
		vkDestroySurfaceKHR(instance, surface, nullptr);
		vkDestroyInstance(instance, nullptr);
	}

	void print_extensions(void)
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

	void create_surface(kwindow &kwindow)
	{
		CALL_VK(glfwCreateWindowSurface(instance, kwindow.__get_window(), nullptr, &surface),
			"failed to create window surface");
	}

	VkInstance __get_instance(void)
	{
		return instance;
	}

	VkSurfaceKHR __get_surface(void)
	{
		return surface;
	}

private:
	VkInstance instance = VK_NULL_HANDLE;
	VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
	VkSurfaceKHR surface = VK_NULL_HANDLE;

	void create_instance(const vkinstance_info &create_info)
	{
		VkApplicationInfo appInfo = {
			.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
			.pNext = nullptr,
			.pApplicationName = "application",
			.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
			.pEngineName = "no engine",
			.engineVersion = VK_MAKE_VERSION(1, 0, 0),
			.apiVersion = VK_API_VERSION_1_0
		};

		auto requieredExtensions = get_required_extentions(
			create_info.debug, create_info.verbose);

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
		if (create_info.debug) {
			populateDebugMessengerCreateInfo(&debugCreateInfo);

			createInfo.enabledLayerCount = (uint32_t)validationLayers.size();
			createInfo.ppEnabledLayerNames = validationLayers.data();
			createInfo.pNext = &debugCreateInfo;
		}

		CALL_VK(vkCreateInstance(&createInfo, nullptr, &instance),
			"failed to create instance");
	}

	void create_debug_messanger(const vkinstance_info &create_info)
	{
		VkDebugUtilsMessengerCreateInfoEXT createInfo;

		if (not create_info.debug) {
			return;
		}

		populateDebugMessengerCreateInfo(&createInfo);
		CALL_VK(CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger),
			"failed to set up debug messenger!");
	}

	std::vector<const char *> get_required_extentions(bool debug, bool verbose)
	{
		uint32_t glfwExtensionCount;
		const char **glfwExtentions;

		glfwExtentions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
		std::vector<const char *> extensions(glfwExtentions, glfwExtentions + glfwExtensionCount);

		if (verbose) {
			std::cout << "requiered glfw extensions:\n";
			for (const char *extension : extensions) {
				std::cout << '\t' << extension << '\n';
			}
			std::cout << '\n';
		}

		if (debug) {
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		return extensions;
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
};

#endif /* VKINSTANCE_HPP */
