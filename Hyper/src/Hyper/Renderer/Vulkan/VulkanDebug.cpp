#include "HyperPCH.h"
#include "VulkanDebug.h"
#include <spdlog/fmt/fmt.h>

namespace Hyper
{
	namespace VkDebug
	{
		PFN_vkCreateDebugUtilsMessengerEXT pfnVkCreateDebugUtilsMessengerExt;
		PFN_vkDestroyDebugUtilsMessengerEXT pfnVkDestroyDebugUtilsMessengerExt;
		vk::DebugUtilsMessengerEXT debugUtilsMessenger;

		VKAPI_ATTR VkResult VKAPI_CALL vkCreateDebugUtilsMessengerEXT(VkInstance instance,
			const VkDebugUtilsMessengerCreateInfoEXT*
			pCreateInfo,
			const VkAllocationCallbacks* pAllocator,
			VkDebugUtilsMessengerEXT* pMessenger)
		{
			return pfnVkCreateDebugUtilsMessengerExt(instance, pCreateInfo, pAllocator, pMessenger);
		}

		VKAPI_ATTR void VKAPI_CALL vkDestroyDebugUtilsMessenger(VkInstance instance, VkDebugUtilsMessengerEXT messenger,
			VkAllocationCallbacks const* pAllocator)
		{
			return pfnVkDestroyDebugUtilsMessengerExt(instance, messenger, pAllocator);
		}

		VkBool32 DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
			VkDebugUtilsMessageTypeFlagsEXT messageType,
			const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void*)
		{
			std::string debugMessage{};

			const std::string severity = vk::to_string(
				static_cast<vk::DebugUtilsMessageSeverityFlagBitsEXT>(messageSeverity));
			const std::string type = vk::to_string(static_cast<vk::DebugUtilsMessageTypeFlagsEXT>(messageType));
			debugMessage += fmt::format("{}: {}:\n", severity, type);


			const std::string messageIdName = pCallbackData->pMessageIdName;
			const i32 messageIdNumber = pCallbackData->messageIdNumber;
			const std::string message = pCallbackData->pMessage;
			debugMessage += fmt::format("\tmessageIDName   = <{}>\n\tmessageIdNumber = {}\n\tmessage         = <{}>\n",
				messageIdName, messageIdNumber, message);

			if (pCallbackData->queueLabelCount > 0)
			{
				debugMessage += fmt::format("\tQueue Labels:\n");
				for (u32 i = 0; i < pCallbackData->queueLabelCount; i++)
				{
					debugMessage += fmt::format("\t\tlabelName = <{}>\n", pCallbackData->pQueueLabels[i].pLabelName);
				}
			}

			if (pCallbackData->cmdBufLabelCount > 0)
			{
				debugMessage += fmt::format("\tCommandBuffer Labels:\n");
				for (u32 i = 0; i < pCallbackData->cmdBufLabelCount; i++)
				{
					debugMessage += fmt::format("\t\tlabelName = <{}>\n", pCallbackData->pCmdBufLabels[i].pLabelName);
				}
			}

			if (pCallbackData->objectCount > 0)
			{
				debugMessage += fmt::format("\tObjects:\n");

				for (u32 i = 0; i < pCallbackData->objectCount; i++)
				{
					const std::string objectType = vk::to_string(
						static_cast<vk::ObjectType>(pCallbackData->pObjects[i].objectType));

					debugMessage += fmt::format("\t\tObject {}\n", i);
					debugMessage += fmt::format("\t\t\tobjectType   = {}\n", objectType);
					debugMessage += fmt::format("\t\t\tobjectHandle = {}\n", pCallbackData->pObjects[i].objectHandle);
					if (pCallbackData->pObjects[i].pObjectName)
					{
						debugMessage += fmt::format("\t\t\tobjectName = <{}>\n",
							pCallbackData->pObjects[i].pObjectName);
					}
				}
			}

			switch (messageSeverity)
			{
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
				HPR_VKLOG_TRACE(debugMessage);
				break;
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
				HPR_VKLOG_INFO(debugMessage);
				break;
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
				HPR_VKLOG_WARN(debugMessage);
				break;
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
				HPR_VKLOG_ERROR(debugMessage);
				break;
			default:;
			}

			return false;
		}

		void Setup(RenderContext* pRenderContext)
		{
			pfnVkCreateDebugUtilsMessengerExt = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(pRenderContext->
				instance.getProcAddr("vkCreateDebugUtilsMessengerEXT"));
			if (!pfnVkCreateDebugUtilsMessengerExt)
			{
				HPR_VKLOG_ERROR("GetInstanceProcAdd: Unable to find PfnVkCreateDebugUtilsMessengerExt function.");
				return;
			}

			pfnVkDestroyDebugUtilsMessengerExt = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(pRenderContext->
				instance.getProcAddr("vkDestroyDebugUtilsMessengerEXT"));
			if (!pfnVkDestroyDebugUtilsMessengerExt)
			{
				HPR_VKLOG_ERROR("GetInstanceProcAdd: Unable to find PfnVkDestroyDebugUtilsMessengerExt function.");
				return;
			}

			const vk::DebugUtilsMessageSeverityFlagsEXT severityFlags{
				vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
				vk::DebugUtilsMessageSeverityFlagBitsEXT::eError |
				vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo
			};

			const vk::DebugUtilsMessageTypeFlagsEXT messageTypeFlags{
				vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
				vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
				vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation
			};

			const vk::DebugUtilsMessengerCreateInfoEXT createInfo{
				{}, severityFlags, messageTypeFlags, &DebugCallback
			};
			pfnVkCreateDebugUtilsMessengerExt(pRenderContext->instance,
				reinterpret_cast<const VkDebugUtilsMessengerCreateInfoEXT*>(&createInfo),
				nullptr,
				reinterpret_cast<VkDebugUtilsMessengerEXT*>(&debugUtilsMessenger));
		}

		void FreeDebugCallback(vk::Instance instance)
		{
			pfnVkDestroyDebugUtilsMessengerExt(
				instance, static_cast<VkDebugUtilsMessengerEXT>(debugUtilsMessenger), nullptr);
		}
	}
}
