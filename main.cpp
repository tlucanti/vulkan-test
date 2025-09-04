
#ifdef __CLANGD__
#include <vulkan/vulkan.hpp>
#else
import vulkan_hpp;
#endif

#include <stdexcept>
#include <iostream>

#include <GLFW/glfw3.h>

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
