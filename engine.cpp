
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

#ifdef __CLANGD__
# undef VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
# undef VULKAN_HPP_DISABLE_ENHANCED_MODE
# undef VULKAN_HPP_NO_STRUCT_SETTERS
# include "vulkan/vulkan.cppm"
# define __CLANGD_NO_ENGINE_HPP__
#else
  import vulkan_hpp;
#endif

#include "engine.hpp"

using namespace std::string_literals;

constexpr uint32_t WIDTH = 800;
constexpr uint32_t HEIGHT = 600;
constexpr const char *SHADER_SPV_PATH = "./shader.spv";

const std::vector<const char *> g_validation_layers = {
#if CONFIG_VALIDATION_LAYERS
    "VK_LAYER_KHRONOS_validation",
#endif
};

void Engine::run(void)
{
    init_window();
    init_vulkan();
    main_loop();
    cleanup();
}

void Engine::init_window(void)
{
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    this->window = glfwCreateWindow(
        WIDTH,
        HEIGHT,
        "vulkan",
        nullptr,
        nullptr
    );
}

void Engine::init_vulkan(void)
{
    create_instance();
    setup_debug_messanger();
    create_surface();
    pick_physical_device();
    create_logical_device();
    create_swapchain();
    create_image_views();
    create_graphics_pipeline();
    create_command_pool();
    create_command_buffer();

}

void Engine::create_instance(void)
{
    std::vector<const char *>            req_extensions  = get_required_instance_extensions();
    std::vector<vk::ExtensionProperties> supp_extensions = this->context.enumerateInstanceExtensionProperties();
    std::vector<vk::LayerProperties>     supp_layers     = this->context.enumerateInstanceLayerProperties();


    if (CONFIG_VERBOSE) {
        std::cout << "Avaliable extensions:\n";
        for (const vk::ExtensionProperties &ext : supp_extensions) {
            std::cout << '\t' << ext.extensionName << '\n';
        }
        std::cout << '\n';
    }

    for (const char *req_ext : req_extensions) {
        if (std::ranges::none_of(supp_extensions,
                                 [req_ext](const auto &supp_ext) {
                                    return strcmp(supp_ext.extensionName, req_ext) == 0;
                                 })) {
            throw std::runtime_error("required GLFW extension not supported: "s + req_ext);
        }
    }

    for (const char *req_layer : g_validation_layers) {
        if (std::ranges::none_of(supp_layers,
                                 [req_layer](const auto &supp_layer) {
                                    return strcmp(supp_layer.layerName, req_layer) == 0;
                                 })) {
            throw std::runtime_error("required validation layer not suported: "s + req_layer);
        }
    }

    const vk::ApplicationInfo app_info(
        "vulkan",
        vk::makeVersion(1, 0, 0),
        "No engine",
        vk::makeVersion(1, 0, 0),
        vk::ApiVersion14
    );
    vk::InstanceCreateInfo create_info(
        {},
        &app_info,
        g_validation_layers,
        req_extensions
    );

    this->instance = vk::raii::Instance(this->context, create_info);
}

void Engine::setup_debug_messanger(void)
{
    if (not CONFIG_VALIDATION_LAYERS) {
        return;
    }

    vk::DebugUtilsMessageSeverityFlagsEXT severity_flags(
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eError
    );
    vk::DebugUtilsMessageTypeFlagsEXT message_type_flags(
        vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
        vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
        vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation
    );

    vk::DebugUtilsMessengerCreateInfoEXT create_info(
        {},
        severity_flags,
        message_type_flags,
        &debug_callback
    );

    this->debug_messenger = this->instance.createDebugUtilsMessengerEXT(create_info);
}

void Engine::create_surface(void)
{
    VkSurfaceKHR csurface;

    if (glfwCreateWindowSurface(*this->instance,
                                this->window,
                                nullptr,
                                &csurface)) {
        throw std::runtime_error("faied to create window surface");
    }

    this->surface = vk::raii::SurfaceKHR(this->instance, csurface);
}

void Engine::pick_physical_device(void)
{
    std::vector<vk::raii::PhysicalDevice>        devices = this->instance.enumeratePhysicalDevices();
    std::multimap<int, vk::raii::PhysicalDevice> candidates;

    if (devices.empty()) {
        throw std::runtime_error("no GPU devices with vulkan support");
    }

    if (CONFIG_VERBOSE) {
        std::cout << "Avaliable physical devices:\n";
        for (const vk::raii::PhysicalDevice &pd : devices) {
            vk::PhysicalDeviceProperties props = pd.getProperties();
            std::cout << '\t' << props.deviceName << '\n';

        }
        std::cout << '\n';
    }

    for (const vk::raii::PhysicalDevice &pd : devices) {
        candidates.insert({get_physical_device_score(pd), pd});
    }

    if (candidates.empty() || candidates.rbegin()->first < 0) {
        throw std::runtime_error("failed to find sutable GPU");
    }

    this->physical_device = candidates.rbegin()->second;
    if (CONFIG_VERBOSE) {
        std::cout << "Selected device: " << this->physical_device.getProperties().deviceName
                  << " (with score " << candidates.rbegin()->first << ")\n";
    }
}

