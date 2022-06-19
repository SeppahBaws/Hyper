#pragma once
#include <vulkan/vulkan.hpp>

#include "VulkanBuffer.h"
#include "VulkanDevice.h"

namespace Hyper
{
	class VulkanImage
	{
	public:
		VulkanImage(RenderContext* pRenderCtx, vk::Format format, vk::ImageType type, vk::ImageUsageFlags usage, vk::ImageAspectFlags aspectFlags,
			const std::string& debugName, u32 width, u32 height, u32 depth = 1);
		~VulkanImage();

		[[nodiscard]] vk::Image GetImage() const { return m_Image; }
		[[nodiscard]] vk::ImageView GetImageView() const { return m_ImageView; }
		[[nodiscard]] vk::ImageLayout GetImageLayout() const { return m_Layout; }
		[[nodiscard]] vk::Format GetFormat() const { return m_Format; }

		void Resize(u32 width, u32 height, u32 depth = 1);

		void TransitionLayout(vk::CommandBuffer cmd, vk::AccessFlags newAccessFlags, vk::ImageLayout newLayout, vk::PipelineStageFlags newStageFlags);
		void CopyFrom(vk::CommandBuffer cmd, const VulkanBuffer& srcBuffer);

	private:
		void CreateImageAndView();
		void DestroyImageAndView();

	private:
		RenderContext* m_pRenderCtx;

		vk::Image m_Image;
		vk::ImageView m_ImageView;
		VmaAllocation m_Allocation{};

		vk::Format m_Format{};
		vk::ImageLayout m_Layout{};
		vk::AccessFlags m_AccessFlags{};
		vk::PipelineStageFlags m_PipelineStageFlags{};
		vk::ImageType m_Type{};
		vk::ImageUsageFlags m_Usage;
		vk::ImageAspectFlags m_AspectFlags;
		u32 m_Width{}, m_Height{}, m_Depth{};
		std::string m_DebugName;
	};
}
