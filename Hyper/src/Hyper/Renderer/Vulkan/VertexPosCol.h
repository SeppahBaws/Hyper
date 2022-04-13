#pragma once
#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>

namespace Hyper
{
	struct VertexPosCol
	{
		glm::vec3 position;
		glm::vec3 color;
		// glm::vec3 normal;
		// glm::vec3 texCoord;
		// glm::vec3 tangent;

		static vk::VertexInputBindingDescription GetBindingDescription()
		{
			vk::VertexInputBindingDescription binding{};
			binding.binding = 0;
			binding.stride = sizeof(VertexPosCol);
			binding.inputRate = vk::VertexInputRate::eVertex;

			return binding;
		}

		static std::array<vk::VertexInputAttributeDescription, 2> GetAttributeDescriptions()
		{
			std::array<vk::VertexInputAttributeDescription, 2> attributes{};

			attributes[0].location = 0;
			attributes[0].binding = 0;
			attributes[0].format = vk::Format::eR32G32B32Sfloat;
			attributes[0].offset = offsetof(VertexPosCol, position);

			attributes[1].location = 1;
			attributes[1].binding = 0;
			attributes[1].format = vk::Format::eR32G32B32Sfloat;
			attributes[1].offset = offsetof(VertexPosCol, color);

			return attributes;
		}
	};
}
