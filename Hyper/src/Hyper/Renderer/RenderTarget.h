#pragma once
#include "Vulkan/VulkanImage.h"

namespace Hyper
{
	class RenderTarget
	{
	public:
		RenderTarget(RenderContext* pRenderCtx, vk::Format format, const std::string& debugName, u32 width, u32 height);
		~RenderTarget();

		[[nodiscard]] std::array<vk::RenderingAttachmentInfo, 2> GetRenderingAttachments() const;
		[[nodiscard]] VulkanImage* GetColorImage() const { return m_pColorImage.get(); }
		[[nodiscard]] VulkanImage* GetDepthImage() const { return m_pDepthImage.get(); }
		[[nodiscard]] vk::Sampler GetColorSampler() const { return m_ColorSampler; }

		void Resize(u32 newWidth, u32 newHeight);

	private:
		RenderContext* m_pRenderCtx;

		std::unique_ptr<VulkanImage> m_pColorImage;
		std::unique_ptr<VulkanImage> m_pDepthImage;
		vk::Sampler m_ColorSampler;
	};
}
