#pragma once
#include "Hyper/Renderer/Base/RenderDevice.h"

#include <vulkan/vulkan.hpp>

namespace Hyper
{
	class VulkanRenderDevice final : public RenderDevice
	{
	public:
		VulkanRenderDevice();
		~VulkanRenderDevice() override;

	private:
		void CreateInstance();
		[[nodiscard]] bool CheckExtensionsAndLayersSupport() const;
		void CreateDevice();
		void CreateCommandPool();

	private:
		const bool HYPER_VALIDATE = true;
		std::vector<const char*> m_RequiredExtensionNames = {};
		std::vector<const char*> m_RequiredLayerNames = {};

		vk::Instance m_Instance;
		vk::Device m_Device;
		vk::PhysicalDevice m_PhysicalDevice;
		u32 m_GraphicsQueueFamilyIndex;
		vk::Queue m_GraphicsQueue;

		vk::CommandPool m_CommandPool;
	};
}
