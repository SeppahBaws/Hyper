#include "HyperPCH.h"
#include "Mesh.h"

#include "Hyper/Debug/Profiler.h"
#include "Vulkan/VulkanDebug.h"
#include "Vulkan/VulkanIndexBuffer.h"

namespace Hyper
{
	Mesh::Mesh(RenderContext* pRenderCtx)
		: m_pRenderCtx(pRenderCtx)
	{
		// Test square
		// m_Vertices = {
		// 	{{ -0.5f,  0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f }},
		// 	{{  0.5f,  0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f }},
		// 	{{  0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f, 1.0f }},
		// 	{{ -0.5f, -0.5f, -0.5f }, { 1.0f, 1.0f, 1.0f }},
		// };
		//
		// m_Indices = {
		// 	0, 1, 2,
		// 	0, 2, 3
		// };

		// Test cube
		m_Vertices = {
			{ {  0.500000, -0.500000, -0.500000 }, { 1.0f, 0.0f, 0.0f } },
			{ {  0.500000,  0.500000, -0.500000 }, { 0.0f, 1.0f, 0.0f } },
			{ {  0.500000, -0.500000,  0.500000 }, { 0.0f, 0.0f, 1.0f } },
			{ {  0.500000,  0.500000,  0.500000 }, { 1.0f, 1.0f, 1.0f } },
			{ { -0.500000, -0.500000, -0.500000 }, { 1.0f, 0.0f, 0.0f } },
			{ { -0.500000,  0.500000, -0.500000 }, { 0.0f, 1.0f, 0.0f } },
			{ { -0.500000, -0.500000,  0.500000 }, { 0.0f, 0.0f, 1.0f } },
			{ { -0.500000,  0.500000,  0.500000 }, { 1.0f, 1.0f, 1.0f } }
		};

		m_Indices = {
			1, 2, 0,
			3, 6, 2,
			7, 4, 6,
			5, 0, 4,
			6, 0, 2,
			3, 5, 7,
			1, 3, 2,
			3, 7, 6,
			7, 5, 4,
			5, 1, 0,
			6, 4, 0,
			3, 1, 5
		};

		// Vertex buffer
		m_pVertexBuffer = std::make_unique<VulkanVertexBuffer>(m_pRenderCtx, "test vertex buffer");
		m_pVertexBuffer->CreateFrom(m_Vertices);

		// Index buffer
		m_pIndexBuffer = std::make_unique<VulkanIndexBuffer>(m_pRenderCtx, "test index buffer");
		m_pIndexBuffer->CreateFrom(m_Indices);
	}

	Mesh::~Mesh()
	{
		m_pVertexBuffer.reset();
	}

	void Mesh::Bind(const vk::CommandBuffer& cmd) const
	{
		m_pVertexBuffer->Bind(cmd);
		m_pIndexBuffer->Bind(cmd);
	}

	void Mesh::Draw(const vk::CommandBuffer& cmd) const
	{
		HPR_PROFILE_SCOPE();

		Bind(cmd);

		cmd.drawIndexed(static_cast<u32>(m_Indices.size()), 1, 0, 0, 0);
	}
}
