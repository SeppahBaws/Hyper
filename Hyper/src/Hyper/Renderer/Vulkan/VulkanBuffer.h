#pragma once
#include <vulkan/vulkan.hpp>
#include <vk_mem_alloc.h>

namespace Hyper
{
	struct RenderContext;

	class VulkanBuffer
	{
	public:
		VulkanBuffer(RenderContext* pRenderCtx, vk::DeviceSize size, vk::BufferUsageFlags bufferUsage, VmaMemoryUsage memoryUsage, const std::string& name, vk::BufferCreateFlags flags = {});
		VulkanBuffer(RenderContext* pRenderCtx, void* data, vk::DeviceSize size, vk::BufferUsageFlags bufferUsage, VmaMemoryUsage memoryUsage, const std::string& name);
		~VulkanBuffer();
		// Make sure we can't copy or move the buffer
		VulkanBuffer(const VulkanBuffer& other) = delete;
		VulkanBuffer(VulkanBuffer&& other) = delete;
		VulkanBuffer& operator=(const VulkanBuffer& other) = delete;
		VulkanBuffer& operator=(VulkanBuffer&& other) = delete;

		[[nodiscard]] void* Map();
		void Unmap();

		[[nodiscard]] vk::Buffer GetBuffer() const { return m_Buffer; }

		void CopyFrom(const VulkanBuffer& srcBuffer);

	private:
		RenderContext* m_pRenderCtx{};

		vk::DeviceSize m_Size{};
		vk::BufferUsageFlags m_UsageFlags{};
		vk::Buffer m_Buffer{};
		VmaAllocation m_Allocation{};
	};
}
