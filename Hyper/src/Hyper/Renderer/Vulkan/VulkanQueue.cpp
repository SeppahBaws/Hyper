#include "HyperPCH.h"
#include "VulkanQueue.h"

#include "Hyper/Debug/Profiler.h"
#include "Hyper/Renderer/RenderContext.h"

#include <GFSDK_Aftermath_GpuCrashDump.h>
#include "NsightAftermath/NsightAftermathHelpers.h"

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

		vk::Result result = queue.submit(1, &info, fence);

		switch (result)
		{
		case vk::Result::eSuccess:
			break;
		case vk::Result::eErrorDeviceLost:
			{
				auto tdrTerminationTimeout = std::chrono::seconds(3);
				auto tStart = std::chrono::steady_clock::now();
				auto tElapsed = std::chrono::milliseconds::zero();

				GFSDK_Aftermath_CrashDump_Status status = GFSDK_Aftermath_CrashDump_Status_Unknown;
				AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_GetCrashDumpStatus(&status));

				while (status != GFSDK_Aftermath_CrashDump_Status_CollectingDataFailed &&
					status != GFSDK_Aftermath_CrashDump_Status_Finished &&
					tElapsed < tdrTerminationTimeout)
				{
					std::this_thread::sleep_for(std::chrono::milliseconds(50));
					AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_GetCrashDumpStatus(&status));

					auto tEnd = std::chrono::steady_clock::now();
					tElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(tEnd - tStart);
				}

				if (status != GFSDK_Aftermath_CrashDump_Status_Finished)
				{
					std::stringstream errMsg;
					errMsg << status;
					HPR_CORE_LOG_CRITICAL("Aftermath Error: Unexpected crash dump status: {}", errMsg.str());
				}
				// just quit on failure.
				exit(1);
				break;
			}
		default:
			throw std::runtime_error("Failed to submit to the graphics queue: "s + vk::to_string(result));
		}
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
