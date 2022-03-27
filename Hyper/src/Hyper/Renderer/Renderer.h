#pragma once
#include <vulkan/vulkan.hpp>

#include "Hyper/Core/Subsystem.h"
#include "Vulkan/VulkanCommands.h"

#include "Vulkan/VulkanDevice.h"
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
		std::unique_ptr<VulkanDevice> m_pDevice;
		std::unique_ptr<VulkanCommandPool> m_pCommandPool;
		std::unique_ptr<VulkanSwapChain> m_pSwapChain;
		std::unique_ptr<VulkanPipeline> m_pPipeline;

		const u32 MAX_FRAMES_IN_FLIGHT = 3;
		u32 m_CurrentFrame = 0;
		u32 m_CurrentBuffer;

		std::vector<vk::CommandBuffer> m_CommandBuffers;
		std::vector<vk::Semaphore> m_ImageAvailableSemaphores;
		std::vector<vk::Semaphore> m_RenderFinishedSemaphores;
		std::vector<vk::Fence> m_InFlightFences;
		vk::RenderPass m_RenderPass;
	};
}
