#include "HyperPCH.h"
#include "VulkanUtility.h"

#include <magic_enum.hpp>

#ifdef HYPER_USE_AFTERMATH
#include <GFSDK_Aftermath_GpuCrashDump.h>
#include "NsightAftermath/NsightAftermathHelpers.h"
#endif

namespace Hyper
{
	namespace VulkanUtils
	{
		void VkCheck(const vk::Result& result)
		{
			switch (result)
			{
			case vk::Result::eSuccess:
				break;
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
				HPR_VKLOG_ERROR("Something went wrong! Expected vk::Result::eSuccess, instead got {}", magic_enum::enum_name(result));
			}
		}

		void VkCheck(const VkResult& result)
		{
			switch (result)
			{
			case VK_SUCCESS:
				break;
			default:
				HPR_VKLOG_ERROR("Something went wrong! Expected vk::Result::eSuccess, instead got {}", magic_enum::enum_name(result));
			}
		}
	}
}
