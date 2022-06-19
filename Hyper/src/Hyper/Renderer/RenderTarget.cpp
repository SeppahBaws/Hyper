#include "HyperPCH.h"
#include "RenderTarget.h"

#include "RenderContext.h"
#include "Vulkan/VulkanUtility.h"

namespace Hyper
{
	RenderTarget::RenderTarget(RenderContext* pRenderCtx, const std::string& debugName, u32 width, u32 height)
		: m_pRenderCtx(pRenderCtx)
	{
		// Color Image
		// TODO: allow for customization.
		m_pColorImage = std::make_unique<VulkanImage>(
			m_pRenderCtx,
			vk::Format::eB8G8R8A8Unorm,
			vk::ImageType::e2D,
			vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eStorage,
			vk::ImageAspectFlagBits::eColor,
			debugName,
			width,
			height
		);

		m_pDepthImage = std::make_unique<VulkanImage>(
			m_pRenderCtx,
			vk::Format::eD24UnormS8Uint,
			vk::ImageType::e2D,
			vk::ImageUsageFlagBits::eDepthStencilAttachment,
			vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil,
			debugName,
			width,
			height
		);

		// Sampler
		vk::SamplerCreateInfo samplerInfo = {};
		samplerInfo.minFilter = vk::Filter::eLinear;
		samplerInfo.magFilter = vk::Filter::eLinear;
		samplerInfo.addressModeU = vk::SamplerAddressMode::eRepeat;
		samplerInfo.addressModeV = vk::SamplerAddressMode::eRepeat;
		samplerInfo.addressModeW = vk::SamplerAddressMode::eRepeat;

		m_ColorSampler = VulkanUtils::Check(m_pRenderCtx->device.createSampler(samplerInfo));

		vk::CommandBuffer cmd = m_pRenderCtx->commandPool->GetCommandBuffer();
		VulkanCommandBuffer::Begin(cmd, vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
		m_pColorImage->TransitionLayout(cmd, {}, vk::ImageLayout::eGeneral, vk::PipelineStageFlagBits::eAllGraphics);
		m_pDepthImage->TransitionLayout(cmd, {}, vk::ImageLayout::eGeneral, vk::PipelineStageFlagBits::eAllGraphics);
		VulkanCommandBuffer::End(cmd);
		m_pRenderCtx->graphicsQueue.Submit({}, {}, {}, cmd, {});
		m_pRenderCtx->graphicsQueue.WaitIdle();
		m_pRenderCtx->commandPool->FreeCommandBuffer(cmd);
	}

	RenderTarget::~RenderTarget()
	{
		m_pColorImage.reset();
		m_pDepthImage.reset();
		m_pRenderCtx->device.destroySampler(m_ColorSampler);
	}

	std::array<vk::RenderingAttachmentInfo, 2> RenderTarget::GetRenderingAttachments() const
	{
		std::array<vk::RenderingAttachmentInfo, 2> attachments{};

		attachments[0].imageView = m_pColorImage->GetImageView();
		attachments[0].imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
		attachments[0].resolveMode = vk::ResolveModeFlagBits::eNone;
		attachments[0].loadOp = vk::AttachmentLoadOp::eClear;
		attachments[0].storeOp = vk::AttachmentStoreOp::eStore;
		attachments[0].clearValue = vk::ClearColorValue(std::array{ 0.0f, 0.0f, 0.0f, 1.0f });

		attachments[1].imageView = m_pDepthImage->GetImageView();
		attachments[1].imageLayout = vk::ImageLayout::eDepthAttachmentOptimal;
		attachments[1].resolveMode = vk::ResolveModeFlagBits::eNone;
		attachments[1].loadOp = vk::AttachmentLoadOp::eClear;
		attachments[1].storeOp = vk::AttachmentStoreOp::eStore;
		attachments[1].clearValue = vk::ClearDepthStencilValue(1.0f);

		return attachments;
	}

	void RenderTarget::Resize(u32 newWidth, u32 newHeight)
	{
		m_pColorImage->Resize(newWidth, newHeight);
	}
}
