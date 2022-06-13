#pragma once
#include "VulkanBuffer.h"

namespace Hyper
{
    class VulkanIndexBuffer
    {
    public:
        explicit VulkanIndexBuffer(RenderContext* pRenderCtx, const std::string& name);
        ~VulkanIndexBuffer();

        void CreateFrom(const std::vector<u32>& indices);
        void CreateFrom(const std::vector<u16>& indices);

        void Bind(const vk::CommandBuffer& cmd) const;

        [[nodiscard]] VulkanBuffer* GetBuffer() const { return m_pIndexBuffer.get(); }

    private:
        RenderContext* m_pRenderCtx;

        std::string m_Name;
        vk::IndexType m_IndexType{};
        std::unique_ptr<VulkanBuffer> m_pIndexBuffer{};
    };
}
