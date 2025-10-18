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

#include <string>
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
        void create_image_views(void);
        void create_graphics_pipeline(void);
        void create_command_pool(void);
    void main_loop(void);
    void cleanup(void);

    // util functions
    [[nodiscard]]
    static vk::raii::ShaderModule    create_shader_module(const vk::raii::Device &dev,
                                                          const std::vector<char> &shader_code);
    [[nodiscard]]
    static vk::SurfaceFormatKHR      choose_swapchain_surface_format(const vk::raii::PhysicalDevice &pd,
                                                                     const vk::raii::SurfaceKHR &surface);
    [[nodiscard]]
    static vk::PresentModeKHR        choose_swapchain_present_mode(const vk::raii::PhysicalDevice &pd,
                                                                   const vk::raii::SurfaceKHR &surface);
    [[nodiscard]]
    static vk::Extent2D              choose_swapchain_extent(const vk::raii::PhysicalDevice &pd,
                                                             const vk::raii::SurfaceKHR &surface,
                                                             GLFWwindow *window);
    [[nodiscard]]
    static uint32_t                  get_queue_family_index(const vk::raii::PhysicalDevice &pd,
                                                            const vk::raii::SurfaceKHR &surface);
    [[nodiscard]]
    static int                       get_physical_device_score(const vk::raii::PhysicalDevice &pd);
    [[nodiscard]]
    static std::vector<const char *> get_required_device_extensions();
    [[nodiscard]]
    static std::vector<const char *> get_required_instance_extensions();
    [[nodiscard]]
    static vk::Bool32                debug_callback(vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
                                                    vk::DebugUtilsMessageTypeFlagsEXT type,
                                                    const vk::DebugUtilsMessengerCallbackDataEXT *callback_data,
                                                    void *);
    [[nodiscard]]
    static std::vector<char>         read_file(const std::string &fname);

private:
    GLFWwindow                       *window         = nullptr;

    vk::raii::Context                context;
    vk::raii::Instance               instance        = nullptr;
    vk::raii::DebugUtilsMessengerEXT debug_messenger = nullptr;
    vk::raii::SurfaceKHR             surface         = nullptr;

    vk::raii::PhysicalDevice         physical_device = nullptr;
    vk::raii::Device                 device          = nullptr;

    vk::raii::Queue                  queue           = nullptr;

    vk::raii::SwapchainKHR           swapchain       = nullptr;
    vk::SurfaceFormatKHR             swapchain_surface_foramt;
    vk::Extent2D                     swapchain_extent;

    std::vector<vk::Image>           swapchain_images;
    std::vector<vk::raii::ImageView> swapchain_image_views;

    vk::raii::PipelineLayout         pipeline_layout = nullptr;
    vk::raii::Pipeline               pipeline        = nullptr;

    vk::raii::CommandPool            command_pool    = nullptr;
};

#endif /* ENGINE_HPP */