void Engine::create_logical_device(void)
{
    std::vector<const char *> extensions     = get_required_device_extensions();
    std::vector<float>        priority       = { 1.0f };

    this->queue_index = get_queue_family_index(this->physical_device, this->surface);

    vk::StructureChain<vk::PhysicalDeviceFeatures2,
                       vk::PhysicalDeviceVulkan11Features,
                       vk::PhysicalDeviceVulkan13Features,
                       vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT> feature_chain = {
        vk::PhysicalDeviceFeatures2(),
        vk::PhysicalDeviceVulkan11Features()
            .setShaderDrawParameters(true),
        vk::PhysicalDeviceVulkan13Features()
            .setDynamicRendering(true),
        vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT()
            .setExtendedDynamicState(true),
    };

    vk::DeviceQueueCreateInfo queue_create_info(
        {},
        this->queue_index,
        priority
    );
    vk::DeviceCreateInfo create_info(
        {},
        { queue_create_info },
        {},
        extensions,
        {},
        &feature_chain.get<vk::PhysicalDeviceFeatures2>()
    );

    this->device = vk::raii::Device(this->physical_device, create_info);
    this->queue = vk::raii::Queue(this->device, this->queue_index, 0);
}

void Engine::create_swapchain(void)
{
    vk::SurfaceCapabilitiesKHR surface_capabilities;
    uint32_t                   image_count;

    surface_capabilities = this->physical_device.getSurfaceCapabilitiesKHR(this->surface);
    this->swapchain_surface_foramt = choose_swapchain_surface_format(this->physical_device, this->surface);
    this->swapchain_extent = choose_swapchain_extent(this->physical_device, this->surface, this->window);

    image_count = surface_capabilities.minImageCount + 1;
    if (surface_capabilities.maxImageCount) {
        image_count = std::min(image_count, surface_capabilities.maxImageCount);
    }

    vk::SwapchainCreateInfoKHR create_info(
        {},
        surface,
        image_count,
        this->swapchain_surface_foramt.format,
        this->swapchain_surface_foramt.colorSpace,
        this->swapchain_extent,
        1,
        vk::ImageUsageFlagBits::eColorAttachment,
        vk::SharingMode::eExclusive,
        {},
        surface_capabilities.currentTransform,
        vk::CompositeAlphaFlagBitsKHR::eOpaque,
        choose_swapchain_present_mode(this->physical_device, this->surface),
        true
    );

    this->swapchain = vk::raii::SwapchainKHR(device, create_info);
    this->swapchain_images = this->swapchain.getImages();
}

void Engine::create_image_views(void)
{
    vk::ImageViewCreateInfo create_info(
        {},
        {},
        vk::ImageViewType::e2D,
        this->swapchain_surface_foramt.format,
        {},
        vk::ImageSubresourceRange(
            vk::ImageAspectFlagBits::eColor,
            0,
            1,
            0,
            1
        )
    );

    for (const vk::Image &image: this->swapchain_images) {
        create_info.image = image;
        this->swapchain_image_views.emplace_back(this->device, create_info);
    }
}

