#include "HyperPCH.h"
#include "VulkanQueue.h"

#include "Hyper/Debug/Profiler.h"
#include "VulkanUtility.h"

namespace Hyper
{
	void VulkanQueue::Submit(const std::vector<vk::PipelineStageFlags>& waitStages,
		const std::vector<vk::Semaphore>& waitSemaphores, const std::vector<vk::Semaphore>& signalSemaphores,
		vk::CommandBuffer cmd, vk::Fence fence)
	{
		HPR_PROFILE_SCOPE();

		vk::SubmitInfo info = {};
		info.setWaitDstStageMask(waitStages);
		info.setWaitSemaphores(waitSemaphores);
		info.setSignalSemaphores(signalSemaphores);
		info.setCommandBuffers(cmd);

		VulkanUtils::Check(queue.submit(1, &info, fence));
	}

	void VulkanQueue::WaitIdle()
	{
		HPR_PROFILE_SCOPE();

		queue.waitIdle();
	}

	vk::Result VulkanQueue::Present(const std::vector<vk::Semaphore>& waitSemaphores, const std::vector<u32>& imageIndices,
	                                const std::vector<vk::SwapchainKHR>& swapchains)
	{
		HPR_PROFILE_SCOPE();

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
