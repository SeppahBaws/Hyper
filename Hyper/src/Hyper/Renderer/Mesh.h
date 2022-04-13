#pragma once
#include "Vulkan/VertexPosCol.h"
#include "Vulkan/VulkanBuffer.h"

namespace Hyper
{
	class Mesh
	{
	public:
		Mesh(RenderContext* pRenderCtx);
		~Mesh();

		void Bind(const vk::CommandBuffer& cmd);
		void Draw(const vk::CommandBuffer& cmd);

	private:
		RenderContext* m_pRenderCtx;

		std::vector<VertexPosCol> m_Vertices;
		std::vector<u32> m_Indices;

		std::unique_ptr<VulkanBuffer> m_pVertexBuffer{};
		std::unique_ptr<VulkanBuffer> m_pIndexBuffer{};
		// vk::Buffer m_VertexBuffer;
		// VmaAllocation m_VertexBufferAllocation;
	};
}
