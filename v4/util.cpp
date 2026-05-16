
#include "config.h"
#include "engine.hpp"

#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_raii.hpp"
#include <iostream>
#include <fstream>
#include <stdexcept>

static inline uint64_t BIT(uint64_t x)
{
    return 1 << x;
}

vk::Format Engine::find_supported_format(
        const std::vector<vk::Format> &candidates,
        vk::ImageTiling tiling,
        vk::FormatFeatureFlagBits features
    )
{
    for (vk::Format format : candidates) {
        vk::FormatProperties props = this->physical_device.getFormatProperties(format);

        if (tiling == vk::ImageTiling::eLinear &&
            (props.linearTilingFeatures & features) == features) {
            return format;
        }

        if (tiling == vk::ImageTiling::eOptimal &&
            (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    throw std::runtime_error("failed to find supported format");
}

void Engine::copy_buffer_to_image(
        const vk::raii::CommandBuffer &cb,
        vk::raii::Image &dst,
        const vk::raii::Buffer &src,
        uint32_t width,
        uint32_t height
    )
{
    vk::BufferImageCopy region(
        0,
        0,
        0,
        vk::ImageSubresourceLayers(
            vk::ImageAspectFlagBits::eColor,
            0,
            0,
            1
        ),
        { 0, 0, 0 },
        { width, height, 1 }
    );

    cb.copyBufferToImage(
        src,
        dst,
        vk::ImageLayout::eTransferDstOptimal, region
    );
}

void Engine::copy_buffer(
        vk::raii::Buffer &dst,
        const vk::raii::Buffer &src,
        vk::DeviceSize size
    )
{
    vk::raii::CommandBuffer cb = begin_single_time_commands(this->command_pool);

    cb.copyBuffer(*src, *dst, vk::BufferCopy(0, 0, size));

    end_single_time_commands(std::move(cb));
}


vk::raii::CommandBuffer Engine::begin_single_time_commands(
        const vk::raii::CommandPool &command_pool
    )
{
    vk::CommandBufferAllocateInfo buffer_info(
        command_pool,
        vk::CommandBufferLevel::ePrimary,
        1
    );

    vk::raii::CommandBuffers cb = this->device.allocateCommandBuffers(buffer_info);

    vk::CommandBufferBeginInfo begin_info(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    cb.front().begin(begin_info);

    return std::move(cb.front());
}

void Engine::end_single_time_commands(
        vk::raii::CommandBuffer &&cb
    )
{
    cb.end();

    vk::SubmitInfo submit_info(
        {},
        {},
        *cb
    );

    queue.submit(submit_info);
    queue.waitIdle();
}

std::pair<vk::raii::Image, vk::raii::DeviceMemory> Engine::create_image(
        uint32_t width,
        uint32_t height,
        vk::Format format,
        vk::ImageUsageFlags usage,
        vk::MemoryPropertyFlags properties
    )
{
    vk::ImageCreateInfo image_info(
        {},
        vk::ImageType::e2D,
        format,
        {width, height, 1},
        1,
        1,
        vk::SampleCountFlagBits::e1,
        vk::ImageTiling::eOptimal,
        usage,
        vk::SharingMode::eExclusive,
        {},
        {},
        vk::ImageLayout::eUndefined
    );

    vk::raii::Image image(this->device, image_info);

    vk::MemoryRequirements mem_req = image.getMemoryRequirements();
    uint32_t type_index = find_memory_type(
        this->physical_device,
        mem_req.memoryTypeBits,
        properties
    );

    vk::MemoryAllocateInfo alloc_info(mem_req.size, type_index);

    vk::raii::DeviceMemory mem(this->device, alloc_info);

    image.bindMemory(mem, 0);

    return { std::move(image), std::move(mem) };
}

vk::raii::ImageView Engine::create_image_view(
        const vk::Image &image,
        vk::Format format,
        vk::ImageAspectFlagBits aspect
    )
{
    vk::ImageViewCreateInfo image_view_info(
        {},
        image,
        vk::ImageViewType::e2D,
        format,
        {},
        vk::ImageSubresourceRange(
            aspect,
            0,
            1,
            0,
            1
        )
    );

    return vk::raii::ImageView(this->device, image_view_info);
}

std::pair<vk::raii::Buffer, vk::raii::DeviceMemory> Engine::create_buffer(
        vk::DeviceSize size,
        vk::BufferUsageFlags usage,
        vk::MemoryPropertyFlags properties
    )
{
    vk::BufferCreateInfo buffer_info(
        {},
        size,
        usage,
        vk::SharingMode::eExclusive
    );

    vk::raii::Buffer buffer(this->device, buffer_info);

    vk::MemoryRequirements mem_req = buffer.getMemoryRequirements();
    uint32_t type_index = find_memory_type(
        this->physical_device,
        mem_req.memoryTypeBits,
        properties
    );

    vk::MemoryAllocateInfo alloc_info(mem_req.size, type_index);
    vk::raii::DeviceMemory mem(this->device, alloc_info);

    buffer.bindMemory(*mem, 0);

    return { std::move(buffer), std::move(mem) };
}

uint32_t Engine::find_memory_type(
        const vk::raii::PhysicalDevice &pd,
        uint32_t type_filter,
        vk::MemoryPropertyFlags properties
    )
{
    vk::PhysicalDeviceMemoryProperties mem_props = pd.getMemoryProperties();

    for (uint32_t i = 0; i < mem_props.memoryTypeCount; i++) {
        if ((type_filter & BIT(i)) == 0) {
            continue;
        }

        if ((mem_props.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("failed to find sutable memory type");
}

void Engine::transition_image_layout(
        vk::raii::CommandBuffer &cb,
        const vk::Image &image,
        vk::ImageAspectFlagBits aspect,
        vk::ImageLayout old_layout,
        vk::ImageLayout new_layout,
        vk::AccessFlags2 scr_access_mask,
        vk::AccessFlags2 dst_access_mask,
        vk::PipelineStageFlags2 src_stage_mask,
        vk::PipelineStageFlags2 dst_stage_mask
    )
{
    vk::ImageSubresourceRange subresource_range(
        aspect,
        0,
        1,
        0,
        1
    );
    vk::ImageMemoryBarrier2 barrier(
        src_stage_mask,
        scr_access_mask,
        dst_stage_mask,
        dst_access_mask,
        old_layout,
        new_layout,
        VK_QUEUE_FAMILY_IGNORED,
        VK_QUEUE_FAMILY_IGNORED,
        image,
        subresource_range
    );
    vk::DependencyInfo dependency_info(
        {},
        {},
        {},
        { barrier }
    );


    cb.pipelineBarrier2(dependency_info);
}

vk::raii::ShaderModule Engine::create_shader_module(
        const vk::raii::Device &dev,
        const std::vector<char> &shader_code
    )
{
    vk::ShaderModuleCreateInfo create_info(
        {},
        shader_code.size() * sizeof(char),
        reinterpret_cast<const uint32_t *>(shader_code.data())
    );

    return vk::raii::ShaderModule(dev, create_info);
}

vk::SurfaceFormatKHR Engine::choose_swapchain_surface_format(
        const vk::raii::PhysicalDevice &pd,
        const vk::raii::SurfaceKHR &surface
    )
{
    std::vector<vk::SurfaceFormatKHR> avaliable_formats = pd.getSurfaceFormatsKHR(surface);

    if (avaliable_formats.empty()) {
        throw std::runtime_error("no avaliable surface formats found");
    }

    // try to find format that supports SRGB
    for (const vk::SurfaceFormatKHR &format : avaliable_formats) {
        if (format.format == vk::Format::eB8G8R8Srgb &&
            format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
            return format;
        }
    }

    // otherwise return any
    return avaliable_formats.front();
}

vk::PresentModeKHR Engine::choose_swapchain_present_mode(
        const vk::raii::PhysicalDevice &pd,
        const vk::raii::SurfaceKHR &surface
    )
{
    std::vector<vk::PresentModeKHR> avaliable_present_modes = pd.getSurfacePresentModesKHR(surface);

    // try to use mailbox present mode if supported
    if (std::ranges::contains(avaliable_present_modes, vk::PresentModeKHR::eMailbox)) {
        return vk::PresentModeKHR::eMailbox;
    }

    // otherwise use FIFO present mode
    if (not std::ranges::contains(avaliable_present_modes, vk::PresentModeKHR::eFifo)) {
        throw std::runtime_error("FIFO present mode is not supported");
    }
    return vk::PresentModeKHR::eFifo;
}

vk::Extent2D Engine::choose_swapchain_extent(
        const vk::raii::PhysicalDevice &pd,
        const vk::raii::SurfaceKHR &surface,
        GLFWwindow *window
    )
{
    vk::SurfaceCapabilitiesKHR capabilities = pd.getSurfaceCapabilitiesKHR(surface);
    int width, height;

    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    }

    glfwGetFramebufferSize(window, &width, &height);
    width = std::clamp<uint32_t>(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    height = std::clamp<uint32_t>(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    return { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
}

uint32_t Engine::get_queue_family_index(
        const vk::raii::PhysicalDevice &pd,
        const vk::raii::SurfaceKHR &surface
    )
{
    std::vector<vk::QueueFamilyProperties> props = pd.getQueueFamilyProperties();
    uint32_t                               idx   = 0;

    for (const vk::QueueFamilyProperties &qfp : props) {
        // try to get single queue with both drawing and presentation support
        if ((qfp.queueFlags & vk::QueueFlagBits::eGraphics) != vk::QueueFlagBits{} &&
            pd.getSurfaceSupportKHR(idx, surface)) {
            return idx;
        }

        idx++;
    }

    throw std::runtime_error("no queue family for graphics and present found");
}

int Engine::get_physical_device_score(const vk::raii::PhysicalDevice &pd)
{
    constexpr int                          NOT_SUTABLE     = -1;

    std::vector<const char *>              req_extensions  = get_required_device_extensions();
    std::vector<vk::ExtensionProperties>   supp_extensions = pd.enumerateDeviceExtensionProperties();
    std::vector<vk::QueueFamilyProperties> queue_families  = pd.getQueueFamilyProperties();
    vk::PhysicalDeviceProperties           props           = pd.getProperties();
    int                                    score           = 0;

    auto features = pd.template getFeatures2<vk::PhysicalDeviceFeatures2,
                                             vk::PhysicalDeviceVulkan11Features,
                                             vk::PhysicalDeviceVulkan13Features,
                                             vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>();

    if (props.apiVersion < vk::ApiVersion13) {
        return NOT_SUTABLE;
    }

    bool has_all_features =
        features.template get<vk::PhysicalDeviceFeatures2>().features.samplerAnisotropy &&
        features.template get<vk::PhysicalDeviceVulkan11Features>().shaderDrawParameters &&
        features.template get<vk::PhysicalDeviceVulkan13Features>().synchronization2 &&
        features.template get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering &&
        features.template get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState;

    if (not has_all_features) {
        return NOT_SUTABLE;
    }

    if (std::ranges::none_of(queue_families,
                             [](const vk::QueueFamilyProperties &qfp) {
                                 return (qfp.queueFlags & vk::QueueFlagBits::eGraphics) != vk::QueueFlagBits{};
                             })) {
        return NOT_SUTABLE;
    }

    for (const char *req_ext : req_extensions) {
        if (std::ranges::none_of(supp_extensions,
                                 [req_ext](const vk::ExtensionProperties &supp_ext) {
                                     return strcmp(supp_ext.extensionName, req_ext) == 0;
                                 })) {
            return NOT_SUTABLE;
        }
    }

    if (props.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
        score += 10000;
    }

    score += props.limits.maxImageDimension2D;

    return score;
}

std::vector<const char *> Engine::get_required_device_extensions(void)
{
    return {
        vk::KHRSwapchainExtensionName,
        vk::KHRSpirv14ExtensionName,
        vk::KHRSynchronization2ExtensionName,
        vk::KHRCreateRenderpass2ExtensionName,
    };
}

std::vector<const char *> Engine::get_required_instance_extensions(void)
{
    uint32_t     glfw_extension_count = 0;
    const char **glfw_extensions      = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

    std::vector extensions(glfw_extensions, glfw_extensions + glfw_extension_count);
    if (CONFIG_VK_VALIDATION_LAYERS) {
        extensions.push_back(vk::EXTDebugUtilsExtensionName);
    }

    return extensions;
}

std::vector<char> Engine::read_file(const std::string &fname)
{
    std::ifstream     f(fname, std::ios::ate | std::ios::binary);
    std::vector<char> buff;

    if (!f.is_open()) {
        throw std::runtime_error("failed to open file: " + fname);
    }

    buff.resize(f.tellg());
    f.seekg(0, std::ios::beg);
    f.read(buff.data(), buff.size());
    f.close();

    return buff;
}

vk::Bool32 Engine::debug_callback(
        vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
        vk::DebugUtilsMessageTypeFlagsEXT type,
        const vk::DebugUtilsMessengerCallbackDataEXT *callback_data,
        void *
    )
{
    (void)type;

    if (severity >= vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo) {
        std::cerr << "validation layer" << " msg: " << callback_data->pMessage << std::endl;
    }
    return vk::False;
}
