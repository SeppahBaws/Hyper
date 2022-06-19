﻿#include "HyperPCH.h"
#include "VulkanBuffer.h"

#include "VulkanUtility.h"
#include "VulkanDebug.h"
#include "Hyper/Renderer/RenderContext.h"

namespace Hyper
{
	VulkanBuffer::VulkanBuffer(RenderContext* pRenderCtx, const void* data, vk::DeviceSize size,
		vk::BufferUsageFlags bufferUsage, VmaMemoryUsage memoryUsage, const std::string& name,
		vk::BufferCreateFlags flags)
		: VulkanBuffer(pRenderCtx, size, bufferUsage, memoryUsage, name, flags)
	{
		// Copy the data into the buffer
		void* mapped = Map();
		memcpy(mapped, data, size);
		Unmap();
	}

	VulkanBuffer::VulkanBuffer(RenderContext* pRenderCtx, vk::DeviceSize size, vk::BufferUsageFlags bufferUsage,
	                           VmaMemoryUsage memoryUsage, const std::string& name, vk::BufferCreateFlags flags)
		: m_pRenderCtx(pRenderCtx)
		, m_Size(size)
		, m_UsageFlags(bufferUsage)
	{
		vk::BufferCreateInfo bufferInfo{};
		bufferInfo.size = m_Size;
		bufferInfo.flags = flags;
		bufferInfo.usage = bufferUsage;
		bufferInfo.sharingMode = vk::SharingMode::eExclusive;

		VmaAllocationCreateInfo allocInfo{};
		allocInfo.usage = memoryUsage;

		VkBuffer tempBuffer;
		VmaAllocation tempAlloc;
		
		VulkanUtils::Check(vmaCreateBuffer(
			pRenderCtx->allocator,
			reinterpret_cast<VkBufferCreateInfo*>(&bufferInfo),
			&allocInfo,
			&tempBuffer,
			&tempAlloc,
			nullptr));

		m_Buffer = tempBuffer;
		m_Allocation = tempAlloc;

		VkDebug::SetObjectName(m_pRenderCtx->device, vk::ObjectType::eBuffer, m_Buffer, name);
	}

	VulkanBuffer::~VulkanBuffer()
	{
		vmaDestroyBuffer(m_pRenderCtx->allocator, m_Buffer, m_Allocation);
	}

	VulkanBuffer::VulkanBuffer(VulkanBuffer&& other)
		: m_pRenderCtx(other.m_pRenderCtx)
		, m_Size(other.m_Size)
		, m_UsageFlags(other.m_UsageFlags)
		, m_Buffer(other.m_Buffer)
		, m_Allocation(other.m_Allocation)
	{
		// Invalidate other's important data
		other.m_Buffer = nullptr;
		other.m_Allocation = nullptr;
	}
	
	VulkanBuffer& VulkanBuffer::operator=(VulkanBuffer&& other)
	{
		// Yoink other's data
		m_pRenderCtx = other.m_pRenderCtx;
		m_Size = other.m_Size;
		m_UsageFlags = other.m_UsageFlags;
		m_Buffer = other.m_Buffer;
		m_Allocation = other.m_Allocation;
	
		// Invalidate other's important data
		other.m_Buffer = nullptr;
		other.m_Allocation = nullptr;
	
		return *this;
	}

	void* VulkanBuffer::Map()
	{
		void* data;
		VulkanUtils::Check(vmaMapMemory(m_pRenderCtx->allocator, m_Allocation, &data));

		return data;
	}

	void VulkanBuffer::Unmap()
	{
		vmaUnmapMemory(m_pRenderCtx->allocator, m_Allocation);
	}

	void VulkanBuffer::SetData(const void* data, size_t size)
	{
		void* mapped = Map();
		memcpy(mapped, data, size);
		Unmap();
	}

	void VulkanBuffer::CopyFrom(const VulkanBuffer& srcBuffer)
	{
		// TODO: replace with asserts
		if (!(srcBuffer.m_UsageFlags & vk::BufferUsageFlagBits::eTransferSrc))
		{
			HPR_VKLOG_ERROR("Cannot copy from a buffer which does not have the VK_BUFFER_USAGE_TRANSFER_SRC_BIT flag set!");
			return;
		}

		if (!(m_UsageFlags & vk::BufferUsageFlagBits::eTransferDst))
		{
			HPR_VKLOG_ERROR("Cannot copy to a buffer which does not have the VK_BUFFER_USAGE_TRANSFER_DST_BIT flag set!");
			return;
		}

		const vk::CommandBuffer cmd = m_pRenderCtx->commandPool->GetCommandBuffer();
		VulkanCommandBuffer::Begin(cmd, vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

		vk::BufferCopy copyRegion = {};
		copyRegion.size = m_Size;
		cmd.copyBuffer(srcBuffer.m_Buffer, m_Buffer, { copyRegion });

		VulkanCommandBuffer::End(cmd);
		m_pRenderCtx->graphicsQueue.Submit({}, {}, {}, cmd, nullptr);
		m_pRenderCtx->graphicsQueue.WaitIdle();
		m_pRenderCtx->commandPool->FreeCommandBuffer(cmd);
	}

	u64 VulkanBuffer::GetDeviceAddress() const
	{
		vk::BufferDeviceAddressInfo info{};
		info.buffer = m_Buffer;
		return m_pRenderCtx->device.getBufferAddressKHR(info);
	}
}
