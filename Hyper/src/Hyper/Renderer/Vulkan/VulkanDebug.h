#pragma once
#include <vulkan/vulkan.hpp>

namespace Hyper
{
	namespace VkDebug
	{
		VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);

		void Setup(vk::Instance instance);

		void FreeDebugCallback(vk::Instance instance);
	}
}
