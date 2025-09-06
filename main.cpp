
#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS

#ifdef __CLANGD__
#include <utility>
#include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#endif

#include <GLFW/glfw3.h>

#include <stdexcept>
#include <iostream>
#include <vector>
#include <cstring>

using namespace std::string_literals;

class Engine {
public:
    void run(void)
    {
        init_window();
        init_vulkan();
        main_loop();
        cleanup();
    }

private:
    GLFWwindow *window;
    vk::raii::Context  context;
    vk::raii::Instance instance = nullptr;

    void init_window(void)
    {
        constexpr uint32_t WIDTH = 800;
        constexpr uint32_t HEIGHT = 600;

        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        window = glfwCreateWindow(WIDTH, HEIGHT, "vulkan", nullptr, nullptr);
    }

    void init_vulkan(void)
    {
        create_instance();
    }

    void create_instance(void)
    {
        uint32_t glfw_extension_count = 0;
        const char **glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
        std::vector<vk::ExtensionProperties> extention_properties = context.enumerateInstanceExtensionProperties();

        std::cout << "Avaliable extensions:\n";
        for (const vk::ExtensionProperties &ext : extention_properties) {
            std::cout << '\t' << ext.extensionName << '\n';
        }

        for (uint32_t i = 0; i < glfw_extension_count; i++) {
            bool found = false;

            for (const vk::ExtensionProperties &ext : extention_properties) {
                if (strcmp(ext.extensionName, glfw_extensions[i]) == 0) {
                    found = true;
                    break;
                }
            }

            if (not found) {
                throw std::runtime_error("required GLFW extension not supported: "s + glfw_extensions[i]);
            }
        }

        constexpr vk::ApplicationInfo app_info = {
            .pApplicationName = "vulkan",
            .applicationVersion = 1,
            .pEngineName = "No engine",
            .engineVersion = 1,
            .apiVersion = vk::ApiVersion14,
        };

        vk::InstanceCreateInfo create_info = {
            .pApplicationInfo = &app_info,
            .enabledExtensionCount = glfw_extension_count,
            .ppEnabledExtensionNames = glfw_extensions,
        };

        instance = vk::raii::Instance(context, create_info);
    }

    void main_loop(void)
    {
        while (not glfwWindowShouldClose(window)) {
            glfwPollEvents();
        }
    }

    void cleanup(void)
    {
        glfwDestroyWindow(window);
        glfwTerminate();
    }
};

int main()
{
    Engine engine;

    try {
        engine.run();
    } catch (const std::exception &e) {
        std::cout << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
