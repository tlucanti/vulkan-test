
#include "engine.hpp"

#include <iostream>
#include <fstream>

static inline uint64_t BIT(uint64_t x)
{
    return 1 << x;
}

uint32_t Engine::find_memory_type(const vk::raii::PhysicalDevice &device, uint32_t type_filter, vk::MemoryPropertyFlags properties)
{
    vk::PhysicalDeviceMemoryProperties mem_props = device.getMemoryProperties();

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

void Engine::transition_image_layout(vk::raii::CommandBuffer &cb,
                                     const vk::Image &current_frame,
                                     vk::ImageLayout old_layout,
                                     vk::ImageLayout new_layout,
                                     vk::AccessFlags2 scr_access_mask,
                                     vk::AccessFlags2 dst_access_mask,
                                     vk::PipelineStageFlags2 src_stage_mask,
                                     vk::PipelineStageFlags2 dst_stage_mask)
{
    vk::ImageSubresourceRange subresource_range(
        vk::ImageAspectFlagBits::eColor,
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
        current_frame,
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

vk::raii::ShaderModule Engine::create_shader_module(const vk::raii::Device &dev,
                                                    const std::vector<char> &shader_code)
{
    vk::ShaderModuleCreateInfo create_info(
        {},
        shader_code.size() * sizeof(char),
        reinterpret_cast<const uint32_t *>(shader_code.data())
    );

    return vk::raii::ShaderModule(dev, create_info);
}

vk::SurfaceFormatKHR Engine::choose_swapchain_surface_format(const vk::raii::PhysicalDevice &pd,
                                                             const vk::raii::SurfaceKHR &surface)
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

vk::PresentModeKHR Engine::choose_swapchain_present_mode(const vk::raii::PhysicalDevice &pd,
                                                         const vk::raii::SurfaceKHR &surface)
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

vk::Extent2D Engine::choose_swapchain_extent(const vk::raii::PhysicalDevice &pd,
                                             const vk::raii::SurfaceKHR &surface,
                                             GLFWwindow *window)
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

uint32_t Engine::get_queue_family_index(const vk::raii::PhysicalDevice &pd,
                                        const vk::raii::SurfaceKHR &surface)
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
    std::vector<const char *>              req_extensions  = get_required_device_extensions();
    std::vector<vk::ExtensionProperties>   supp_extensions = pd.enumerateDeviceExtensionProperties();
    std::vector<vk::QueueFamilyProperties> queue_families  = pd.getQueueFamilyProperties();
    vk::PhysicalDeviceProperties           props           = pd.getProperties();
    vk::PhysicalDeviceFeatures             features        = pd.getFeatures();
    int                                    score           = 0;
    constexpr int                          NOT_SUTABLE     = -1;

    if (props.apiVersion < vk::ApiVersion13) {
        return NOT_SUTABLE;
    }

    if (not features.geometryShader) {
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
    if (CONFIG_VALIDATION_LAYERS) {
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
    const vk::DebugUtilsMessengerCallbackDataEXT *callback_data, void *)
{
    (void)type;

    if (severity >= vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo) {
        std::cerr << "validation layer" << " msg: " << callback_data->pMessage << std::endl;
    }
    return vk::False;
}
