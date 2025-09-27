
#ifdef __CLANGD__
#undef VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#undef VULKAN_HPP_DISABLE_ENHANCED_MODE
#undef VULKAN_HPP_NO_STRUCT_SETTERS
#include "vulkan/vulkan.cppm"
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

};
