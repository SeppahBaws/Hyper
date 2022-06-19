#include "HyperPCH.h"
#include "VulkanCommands.h"

#include "VulkanUtility.h"
#include "Hyper/Renderer/RenderContext.h"

namespace Hyper
{
	VulkanCommandPool::VulkanCommandPool(RenderContext* pRenderCtx)
		: m_pRenderCtx(pRenderCtx)
	{
		vk::CommandPoolCreateInfo info{};
		info.flags=vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
		info.queueFamilyIndex = m_pRenderCtx->graphicsQueue.familyIndex;
		
		m_Pool = VulkanUtils::Check(m_pRenderCtx->device.createCommandPool(info));

		m_pRenderCtx->commandPool = this;

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

		std::vector<vk::CommandBuffer> buffers = VulkanUtils::Check(m_pRenderCtx->device.allocateCommandBuffers(allocInfo));

		return buffers;
	}

	vk::CommandBuffer VulkanCommandPool::GetCommandBuffer(vk::CommandBufferLevel level)
	{
		return GetCommandBuffers(1, level)[0];
	}

	void VulkanCommandPool::FreeCommandBuffers(const std::vector<vk::CommandBuffer>& commandBuffers) const
	{
		m_pRenderCtx->device.freeCommandBuffers(m_Pool, commandBuffers);
	}

	void VulkanCommandPool::FreeCommandBuffer(const vk::CommandBuffer& cmd) const
	{
		FreeCommandBuffers({ cmd });
	}

	namespace VulkanCommandBuffer
	{
		void Begin(vk::CommandBuffer cmd, vk::CommandBufferUsageFlags flags)
		{
			vk::CommandBufferBeginInfo info = {};
			info.flags = flags;

			VulkanUtils::Check(cmd.begin(info));
		}

		void End(vk::CommandBuffer cmd)
		{
			VulkanUtils::Check(cmd.end());
		}
	}
}
