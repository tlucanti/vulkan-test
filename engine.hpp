#ifndef ENGINE_HPP
#define ENGINE_HPP

#if defined(__CLANGD__) && !defined(__CLANGD_NO_ENGINE_HPP__)
#include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#endif

#include <GLFW/glfw3.h>

#include <vector>

class Engine {
public:
    void run(void);

private:
    // run functions
    void init_window(void);
    void init_vulkan(void);
        // init_vulkan functions
        void create_instance(void);
        void setup_debug_messanger(void);
        void pick_physical_device(void);
    void main_loop(void);
    void cleanup(void);

    // util functions
    static std::vector<const char *> get_required_extensions();

    static vk::Bool32 debug_callback(
        vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
        vk::DebugUtilsMessageTypeFlagsEXT type,
        const vk::DebugUtilsMessengerCallbackDataEXT *callback_data, void *);

private:
    GLFWwindow *window;
    vk::raii::Context  context;
    vk::raii::Instance instance = nullptr;
    vk::raii::DebugUtilsMessengerEXT debug_messenger = nullptr;
    vk::raii::PhysicalDevice physical_device = nullptr;

};

#endif /* ENGINE_HPP */
