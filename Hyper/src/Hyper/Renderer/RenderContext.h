#pragma once
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

namespace Hyper
{
	struct RenderContext
	{
		vk::Instance instance;
		vk::PhysicalDevice physicalDevice;
		vk::Device device;
		VmaAllocator allocator;
	};
}
