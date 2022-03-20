#pragma once
#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>

namespace Hyper
{
	struct Vertex
	{
		glm::vec3 pos;
		// glm::vec3 normal;
		// glm::vec3 texCoord;
		// glm::vec3 tangent;

		static vk::VertexInputBindingDescription GetBindingDescription()
		{
			vk::VertexInputBindingDescription binding{};
			binding.binding = 0;
			binding.stride = sizeof(Vertex);
			binding.inputRate = vk::VertexInputRate::eVertex;

			return binding;
		}

		static std::array<vk::VertexInputAttributeDescription, 1> GetAttributeDescriptions()
		{
			std::array<vk::VertexInputAttributeDescription, 1> attributes{};

			attributes[0].location = 0;
			attributes[0].binding = 0;
			attributes[0].format = vk::Format::eR32G32B32Sfloat;
			attributes[0].offset = offsetof(Vertex, pos);

			return attributes;
		}
	};
}
