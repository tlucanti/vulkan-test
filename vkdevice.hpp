#ifndef VKDEVICE_HPP
#define VKDEVICE_HPP

#include "vkcommon.hpp"
#include "vkinstance.hpp"

#include <set>
#include <vector>
#include <vulkan/vulkan_core.h>

struct vkdevice_info {
	bool verbose = true;
};

struct QueueFamilyIndices {
	uint32_t graphicsFamily;
	uint32_t presentFamily;
};

struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

const std::vector<const char *> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};

struct vkdevice {
	void create(const vkdevice_info &create_info, vkinstance &vkinstance)
	{
		if (create_info.verbose) {
			printPhysicalDevices(vkinstance);
		}
		pickPhysicalDevice(vkinstance);
		createLogicalDevice(vkinstance);
	}

	void destroy(void)
	{
		vkDestroyDevice(this->device, nullptr);
	}

	void wait_idle(void)
	{
		vkDeviceWaitIdle(this->device);
	}

	SwapChainSupportDetails query_swapchain_support(vkinstance &vkinstance)
	{
		return querySwapChainSupport(this->physicalDevice, vkinstance);
	}

	QueueFamilyIndices find_queue_families(vkinstance &vkinstance)
	{
		return findQueueFamilies(this->physicalDevice, vkinstance);
	}

	VkPhysicalDevice __get_physical_device(void)
	{
		return this->physicalDevice;
	}

	VkDevice __get_device(void)
	{
		return this->device;
	}

	VkQueue __get_graphics_queue(void)
	{
		return this->graphicsQueue;
	}

	VkQueue __get_present_queue(void)
	{
		return this->presentQueue;
	}

private:
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkDevice device = VK_NULL_HANDLE;
	VkQueue graphicsQueue = VK_NULL_HANDLE;
	VkQueue presentQueue = VK_NULL_HANDLE;

	void printDeviceExtensinos(VkPhysicalDevice pdev)
	{
		uint32_t extensionCount;
		std::vector<VkExtensionProperties> avaliableExtensions;

		CALL_VK(vkEnumerateDeviceExtensionProperties(pdev, nullptr, &extensionCount, nullptr),
			"failed to get device extensions");
		avaliableExtensions.resize(extensionCount);
		CALL_VK(vkEnumerateDeviceExtensionProperties(pdev, nullptr, &extensionCount, avaliableExtensions.data()),
			"failed to get device extensions");

		std::cout << "\t\t";
		for (const auto &extension : avaliableExtensions) {
			std::cout << extension.extensionName << ", ";
		}
		std::cout << '\n';
	}

