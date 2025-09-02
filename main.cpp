
import vulkan_hpp;

#include <stdexcept>
#include <iostream>

class Engine {
public:
    void run(void)
    {
        init_vulkan();
        main_loop();
        cleanup();
    }

private:
    void init_vulkan(void)
    {

    }

    void main_loop(void)
    {

    }

    void cleanup(void)
    {

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
