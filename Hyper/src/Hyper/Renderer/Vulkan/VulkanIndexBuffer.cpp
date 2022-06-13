#include "HyperPCH.h"
#include "VulkanIndexBuffer.h"

namespace Hyper
{
    VulkanIndexBuffer::VulkanIndexBuffer(RenderContext* pRenderCtx, const std::string& name)
        : m_pRenderCtx(pRenderCtx)
        , m_Name(name)
    {
    }

    VulkanIndexBuffer::~VulkanIndexBuffer()
    {
        m_pIndexBuffer.reset();
    }

    void VulkanIndexBuffer::CreateFrom(const std::vector<u32>& indices)
    {
        m_IndexType = vk::IndexType::eUint32;

        const VulkanBuffer staging{
            m_pRenderCtx,
            indices.data(),
            indices.size() * sizeof(u32),
            vk::BufferUsageFlagBits::eTransferSrc,
            VMA_MEMORY_USAGE_CPU_ONLY,
            fmt::format("{} staging buffer", m_Name)
        };
        m_pIndexBuffer = std::make_unique<VulkanBuffer>(
            m_pRenderCtx,
            indices.size() * sizeof(u32),
            vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR,
            VMA_MEMORY_USAGE_GPU_ONLY,
            m_Name);

        m_pIndexBuffer->CopyFrom(staging);
    }

    void VulkanIndexBuffer::CreateFrom(const std::vector<u16>& indices)
    {
        m_IndexType = vk::IndexType::eUint16;

        const VulkanBuffer staging{
            m_pRenderCtx,
            indices.data(),
            indices.size() * sizeof(u16),
            vk::BufferUsageFlagBits::eTransferSrc,
            VMA_MEMORY_USAGE_CPU_ONLY,
            fmt::format("{} staging buffer", m_Name)
        };
        m_pIndexBuffer = std::make_unique<VulkanBuffer>(
            m_pRenderCtx,
            indices.size() * sizeof(u16),   
            vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR,
            VMA_MEMORY_USAGE_GPU_ONLY,
            m_Name);

        m_pIndexBuffer->CopyFrom(staging);
    }

    void VulkanIndexBuffer::Bind(const vk::CommandBuffer& cmd) const
    {
        cmd.bindIndexBuffer(m_pIndexBuffer->GetBuffer(), 0, m_IndexType);
    }
}
