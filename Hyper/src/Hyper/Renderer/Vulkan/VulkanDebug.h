#pragma once
#include <glm/vec4.hpp>
#include <vulkan/vulkan.hpp>

#include "Hyper/Renderer/RenderContext.h"

namespace Hyper::VkDebug
{
	VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);

	void Setup(RenderContext* renderContext);
	void FreeDebugCallback(vk::Instance instance);

	void SetObjectName(vk::Device device, vk::ObjectType type, void* handle, const std::string& name);

	void BeginRegion(vk::CommandBuffer commandBuffer, const char* markerName, const glm::vec4& color);
	void Insert(vk::CommandBuffer commandBuffer, const char* markerName, const glm::vec4& color);
	void EndRegion(vk::CommandBuffer commandBuffer);
}
