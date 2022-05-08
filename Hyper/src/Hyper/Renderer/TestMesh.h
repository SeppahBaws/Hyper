#pragma once
#include "Vulkan/Vertex.h"
#include "Vulkan/VulkanBuffer.h"
#include "Vulkan/VulkanIndexBuffer.h"
#include "Vulkan/VulkanVertexBuffer.h"

namespace Hyper
{
	class TestMesh
	{
	public:
		explicit TestMesh(RenderContext* pRenderCtx);
		~TestMesh();

		void Bind(const vk::CommandBuffer& cmd) const;
		void Draw(const vk::CommandBuffer& cmd) const;

		void Import();

	private:
		RenderContext* m_pRenderCtx;

		std::vector<VertexPosNormTex> m_Vertices;
		std::vector<u32> m_Indices;

		std::unique_ptr<VulkanVertexBuffer> m_pVertexBuffer{};
		std::unique_ptr<VulkanIndexBuffer> m_pIndexBuffer{};
	};
}
