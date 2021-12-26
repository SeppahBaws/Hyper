#pragma once
#include "Hyper/Renderer/Base/RendererAPI.h"

namespace Hyper
{
	class VulkanRenderDevice;

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
		std::shared_ptr<VulkanRenderDevice> m_Device;
	};
}
