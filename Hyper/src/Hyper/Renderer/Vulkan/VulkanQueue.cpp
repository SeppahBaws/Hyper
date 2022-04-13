#include "HyperPCH.h"
#include "VulkanQueue.h"

#include "Hyper/Renderer/RenderContext.h"

namespace Hyper
{
	void VulkanQueue::Submit(const std::vector<vk::PipelineStageFlags>& waitStages,
		const std::vector<vk::Semaphore>& waitSemaphores, const std::vector<vk::Semaphore>& signalSemaphores,
		vk::CommandBuffer cmd, vk::Fence fence)
	{
		vk::SubmitInfo info = {};
		info.setWaitDstStageMask(waitStages);
		info.setWaitSemaphores(waitSemaphores);
		info.setSignalSemaphores(signalSemaphores);
		info.setCommandBuffers(cmd);

		try
		{
			queue.submit(info, fence);
		}
		catch (vk::SystemError& e)
		{
			throw std::runtime_error("Failed to submit to the graphics queue: "s + e.what());
		}
	}

	void VulkanQueue::WaitIdle()
	{
		queue.waitIdle();
	}

	vk::Result VulkanQueue::Present(const std::vector<vk::Semaphore>& waitSemaphores, const std::vector<u32>& imageIndices,
	                                const std::vector<vk::SwapchainKHR>& swapchains)
	{
		vk::PresentInfoKHR info = {};
		info.setWaitSemaphores(waitSemaphores);
		info.setResults(nullptr);
		info.setImageIndices(imageIndices);
		info.setSwapchains(swapchains);

		vk::Result result = vk::Result::eSuccess;

		try
		{
			result = queue.presentKHR(info);
		}
		catch (...)
		{
			// Do nothing, we will forward our result code to the outside
		}

		return result;
	}
}
