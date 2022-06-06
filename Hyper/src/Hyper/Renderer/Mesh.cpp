#include "HyperPCH.h"
#include "Mesh.h"

#include "Hyper/Debug/Profiler.h"
#include "Vulkan/VulkanDebug.h"
#include "Vulkan/VulkanIndexBuffer.h"

namespace Hyper
{
	Mesh::Mesh(RenderContext* pRenderCtx, u32 materialIdx, const std::vector<VertexPosNormTex>& vertices, const std::vector<u32>& indices)
		: m_pRenderCtx(pRenderCtx), m_MaterialIdx(materialIdx), m_Vertices(vertices), m_Indices(indices)
	{
		// Vertex buffer
		m_pVertexBuffer = std::make_unique<VulkanVertexBuffer>(m_pRenderCtx, "vertex buffer");
		m_pVertexBuffer->CreateFrom(m_Vertices);

		// Index buffer
		m_pIndexBuffer = std::make_unique<VulkanIndexBuffer>(m_pRenderCtx, "index buffer");
		m_pIndexBuffer->CreateFrom(m_Indices);
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

		cmd.drawIndexed(static_cast<u32>(m_Indices.size()), 1, 0, 0, 0);
	}
}
