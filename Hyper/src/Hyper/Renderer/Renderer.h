#pragma once
#include "Hyper/Core/Subsystem.h"

#include "Hyper/Renderer/Vulkan/VulkanDevice.h"
#include "Vulkan/VulkanSwapChain.h"

namespace Hyper
{
	class Renderer final : public Subsystem
	{
	public:
		Renderer(Context* pContext);
		~Renderer() override = default;

		bool OnInitialize() override;
		void OnTick() override;
		void OnShutdown() override;

		[[nodiscard]] std::shared_ptr<VulkanDevice> GetDevice() const { return m_Device; };

	private:
		std::shared_ptr<RenderContext> m_pRenderContext;
		std::shared_ptr<VulkanDevice> m_Device;
		std::shared_ptr<VulkanSwapChain> m_SwapChain;

		u64 m_FrameCount{};
	};
}
