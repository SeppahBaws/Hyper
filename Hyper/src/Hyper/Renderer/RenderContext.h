#pragma once
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

#include "Hyper/Renderer/Vulkan/VulkanQueue.h"

namespace Hyper
{
	struct RenderContext
	{
		vk::Instance instance;
		vk::PhysicalDevice physicalDevice;
		vk::Device device;
		VulkanQueue graphicsQueue;
		VmaAllocator allocator;

		vk::Format imageFormat;
		vk::Extent2D imageExtent;
	};
}