void Engine::create_graphics_pipeline(void)
{
    vk::raii::ShaderModule shader_module = create_shader_module(this->device, read_file(SHADER_SPV_PATH));

    // shader stages
    vk::PipelineShaderStageCreateInfo vert_create_info(
        {},
        vk::ShaderStageFlagBits::eVertex,
        shader_module,
        "vert_main"
    );
    vk::PipelineShaderStageCreateInfo frag_create_info(
        {},
        vk::ShaderStageFlagBits::eFragment,
        shader_module,
        "frag_main"
    );
    vk::PipelineShaderStageCreateInfo shader_stages[] = {
        vert_create_info,
        frag_create_info
    };

    // vertex input state
    vk::PipelineVertexInputStateCreateInfo vertex_input;

    // input assembly state
    vk::PipelineInputAssemblyStateCreateInfo assembler(
        {},
        vk::PrimitiveTopology::eTriangleList
    );

    // viewport state
    vk::PipelineViewportStateCreateInfo viewport(
        {},
        1,
        {},
        1
    );

    // rasterization state
    vk::PipelineRasterizationStateCreateInfo rasterizer(
        {},
        vk::False,
        vk::False,
        vk::PolygonMode::eFill,
        vk::CullModeFlagBits::eBack,
        vk::FrontFace::eClockwise,
        vk::False,
        0,
        0,
        0,
        1
    );

    // multisampling state
    vk::PipelineMultisampleStateCreateInfo multisampler(
        {},
        vk::SampleCountFlagBits::e1,
        vk::False
    );

    // depth-stencil state
    // (skipped)

    // color blending state
    auto color_blend_attachment =
        vk::PipelineColorBlendAttachmentState (vk::False)
        .setColorWriteMask(
            vk::ColorComponentFlagBits::eR |
            vk::ColorComponentFlagBits::eG |
            vk::ColorComponentFlagBits::eB |
            vk::ColorComponentFlagBits::eA
        );
    vk::PipelineColorBlendStateCreateInfo color_blender(
        {},
        vk::False,
        vk::LogicOp::eCopy,
        { color_blend_attachment }
    );

    // dynamic states
    std::vector<vk::DynamicState> dynamic;
    if (CONFIG_RESIZABLE) {
        dynamic = {
            vk::DynamicState::eViewport,
            vk::DynamicState::eScissor,
        };
    }
    vk::PipelineDynamicStateCreateInfo dynamic_states(
        {},
        dynamic
    );

    // pipeline layout
    vk::PipelineLayoutCreateInfo pipeline_layout_create_info;
    this->pipeline_layout = vk::raii::PipelineLayout(this->device, pipeline_layout_create_info);

    // pipeline rendering
    vk::PipelineRenderingCreateInfo pipeline_rendering(
        {},
        { swapchain_surface_foramt.format }
    );

    // graphics pipeline
    vk::GraphicsPipelineCreateInfo pipeline_create_info(
        {},
        shader_stages,
        &vertex_input,
        &assembler,
        {},
        &viewport,
        &rasterizer,
        &multisampler,
        {},
        &color_blender,
        &dynamic_states,
        this->pipeline_layout,
        {},
        {},
        {},
        {},
        &pipeline_rendering
    );

    this->pipeline = vk::raii::Pipeline(this->device, nullptr, pipeline_create_info);
}

void Engine::create_command_pool(void)
{
    vk::CommandPoolCreateInfo create_info(
        vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        this->queue_index
    );

    this->command_pool = vk::raii::CommandPool(this->device, create_info);
}

void Engine::create_command_buffer(void)
{
    vk::CommandBufferAllocateInfo allocate_info(
        this->command_pool,
        vk::CommandBufferLevel::ePrimary,
        1
    );

    this->command_buffer = std::move(
        vk::raii::CommandBuffers(this->device, allocate_info).front()
    );
}

void Engine::record_command_buffer(uint32_t image_index)
{
    // begin command buffer
    this->command_buffer.begin({});

    // transition the swapchain image to COLOR_ATTACHMENT_OPTIMAL
    vk::DependencyInfo to_color_attachment = transition_image_layout(
        this->swapchain_images.at(image_index),
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eColorAttachmentOptimal,
        {},
        vk::AccessFlagBits2::eColorAttachmentWrite,
        vk::PipelineStageFlagBits2::eTopOfPipe,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput
    );
    this->command_buffer.pipelineBarrier2(to_color_attachment);

    // start dynamic rendering
    vk::RenderingAttachmentInfo attachment_info(
        this->swapchain_image_views.at(image_index),
        vk::ImageLayout::eColorAttachmentOptimal,
        {},
        {},
        {},
        vk::AttachmentLoadOp::eClear,
        vk::AttachmentStoreOp::eStore,
        vk::ClearColorValue(0, 0, 0, 1)
    );
    vk::RenderingInfo rendering_info(
        {},
        vk::Rect2D({ 0, 0 }, this->swapchain_extent),
        1,
        {},
        { attachment_info }
    );
    this->command_buffer.beginRendering(rendering_info);

    // bind pipeline
    this->command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, this->pipeline);

    // set dynamic states
    this->command_buffer.setViewport(
        0,
        vk::Viewport(
            0,
            0,
            this->swapchain_extent.width,
            this->swapchain_extent.height,
            0,
            1
        )
    );
    this->command_buffer.setScissor(
        0,
        vk::Rect2D(vk::Offset2D(0, 0), this->swapchain_extent)
    );

    // draw
    this->command_buffer.draw(3, 1, 0, 0);

    // end rendering
    this->command_buffer.endRendering();

    // transition the swapchain image to PRESENT_SRC
    vk::DependencyInfo to_present_src = transition_image_layout(
        this->swapchain_images.at(image_index),
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::ImageLayout::ePresentSrcKHR,
        vk::AccessFlagBits2::eColorAttachmentWrite,
        {},
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::PipelineStageFlagBits2::eBottomOfPipe
    );
    this->command_buffer.pipelineBarrier2(to_present_src);

    // end command buffer
    this->command_buffer.end();
}

void Engine::main_loop(void)
{
    while (not glfwWindowShouldClose(this->window)) {
        glfwPollEvents();
        draw_frame();
    }
}

void Engine::draw_frame(void)
{
}

void Engine::cleanup(void)
{
    glfwDestroyWindow(this->window);
    glfwTerminate();
}

// ====================================================================================================================
// util functions

vk::DependencyInfo Engine::transition_image_layout(const vk::Image &current_frame,
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

    return dependency_info;
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
