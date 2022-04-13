#include "HyperPCH.h"
#include "VulkanCommands.h"

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

			try
			{
				cmd.begin(info);
			}
			catch (vk::SystemError& e)
			{
				throw std::runtime_error("Failed to begin command buffer: "s + e.what());
			}
		}

		void End(vk::CommandBuffer cmd)
		{
			try
			{
				cmd.end();
			}
			catch (vk::SystemError& e)
			{
				throw std::runtime_error("Failed to end command buffer: "s + e.what());
			}
		}
	}
}
