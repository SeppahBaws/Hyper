#include "HyperPCH.h"
#include "Mesh.h"

#include "Vulkan/VulkanDebug.h"

namespace Hyper
{
	Mesh::Mesh(RenderContext* pRenderCtx)
		: m_pRenderCtx(pRenderCtx)
	{
		m_Vertices = {
			{{ -0.5f,  0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f }},
			{{  0.5f,  0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f }},
			{{  0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f }},
			{{ -0.5f, -0.5f, 0.0f }, { 1.0f, 1.0f, 1.0f }},
		};

		m_Indices = {
			0, 1, 2,
			0, 2, 3
		};

		// Vertex buffer
		VulkanBuffer vertexStaging = VulkanBuffer{
			m_pRenderCtx,
			m_Vertices.data(),
			m_Vertices.size() * sizeof(VertexPosCol),
			vk::BufferUsageFlagBits::eTransferSrc,
			VMA_MEMORY_USAGE_CPU_ONLY,
			"Vertex staging buffer"
		};
		m_pVertexBuffer = std::make_unique<VulkanBuffer>(
			m_pRenderCtx,
			m_Vertices.size() * sizeof(VertexPosCol),
			vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
			VMA_MEMORY_USAGE_GPU_ONLY,
			"Vertex buffer");

		m_pVertexBuffer->CopyFrom(vertexStaging);


		// Index buffer
		VulkanBuffer indexStaging = VulkanBuffer{
			pRenderCtx,
			m_Indices.data(),
			m_Indices.size() * sizeof(u32),
			vk::BufferUsageFlagBits::eTransferSrc,
			VMA_MEMORY_USAGE_CPU_ONLY,
			"Index staging buffer"
		};
		m_pIndexBuffer = std::make_unique<VulkanBuffer>(
			m_pRenderCtx,
			m_Indices.size() * sizeof(u32),
			vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
			VMA_MEMORY_USAGE_GPU_ONLY,
			"Index buffer");

		m_pIndexBuffer->CopyFrom(indexStaging);
	}

	Mesh::~Mesh()
	{
		m_pVertexBuffer.reset();
		// vmaDestroyBuffer(m_pRenderCtx->allocator, m_VertexBuffer, m_VertexBufferAllocation);
	}

	void Mesh::Bind(const vk::CommandBuffer& cmd)
	{
		cmd.bindVertexBuffers(0, { m_pVertexBuffer->GetBuffer() }, { 0 });
		cmd.bindIndexBuffer(m_pIndexBuffer->GetBuffer(), 0, vk::IndexType::eUint32);
	}

	void Mesh::Draw(const vk::CommandBuffer& cmd)
	{
		cmd.drawIndexed(static_cast<u32>(m_Indices.size()), 1, 0, 0, 0);
	}
}
