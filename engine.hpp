#ifndef ENGINE_HPP
#define ENGINE_HPP

#if defined(__CLANGD__)
# if !defined(__CLANGD_NO_ENGINE_HPP__)
#  include <vulkan/vulkan_raii.hpp>
# endif
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
        void create_surface();
        void pick_physical_device(void);
        void create_logical_device(void);
    void main_loop(void);
    void cleanup(void);

    // util functions
    static std::pair<uint32_t, uint32_t> get_graphics_present_family_indexes(const vk::raii::PhysicalDevice &pd,
                                                                             const vk::raii::SurfaceKHR &surface);
    static int                           get_physical_device_score(const vk::raii::PhysicalDevice &pd);
    static std::vector<const char *>     get_required_device_extensions();
    static std::vector<const char *>     get_required_instance_extensions();
    static vk::Bool32                    debug_callback(vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
                                                        vk::DebugUtilsMessageTypeFlagsEXT type,
                                                        const vk::DebugUtilsMessengerCallbackDataEXT *callback_data,
                                                        void *);

private:
    GLFWwindow                       *window;
    vk::raii::Context                context;
    vk::raii::Instance               instance        = nullptr;
    vk::raii::DebugUtilsMessengerEXT debug_messenger = nullptr;
    vk::raii::SurfaceKHR             surface         = nullptr;
    vk::raii::PhysicalDevice         physical_device = nullptr;
    vk::raii::Device                 device          = nullptr;
    vk::raii::Queue                  graphics_queue  = nullptr;
    vk::raii::Queue                  present_queue   = nullptr;

};

#endif /* ENGINE_HPP */
