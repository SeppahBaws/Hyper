#pragma once
#include "Hyper/Core/Subsystem.h"

#include "Hyper/Renderer/Vulkan/VulkanDevice.h"
#include "Vulkan/VulkanPipeline.h"
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

	private:
		std::unique_ptr<RenderContext> m_pRenderContext;
		std::unique_ptr<VulkanDevice> m_Device;
		std::unique_ptr<VulkanSwapChain> m_SwapChain;
		std::unique_ptr<VulkanPipeline> m_Pipeline;

		const u32 MAX_FRAMES_IN_FLIGHT = 2;
		u32 m_CurrentFrame = 0;
		u32 m_CurrentBuffer;

		std::vector<vk::CommandBuffer> m_CommandBuffers;
		std::vector<vk::Semaphore> m_ImageAvailableSemaphores;
		std::vector<vk::Semaphore> m_RenderFinishedSemaphores;
		std::vector<vk::Fence> m_InFlightFences;
		vk::RenderPass m_RenderPass;
	};
}
