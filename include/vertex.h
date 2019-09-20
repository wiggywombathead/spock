#ifndef _VERTEX_H
#define _VERTEX_H

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

struct Vertex {
	static const uint32_t attributeCount = 3;

	glm::vec3 position;
	glm::vec3 color;
	glm::vec2 texCoord;

	static VkVertexInputBindingDescription getInputBindingDescription() {
		VkVertexInputBindingDescription bindingDescription = {};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		
		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions = {};

		/* position */
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].binding  = 0;
		attributeDescriptions[0].format   = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset   = offsetof(Vertex, position);

		/* color */
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].binding  = 0;
		attributeDescriptions[1].format   = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset   = offsetof(Vertex, color);

		/* texture coordinate */
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].binding  = 0;
		attributeDescriptions[2].format   = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[2].offset   = offsetof(Vertex, texCoord);

		return attributeDescriptions;
	}

};

#endif
