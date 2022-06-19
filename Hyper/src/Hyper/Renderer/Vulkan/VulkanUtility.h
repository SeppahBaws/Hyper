#pragma once
#include <vulkan/vulkan.hpp>

#include "NsightAftermath/NsightAftermathHelpers.h"

namespace Hyper
{
	class VulkanCommandPool;

	namespace VulkanUtils
	{
		static bool CheckResult(vk::Result result)
		{
			switch (result)
			{
			case vk::Result::eSuccess:
				return true;
#ifdef HYPER_USE_AFTERMATH
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
			}
#endif
			default:
				break;
			}

			return false;
		}

		template<typename T>
		T Check(const vk::ResultValue<T>& result)
		{
			if (CheckResult(result.result))
			{
				return result.value;
			}

			HPR_VKLOG_ERROR("Something went wrong! Expected vk::Result::eSuccess, instead got {}", vk::to_string(result.result));
			throw std::runtime_error("Something went wrong! Check the latest error message");
		}

		inline void Check(const vk::Result& result)
		{
			if (CheckResult(result))
			{
				return;
			}

			HPR_VKLOG_ERROR("Something went wrong! Expected vk::Result::eSuccess, instead got {}", vk::to_string(result));
			throw std::runtime_error("Something went wrong! Check the latest error message");
		}

		inline void Check(const VkResult& result)
		{
			auto vkResult = static_cast<vk::Result>(result);
			if (CheckResult(vkResult))
			{
				return;
			}

			HPR_VKLOG_ERROR("Something went wrong! Expected vk::Result::eSuccess, instead got {}", vk::to_string(vkResult));
			throw std::runtime_error("Something went wrong! Check the latest error message");
		}

		void ImageBarrier();
	}
}
