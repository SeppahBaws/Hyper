#pragma once
#include <vulkan/vulkan.hpp>

#include "Hyper/Renderer/RenderContext.h"

namespace Hyper
{
	namespace VkDebug
	{
		VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);

		void Setup(RenderContext* renderContext);

		void FreeDebugCallback(vk::Instance instance);
	}
}
