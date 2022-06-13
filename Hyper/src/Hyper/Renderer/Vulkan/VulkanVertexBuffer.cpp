#include "HyperPCH.h"
#include "VulkanVertexBuffer.h"

namespace Hyper
{
    VulkanVertexBuffer::VulkanVertexBuffer(RenderContext* pRenderCtx, const std::string& name)
        : m_pRenderCtx(pRenderCtx)
		, m_Name(name)
    {
    }

    VulkanVertexBuffer::~VulkanVertexBuffer()
    {
    	m_pVertexBuffer.reset();
    }

    void VulkanVertexBuffer::CreateFrom(const std::vector<VertexPosNormTex>& vertices)
    {
        const VulkanBuffer staging{
        	m_pRenderCtx,
        	vertices.data(),
        	vertices.size() * sizeof(VertexPosNormTex),
        	vk::BufferUsageFlagBits::eTransferSrc,
        	VMA_MEMORY_USAGE_CPU_ONLY,
        	fmt::format("{} staging buffer", m_Name)
        };
        m_pVertexBuffer = std::make_unique<VulkanBuffer>(
        	m_pRenderCtx,
        	vertices.size() * sizeof(VertexPosNormTex),
        	vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR,
        	VMA_MEMORY_USAGE_GPU_ONLY,
        	m_Name);
        
        m_pVertexBuffer->CopyFrom(staging);
    }

    void VulkanVertexBuffer::Bind(const vk::CommandBuffer& cmd) const
    {
    	cmd.bindVertexBuffers(0, { m_pVertexBuffer->GetBuffer() }, { 0 });
    }
}
