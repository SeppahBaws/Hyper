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
		explicit Mesh(RenderContext* pRenderCtx, const UUID& materialId, const std::vector<VertexPosNormTex>& vertices, const std::vector<u32>& indices, u32 triCount);
		~Mesh();

		void Draw(const vk::CommandBuffer& cmd) const;
		
		[[nodiscard]] UUID GetMaterialId() const { return m_MaterialId; }

		[[nodiscard]] VulkanVertexBuffer* GetVertexBuffer() const { return m_pVertexBuffer.get(); }
		[[nodiscard]] VulkanIndexBuffer* GetIndexBuffer() const { return m_pIndexBuffer.get(); }

		[[nodiscard]] u32 GetVertexCount() const { return m_VertexCount; }
		[[nodiscard]] u32 GetTriCount() const { return m_TriCount; }

	private:
		RenderContext* m_pRenderCtx;

		glm::vec3 m_Position;
		glm::vec3 m_Rotation;
		glm::vec3 m_Scale;

		UUID m_MaterialId;
		u32 m_VertexCount{};
		u32 m_IndexCount{};
		u32 m_TriCount{};

		std::unique_ptr<VulkanVertexBuffer> m_pVertexBuffer{};
		std::unique_ptr<VulkanIndexBuffer> m_pIndexBuffer{};
	};
}
