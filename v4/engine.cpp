
#include "config.h"
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_raii.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <algorithm>
#include <chrono>
#include <cstring>
#include <iostream>
#include <map>
#include <stdexcept>
#include <vector>

#include "engine.hpp"
#include "vertex.hpp"

using namespace std::string_literals;

static const std::vector<const char *> g_validation_layers = {
#if CONFIG_VK_VALIDATION_LAYERS
    "VK_LAYER_KHRONOS_validation",
#endif
};

struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

static const std::vector<Vertex> g_vertices = {
    {{-0.5f, -0.5f}, {1.0f, 0.0f}},
    {{ 0.5f, -0.5f}, {0.0f, 0.0f}},
    {{ 0.5f,  0.5f}, {0.0f, 1.0f}},
    {{-0.5f,  0.5f}, {1.0f, 1.0f}}
};

static const std::vector<uint16_t> g_indices = {
    0, 1, 2, 2, 3, 0,
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
    if (CONFIG_WINDOW_RESIZABLE) {
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    } else {
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    }

    this->window = glfwCreateWindow(
        CONFIG_WINDOW_WIDTH,
        CONFIG_WINDOW_HEIGHT,
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
    create_descriptor_set_layout();
    create_graphics_pipeline();
    create_command_pool();
    create_texture_image();
    create_texture_image_view();
    create_texture_sampler();
    create_vertex_buffer();
    create_index_buffer();
    create_uniform_buffers();
    create_command_buffers();
    create_descriptor_pool();
    create_descriptor_sets();
    create_sync_objects();
    create_swapchain_sync_objects();
}

void Engine::create_instance(void)
{
    std::vector<const char *>            req_extensions  = get_required_instance_extensions();
    std::vector<vk::ExtensionProperties> supp_extensions = this->context.enumerateInstanceExtensionProperties();
    std::vector<vk::LayerProperties>     supp_layers     = this->context.enumerateInstanceLayerProperties();


    if (CONFIG_DEBUG_VERBOSE) {
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
    if (not CONFIG_VK_VALIDATION_LAYERS) {
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

    if (CONFIG_DEBUG_VERBOSE) {
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
    if (CONFIG_DEBUG_VERBOSE) {
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
        vk::PhysicalDeviceFeatures2()
            .features.setSamplerAnisotropy(true),
        vk::PhysicalDeviceVulkan11Features()
            .setShaderDrawParameters(true),
        vk::PhysicalDeviceVulkan13Features()
            .setSynchronization2(true)
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
        queue_create_info,
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

    if (CONFIG_DEBUG_VERBOSE) {
        std::cout << "current window size:";
        std::cout << this->swapchain_extent.width << 'x' << this->swapchain_extent.height << '\n';
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

void Engine::create_descriptor_set_layout(void)
{
    vk::DescriptorSetLayoutBinding ubo_binding(
        0,
        vk::DescriptorType::eUniformBuffer,
        1,
        vk::ShaderStageFlagBits::eVertex
    );

    vk::DescriptorSetLayoutBinding sampler_binding(
        1,
        vk::DescriptorType::eCombinedImageSampler,
        1,
        vk::ShaderStageFlagBits::eFragment
    );

    std::vector<vk::DescriptorSetLayoutBinding> bindings = {
        ubo_binding,
        sampler_binding,
    };

    vk::DescriptorSetLayoutCreateInfo layout_info(
        {},
        bindings
    );
    this->descriptor_layout = vk::raii::DescriptorSetLayout(this->device, layout_info);
}

void Engine::create_graphics_pipeline(void)
{
    vk::raii::ShaderModule shader_module = create_shader_module(
        this->device,
        read_file(CONFIG_SHADER_SPV_PATH)
    );

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
    auto binding_description = Vertex::get_binding_description();
    auto attribute_descriptions = Vertex::get_attribute_descriptions();
    vk::PipelineVertexInputStateCreateInfo vertex_input(
        {},
        binding_description,
        attribute_descriptions
    );

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
        vk::FrontFace::eCounterClockwise,
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
        color_blend_attachment
    );

    // dynamic states
    std::vector<vk::DynamicState> dynamic;
    if (CONFIG_WINDOW_RESIZABLE) {
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
    vk::PipelineLayoutCreateInfo pipeline_layout_create_info(
        {},
        *this->descriptor_layout,
        {}
    );
    this->pipeline_layout = vk::raii::PipelineLayout(
        this->device,
        pipeline_layout_create_info
    );

    // pipeline rendering
    vk::PipelineRenderingCreateInfo pipeline_rendering(
        {},
        swapchain_surface_foramt.format
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

void Engine::create_texture_image(void)
{
    int width, height, channels;

    stbi_uc *pixels = stbi_load(
        CONFIG_TEXTURE_PATH,
        &width,
        &height,
        &channels,
        STBI_rgb_alpha
    );
    if (pixels == nullptr) {
        throw std::runtime_error("failed to load texture at path "s + CONFIG_TEXTURE_PATH);
    }

    vk::DeviceSize size = width * height * 4;

    auto [staging_buffer, staging_buffer_mem] = create_buffer(
        size,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
    );

    void *ptr = staging_buffer_mem.mapMemory(0, size);
    memcpy(ptr, pixels, size);
    staging_buffer_mem.unmapMemory();

    stbi_image_free(pixels);

    std::tie(this->texture_image, this->texture_image_mem) = create_image(
        width,
        height,
        vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
        vk::MemoryPropertyFlagBits::eDeviceLocal
    );

    vk::raii::CommandBuffer cb = begin_single_time_commands(this->command_pool);
    transition_image_layout(
        cb,
        this->texture_image,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eTransferDstOptimal,
        {},
        vk::AccessFlagBits2::eTransferWrite,
        vk::PipelineStageFlagBits2::eTopOfPipe,
        vk::PipelineStageFlagBits2::eTransfer
    );

    copy_buffer_to_image(cb, this->texture_image, staging_buffer, width, height);

    transition_image_layout(
        cb,
        this->texture_image,
        vk::ImageLayout::eTransferDstOptimal,
        vk::ImageLayout::eShaderReadOnlyOptimal,
        vk::AccessFlagBits2::eTransferWrite,
        vk::AccessFlagBits2::eShaderRead,
        vk::PipelineStageFlagBits2::eTransfer,
        vk::PipelineStageFlagBits2::eFragmentShader
    );

    end_single_time_commands(std::move(cb));
}

void Engine::create_texture_image_view(void)
{
    this->texture_image_view = create_image_view(this->texture_image, vk::Format::eR8G8B8A8Srgb);
}

void Engine::create_texture_sampler(void)
{
    vk::PhysicalDeviceProperties properties = this->physical_device.getProperties();

    vk::SamplerCreateInfo sampler_info(
        {},
        vk::Filter::eLinear,
        vk::Filter::eLinear,
        vk::SamplerMipmapMode::eLinear,
        vk::SamplerAddressMode::eRepeat,
        vk::SamplerAddressMode::eRepeat,
        vk::SamplerAddressMode::eRepeat,
        0,
        vk::True,
        properties.limits.maxSamplerAnisotropy,
        vk::False,
        vk::CompareOp::eNever,
        0,
        0,
        vk::BorderColor::eIntOpaqueBlack,
        vk::False
    );

    this->texture_sampler = vk::raii::Sampler(this->device, sampler_info);
}

void Engine::create_vertex_buffer(void)
{
    vk::DeviceSize size = sizeof(g_vertices[0]) * g_vertices.size();

    auto [staging_buffer, staging_buffer_mem] = create_buffer(
        size,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
    );

    void *ptr = staging_buffer_mem.mapMemory(0, size);
    memcpy(ptr, g_vertices.data(), size);
    staging_buffer_mem.unmapMemory();

    std::tie(this->vertex_buffer, this->vertex_buffer_mem) = create_buffer(
        size,
        vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eDeviceLocal
    );

    copy_buffer(this->vertex_buffer, staging_buffer, size);
}

void Engine::create_index_buffer(void)
{
    vk::DeviceSize size = sizeof(g_indices[0]) * g_indices.size();

    auto [staging_buffer, staging_buffer_mem] = create_buffer(
        size,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
    );

    void *ptr = staging_buffer_mem.mapMemory(0, size);
    memcpy(ptr, g_indices.data(), size);
    staging_buffer_mem.unmapMemory();

    std::tie(this->index_buffer, this->index_buffer_mem) = create_buffer(
        size,
        vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eDeviceLocal
    );

    copy_buffer(this->index_buffer, staging_buffer, size);
}

void Engine::create_uniform_buffers(void)
{
    this->uniform_buffers.clear();

    for (int i = 0; i < CONFIG_VK_MAX_FRAMES_IN_FLIGHT; i++) {
        vk::DeviceSize size = sizeof(UniformBufferObject);

        auto [buffer, buffer_mem] = create_buffer(
            size,
            vk::BufferUsageFlagBits::eUniformBuffer,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
        );

        this->uniform_buffers_map.emplace_back(std::move(buffer_mem.mapMemory(0, size)));
        this->uniform_buffers.emplace_back(std::move(buffer));
        this->uniform_buffers_mem.emplace_back(std::move(buffer_mem));
    }
}

void Engine::create_command_pool(void)
{
    vk::CommandPoolCreateInfo create_info(
        vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        this->queue_index
    );

    this->command_pool = vk::raii::CommandPool(this->device, create_info);
}

void Engine::create_command_buffers(void)
{
    vk::CommandBufferAllocateInfo allocate_info(
        this->command_pool,
        vk::CommandBufferLevel::ePrimary,
        CONFIG_VK_MAX_FRAMES_IN_FLIGHT
    );

    this->command_buffers = vk::raii::CommandBuffers(this->device, allocate_info);
}

void Engine::record_command_buffer(uint32_t image_index, uint32_t frame_index)
{
    vk::raii::CommandBuffer &cb = this->command_buffers.at(frame_index);

    // begin command buffer
    cb.begin({});

    // transition the swapchain image to COLOR_ATTACHMENT_OPTIMAL
    transition_image_layout(
        cb,
        this->swapchain_images.at(image_index),
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eColorAttachmentOptimal,
        {},
        vk::AccessFlagBits2::eColorAttachmentWrite,
        vk::PipelineStageFlagBits2::eTopOfPipe,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput
    );

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
        attachment_info
    );
    cb.beginRendering(rendering_info);

    // bind pipeline
    cb.bindPipeline(
        vk::PipelineBindPoint::eGraphics,
        this->pipeline
    );

    // bind vertex buffer
    cb.bindVertexBuffers(
        0,
        { this->vertex_buffer },
        { 0 }
    );

    // bind index buffer
    cb.bindIndexBuffer(
        this->index_buffer,
        0,
        vk::IndexType::eUint16
    );

    // bind descriptor sets
    cb.bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics,
        this->pipeline_layout,
        0,
        *this->descriptor_sets.at(frame_index),
        {}
    );

    // set dynamic states
    cb.setViewport(
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
    cb.setScissor(
        0,
        vk::Rect2D(vk::Offset2D(0, 0), this->swapchain_extent)
    );

    // draw
    cb.drawIndexed(
        g_indices.size(),
        1,
        0,
        0,
        0
    );

    // end rendering
    cb.endRendering();

    // transition the swapchain image to PRESENT_SRC
    transition_image_layout(
        cb,
        this->swapchain_images.at(image_index),
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::ImageLayout::ePresentSrcKHR,
        vk::AccessFlagBits2::eColorAttachmentWrite,
        {},
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::PipelineStageFlagBits2::eBottomOfPipe
    );

    // end command buffer
    cb.end();
}

void Engine::create_descriptor_pool(void)
{
    vk::DescriptorPoolSize uniform_pool_size(
        vk::DescriptorType::eUniformBuffer,
        CONFIG_VK_MAX_FRAMES_IN_FLIGHT
    );

    vk::DescriptorPoolSize sampler_pool_size(
        vk::DescriptorType::eCombinedImageSampler,
        CONFIG_VK_MAX_FRAMES_IN_FLIGHT
    );

    std::vector<vk::DescriptorPoolSize> pool_size = {
        uniform_pool_size,
        sampler_pool_size,
    };

    vk::DescriptorPoolCreateInfo pool_info(
        vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        CONFIG_VK_MAX_FRAMES_IN_FLIGHT,
        pool_size
    );

    this->descriptor_pool = vk::raii::DescriptorPool(this->device, pool_info);
}

void Engine::create_descriptor_sets(void)
{
    std::vector<vk::DescriptorSetLayout> layouts(
        CONFIG_VK_MAX_FRAMES_IN_FLIGHT,
        *this->descriptor_layout
    );

    vk::DescriptorSetAllocateInfo set_info(
        this->descriptor_pool,
        layouts
    );

    this->descriptor_sets = this->device.allocateDescriptorSets(set_info);

    for (int i = 0; i < CONFIG_VK_MAX_FRAMES_IN_FLIGHT; i++) {
        vk::DescriptorBufferInfo buffer_info(
            this->uniform_buffers.at(i),
            0,
            sizeof(UniformBufferObject)
        );

        vk::WriteDescriptorSet buffer_write(
            this->descriptor_sets.at(i),
            0,
            0,
            vk::DescriptorType::eUniformBuffer,
            {},
            buffer_info
        );

        vk::DescriptorImageInfo image_info(
            this->texture_sampler,
            this->texture_image_view,
            vk::ImageLayout::eShaderReadOnlyOptimal
        );

        vk::WriteDescriptorSet image_write(
            this->descriptor_sets.at(i),
            1,
            0,
            vk::DescriptorType::eCombinedImageSampler,
            image_info,
            {}
        );

        std::vector<vk::WriteDescriptorSet> descriptor_writes = {
            buffer_write,
            image_write
        };

        this->device.updateDescriptorSets(descriptor_writes, {});
    }
}

void Engine::create_sync_objects(void)
{
    for (int i = 0; i < CONFIG_VK_MAX_FRAMES_IN_FLIGHT; i++) {
        this->present_complete.push_back(
            vk::raii::Semaphore(this->device, vk::SemaphoreCreateInfo())
        );
        this->render_finished.push_back(
            vk::raii::Semaphore(this->device, vk::SemaphoreCreateInfo())
        );
        this->frame_finished.push_back(
            vk::raii::Fence(this->device, vk::FenceCreateInfo())
        );
    }
}

void Engine::create_swapchain_sync_objects(void)
{
    this->render_finished.clear();
    this->render_finished.reserve(this->swapchain_images.size());

    for (size_t i = 0; i < this->swapchain_images.size(); i++) {
        this->render_finished.emplace_back(
            vk::raii::Semaphore(this->device, vk::SemaphoreCreateInfo())
        );
    }
}

void Engine::recreate_swapchain(void)
{
    this->device.waitIdle();

    cleanup_swapchain();

    create_swapchain();
    create_image_views();
    create_swapchain_sync_objects();
}

void Engine::cleanup_swapchain(void)
{
    this->swapchain_image_views.clear();
    this->swapchain = nullptr;
}

void Engine::main_loop(void)
{
    uint32_t current_frame = 0;

    while (not glfwWindowShouldClose(this->window)) {
        glfwPollEvents();
        draw_frame(current_frame);
        current_frame = (current_frame + 1) % CONFIG_VK_MAX_FRAMES_IN_FLIGHT;
    }

    this->device.waitIdle();
}

void Engine::update_uniform_buffer(int frame_idx)
{
    static auto start = std::chrono::high_resolution_clock::now();

    auto now = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float>(now - start).count();

    UniformBufferObject ubo;
    ubo.model = glm::rotate(
        glm::mat4(1.0f),
        time * glm::radians(90.0f),
        glm::vec3(0.0f, 0.0f, 1.0f)
    );
    ubo.view = glm::lookAt(
        glm::vec3(2.0f, 2.0f, 2.0f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 1.0f)
    );
    ubo.proj = glm::perspective(
        glm::radians(45.0f),
        static_cast<float>(this->swapchain_extent.width) / this->swapchain_extent.height,
        0.1f,
        10.0f
    );
    ubo.proj[1][1] *= -1;

    memcpy(this->uniform_buffers_map.at(frame_idx), &ubo, sizeof(UniformBufferObject));
}

void Engine::draw_frame(int frame_idx)
{
    update_uniform_buffer(frame_idx);

    auto [result, image_index] = this->swapchain.acquireNextImage(
        UINT64_MAX,
        present_complete.at(frame_idx),
        nullptr
    );
    record_command_buffer(image_index, frame_idx);

    vk::PipelineStageFlags stage_flags(vk::PipelineStageFlagBits::eColorAttachmentOutput);
    vk::SubmitInfo submit_info(
        *present_complete.at(frame_idx),
        stage_flags,
        *this->command_buffers.at(frame_idx),
        *render_finished.at(image_index)
    );
    this->queue.submit(submit_info, *frame_finished.at(frame_idx));

    while (this->device.waitForFences({ frame_finished.at(frame_idx) },
                                      true,
                                      UINT64_MAX) ==
           vk::Result::eTimeout) {
        /* do nothing */
    }
    this->device.resetFences({ frame_finished.at(frame_idx) });

    vk::PresentInfoKHR present_info(
        *render_finished.at(image_index),
        *this->swapchain,
        image_index
    );

    result = this->queue.presentKHR(present_info);
    if (CONFIG_DEBUG_VERBOSE) {
        switch (result) {
        case vk::Result::eSuccess:
            break;
        case vk::Result::eSuboptimalKHR:
            std::cout << "vk::Queue::PresentKHR: eSuboptimalKHR\n";
            recreate_swapchain();
            break;
        default:
            std::cout << "vk::Queue::PresentKHR returned unexpected result: " << result << '\n';
            break;
        }
    }
}

void Engine::cleanup(void)
{
    cleanup_swapchain();

    glfwDestroyWindow(this->window);
    glfwTerminate();
}
