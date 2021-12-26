#pragma once
#include "Hyper/Renderer/Base/RenderDevice.h"

#include <vulkan/vulkan.hpp>

namespace Hyper
{
	class VulkanRenderDevice final : public RenderDevice
	{
	public:
		VulkanRenderDevice();
		~VulkanRenderDevice();

	private:
		void CreateInstance();
		bool CheckValidationLayerSupport() const;
		void PickDevice();

	private:
		const bool HYPER_VALIDATE = true;
		const std::vector<const char*> m_ValidationLayers = {
			"VK_LAYER_KHRONOS_validation"
		};

		vk::Instance m_Instance;
		vk::Device m_Device;
		vk::PhysicalDevice m_PhysicalDevice;
	};
}
