#pragma once
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

#include "Hyper/Renderer/Vulkan/VulkanQueue.h"
#include "Hyper/Renderer/Vulkan/VulkanCommands.h"

namespace Hyper
{
	struct RenderContext
	{
		vk::Instance instance;
		vk::PhysicalDevice physicalDevice;
		vk::Device device;
		VulkanQueue graphicsQueue;
		VulkanCommandPool* commandPool;
		VmaAllocator allocator;
		u32 imagesInFlight;

		vk::Format imageFormat;
		vk::Extent2D imageExtent;
	};
}
