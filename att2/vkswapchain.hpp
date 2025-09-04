#ifndef VKSWAPCHAIN_HPP
#define VKSWAPCHAIN_HPP

#include "vkcommon.hpp"
#include "vkdevice.hpp"

#include <array>
#include <vulkan/vulkan_core.h>

struct vkswapchain_info {
	bool verbose = true;
};

struct vkswapchain {
	void create(const vkswapchain_info &create_info, vkdevice &vkdevice,
		    std::pair<unsigned, unsigned> frame_buffer_size)
	{
		createSwapChain(vkdevice, create_info.verbose, frame_buffer_size);
	}

	void create_image_views(void)
	{
		this->swapChainImageViews.resize(this->swapChainImages.size());

		for (int i = 0; i < (int)swapChainImages.size(); i++) {
			this->swapChainImageViews[i] =
				createImageView(this->swapChainImages[i],
						this->swapChainImageFormat,
						VK_IMAGE_ASPECT_COLOR_BIT);
		}
	}

	void create_render_pass(void)
	{
		createRenderPass();
	}

	void create_depth_resources(void)
	{
		createDepthResources();
	}

	void create_framebuffers(void)
	{
		createFramebuffers();
	}

	void destroy(void)
	{}

	VkRenderPass __get_render_pass(void)
	{
		return this->renderPass;
	}

private:
	vkdevice *vkdevice = nullptr;

	VkSwapchainKHR swapChain = VK_NULL_HANDLE;
	VkRenderPass renderPass = VK_NULL_HANDLE;

	std::vector<VkImage> swapChainImages;
	std::vector<VkImageView> swapChainImageViews;
	std::vector<VkFramebuffer> swapChainFramebuffers;

	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;

	VkImage depthImage = VK_NULL_HANDLE;
	VkDeviceMemory depthImageMemory = VK_NULL_HANDLE;
	VkImageView depthImageView = VK_NULL_HANDLE;

	void createSwapChain(vkdevice &vkdevice, bool verbose,
			     std::pair<unsigned, unsigned> framebuffer_size)
	{
		SwapChainSupportDetails swapChainSupport;

		swapChainSupport = vkdevice.query_swapchain_support(
			vkdevice.get_vkinstance());

		VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
		VkPresentModeKHR presentMode = choosePresentMode(swapChainSupport.presentModes);
		VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities, framebuffer_size);

		uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
		if (swapChainSupport.capabilities.maxImageCount != 0 &&
		    imageCount > swapChainSupport.capabilities.maxImageCount) {
			imageCount = swapChainSupport.capabilities.minImageCount;
		}
		if (verbose) {
			std::cout << "using " << imageCount << " images in swap chain\n";
		}

		VkSwapchainCreateInfoKHR createInfo = {
			.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
			.pNext = nullptr,
			.flags = 0,
			.surface = vkdevice.get_vkinstance().__get_surface(),
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

		QueueFamilyIndices indices = vkdevice.find_queue_families(vkdevice.get_vkinstance());
		uint32_t queueFamilyIndices[] = { indices.graphicsFamily, indices.presentFamily };
		if (indices.graphicsFamily != indices.presentFamily) {
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		}

		CALL_VK(vkCreateSwapchainKHR(vkdevice.__get_device(), &createInfo, nullptr, &swapChain),
			"failed to create swap chain");

		CALL_VK(vkGetSwapchainImagesKHR(vkdevice.__get_device(), swapChain, &imageCount, nullptr),
			"failed to get image handles");
		swapChainImages.resize(imageCount);
		CALL_VK(vkGetSwapchainImagesKHR(vkdevice.__get_device(), swapChain, &imageCount, swapChainImages.data()),
			"failed to get image handles");

		swapChainImageFormat = surfaceFormat.format;
		swapChainExtent = extent;
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

	VkExtent2D
	chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities,
			 std::pair<unsigned, unsigned> framebuffer_size)
	{
		uint32_t width, height;
		VkExtent2D extent;

		std::tie(width, height) = framebuffer_size;

		width = std::max(width, capabilities.minImageExtent.width);
		width = std::min(width, capabilities.maxImageExtent.width);
		height = std::max(height, capabilities.minImageExtent.height);
		height = std::min(height, capabilities.maxImageExtent.height);

		extent.width = width;
		extent.height = height;

		return extent;
	}

	void cleanupSwapChain(void)
	{
		vkDestroyImageView(vkdevice->__get_device(), depthImageView, nullptr);
		vkDestroyImage(vkdevice->__get_device(), depthImage, nullptr);
		vkFreeMemory(vkdevice->__get_device(), depthImageMemory, nullptr);

		for (auto &framebuffer : swapChainFramebuffers) {
			vkDestroyFramebuffer(vkdevice->__get_device(), framebuffer, nullptr);
		}

		for (auto &imageView : swapChainImageViews) {
			vkDestroyImageView(vkdevice->__get_device(), imageView, nullptr);
		}

		vkDestroySwapchainKHR(vkdevice->__get_device(), swapChain, nullptr);
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
		CALL_VK(vkCreateImageView(vkdevice->__get_device(), &imageViewCreateInfo, nullptr, &imageView),
			"failed to create image view");

		return imageView;
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

		CALL_VK(vkCreateRenderPass(vkdevice->__get_device(), &renderPassCreateInfo, nullptr, &renderPass),
			"failed to create render pass");
	}

	VkFormat findDepthFormat(void)
	{
		return findSupportedFormat(
			{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
			VK_IMAGE_TILING_OPTIMAL,
			VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
		);
	}

	VkFormat findSupportedFormat(const std::vector<VkFormat> &candidates,
				     VkImageTiling tiling, VkFormatFeatureFlags features)
	{
		for (VkFormat format : candidates) {
			VkFormatProperties props;

			vkGetPhysicalDeviceFormatProperties(vkdevice->__get_physical_device(), format, &props);

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

			CALL_VK(vkCreateFramebuffer(vkdevice->__get_device(), &framebufferCreateInfo, nullptr, &swapChainFramebuffers.at(i)),
				"failed to create framebuffer");
		}
		std::cout << "created " << swapChainFramebuffers.size() << " frame buffers\n";
	}

	void createDepthResources(void)
	{
		VkFormat depthFormat = findDepthFormat();

		vkdevice->create_image(
			    swapChainExtent.width, swapChainExtent.height, depthFormat,
			    VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
			    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &depthImage, &depthImageMemory,
			    "depth buffer");

		depthImageView = createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);

		transitionImageLayout(depthImage, depthFormat,
				      VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
	}

};

#endif /* VKSWAPCHAIN_HPP */
