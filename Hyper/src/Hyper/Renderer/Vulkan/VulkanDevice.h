#pragma once
#include <vulkan/vulkan.hpp>

#define WIN32_LEAN_AND_MEAN // VMA includes windows.h, so make sure that this is not enabled.
#include <vk_mem_alloc.h>

#include "Hyper/Renderer/RenderContext.h"

namespace Hyper
{
	class VulkanDevice final
	{
	public:
		VulkanDevice(RenderContext* pRenderCtx);
		~VulkanDevice();

		[[nodiscard]] std::vector<vk::CommandBuffer> GetCommandBuffers(u32 count);
		[[nodiscard]] vk::Queue GetGraphicsQueue() const { return m_GraphicsQueue; }

	private:
		[[nodiscard]] bool CheckExtensionsAndLayersSupport() const;

	private:
		const bool HYPER_VALIDATE = true;
		std::vector<const char*> m_RequiredInstanceLayerNames = {};
		std::vector<const char*> m_RequiredInstanceExtensionNames = {};
		std::vector<const char*> m_RequiredDeviceLayerNames = {};
		std::vector<const char*> m_RequiredDeviceExtensionNames = {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME
		};

		RenderContext* m_pRenderCtx;

		u32 m_GraphicsQueueFamilyIndex;
		vk::Queue m_GraphicsQueue;

		vk::CommandPool m_CommandPool;

		VmaAllocator m_Allocator;
	};
}
