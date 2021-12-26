#pragma once
#include "Hyper/Renderer/Base/RendererAPI.h"

#include "VulkanRenderDevice.h"

namespace Hyper
{
	class VulkanRendererAPI final : public RendererAPI
	{
	public:
		VulkanRendererAPI() = default;

		void Init() override;

		void BeginScene() override;
		void EndScene() override;

		void Clear() override;
		void DrawIndexed() override;

	private:
		std::unique_ptr<VulkanRenderDevice> m_Device;
	};
}
