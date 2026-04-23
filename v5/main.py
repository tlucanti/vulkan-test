
import vulkan as vk
import glfw

from typing import final

GLFWwindow = glfw._GLFWwindow

@final
class HelloTriangle:
    def __init__(self, width: int, height: int):
        self.window: GLFWwindow
        self.width = width
        self.height = height

        self._init_window()
        self._create_instance()

    def run(self):
        while not glfw.window_should_close(self.window):
            glfw.poll_events()

    def __del__(self):
        glfw.destroy_window(self.window)
        glfw.terminate()


    def _init_window(self):
        if not glfw.init():
            raise RuntimeError("can not init glfw")
        glfw.window_hint(glfw.CLIENT_API, glfw.NO_API)
        glfw.window_hint(glfw.RESIZABLE, glfw.FALSE)

        self.window = glfw.create_window(self.width, self.height, "vulkan", None, None)

    def _create_instance(self):
        appInfo = vk.VkApplicationInfo(
            pApplicationName="hello triangle",
            applicationVersion=vk.VK_MAKE_VERSION(1, 0, 0),
            pEngineName="no engine",
            engineVersion=vk.VK_MAKE_VERSION(1, 0, 0),
            apiVersion=vk.VK_MAKE_VERSION(1, 4, 0),
        )

        createInfo = vk.VkInstanceCreateInfo(
            pApplicationInfo = appInfo,
        )




def main():
    HelloTriangle(800, 600).run()

if __name__ == '__main__':
    main()
