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
        void create_swapchain(void);
    void main_loop(void);
    void cleanup(void);

    // util functions
    static vk::SurfaceFormatKHR      choose_swapchain_surface_format(const vk::raii::PhysicalDevice &pd,
                                                                     const vk::raii::SurfaceKHR &surface);
    static vk::PresentModeKHR        choose_swapchain_present_mode(const vk::raii::PhysicalDevice &pd,
                                                                   const vk::raii::SurfaceKHR &surface);
    static vk::Extent2D              choose_swapchain_extent(const vk::raii::PhysicalDevice &pd,
                                                             const vk::raii::SurfaceKHR &surface,
                                                             GLFWwindow *window);
    static uint32_t                  get_queue_family_index(const vk::raii::PhysicalDevice &pd,
                                                            const vk::raii::SurfaceKHR &surface);
    static int                       get_physical_device_score(const vk::raii::PhysicalDevice &pd);
    static std::vector<const char *> get_required_device_extensions();
    static std::vector<const char *> get_required_instance_extensions();
    static vk::Bool32                debug_callback(vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
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

    vk::raii::Queue                  queue           = nullptr;

    vk::raii::SwapchainKHR           swapchain        = nullptr;
    std::vector<vk::Image>           swapchain_images;
    vk::SurfaceFormatKHR             swapchain_surface_foramt;
    vk::Extent2D                     swapchain_extent;

};

#endif /* ENGINE_HPP */
