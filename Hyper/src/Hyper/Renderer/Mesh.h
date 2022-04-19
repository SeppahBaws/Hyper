#pragma once
#include "Vulkan/VertexPosCol.h"
#include "Vulkan/VulkanBuffer.h"
#include "Vulkan/VulkanIndexBuffer.h"
#include "Vulkan/VulkanVertexBuffer.h"

namespace Hyper
{
	class Mesh
	{
	public:
		explicit Mesh(RenderContext* pRenderCtx);
		~Mesh();

		void Bind(const vk::CommandBuffer& cmd) const;
		void Draw(const vk::CommandBuffer& cmd) const;

	private:
		RenderContext* m_pRenderCtx;

		std::vector<VertexPosCol> m_Vertices;
		std::vector<u32> m_Indices;

		std::unique_ptr<VulkanVertexBuffer> m_pVertexBuffer{};
		std::unique_ptr<VulkanIndexBuffer> m_pIndexBuffer{};
		// std::unique_ptr<VulkanBuffer> m_pVertexBuffer{};
		// std::unique_ptr<VulkanBuffer> m_pIndexBuffer{};
	};
}
