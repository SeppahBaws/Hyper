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
		VulkanBuffer(RenderContext* pRenderCtx, const void* data, vk::DeviceSize size, vk::BufferUsageFlags bufferUsage, VmaMemoryUsage memoryUsage, const std::string& name, vk::BufferCreateFlags flags = {});
		~VulkanBuffer();
		// Make sure we can't copy the buffer
		VulkanBuffer(const VulkanBuffer& other) = delete;
		VulkanBuffer& operator=(const VulkanBuffer& other) = delete;

		// Overloads for moving
		VulkanBuffer(VulkanBuffer&& other);
		VulkanBuffer& operator=(VulkanBuffer&& other);

		[[nodiscard]] void* Map();
		void Unmap();

		void SetData(const void* data, size_t size);

		[[nodiscard]] vk::Buffer GetBuffer() const { return m_Buffer; }

		void CopyFrom(const VulkanBuffer& srcBuffer);
		u64 GetDeviceAddress() const;

	private:
		RenderContext* m_pRenderCtx{};

		vk::DeviceSize m_Size{};
		vk::BufferUsageFlags m_UsageFlags{};
		vk::Buffer m_Buffer{};
		VmaAllocation m_Allocation{};
	};
}
