#pragma once
#include <vulkan/vulkan.hpp>

#include "Hyper/Debug/Profiler.h"

namespace Hyper
{
	void InsertImageMemoryBarrier(
		vk::CommandBuffer cmd,
		vk::Image image,
		vk::AccessFlagBits srcAccessMask,
		vk::AccessFlagBits dstAccessMask,
		vk::ImageLayout oldLayout,
		vk::ImageLayout newLayout,
		vk::PipelineStageFlags srcStageMask,
		vk::PipelineStageFlags dstStageMask,
		vk::ImageSubresourceRange subresourceRange)
	{
		HPR_PROFILE_SCOPE();

		vk::ImageMemoryBarrier barrier = {};
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.srcAccessMask = srcAccessMask;
		barrier.dstAccessMask = dstAccessMask;
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;
		barrier.image = image;
		barrier.subresourceRange = subresourceRange;

		cmd.pipelineBarrier(
			srcStageMask, dstStageMask,
			{},
			0, nullptr,
			0, nullptr,
			1, &barrier);
	}
}
