#include "HyperPCH.h"
#include "Mesh.h"

#include "Hyper/Debug/Profiler.h"
#include "Vulkan/VulkanDebug.h"
#include "Vulkan/VulkanIndexBuffer.h"

namespace Hyper
{
	Mesh::Mesh(RenderContext* pRenderCtx, const UUID& materialId, const std::vector<VertexPosNormTex>& vertices, const std::vector<u32>& indices, u32 triCount)
		: m_pRenderCtx(pRenderCtx), m_MaterialId(materialId), m_TriCount(triCount)
	{
		m_VertexCount = vertices.size();
		m_IndexCount = indices.size();

		// Vertex buffer
		m_pVertexBuffer = std::make_unique<VulkanVertexBuffer>(m_pRenderCtx, "vertex buffer");
		m_pVertexBuffer->CreateFrom(vertices);

		// Index buffer
		m_pIndexBuffer = std::make_unique<VulkanIndexBuffer>(m_pRenderCtx, "index buffer");
		m_pIndexBuffer->CreateFrom(indices);
	}

	Mesh::~Mesh()
	{
		m_pVertexBuffer.reset();
	}

	void Mesh::Draw(const vk::CommandBuffer& cmd) const
	{
		HPR_PROFILE_SCOPE();

		m_pVertexBuffer->Bind(cmd);
		m_pIndexBuffer->Bind(cmd);

		cmd.drawIndexed(m_IndexCount, 1, 0, 0, 0);
	}
}
