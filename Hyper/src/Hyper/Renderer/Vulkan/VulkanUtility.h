#pragma once
#include <vulkan/vulkan.hpp>

namespace Hyper
{
	class VulkanCommandPool;

	namespace VulkanUtils
	{
		void VkCheck(const vk::Result& result);
		void VkCheck(const VkResult& result);

		void ImageBarrier();
	}
}
