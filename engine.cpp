
#include <GLFW/glfw3.h>

#include <algorithm>
#include <cstring>
#include <iostream>
#include <map>
#include <stdexcept>
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

    this->window = glfwCreateWindow(WIDTH, HEIGHT, "vulkan", nullptr, nullptr);
}

void Engine::init_vulkan(void)
{
    create_instance();
    setup_debug_messanger();
    pick_physical_device();
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

    const vk::ApplicationInfo app_info("vulkan",
                                       vk::makeVersion(1, 0, 0),
                                       "No engine",
                                       vk::makeVersion(1, 0, 0),
                                       vk::ApiVersion14);

    vk::InstanceCreateInfo create_info({},
                                       &app_info,
                                       g_validation_layers,
                                       req_extensions);

    this->instance = vk::raii::Instance(this->context, create_info);
}

void Engine::setup_debug_messanger(void)
{
    if (not CONFIG_VALIDATION_LAYERS) {
        return;
    }

    vk::DebugUtilsMessageSeverityFlagsEXT severity_flags(vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
                                                         vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                                                         vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);

    vk::DebugUtilsMessageTypeFlagsEXT message_type_flags(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                                                         vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
                                                         vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);

    vk::DebugUtilsMessengerCreateInfoEXT create_info({},
                                                     severity_flags,
                                                     message_type_flags,
                                                     &debug_callback);

    this->debug_messenger = this->instance.createDebugUtilsMessengerEXT(create_info);
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
        vk::PhysicalDeviceProperties props = pd.getProperties();
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
    uint32_t                  graphics_index = get_graphics_family_index(this->physical_device);
    std::vector<const char *> extensions     = get_required_device_extensions();
    std::vector<float>        priority       = { 1.0f };

    vk::StructureChain<vk::PhysicalDeviceFeatures2,
                       vk::PhysicalDeviceVulkan13Features,
                       vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT> feature_chain = {
        vk::PhysicalDeviceFeatures2(),
        vk::PhysicalDeviceVulkan13Features().setDynamicRendering(true),
        vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT().setExtendedDynamicState(true),
    };

    vk::DeviceQueueCreateInfo queue_create_info({},
                                                graphics_index,
                                                priority);

    std::vector<vk::DeviceQueueCreateInfo> queue_infos = { queue_create_info };

    vk::DeviceCreateInfo create_info({},
                                     queue_infos,
                                     {},
                                     extensions,
                                     {},
                                     &feature_chain.get<vk::PhysicalDeviceFeatures2>());

    this->device = vk::raii::Device(this->physical_device, create_info);
    this->graphics_queue = vk::raii::Queue(this->device, graphics_index, 0);
}

void Engine::main_loop(void)
{
    while (not glfwWindowShouldClose(this->window)) {
        glfwPollEvents();
    }
}

void Engine::cleanup(void)
{
    glfwDestroyWindow(this->window);
    glfwTerminate();
}

// ====================================================================================================================
// util functions

uint32_t Engine::get_graphics_family_index(const vk::raii::PhysicalDevice &pd)
{
    std::vector<vk::QueueFamilyProperties> props = pd.getQueueFamilyProperties();
    uint32_t                               idx   = 0;

    for (const vk::QueueFamilyProperties &qfp : props) {
        if ((qfp.queueFlags & vk::QueueFlagBits::eGraphics) != vk::QueueFlagBits{}) {
            return idx;
        }
        idx++;
    }

    throw std::runtime_error("no graphics queue family found");
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

vk::Bool32 Engine::debug_callback(
    vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
    vk::DebugUtilsMessageTypeFlagsEXT type,
    const vk::DebugUtilsMessengerCallbackDataEXT *callback_data, void *)
{
    if (severity >= vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo) {
        std::cerr << "validation layer" << " msg: " << callback_data->pMessage << std::endl;
    }
    return vk::False;
}