	void printPhysicalDevices(vkinstance &vkinstance)
	{
		uint32_t deviceCount;
		std::vector<VkPhysicalDevice> devices;

		CALL_VK(vkEnumeratePhysicalDevices(vkinstance.__get_instance(), &deviceCount, nullptr),
			"failed to get physical device count");

		std::cout << "avaliable physical devices:\n";
		if (deviceCount == 0) {
			std::cout << "\tNO DEVICES AVALIABLE\n";
			return;
		}

		devices.resize(deviceCount);
		CALL_VK(vkEnumeratePhysicalDevices(vkinstance.__get_instance(), &deviceCount, devices.data()),
			"failed to get physical devices list");

		for (const auto &pdev : devices) {
			VkPhysicalDeviceProperties deviceProperties;

			vkGetPhysicalDeviceProperties(pdev, &deviceProperties);
			std::cout << '\t' << deviceProperties.deviceName << '\n';
			std::cout << "\tdevice extensions:\n";

			printDeviceExtensinos(pdev);
			std::cout << '\n';
		}
		std::cout << '\n';
	}

	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice pdev, vkinstance &vkinstance)
	{
		uint32_t queueFamilyCount;
		std::vector<VkQueueFamilyProperties> queueFamilies;

		uint32_t i = 0;
		QueueFamilyIndices indices = {
			.graphicsFamily = (uint32_t)-1,
			.presentFamily = (uint32_t)-1,
		};

		vkGetPhysicalDeviceQueueFamilyProperties(pdev, &queueFamilyCount, nullptr);
		queueFamilies.resize(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(pdev, &queueFamilyCount, queueFamilies.data());

		for (const auto &queueFamily : queueFamilies) {
			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				indices.graphicsFamily = i;
			}

			VkBool32 surfaceSupport = false;
			CALL_VK(vkGetPhysicalDeviceSurfaceSupportKHR(pdev, i, vkinstance.__get_surface(), &surfaceSupport),
				"failed to check device surface support");
			if (surfaceSupport) {
				indices.presentFamily = i;
			}

			i++;
		}

		return indices;
	}

	bool checkDeviceExtensionSupport(VkPhysicalDevice pdev)
	{
		uint32_t extensionCount;
		std::vector<VkExtensionProperties> avaliableExtensions;

		CALL_VK(vkEnumerateDeviceExtensionProperties(pdev, nullptr, &extensionCount, nullptr),
			"failed to get device extensions");
		avaliableExtensions.resize(extensionCount);
		CALL_VK(vkEnumerateDeviceExtensionProperties(pdev, nullptr, &extensionCount, avaliableExtensions.data()),
			"failed to get device extensions");

		std::set<std::string> requieredExtensions(deviceExtensions.begin(), deviceExtensions.end());
		for (const auto &extension : avaliableExtensions) {
			requieredExtensions.erase(extension.extensionName);
		}

		return requieredExtensions.empty();
	}

	bool isDeviceSutable(VkPhysicalDevice pdev, vkinstance &vkinstance)
	{
		VkPhysicalDeviceProperties deviceProperties;
		VkPhysicalDeviceFeatures deviceFeatures;
		QueueFamilyIndices indices;
		SwapChainSupportDetails swapChainSupport;
		bool deviceOk = true;

		vkGetPhysicalDeviceProperties(pdev, &deviceProperties);
		vkGetPhysicalDeviceFeatures(pdev, &deviceFeatures);

		deviceOk &= deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ||
			    deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU ||
			    deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU;

		deviceOk &= deviceFeatures.geometryShader == VK_TRUE;
		deviceOk &= deviceFeatures.samplerAnisotropy == VK_TRUE;

		indices = findQueueFamilies(pdev, vkinstance);
		deviceOk &= (indices.graphicsFamily != (uint32_t)-1);

		deviceOk &= checkDeviceExtensionSupport(pdev);

		swapChainSupport = querySwapChainSupport(pdev, vkinstance);
		deviceOk &= (not swapChainSupport.formats.empty() &&
			     not swapChainSupport.presentModes.empty());

		if (deviceOk) {
			std::cout << "selecting physical device: " << deviceProperties.deviceName << '\n';
		}
		return deviceOk;
	}

	void pickPhysicalDevice(vkinstance &vkinstance)
	{
		uint32_t deviceCount;
		std::vector<VkPhysicalDevice> devices;

		CALL_VK(vkEnumeratePhysicalDevices(vkinstance.__get_instance(), &deviceCount, nullptr),
			"failed to get physical devices count");

		if (deviceCount == 0) {
			PANIC_VK("no GPU devices with vulkan support");
		}

		devices.resize(deviceCount);
		CALL_VK(vkEnumeratePhysicalDevices(vkinstance.__get_instance(), &deviceCount, devices.data()),
			"failed to get physical devices list");

		for (const auto &pdev : devices) {
			if (isDeviceSutable(pdev, vkinstance)) {
				physicalDevice = pdev;
				break;
			}
		}

		if (this->physicalDevice == VK_NULL_HANDLE) {
			PANIC_VK("failed to find sutable GPU");
		}
	}

	void createLogicalDevice(vkinstance &vkinstance)
	{
		QueueFamilyIndices indices = find_queue_families(vkinstance);
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

		CALL_VK(vkCreateDevice(this->physicalDevice, &createInfo, nullptr, &this->device),
			"failed to create logical device");

		vkGetDeviceQueue(this->device, indices.graphicsFamily, 0, &this->graphicsQueue);
		vkGetDeviceQueue(this->device, indices.presentFamily, 0, &this->presentQueue);
	}

	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice pdev, vkinstance &vkinstance)
	{
		SwapChainSupportDetails details;
		uint32_t formatCount;
		uint32_t presentModeCount;

		CALL_VK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pdev, vkinstance.__get_surface(), &details.capabilities),
			"failed to get surface capabilities");

		CALL_VK(vkGetPhysicalDeviceSurfaceFormatsKHR(pdev, vkinstance.__get_surface(), &formatCount, nullptr),
			"failed to get surface formats");
		details.formats.resize(formatCount);
		CALL_VK(vkGetPhysicalDeviceSurfaceFormatsKHR(pdev, vkinstance.__get_surface(), &formatCount, details.formats.data()),
			"failed to get surface formats");

		CALL_VK(vkGetPhysicalDeviceSurfacePresentModesKHR(pdev, vkinstance.__get_surface(), &presentModeCount, nullptr),
			"failed to get surface present modes");
		details.presentModes.resize(presentModeCount);
		CALL_VK(vkGetPhysicalDeviceSurfacePresentModesKHR(pdev, vkinstance.__get_surface(), &presentModeCount, details.presentModes.data()),
			"failed to get surface present modes");

		return details;
	}

};

#endif /* VKDEVICE_HPP */
