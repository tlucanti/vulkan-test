
import vulkan
import glfw

from typing import final

@final
class HelloTriangle:
    def __init__(self, width, height):
        self.window = None
        self.width = width
        self.height = height
        self._init_window()

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



def main():
    HelloTriangle(800, 600).run()

if __name__ == '__main__':
    main()
