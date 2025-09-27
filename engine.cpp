
#include <GLFW/glfw3.h>

#include <stdexcept>
#include <iostream>
#include <vector>
#include <cstring>
#include <algorithm>

#ifdef __CLANGD__
#undef VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#undef VULKAN_HPP_DISABLE_ENHANCED_MODE
#undef VULKAN_HPP_NO_STRUCT_SETTERS
#include "vulkan/vulkan.cppm"
#define __CLANGD_NO_ENGINE_HPP__
#else
import vulkan_hpp;
#endif

#include "engine.hpp"

using namespace std::string_literals;

constexpr uint32_t WIDTH = 800;
constexpr uint32_t HEIGHT = 600;

const std::vector<const char *> validation_layers = {
    "VK_LAYER_KHRONOS_validation",
};

#if CONFIG_VALIDATION_LAYERS
constexpr bool enable_validation_layers = true;
#else
constexpr bool enable_validation_layers = false;
#endif

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

    window = glfwCreateWindow(WIDTH, HEIGHT, "vulkan", nullptr, nullptr);
}

void Engine::init_vulkan(void)
{
    create_instance();
    setup_debug_messanger();
    pick_physical_device();
}

void Engine::create_instance(void)
{
    std::vector<const char *>req_extensions = get_required_extensions();
    std::vector<vk::ExtensionProperties> supp_extensions = context.enumerateInstanceExtensionProperties();
    std::vector<vk::LayerProperties> supp_layers = context.enumerateInstanceLayerProperties();

    std::cout << "Avaliable extensions:\n";
    for (const vk::ExtensionProperties &ext : supp_extensions) {
        std::cout << '\t' << ext.extensionName << '\n';
    }

    for (const char *req_ext : req_extensions) {
        if (std::ranges::none_of(supp_extensions,
                                 [req_ext](const auto &supp_ext) {
                                    return strcmp(supp_ext.extensionName, req_ext) == 0;
                                 })) {
            throw std::runtime_error("required GLFW extension not supported: "s + req_ext);
        }
    }

    for (const char *req_layer : validation_layers) {
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
    vk::InstanceCreateInfo create_info({}, &app_info, validation_layers, req_extensions);
    instance = vk::raii::Instance(context, create_info);
}

void Engine::setup_debug_messanger(void)
{
    vk::DebugUtilsMessageSeverityFlagsEXT severity_flags(vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
                                                         vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                                                         vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);

    vk::DebugUtilsMessageTypeFlagsEXT message_type_flags(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                                                         vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
                                                         vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);

    vk::DebugUtilsMessengerCreateInfoEXT create_info({}, severity_flags, message_type_flags, &debug_callback);

    debug_messenger = instance.createDebugUtilsMessengerEXT(create_info);
}

void Engine::pick_physical_device(void)
{
    std::vector<vk::raii::PhysicalDevice> devices = instance.enumeratePhysicalDevices();
}

void Engine::main_loop(void)
{
    while (not glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }
}

void Engine::cleanup(void)
{
    glfwDestroyWindow(window);
    glfwTerminate();
}

// ====================================================================================================================
// util functions

std::vector<const char *> Engine::get_required_extensions()
{
    uint32_t glfw_extension_count = 0;
    const char **glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

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
    if (severity >= vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose) {
        std::cerr << "validation layer" << " msg: " << callback_data->pMessage << std::endl;
    }
    return vk::False;
}
