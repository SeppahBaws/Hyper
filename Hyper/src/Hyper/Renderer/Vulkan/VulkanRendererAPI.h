#pragma once
#include "Hyper/Renderer/Base/RendererAPI.h"

namespace Hyper
{
	class VulkanRenderContext;

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
		std::shared_ptr<VulkanRenderContext> m_Context;
	};
}
