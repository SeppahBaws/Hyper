#pragma once
#include "Vulkan/Vertex.h"
#include "Vulkan/VulkanBuffer.h"
#include "Vulkan/VulkanIndexBuffer.h"
#include "Vulkan/VulkanVertexBuffer.h"

namespace Hyper
{
	class Mesh
	{
	public:
		explicit Mesh(RenderContext* pRenderCtx, u32 materialIdx, const std::vector<VertexPosNormTex>& vertices, const std::vector<u32>& indices);
		~Mesh();

		void Draw(const vk::CommandBuffer& cmd) const;
		
		[[nodiscard]] u32 GetMaterialIdx() const { return m_MaterialIdx; }

	private:
		RenderContext* m_pRenderCtx;

		u32 m_MaterialIdx;
		std::vector<VertexPosNormTex> m_Vertices;
		std::vector<u32> m_Indices;

		std::unique_ptr<VulkanVertexBuffer> m_pVertexBuffer{};
		std::unique_ptr<VulkanIndexBuffer> m_pIndexBuffer{};
	};
}
