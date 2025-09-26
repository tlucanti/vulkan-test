
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
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        window = glfwCreateWindow(WIDTH, HEIGHT, "vulkan", nullptr, nullptr);
    }

    void init_vulkan(void)
    {
        create_instance();
    }

    std::vector<const char *> get_required_extensions()
    {
        uint32_t glfw_extension_count = 0;
        const char **glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

        std::vector extensions(glfw_extensions, glfw_extensions + glfw_extension_count);
        if (CONFIG_VALIDATION_LAYERS) {
            extensions.push_back(vk::EXTDebugUtilsExtensionName);
        }

        return extensions;
    }

    void create_instance(void)
    {
        std::vector<const char *>req_extensions = get_required_extensions();
        std::vector<vk::ExtensionProperties> supp_extensions = context.enumerateInstanceExtensionProperties();
        std::vector<vk::LayerProperties> supp_layers = context.enumerateInstanceLayerProperties();

        std::cout << "Avaliable extensions:\n";
        for (const vk::ExtensionProperties &ext : supp_extensions) {
            std::cout << '\t' << ext.extensionName << '\n';
        }

        for (const char *req_ext : req_extensions) {
            bool found = false;

            for (const vk::ExtensionProperties &supp_ext : supp_extensions) {
                if (strcmp(supp_ext.extensionName, req_ext) == 0) {
                    found = true;
                    break;
                }
            }

            if (not found) {
                throw std::runtime_error("required GLFW extension not supported: "s + req_ext);
            }
        }

        for (const char *req_layer : validation_layers) {
            bool found = false;

            for (const vk::LayerProperties &supp_layer : supp_layers) {
                if (strcmp(supp_layer.layerName, req_layer) == 0) {
                    found = true;
                    break;
                }
            }

            if (not found) {
                throw std::runtime_error("required validation layer not suported: "s + req_layer);
            }
        }

        constexpr vk::ApplicationInfo app_info("vulkan", 1, "No engine", 1, vk::ApiVersion14);
        vk::InstanceCreateInfo create_info({}, &app_info, validation_layers, req_extensions);
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
