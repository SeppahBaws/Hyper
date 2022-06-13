#pragma once
#include "Vertex.h"
#include "VulkanBuffer.h"

namespace Hyper
{
    class VulkanVertexBuffer
    {
    public:
        explicit VulkanVertexBuffer(RenderContext* pRenderCtx, const std::string& name);
        ~VulkanVertexBuffer();

        void CreateFrom(const std::vector<VertexPosNormTex>& vertices);

        void Bind(const vk::CommandBuffer& cmd) const;

        [[nodiscard]] VulkanBuffer* GetBuffer() const { return m_pVertexBuffer.get(); }

    private:
        RenderContext* m_pRenderCtx{};

        std::string m_Name;
        std::unique_ptr<VulkanBuffer> m_pVertexBuffer{};
    };
}
