
#ifndef VKCOMMON_HPP
#define VKCOMMON_HPP

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <stdexcept>

__attribute__((__noreturn__))
static inline void PANIC_VK(const std::string &message)
{
	throw std::runtime_error(message);
}

static inline void CALL_VK(VkResult result, std::string message)
{
	if (result != VK_SUCCESS) {
		message += " (errorcode: " + std::to_string(result) + ")";
		PANIC_VK(message);
	}
}

#include <vector>
static const std::vector<const char *> validationLayers = {
	"VK_LAYER_KHRONOS_validation",
};

namespace Color {
	static inline const char *blue   = "";
	static inline const char *yellow = "";
	static inline const char *red    = "";
	static inline const char *reset  = "";
}

#endif /* VKCOMMON_HPP */
