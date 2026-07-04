
#ifndef VERTEX_HPP
#define VERTEX_HPP

#include "vulkan/vulkan.hpp"
#include <vulkan/vulkan.hpp>

#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>

#include <array>
#include <bit>

struct Vertex {
    alignas(16) glm::vec3 pos;
    alignas(8)  glm::vec2 tex_coord;

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
            vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, pos)),
            vk::VertexInputAttributeDescription(1, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, tex_coord)),
        };
    }

    bool operator ==(const Vertex &other) const {
        return pos == other.pos && tex_coord == other.tex_coord;
    }
};

namespace std {
    template<> struct hash<Vertex> {
        size_t operator()(const Vertex &vertex) const {
            return std::bit_cast<uint32_t>(vertex.pos.x) ^
                   std::bit_cast<uint32_t>(vertex.pos.y) ^
                   std::bit_cast<uint32_t>(vertex.pos.z) ^
                   std::bit_cast<uint32_t>(vertex.tex_coord.x) ^
                   std::bit_cast<uint32_t>(vertex.tex_coord.y);
        }
    };
}

#endif /* VERTEX_HPP */
