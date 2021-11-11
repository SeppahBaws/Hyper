#pragma once
#include "Hyper/Renderer/Base/RenderContext.h"

#include <vulkan/vulkan.hpp>

namespace Hyper
{
	class VulkanRenderContext final : public RenderContext
	{
	public:
		VulkanRenderContext();
		~VulkanRenderContext() = default;

	private:
		vk::Instance m_Instance;

		const bool HYPER_VALIDATE = true;
		const std::vector<const char*> m_ValidationLayers = {
			"VK_LAYER_KHRONOS_validation"
		};
	};
}
