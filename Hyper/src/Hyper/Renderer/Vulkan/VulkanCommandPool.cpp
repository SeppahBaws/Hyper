#include "HyperPCH.h"
#include "VulkanCommandPool.h"

#include "Hyper/Renderer/RenderContext.h"

namespace Hyper
{
	VulkanCommandPool::VulkanCommandPool(RenderContext* pRenderCtx)
		: m_pRenderCtx(pRenderCtx)
	{
		vk::CommandPoolCreateInfo info{};
		info.flags=vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
		info.queueFamilyIndex = m_pRenderCtx->graphicsQueue.familyIndex;
		
		try
		{
			m_Pool = m_pRenderCtx->device.createCommandPool(info);
		}
		catch (vk::SystemError& e)
		{
			throw std::runtime_error("Failed to create command pool: "s + e.what());
		}

		HPR_VKLOG_INFO("Successfully created the command pool");
	}

	VulkanCommandPool::~VulkanCommandPool()
	{
		m_pRenderCtx->device.destroy(m_Pool);
	}

	std::vector<vk::CommandBuffer> VulkanCommandPool::GetCommandBuffers(u32 count, vk::CommandBufferLevel level)
	{
		vk::CommandBufferAllocateInfo allocInfo = {};
		allocInfo.commandPool = m_Pool;
		allocInfo.level = level;
		allocInfo.commandBufferCount = count;

		std::vector<vk::CommandBuffer> buffers;
		try
		{
			buffers = m_pRenderCtx->device.allocateCommandBuffers(allocInfo);
		}
		catch (vk::SystemError& e)
		{
			throw std::runtime_error("Failed to allocate command buffer: "s + e.what());
		}

		return buffers;
	}

	void VulkanCommandPool::FreeCommandBuffers(const std::vector<vk::CommandBuffer>& commandBuffers) const
	{
		m_pRenderCtx->device.freeCommandBuffers(m_Pool, commandBuffers);
	}
}
