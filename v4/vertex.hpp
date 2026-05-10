
#include "vulkan/vulkan.hpp"
#include <vulkan/vulkan.hpp>

#include <glm/glm.hpp>

#include <array>

struct Vertex {
    glm::vec2 pos;
    glm::vec2 tex_coord;

    static vk::VertexInputBindingDescription get_binding_description(void)
    {
        return vk::VertexInputBindingDescription(
            0,
            sizeof(Vertex),
            vk::VertexInputRate::eVertex
        );
    }

    static std::array<vk::VertexInputAttributeDescription, 2> get_attribute_descriptions(void)
    {
        return {
            vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, pos)),
            vk::VertexInputAttributeDescription(1, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, tex_coord)),
        };
    }
};

