#pragma once
#include <vulkan/vulkan.hpp>

namespace Hyper
{
	struct RenderContext;

	class VulkanCommandPool
	{
	public:
		VulkanCommandPool(RenderContext* pRenderCtx);
		~VulkanCommandPool();

		[[nodiscard]] std::vector<vk::CommandBuffer> GetCommandBuffers(u32 count, vk::CommandBufferLevel level = vk::CommandBufferLevel::ePrimary);
		[[nodiscard]] vk::CommandBuffer GetCommandBuffer(vk::CommandBufferLevel level = vk::CommandBufferLevel::ePrimary);
		void FreeCommandBuffers(const std::vector<vk::CommandBuffer>& commandBuffers) const;
		void FreeCommandBuffer(const vk::CommandBuffer& cmd) const;

	private:
		RenderContext* m_pRenderCtx;
		vk::CommandPool m_Pool;
	};

	// Helper functions for vulkan command buffers
	namespace VulkanCommandBuffer
	{
		void Begin(vk::CommandBuffer cmd, vk::CommandBufferUsageFlags flags = {});
		void End(vk::CommandBuffer cmd);
	}
}
