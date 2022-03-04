#include "HyperPCH.h"
#include "VulkanUtility.h"

#include <magic_enum.hpp>

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
