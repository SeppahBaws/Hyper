#pragma once
#include <vulkan/vulkan.hpp>

namespace Hyper
{
	struct RenderContext;

	struct VulkanQueue
	{
		vk::Queue queue{};
		u32 familyIndex{};
		vk::QueueFlags flags{};

		void Submit(const std::vector<vk::PipelineStageFlags>& waitStages, const std::vector<vk::Semaphore>& waitSemaphores,
			const std::vector<vk::Semaphore>& signalSemaphores, vk::CommandBuffer cmd, vk::Fence fence);
		vk::Result Present(const std::vector<vk::Semaphore>& waitSemaphores, const std::vector<u32>& imageIndices,
			const std::vector<vk::SwapchainKHR>& swapchains);
	};
}
