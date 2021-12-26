#pragma once
#include <vulkan/vulkan.hpp>
#include <magic_enum.hpp>

namespace Hyper
{
	namespace VulkanUtils
	{
		void VkCheck(const vk::Result& result);
	}
}