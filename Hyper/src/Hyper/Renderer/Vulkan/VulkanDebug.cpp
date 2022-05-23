#include "HyperPCH.h"
#include "VulkanDebug.h"
#include <spdlog/fmt/fmt.h>

namespace Hyper::VkDebug
{
	static bool g_VkDebugEnabled = false;

	static PFN_vkCreateDebugUtilsMessengerEXT pfnVkCreateDebugUtilsMessengerExt;
	static PFN_vkDestroyDebugUtilsMessengerEXT pfnVkDestroyDebugUtilsMessengerExt;
	static PFN_vkSetDebugUtilsObjectNameEXT pfnVkSetDebugUtilsObjectNameEXT;
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

	VKAPI_ATTR VkResult VKAPI_CALL vkSetDebugUtilsObjectNameEXT(VkDevice device, VkDebugUtilsObjectNameInfoEXT* pNameInfo)
	{
		return pfnVkSetDebugUtilsObjectNameEXT(device, pNameInfo);
	}

	VkBool32 DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void*)
	{
		const std::string severity = vk::to_string(static_cast<vk::DebugUtilsMessageSeverityFlagBitsEXT>(messageSeverity));
		const std::string type = vk::to_string(static_cast<vk::DebugUtilsMessageTypeFlagsEXT>(messageType));
		const std::string message = pCallbackData->pMessage;

		switch (messageSeverity)
		{
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
			HPR_VKLOG_TRACE("{} {} {}", severity, type, message);
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
			HPR_VKLOG_INFO("{} {} {}", severity, type, message);
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
			HPR_VKLOG_WARN("{} {} {}", severity, type, message);
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
			HPR_VKLOG_ERROR("{} {} {}", severity, type, message);
			break;
		default: ;
		}

		return false;
	}

	void Setup(RenderContext* pRenderContext)
	{
		pfnVkCreateDebugUtilsMessengerExt = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(pRenderContext->instance.getProcAddr("vkCreateDebugUtilsMessengerEXT"));
		if (!pfnVkCreateDebugUtilsMessengerExt)
		{
			HPR_VKLOG_ERROR("GetInstanceProcAdd: Unable to find PfnVkCreateDebugUtilsMessengerExt function.");
			return;
		}

		pfnVkDestroyDebugUtilsMessengerExt = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(pRenderContext->instance.
			getProcAddr("vkDestroyDebugUtilsMessengerEXT"));
		if (!pfnVkDestroyDebugUtilsMessengerExt)
		{
			HPR_VKLOG_ERROR("GetInstanceProcAdd: Unable to find PfnVkDestroyDebugUtilsMessengerExt function.");
			return;
		}

		pfnVkSetDebugUtilsObjectNameEXT = reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(pRenderContext->instance.getProcAddr("vkSetDebugUtilsObjectNameEXT"));
		if (!pfnVkSetDebugUtilsObjectNameEXT)
		{
			HPR_VKLOG_ERROR("GetInstanceProcAdd: Unable to find PfnVkSetDebugUtilsObjectNameEXT function.");
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

		g_VkDebugEnabled = true;
	}

	void FreeDebugCallback(vk::Instance instance)
	{
		pfnVkDestroyDebugUtilsMessengerExt(
			instance, static_cast<VkDebugUtilsMessengerEXT>(debugUtilsMessenger), nullptr);

		g_VkDebugEnabled = false;
	}

	void SetObjectName(vk::Device device, vk::ObjectType type, void* handle, const std::string& name)
	{
		if (!g_VkDebugEnabled)
			return;

		VkDebugUtilsObjectNameInfoEXT nameInfo{ VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT };
		nameInfo.objectType = static_cast<VkObjectType>(type);
		nameInfo.objectHandle = reinterpret_cast<u64>(handle);
		nameInfo.pObjectName = name.c_str();

		vkSetDebugUtilsObjectNameEXT(device, &nameInfo);
	}
}
